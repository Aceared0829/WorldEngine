#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <RendererCore/Components/AlwaysVisibleComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
WCVarBool WRenderPipeline::cvar_SpatialCullingVis("Spatial.Culling.Vis", false, WCVarFlags::Default, "Enables debug visualization of visibility culling");
WCVarBool cvar_SpatialCullingShowStats("Spatial.Culling.ShowStats", false, WCVarFlags::Default, "Display some stats of the visibility culling");
WCVarBool cvar_RenderingShowStats("Rendering.ShowStats", false, WCVarFlags::Default, "Display the number of drawcalls and triangles rendered in the previous frame (all views and passes combined)");
#endif

WCVarBool cvar_SpatialCullingOcclusionEnable("Spatial.Occlusion.Enable", true, WCVarFlags::Default, "Use software rasterization for occlusion culling.");
WCVarBool cvar_SpatialCullingOcclusionVisView("Spatial.Occlusion.VisView", false, WCVarFlags::Default, "Render the occlusion framebuffer as an overlay.");
WCVarFloat cvar_SpatialCullingOcclusionBoundsInlation("Spatial.Occlusion.BoundsInflation", 0.5f, WCVarFlags::Default, "How much to inflate bounds during occlusion check.");
WCVarFloat cvar_SpatialCullingOcclusionFarPlane("Spatial.Occlusion.FarPlane", 50.0f, WCVarFlags::Default, "Far plane distance for finding occluders.");

WRenderPipeline::WRenderPipeline(WDynamicArray<WUniquePtr<WRenderPipelinePass>>&& passes, WDynamicArray<WUniquePtr<WExtractor>>&& extractors, WArrayPtr<const WRenderPipelineResourceLoaderConnection> connections)
  : m_PassGraph(std::move(passes), std::move(extractors), connections)
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  m_AverageCullingTime = WTime::MakeFromSeconds(0.1f);
#endif

  for (const WUniquePtr<WRenderPipelinePass>& pPass : m_PassGraph.GetPasses())
  {
    pPass->m_pPipeline = this;
  }

  WRenderGraphManager::s_RenderEvent.AddEventHandler(WMakeDelegate(&WRenderPipeline::OnRenderEvent, this));
}

WRenderPipeline::~WRenderPipeline()
{
  WRenderGraphManager::s_RenderEvent.RemoveEventHandler(WMakeDelegate(&WRenderPipeline::OnRenderEvent, this));

  WGALDevice::GetDefaultDevice()->DestroyTexture(m_hOcclusionDebugViewTexture);

  m_Data[0].Clear();
  m_Data[1].Clear();
}

void WRenderPipeline::OnRenderEvent(const WRenderGraphRenderEvent& e)
{
  if (e.m_Type == WRenderGraphRenderEvent::Type::BeforeGraphExecution && e.m_pGraph == m_pRenderGraph)
  {
    UpdateRenderContext(*e.m_pContext);
  }
  else if (e.m_Type == WRenderGraphRenderEvent::Type::AfterGraphExecution && e.m_pGraph == m_pRenderGraph)
  {
    {
      WRenderWorldRenderEvent renderEvent;
      renderEvent.m_Type = WRenderWorldRenderEvent::Type::AfterPipelineExecution;
      renderEvent.m_pRenderViewContext = &m_RenderViewContext;
      renderEvent.m_uiFrameCounter = WRenderWorld::GetFrameCounter();

      W_PROFILE_SCOPE("AfterPipelineExecution");
      WRenderWorld::s_RenderEvent.Broadcast(renderEvent);
    }

    auto& data = m_Data[WRenderWorld::GetDataIndexForRendering()];
    data.Clear();
    m_CurrentRenderThread = (WThreadID)0;
  }
}

void WRenderPipeline::GetPasses(WDynamicArray<const WRenderPipelinePass*>& ref_passes) const
{
  ref_passes.Reserve(ref_passes.GetCount() + m_PassGraph.GetPasses().GetCount());

  for (const WUniquePtr<WRenderPipelinePass>& pPass : m_PassGraph.GetPasses())
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

void WRenderPipeline::GetPasses(WDynamicArray<WRenderPipelinePass*>& ref_passes)
{
  ref_passes.Reserve(ref_passes.GetCount() + m_PassGraph.GetPasses().GetCount());

  for (const WUniquePtr<WRenderPipelinePass>& pPass : m_PassGraph.GetPasses())
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

WRenderPipelinePass* WRenderPipeline::GetPassByName(const WStringView& sPassName)
{
  return m_PassGraph.GetPassByName(sPassName);
}

WHashedString WRenderPipeline::GetViewName() const
{
  return m_sName;
}

bool WRenderPipeline::ShouldRender() const
{
  auto& data = m_Data[WRenderWorld::GetDataIndexForRendering()];

  const WWorld* pWorld = WWorld::GetWorld(data.GetWorldHandle());
  if (pWorld == nullptr)
    return false;

  return true;
}

WRenderPipeline::PipelineState WRenderPipeline::Rebuild(const WView& view)
{
  WLogBlock b("WRenderPipeline::Rebuild");

  bool bRes = RebuildInternal(view);
  if (!bRes)
    m_PipelineState = PipelineState::RebuildError;
  return m_PipelineState;
}

bool WRenderPipeline::RebuildInternal(const WView& view)
{
  UpdateViewData(view, WRenderWorld::GetDataIndexForRendering());
  if (m_PassGraph.CullDeadPasses().Failed() || m_PassGraph.SortPasses().Failed())
    return false;
  m_PipelineState = PipelineState::Initialized;
  return true;
}

bool WRenderPipeline::RebuildRenderGraph(const WViewData& viewData, const WCamera& camera)
{
  m_uiSettingsModificationCounter = camera.GetSettingsModificationCounter();

  if (!m_pRenderGraph)
  {
    m_pRenderGraph = WRenderGraphManager::CreateRenderGraph("RenderPipeline");
    m_pRenderGraph->SetUserData(&m_RenderViewContext);
  }

  m_pRenderGraph->SetUserName(viewData.m_sName);
  m_pRenderGraph->Reset();
  if (!AddRenderPasses(viewData, camera))
    return false;
  if (!UpdateTextureProviders())
    return false;

  m_PipelineState = PipelineState::RenderGraphBuilt;
  return true;
}

bool WRenderPipeline::AddRenderPasses(const WViewData& viewData, const WCamera& camera)
{
  const WStatus res = m_PassGraph.AddRenderPasses(viewData, camera, *m_pRenderGraph);
  if (res.Failed())
  {
    WLog::Error("Failed to add render passes: {0}", res.GetMessageString());
    return false;
  }
  return true;
}

bool WRenderPipeline::UpdateTextureProviders()
{
  const WStatus res = m_PassGraph.UpdateTextureProviders(*m_pRenderGraph);
  if (res.Failed())
  {
    WLog::Error("Failed to update texture providers: {0}", res.GetMessageString());
    return false;
  }
  return true;
}


void WRenderPipeline::UpdateViewData(const WView& view, WUInt32 uiDataIndex)
{
  if (!view.IsValid())
    return;

  if (uiDataIndex == WRenderWorld::GetDataIndexForExtraction() && m_CurrentExtractThread != (WThreadID)0)
    return;

  W_ASSERT_DEV(uiDataIndex <= 1, "Data index must be 0 or 1");
  auto& data = m_Data[uiDataIndex];

  data.SetCamera(*view.GetCamera());
  data.SetViewData(view.GetData());
}

void WRenderPipeline::GetExtractors(WDynamicArray<const WExtractor*>& ref_extractors) const
{
  ref_extractors.Reserve(ref_extractors.GetCount() + m_PassGraph.GetExtractors().GetCount());

  for (const WUniquePtr<WExtractor>& pExtractor : m_PassGraph.GetExtractors())
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}

void WRenderPipeline::GetExtractors(WDynamicArray<WExtractor*>& ref_extractors)
{
  ref_extractors.Reserve(ref_extractors.GetCount() + m_PassGraph.GetExtractors().GetCount());

  for (const WUniquePtr<WExtractor>& pExtractor : m_PassGraph.GetExtractors())
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}


WExtractor* WRenderPipeline::GetExtractorByName(const WStringView& sExtractorName)
{
  return m_PassGraph.GetExtractorByName(sExtractorName);
}

WFrameDataProviderBase* WRenderPipeline::GetFrameDataProvider(const WRTTI* pRtti) const
{
  WUInt32 uiIndex = 0;
  if (m_TypeToDataProviderIndex.TryGetValue(pRtti, uiIndex))
  {
    return m_DataProviders[uiIndex].Borrow();
  }

  WUniquePtr<WFrameDataProviderBase> pNewDataProvider = pRtti->GetAllocator()->Allocate<WFrameDataProviderBase>();
  WFrameDataProviderBase* pResult = pNewDataProvider.Borrow();
  pResult->m_pOwnerPipeline = this;

  m_TypeToDataProviderIndex.Insert(pRtti, m_DataProviders.GetCount());
  m_DataProviders.PushBack(std::move(pNewDataProvider));

  return pResult;
}

void WRenderPipeline::ExtractData(const WView& view)
{
  W_ASSERT_DEV(m_CurrentExtractThread == (WThreadID)0, "Extract must not be called from multiple threads.");
  m_CurrentExtractThread = WThreadUtils::GetCurrentThreadID();

  // Is this view already extracted?
  if (m_uiLastExtractionFrame == WRenderWorld::GetFrameCounter())
  {
    W_REPORT_FAILURE("View '{0}' is extracted multiple times", view.GetName());
    return;
  }

  W_PROFILE_SCOPE("WRenderPipeline::ExtractData");

  m_uiLastExtractionFrame = WRenderWorld::GetFrameCounter();

  // Determine visible objects
  FindVisibleObjects(view);

  // Extract and sort data
  auto& data = m_Data[WRenderWorld::GetDataIndexForExtraction()];

  // Usually clear is not needed, only if the multithreading flag is switched during runtime.
  data.Clear();

  WRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type = WRenderWorldExtractionEvent::Type::BeforeViewExtraction;
  extractionEvent.m_pView = &view;
  extractionEvent.m_pExtractedRenderData = &data;
  extractionEvent.m_uiFrameCounter = WRenderWorld::GetFrameCounter();
  WRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);

  // Store camera and viewdata
  data.SetCamera(*view.GetCamera());
  data.SetViewData(view.GetData());
  data.SetWorldHandle(view.GetWorld()->GetHandle());
  data.SetWorldTime(view.GetWorld()->GetClock().GetAccumulatedTime());
  data.SetWorldDebugContext(view.GetWorld());
  data.SetViewDebugContext(view.GetHandle());

  // Extract object render data
  for (const WUniquePtr<WExtractor>& pExtractor : m_PassGraph.GetExtractors())
  {
    if (pExtractor->m_bActive)
    {
      W_PROFILE_SCOPE(pExtractor->m_sName.GetData());

      pExtractor->Extract(view, m_VisibleObjects, data);
    }
  }

  for (auto& processor : m_RenderDataProcessors)
  {
    processor(data);
  }

  data.SortAndBatch();

  for (const WUniquePtr<WExtractor>& pExtractor : m_PassGraph.GetExtractors())
  {
    if (pExtractor->m_bActive)
    {
      W_PROFILE_SCOPE(pExtractor->m_sName.GetData());

      pExtractor->PostSortAndBatch(view, m_VisibleObjects, data);
    }
  }

  extractionEvent.m_Type = WRenderWorldExtractionEvent::Type::AfterViewExtraction;
  WRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);

  m_CurrentExtractThread = (WThreadID)0;
}

WUniquePtr<WRasterizerViewPool> g_pRasterizerViewPool;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, SwRasterizer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    g_pRasterizerViewPool = W_DEFAULT_NEW(WRasterizerViewPool);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    g_pRasterizerViewPool.Clear();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

void WRenderPipeline::FindVisibleObjects(const WView& view)
{
  W_PROFILE_SCOPE("WRenderPipeline::FindVisibleObjects");

  WFrustum frustum;
  view.ComputeCullingFrustum(frustum);

  W_LOCK(view.GetWorld()->GetReadMarker());

  const bool bIsMainView = (view.GetCameraUsageHint() == WCameraUsageHint::MainView || view.GetCameraUsageHint() == WCameraUsageHint::EditorView);
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const bool bRecordStats = cvar_SpatialCullingShowStats && bIsMainView;
  WSpatialSystem::QueryStats stats;
#endif

  WSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = WDefaultSpatialDataCategories::RenderStatic.GetBitmask() | WDefaultSpatialDataCategories::RenderDynamic.GetBitmask();
  queryParams.m_pIncludeTags = &view.m_IncludeTags;
  queryParams.m_pExcludeTags = &view.m_ExcludeTags;
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  queryParams.m_pStats = bRecordStats ? &stats : nullptr;
#endif

  WFrustum limitedFrustum = frustum;
  const WPlane farPlane = limitedFrustum.GetPlane(WFrustum::PlaneType::FarPlane);
  limitedFrustum.AccessPlane(WFrustum::PlaneType::FarPlane) = WPlane::MakeFromNormalAndPoint(farPlane.m_vNormal, view.GetCullingCamera()->GetCenterPosition() + farPlane.m_vNormal * cvar_SpatialCullingOcclusionFarPlane.GetValue()); // only use occluders closer than this

  WRasterizerView* pRasterizer = PrepareOcclusionCulling(limitedFrustum, view);
  W_SCOPE_EXIT(g_pRasterizerViewPool->ReturnRasterizerView(pRasterizer));

  const WVisibilityState::Enum visType = bIsMainView ? WVisibilityState::Direct : WVisibilityState::Indirect;

  if (pRasterizer != nullptr && pRasterizer->HasRasterizedAnyOccluders())
  {
    auto IsOccluded = [=](const WSimdBBox& aabb)
    {
      // grow the bbox by some percent to counter the lower precision of the occlusion buffer

      const WSimdVec4f c = aabb.GetCenter();
      const WSimdVec4f e = aabb.GetHalfExtents();
      const WSimdBBox aabb2 = WSimdBBox::MakeFromCenterAndHalfExtents(c, e.CompMul(WSimdVec4f(1.0f + cvar_SpatialCullingOcclusionBoundsInlation)));

      return !pRasterizer->IsVisible(aabb2);
    };

    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, IsOccluded, visType);
  }
  else
  {
    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, {}, visType);
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (pRasterizer)
  {
    if (view.GetCameraUsageHint() == WCameraUsageHint::EditorView || view.GetCameraUsageHint() == WCameraUsageHint::MainView)
    {
      PreviewOcclusionBuffer(*pRasterizer, view);
    }
  }
#endif

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  WViewHandle hView = view.GetHandle();

  if (cvar_SpatialCullingVis && bIsMainView)
  {
    WDebugRenderer::DrawLineFrustum(view.GetWorld(), frustum, WColor::LimeGreen, false);
  }

  if (bRecordStats)
  {
    WStringBuilder sb;

    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "VisCulling", "Visibility Culling Stats", WColor::LimeGreen);

    sb.SetFormat("Total Num Objects: {0}", stats.m_uiTotalNumObjects);
    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "VisCulling", sb, WColor::LimeGreen);

    sb.SetFormat("Num Objects Tested: {0}", stats.m_uiNumObjectsTested);
    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "VisCulling", sb, WColor::LimeGreen);

    sb.SetFormat("Num Objects Passed: {0}", stats.m_uiNumObjectsPassed);
    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "VisCulling", sb, WColor::LimeGreen);

    // Exponential moving average for better readability.
    m_AverageCullingTime = WMath::Lerp(m_AverageCullingTime, stats.m_TimeTaken, 0.05f);

    sb.SetFormat("Time Taken: {0}ms", m_AverageCullingTime.GetMilliseconds());
    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "VisCulling", sb, WColor::LimeGreen);

    view.GetWorld()->GetSpatialSystem()->GetInternalStats(sb);
    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "VisCulling", sb, WColor::AntiqueWhite);
  }

  if (cvar_RenderingShowStats && bIsMainView)
  {
    // these numbers are gathered while rendering, so they are one frame old and they cover all views and passes,
    // including shadow map rendering
    const WRenderContext::Statistics& renderStats = WRenderContext::GetLastFrameStatistics();

    WStringBuilder sb;

    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "RenderStats", "Render Stats (whole frame)", WColor::Yellow);

    sb.SetFormat("Drawcalls: {0}", renderStats.m_uiDrawcalls);
    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "RenderStats", sb, WColor::Yellow);

    sb.SetFormat("Triangles: {0}", WArgHumanReadable((WInt64)renderStats.m_uiTriangles));
    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "RenderStats", sb, WColor::Yellow);
  }
#endif
}

void WRenderPipeline::EnqueueRenderGraph(WRenderContext* pRenderContext)
{
  WLogBlock b("EnqueueRenderGraph");

  W_PROFILE_SCOPE(m_sName.GetData());

  WUInt32 uiDataIndex = WRenderWorld::GetDataIndexForRendering();
  auto& data = m_Data[uiDataIndex];

  W_ASSERT_DEV(m_PipelineState >= PipelineState::Initialized, "Pipeline must be rebuild before rendering.");
  m_PipelineState = PipelineState::Initialized;

  if (!RebuildRenderGraph(data.GetViewData(), data.GetCamera()))
  {
    WLog::Error("Failed call to RebuildRenderGraph, pipeline rendering aborted");
    return;
  }

  if (m_pRenderGraph->ValidateImportedResources().Failed())
  {
    WLog::Error("Imported resources are no longer valid, pipeline rendering aborted");
    m_PipelineState = PipelineState::RebuildError;
    return;
  }

  // Apply view dependencies: import shared textures with the required initial state.
  for (const WTextureDependency& dep : data.GetTextureViewDependencies())
  {
    m_pRenderGraph->ImportTexture(dep.m_hTexture, dep.m_RequiredState, dep.m_Stage);
  }

  // Apply view dependencies: import shared buffers with the required initial state.
  for (const WBufferDependency& dep : data.GetBufferViewDependencies())
  {
    m_pRenderGraph->ImportBuffer(dep.m_hBuffer, dep.m_RequiredState, dep.m_Stage);
  }

  W_ASSERT_DEV(m_CurrentRenderThread == (WThreadID)0, "Render must not be called from multiple threads.");
  m_CurrentRenderThread = WThreadUtils::GetCurrentThreadID();

  W_ASSERT_DEV(m_uiLastRenderFrame != WRenderWorld::GetFrameCounter(), "Render must not be called multiple times per frame.");
  m_uiLastRenderFrame = WRenderWorld::GetFrameCounter();
  m_pRenderGraph->SetUserName(m_sName);
  WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
}

void WRenderPipeline::UpdateRenderContext(WRenderGraphContext& ctx)
{
  W_ASSERT_DEV(m_CurrentRenderThread == WThreadUtils::GetCurrentThreadID(), "Graph executed on wrong thread");
  auto pRenderContext = ctx.GetRenderContext();
  auto& data = m_Data[WRenderWorld::GetDataIndexForRendering()];
  const WCamera* pCamera = &data.GetCamera();
  const WViewData* pViewData = &data.GetViewData();

  auto& gc = pRenderContext->WriteGlobalConstants();
  for (int i = 0; i < 2; ++i)
  {
    gc.CameraToScreenMatrix[i] = pViewData->m_ProjectionMatrix[i];
    gc.ScreenToCameraMatrix[i] = pViewData->m_InverseProjectionMatrix[i];
    gc.WorldToCameraMatrix[i] = pViewData->m_ViewMatrix[i];
    gc.CameraToWorldMatrix[i] = pViewData->m_InverseViewMatrix[i];
    gc.WorldToScreenMatrix[i] = pViewData->m_ViewProjectionMatrix[i];
    gc.ScreenToWorldMatrix[i] = pViewData->m_InverseViewProjectionMatrix[i];
  }

  const WRectFloat& viewport = pViewData->m_ViewPortRect;
  gc.ViewportSize = WVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);

  float fNear = pCamera->GetNearPlane();
  float fFar = pCamera->GetFarPlane();
  gc.ClipPlanes = WVec4(fNear, fFar, 1.0f / fFar, 0.0f);

  const bool bIsShadowPass = pViewData->m_CameraUsageHint == WCameraUsageHint::Shadow;
  const bool bIsDirectionalLightShadow = bIsShadowPass && pCamera->IsOrthographic();
  gc.MaxZValue = bIsDirectionalLightShadow ? 0.0f : WMath::MinValue<float>();

  gc.Exposure = pCamera->GetExposure();
  gc.RenderPass = WViewRenderMode::GetRenderPassForShader(pViewData->m_ViewRenderMode);
  gc.IsShadowPass = bIsShadowPass;

  pRenderContext->SetGlobalAndWorldTimeConstants(data.GetWorldTime());

  m_RenderViewContext.m_pPipeline = this;
  m_RenderViewContext.m_pCamera = pCamera;
  m_RenderViewContext.m_pViewData = pViewData;
  m_RenderViewContext.m_pRenderContext = pRenderContext;
  m_RenderViewContext.m_pWorldDebugContext = &data.GetWorldDebugContext();
  m_RenderViewContext.m_pViewDebugContext = &data.GetViewDebugContext();

  W_ASSERT_DEBUG(WWorld::GetWorld(data.GetWorldHandle()) != nullptr, "Trying to render a deleted world");

  // Set camera mode permutation variable here since it doesn't change throughout the frame
  static WHashedString sCameraMode = WMakeHashedString("CAMERA_MODE");
  static WHashedString sOrtho = WMakeHashedString("CAMERA_MODE_ORTHO");
  static WHashedString sPerspective = WMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static WHashedString sStereo = WMakeHashedString("CAMERA_MODE_STEREO");

  static WHashedString sClipSpaceFlipped = WMakeHashedString("CLIP_SPACE_FLIPPED");
  static WHashedString sTrue = WMakeHashedString("TRUE");
  static WHashedString sFalse = WMakeHashedString("FALSE");

  if (pCamera->IsOrthographic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sOrtho);
  else if (pCamera->IsStereoscopic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sStereo);
  else
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sPerspective);

  W_ASSERT_DEV(pCamera->IsStereoscopic() == false || WGALDevice::GetDefaultDevice()->GetCapabilities().m_bSupportsVSRenderTargetArrayIndex, "Vertex shader render target index must be supported for stereo rendering.");

  pRenderContext->SetShaderPermutationVariable(sClipSpaceFlipped, WClipSpaceYMode::RenderToTextureDefault == WClipSpaceYMode::Flipped ? sTrue : sFalse);

  // Also set pipeline specific permutation vars
  for (auto& var : m_PermutationVars)
  {
    pRenderContext->SetShaderPermutationVariable(var.m_sName, var.m_sValue);
  }

  // Apply bindings
  WBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_FRAME);
  for (const WSamplerBinding& binding : data.GetSamplerBindings())
  {
    bindGroup.BindSampler(binding.m_sSlotName, binding.m_Sampler.m_hSampler);
  }
  for (const WBufferBinding& binding : data.GetBufferBindings())
  {
    bindGroup.BindBuffer(binding.m_sSlotName, binding.m_Buffer.m_hBuffer, binding.m_Buffer.m_BufferRange, binding.m_Buffer.m_OverrideTexelBufferFormat);
  }
  for (const WTextureBinding& binding : data.GetTextureBindings())
  {
    bindGroup.BindTexture(binding.m_sSlotName, binding.m_Texture.m_hTexture, binding.m_Texture.m_TextureRange, binding.m_Texture.m_OverrideViewFormat, binding.m_Texture.m_OverrideViewType);
  }

  {
    WRenderWorldRenderEvent renderEvent;
    renderEvent.m_Type = WRenderWorldRenderEvent::Type::BeforePipelineExecution;
    renderEvent.m_pRenderViewContext = &m_RenderViewContext;
    renderEvent.m_uiFrameCounter = WRenderWorld::GetFrameCounter();

    W_PROFILE_SCOPE("BeforePipelineExecution");
    WRenderWorld::s_RenderEvent.Broadcast(renderEvent);
  }
}

const WExtractedRenderData& WRenderPipeline::GetRenderData() const
{
  return m_Data[WRenderWorld::GetDataIndexForRendering()];
}

void WRenderPipeline::AddViewDependency(WGALTextureHandle hTexture, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage)
{
  m_Data[WRenderWorld::GetDataIndexForExtraction()].AddViewDependency(hTexture, requiredState, stage);
}

void WRenderPipeline::AddViewDependency(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage)
{
  m_Data[WRenderWorld::GetDataIndexForExtraction()].AddViewDependency(hBuffer, requiredState, stage);
}

WRenderDataBatchList WRenderPipeline::GetRenderDataBatchesWithCategory(WRenderData::Category category) const
{
  auto& data = m_Data[WRenderWorld::GetDataIndexForRendering()];
  return data.GetRenderDataBatchesWithCategory(category);
}

WArrayPtr<const WTextureDependency> WRenderPipeline::GetTextureDependenciesWithCategory(WRenderData::Category category) const
{
  auto& data = m_Data[WRenderWorld::GetDataIndexForRendering()];
  return data.GetTextureDependenciesWithCategory(category);
}

WArrayPtr<const WBufferDependency> WRenderPipeline::GetBufferDependenciesWithCategory(WRenderData::Category category) const
{
  auto& data = m_Data[WRenderWorld::GetDataIndexForRendering()];
  return data.GetBufferDependenciesWithCategory(category);
}

WUInt32 WRenderPipeline::AddRenderDataProcessor(RenderDataProcessor processor)
{
  WUInt32 uiIndex = m_RenderDataProcessors.GetCount();
  m_RenderDataProcessors.PushBack(processor);
  return uiIndex;
}

void WRenderPipeline::CreateDgmlGraph(WDGMLGraph& ref_graph)
{
  /*
  WStringBuilder sTmp;
  WHashTable<const WRenderPipelineNode*, WUInt32> nodeMap;
  nodeMap.Reserve(m_Passes.GetCount() + m_TextureUsage.GetCount() * 3);
  for (WUInt32 p = 0; p < m_Passes.GetCount(); ++p)
  {
    const auto& pPass = m_Passes[p];
    sTmp.SetFormat("#{}: {}", p, WStringUtils::IsNullOrEmpty(pPass->GetName()) ? pPass->GetDynamicRTTI()->GetTypeName() : pPass->GetName());

    WDGMLGraph::NodeDesc nd;
    nd.m_Color = WColor::Gray;
    nd.m_Shape = WDGMLGraph::NodeShape::Rectangle;
    WUInt32 uiGraphNode = ref_graph.AddNode(sTmp, &nd);
    nodeMap.Insert(pPass.Borrow(), uiGraphNode);
  }

  for (WUInt32 i = 0; i < m_TextureUsage.GetCount(); ++i)
  {
    const TextureUsageData& data = m_TextureUsage[i];

    for (const WRenderPipelinePassConnection* pCon : data.m_UsedBy)
    {
      WDGMLGraph::NodeDesc nd;
      nd.m_Color = data.m_pTextureProvider ? WColor::Black : WColorScheme::GetColor(static_cast<WColorScheme::Enum>(i % WColorScheme::Count), 4);
      nd.m_Shape = WDGMLGraph::NodeShape::RoundedRectangle;

      WStringBuilder sFormat;
      if (!WReflectionUtils::EnumerationToString(WGetStaticRTTI<WGALResourceFormat>(), pCon->m_Desc.m_Format, sFormat, WReflectionUtils::EnumConversionMode::ValueNameOnly))
      {
        sFormat.SetFormat("Unknown Format {}", (int)pCon->m_Desc.m_Format);
      }
      sTmp.SetFormat("{} #{}: {}x{}:{}, MSAA:{}, {}Format: {}", data.m_pTextureProvider ? "External" : "PoolTexture", i, pCon->m_Desc.m_uiWidth, pCon->m_Desc.m_uiHeight, pCon->m_Desc.m_uiArraySize, (int)pCon->m_Desc.m_SampleCount, WGALResourceFormat::IsDepthFormat(pCon->m_Desc.m_Format) ? "Depth" : "Color", sFormat);
      WUInt32 uiTextureNode = ref_graph.AddNode(sTmp, &nd);

      WUInt32 uiOutputNode = *nodeMap.GetValue(pCon->m_pOutput->m_pParent);
      ref_graph.AddConnection(uiOutputNode, uiTextureNode, pCon->m_pOutput->m_pParent->GetPinName(pCon->m_pOutput));
      for (const WRenderPipelineNodePin* pInput : pCon->m_Inputs)
      {
        WUInt32 uiInputNode = *nodeMap.GetValue(pInput->m_pParent);
        ref_graph.AddConnection(uiTextureNode, uiInputNode, pInput->m_pParent->GetPinName(pInput));
      }
    }
  }
  */
}

WRasterizerView* WRenderPipeline::PrepareOcclusionCulling(const WFrustum& frustum, const WView& view)
{
#if W_ENABLED(W_PLATFORM_ARCH_X86)
  if (!cvar_SpatialCullingOcclusionEnable)
    return nullptr;

  auto& cpuFeatures = WSystemInformation::Get().GetCpuFeatures();
  if (!cpuFeatures.IsAvx1Available() || !cpuFeatures.HW_FMA3)
    return nullptr;

  WRasterizerView* pRasterizer = nullptr;

  // extract all occlusion geometry from the scene
  W_PROFILE_SCOPE("PrepareOcclusionCulling");

  pRasterizer = g_pRasterizerViewPool->GetRasterizerView(static_cast<WUInt32>(view.GetViewport().width / 2), static_cast<WUInt32>(view.GetViewport().height / 2), (float)view.GetViewport().width / (float)view.GetViewport().height);
  pRasterizer->SetCamera(view.GetCullingCamera());

  {
    W_PROFILE_SCOPE("FindOccluders");

    WSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = WDefaultSpatialDataCategories::OcclusionStatic.GetBitmask() | WDefaultSpatialDataCategories::OcclusionDynamic.GetBitmask();
    queryParams.m_pIncludeTags = &view.m_IncludeTags;
    queryParams.m_pExcludeTags = &view.m_ExcludeTags;

    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, {}, WVisibilityState::Indirect);
  }

  pRasterizer->BeginScene();

  {
    W_PROFILE_SCOPE("ExtractOccluders");

    for (const WGameObject* pObj : m_VisibleObjects)
    {
      WMsgExtractOccluderData msg;
      pObj->SendMessage(msg);

      for (const auto& ed : msg.m_ExtractedOccluderData)
      {
        pRasterizer->AddObject(ed.m_pObject, ed.m_Transform);
      }
    }
  }

  pRasterizer->EndScene();

  return pRasterizer;
#else
  return nullptr;
#endif
}

void WRenderPipeline::PreviewOcclusionBuffer(const WRasterizerView& rasterizer, const WView& view)
{
  if (!cvar_SpatialCullingOcclusionVisView || !rasterizer.HasRasterizedAnyOccluders())
    return;

  W_PROFILE_SCOPE("Occlusion::DebugPreview");

  const WUInt32 uiImgWidth = rasterizer.GetResolutionX();
  const WUInt32 uiImgHeight = rasterizer.GetResolutionY();

  // get the debug image from the rasterizer
  WDynamicArray<WColorLinearUB> fb;
  fb.SetCountUninitialized(uiImgWidth * uiImgHeight);
  rasterizer.ReadBackFrame(fb);

  const float w = (float)uiImgWidth;
  const float h = (float)uiImgHeight;
  WRectFloat rectInPixel1 = WRectFloat(5.0f, 5.0f, w + 10, h + 10);
  WRectFloat rectInPixel2 = WRectFloat(10.0f, 10.0f, w, h);

  WDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel1, 0.0f, WColor::MediumPurple);

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  // check whether we need to re-create the texture
  if (!m_hOcclusionDebugViewTexture.IsInvalidated())
  {
    const WGALTexture* pTexture = pDevice->GetTexture(m_hOcclusionDebugViewTexture);

    if (pTexture->GetDescription().m_uiWidth != uiImgWidth ||
        pTexture->GetDescription().m_uiHeight != uiImgHeight)
    {
      pDevice->DestroyTexture(m_hOcclusionDebugViewTexture);
    }
  }

  // create the texture
  if (m_hOcclusionDebugViewTexture.IsInvalidated())
  {
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = uiImgWidth;
    desc.m_uiHeight = uiImgHeight;
    desc.m_Format = WGALResourceFormat::RGBAUByteNormalized;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hOcclusionDebugViewTexture = pDevice->CreateTexture(desc);
  }

  // upload the image to the texture
  {
    WGALSystemMemoryDescription sourceData;
    sourceData.m_pData = fb.GetByteArrayPtr();
    sourceData.m_uiRowPitch = uiImgWidth * sizeof(WColorLinearUB);

    pDevice->UpdateTextureForNextFrame(m_hOcclusionDebugViewTexture, sourceData);
  }

  WDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel2, 0.0f, WColor::White, m_hOcclusionDebugViewTexture, WVec2(1, -1));
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipeline);
