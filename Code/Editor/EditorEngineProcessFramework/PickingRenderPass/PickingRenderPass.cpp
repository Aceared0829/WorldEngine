#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/PickingRenderPass/PickingRenderPass.h>
#include <RendererCore/Pipeline/SortingFunctions.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPickingRenderPass, 1, WRTTIDefaultAllocator<WPickingRenderPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("PickSelected", m_bPickSelected),
    W_MEMBER_PROPERTY("PickTransparent", m_bPickTransparent),
    W_MEMBER_PROPERTY("PickingPosition", m_PickingPosition),
    W_MEMBER_PROPERTY("MarqueePickPos0", m_MarqueePickPosition0),
    W_MEMBER_PROPERTY("MarqueePickPos1", m_MarqueePickPosition1),
    W_MEMBER_PROPERTY("MarqueeActionID", m_uiMarqueeActionID),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static WRenderData::Category s_LitOpaqueWithoutSelection = WRenderData::RegisterDerivedCategory("LitOpaqueWithoutSelection", WDefaultRenderDataCategories::LitOpaqueStatic);
static WRenderData::Category s_LitMaskedWithoutSelection = WRenderData::RegisterDerivedCategory("LitMaskedWithoutSelection", WDefaultRenderDataCategories::LitMaskedStatic);
static WRenderData::Category s_LitMaskedDynamicWithoutSelection = WRenderData::RegisterDerivedCategory("LitMaskedDynamicWithoutSelection", WDefaultRenderDataCategories::LitMaskedDynamic);

static WRenderData::Category s_LitTransparentWithoutSelection = WRenderData::RegisterDerivedCategory("LitTransparentWithoutSelection", WDefaultRenderDataCategories::LitTransparent);
static WRenderData::Category s_SimpleTransparentWithoutSelection = WRenderData::RegisterDerivedCategory("SimpleTransparentWithoutSelection", WDefaultRenderDataCategories::SimpleTransparent);

WPickingRenderPass::WPickingRenderPass()
  : WRenderPipelinePass("EditorPickingRenderPass")
{
  m_pGridRenderDataType = WRTTI::FindTypeByName("WGridRenderData");
  W_ASSERT_DEV(m_pGridRenderDataType != nullptr, "WGridRenderData type not found. Type renamed?");
}

WPickingRenderPass::~WPickingRenderPass()
{
  DestroyTarget();
}

WGALTextureHandle WPickingRenderPass::GetPickingIdRT() const
{
  return m_hPickingIdRT;
}

WGALTextureHandle WPickingRenderPass::GetPickingDepthRT() const
{
  return m_hPickingDepthRT;
}

WStatus WPickingRenderPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  m_TargetRect = viewData.m_ViewPortRect;
  DestroyTarget();
  CreateTarget();

  if (m_uiProcessorId == WInvalidIndex)
  {
    m_uiProcessorId = GetPipeline()->AddRenderDataProcessor(WMakeDelegate(&WPickingRenderPass::ProcessPickingRenderData, this));
  }

  m_uiWindowWidth = (WUInt32)m_TargetRect.width;
  m_uiWindowHeight = (WUInt32)m_TargetRect.height;

  const WGALTexture* pDepthTexture = WGALDevice::GetDefaultDevice()->GetTexture(m_hPickingDepthRT);
  W_ASSERT_DEV(m_uiWindowWidth == pDepthTexture->GetDescription().m_uiWidth, "");
  W_ASSERT_DEV(m_uiWindowHeight == pDepthTexture->GetDescription().m_uiHeight, "");

  m_hPickingIdGraphRT = ref_graph.ImportTexture(m_hPickingIdRT);
  m_hPickingDepthGraphRT = ref_graph.ImportTexture(m_hPickingDepthRT);
  {
    auto pass = ref_graph.AddGraphicsPass(GetName());
    pass.AddColorTarget(m_hPickingIdGraphRT, {}, WGALRenderTargetLoadOp::Clear);
    pass.AddDepthStencilTarget(m_hPickingDepthGraphRT, {}, WGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0);
    pass.SetClearDepth().SetClearStencil();
    pass.HasSideEffects();

    DeclareRendererDependenciesForCategory(s_LitOpaqueWithoutSelection, ref_graph, pass);
    DeclareRendererDependenciesForCategory(s_LitMaskedWithoutSelection, ref_graph, pass);
    if (m_bPickTransparent)
    {
      DeclareRendererDependenciesForCategory(s_LitTransparentWithoutSelection, ref_graph, pass);
      DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitForeground, ref_graph, pass);
    }
    if (m_bPickSelected)
    {
      DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::Selection, ref_graph, pass);
    }
    DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::SimpleOpaque, ref_graph, pass);
    if (m_bPickTransparent)
    {
      DeclareRendererDependenciesForCategory(s_SimpleTransparentWithoutSelection, ref_graph, pass);
    }
    DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::SimpleForeground, ref_graph, pass);

    pass.SetExecuteCallback([this](const WRenderGraphContext& ctx)
      {
        const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
        renderViewContext.UpdateViewport();

        WViewRenderMode::Enum viewRenderMode = renderViewContext.m_pViewData->m_ViewRenderMode;
        if (viewRenderMode == WViewRenderMode::WireframeColor || viewRenderMode == WViewRenderMode::WireframeMonochrome)
          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING_WIREFRAME");
        else
          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING");

        RenderDataWithCategory(renderViewContext, s_LitOpaqueWithoutSelection);
        RenderDataWithCategory(renderViewContext, s_LitMaskedWithoutSelection);

        if (m_bPickTransparent)
        {
          RenderDataWithCategory(renderViewContext, s_LitTransparentWithoutSelection);

          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
          RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitForeground);

          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
          RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitForeground);
        }

        if (m_bPickSelected)
        {
          RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::Selection);
        }

        RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::SimpleOpaque);

        if (m_bPickTransparent)
        {
          RenderDataWithCategory(renderViewContext, s_SimpleTransparentWithoutSelection);
        }

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
        RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::SimpleForeground);

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
        RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::SimpleForeground);

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_FORWARD"); //
      });
  }

  if (m_PendingReadback.m_bReadbackInProgress)
  {
    // Poll the readback non-blockingly (4ms budget). If the results aren't ready yet, bail out
    // and try again next frame, rather than stalling the editor process on the GPU.
    {
      if (m_PendingReadback.m_PickingReadback.GetReadbackResult(WTime::MakeFromMilliseconds(4)) != WGALAsyncResult::Ready)
        return W_SUCCESS;

      if (m_PendingReadback.m_PickingDepthReadback.GetReadbackResult(WTime::MakeFromMilliseconds(4)) != WGALAsyncResult::Ready)
        return W_SUCCESS;

      m_PendingReadback.m_bReadbackInProgress = false;
    }
    auto pass = ref_graph.AddTransferPass("PickingProcessResults");
    pass.HasSideEffects();
    pass.SetExecuteCallback([this](const WRenderGraphContext& ctx)
      {
        WGALTextureSubresource sourceSubResource;
        WArrayPtr<WGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
        WTempHybridArray<WGALSystemMemoryDescription, 1> memory;

        m_PickingResultsDepth.Clear();
        m_PickingResultsID.Clear();
        m_mPickingInverseViewProjectionMatrix = WMat4::MakeZero();
        // If the resolution has changed, discard the readback result.
        if (m_uiWindowHeight == m_PendingReadback.m_uiWindowHeight && m_uiWindowWidth == m_PendingReadback.m_uiWindowWidth)
        {
          {
            m_PickingResultsDepth.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);
            WReadbackTextureLock lock = m_PendingReadback.m_PickingDepthReadback.LockTexture(sourceSubResources, memory);
            W_ASSERT_ALWAYS(lock, "Failed to lock readback texture");
            const WGALTexture* pReadbackTexture = WGALDevice::GetDefaultDevice()->GetTexture(GetPickingDepthRT());
            WTextureUtils::CopySubResourceToMemory(pReadbackTexture->GetDescription(), sourceSubResource, memory[0], m_PickingResultsDepth.GetByteArrayPtr(), m_uiWindowWidth * sizeof(float));
          }
          {
            m_PickingResultsID.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);
            WReadbackTextureLock lock = m_PendingReadback.m_PickingReadback.LockTexture(sourceSubResources, memory);
            W_ASSERT_ALWAYS(lock, "Failed to lock readback texture");
            const WGALTexture* pReadbackTexture = WGALDevice::GetDefaultDevice()->GetTexture(GetPickingIdRT());
            WTextureUtils::CopySubResourceToMemory(pReadbackTexture->GetDescription(), sourceSubResource, memory[0], m_PickingResultsID.GetByteArrayPtr(), m_uiWindowWidth * sizeof(WUInt32));
          }
          m_mPickingInverseViewProjectionMatrix = m_PendingReadback.m_mPickingInverseViewProjectionMatrix;
        } //
      });
  }

  // Start transferring the picking information from the GPU to the CPU
  if (m_uiWindowWidth != 0 && m_uiWindowHeight != 0)
  {
    auto pass = ref_graph.AddTransferPass("PickingReadback");
    pass.ReadTexture(m_hPickingIdGraphRT, {}, WGALResourceState::CopySource);
    pass.ReadTexture(m_hPickingDepthGraphRT, {}, WGALResourceState::CopySource);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this](const WRenderGraphContext& ctx)
      {
        const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
        m_PendingReadback.m_PickingReadback.ReadbackTexture(*ctx.GetCommandEncoder(), ctx.ResolveTexture(m_hPickingIdGraphRT));
        m_PendingReadback.m_PickingDepthReadback.ReadbackTexture(*ctx.GetCommandEncoder(), ctx.ResolveTexture(m_hPickingDepthGraphRT));
        ctx.GetCommandEncoder()->Flush();

        WMat4 mProj;
        renderViewContext.m_pCamera->GetProjectionMatrix((float)m_uiWindowWidth / m_uiWindowHeight, mProj);
        WMat4 mView = renderViewContext.m_pCamera->GetViewMatrix();

        if (mProj.IsNaN())
          return;

        WMat4 inv = mProj * mView;
        if (inv.Invert(0).Failed())
        {
          WLog::Warning("Inversion of View-Projection-Matrix failed. Picking results will be wrong.");
          return;
        }

        m_PendingReadback.m_mPickingInverseViewProjectionMatrix = inv;
        m_PendingReadback.m_uiWindowWidth = m_uiWindowWidth;
        m_PendingReadback.m_uiWindowHeight = m_uiWindowHeight;
        m_PendingReadback.m_bReadbackInProgress = true; //
      });
  }
  return W_SUCCESS;
}

void WPickingRenderPass::ReadBackProperties(WView* pView)
{
  ReadBackPropertiesSinglePick(pView);
  ReadBackPropertiesMarqueePick(pView);
}

void WPickingRenderPass::CreateTarget()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  // Create render target for picking
  WGALTextureCreationDescription tcd;
  tcd.m_TextureFlags = WGALTextureUsageFlags::RenderTarget | WGALTextureUsageFlags::ShaderResource;
  tcd.m_Format = WGALResourceFormat::RGBAUByteNormalized;
  tcd.m_Type = WGALTextureType::Texture2D;
  tcd.m_uiWidth = (WUInt32)m_TargetRect.width;
  tcd.m_uiHeight = (WUInt32)m_TargetRect.height;
  tcd.m_ResourceAccess.m_bImmutable = false;

  m_hPickingIdRT = pDevice->CreateTexture(tcd);

  tcd.m_Format = WGALResourceFormat::DFloat;

  m_hPickingDepthRT = pDevice->CreateTexture(tcd);
}

void WPickingRenderPass::DestroyTarget()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  pDevice->DestroyTexture(m_hPickingIdRT);
  pDevice->DestroyTexture(m_hPickingDepthRT);
}

void WPickingRenderPass::ReadBackPropertiesSinglePick(WView* pView)
{
  const WUInt32 x = (WUInt32)m_PickingPosition.x;
  const WUInt32 y = (WUInt32)m_PickingPosition.y;
  const WUInt32 uiIndex = (y * m_uiWindowWidth) + x;

  if (uiIndex >= m_PickingResultsDepth.GetCount() || x >= m_uiWindowWidth || y >= m_uiWindowHeight)
  {
    // WLog::Error("Picking position {0}, {1} is outside the available picking area of {2} * {3}", x, y, m_uiWindowWidth,
    // m_uiWindowHeight);
    return;
  }

  m_PickingPosition.Set(-1);

  WVec3 vNormal(0);
  WVec3 vPickingRayStartPosition(0);
  WVec3 vPickedPosition(0);
  {
    const float fDepth = m_PickingResultsDepth[uiIndex];
    WGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, WVec3((float)x, (float)y, fDepth), vPickedPosition).IgnoreResult();
    WGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, WVec3((float)x, (float)y, 0), vPickingRayStartPosition).IgnoreResult();

    float fOtherDepths[4] = {fDepth, fDepth, fDepth, fDepth};
    WVec3 vOtherPos[4];
    WVec3 vNormals[4];

    if ((WUInt32)x + 1 < m_uiWindowWidth)
      fOtherDepths[0] = m_PickingResultsDepth[(y * m_uiWindowWidth) + x + 1];
    if (x > 0)
      fOtherDepths[1] = m_PickingResultsDepth[(y * m_uiWindowWidth) + x - 1];
    if ((WUInt32)y + 1 < m_uiWindowHeight)
      fOtherDepths[2] = m_PickingResultsDepth[((y + 1) * m_uiWindowWidth) + x];
    if (y > 0)
      fOtherDepths[3] = m_PickingResultsDepth[((y - 1) * m_uiWindowWidth) + x];

    WGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, WVec3((float)(x + 1), (float)y, fOtherDepths[0]), vOtherPos[0]).IgnoreResult();
    WGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, WVec3((float)(x - 1), (float)y, fOtherDepths[1]), vOtherPos[1]).IgnoreResult();
    WGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, WVec3((float)x, (float)(y + 1), fOtherDepths[2]), vOtherPos[2]).IgnoreResult();
    WGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, WVec3((float)x, (float)(y - 1), fOtherDepths[3]), vOtherPos[3]).IgnoreResult();

    vNormals[0].CalculateNormal(vPickedPosition, vOtherPos[0], vOtherPos[2]).IgnoreResult();
    vNormals[1].CalculateNormal(vPickedPosition, vOtherPos[2], vOtherPos[1]).IgnoreResult();
    vNormals[2].CalculateNormal(vPickedPosition, vOtherPos[1], vOtherPos[3]).IgnoreResult();
    vNormals[3].CalculateNormal(vPickedPosition, vOtherPos[3], vOtherPos[0]).IgnoreResult();

    vNormal = vNormals[0] + vNormals[1] + vNormals[2] + vNormals[3];
    vNormal.NormalizeIfNotZero().IgnoreResult();
  }

  WUInt32 uiPickID = m_PickingResultsID[uiIndex];
  if (uiPickID == 0)
  {
    for (WInt32 radius = 1; radius < 10; ++radius)
    {
      WInt32 left = WMath::Max<WInt32>(x - radius, 0);
      WInt32 right = WMath::Min<WInt32>(x + radius, m_uiWindowWidth - 1);
      WInt32 top = WMath::Max<WInt32>(y - radius, 0);
      WInt32 bottom = WMath::Min<WInt32>(y + radius, m_uiWindowHeight - 1);

      for (WInt32 xt = left; xt <= right; ++xt)
      {
        const WUInt32 idxt = (top * m_uiWindowWidth) + xt;

        uiPickID = m_PickingResultsID[idxt];

        if (uiPickID != 0)
          goto done;
      }

      for (WInt32 xt = left; xt <= right; ++xt)
      {
        const WUInt32 idxt = (bottom * m_uiWindowWidth) + xt;

        uiPickID = m_PickingResultsID[idxt];

        if (uiPickID != 0)
          goto done;
      }
    }

  done:;
  }

  SetReadBackProperty(pView, "PickedMatrix", m_mPickingInverseViewProjectionMatrix);
  SetReadBackProperty(pView, "PickedID", uiPickID);
  SetReadBackProperty(pView, "PickedDepth", m_PickingResultsDepth[uiIndex]);
  SetReadBackProperty(pView, "PickedNormal", vNormal);
  SetReadBackProperty(pView, "PickedRayStartPosition", vPickingRayStartPosition);
  SetReadBackProperty(pView, "PickedPosition", vPickedPosition);
}

void WPickingRenderPass::ReadBackPropertiesMarqueePick(WView* pView)
{
  const WUInt32 x0 = (WUInt32)m_MarqueePickPosition0.x;
  const WUInt32 y0 = (WUInt32)m_MarqueePickPosition0.y;
  const WUInt32 x1 = (WUInt32)m_MarqueePickPosition1.x;
  const WUInt32 y1 = (WUInt32)m_MarqueePickPosition1.y;
  const WUInt32 uiIndex1 = (y0 * m_uiWindowWidth) + x0;
  const WUInt32 uiIndex2 = (y0 * m_uiWindowWidth) + x0;

  if ((uiIndex1 >= m_PickingResultsDepth.GetCount() || x0 >= m_uiWindowWidth || y0 >= m_uiWindowHeight) || (uiIndex2 >= m_PickingResultsDepth.GetCount() || x1 >= m_uiWindowWidth || y1 >= m_uiWindowHeight))
  {
    return;
  }

  // We must not reset the marquee pick positions here since the marquee action is active over multiple frames and the view only sets properties when they change.
  // They are reseted manually by the editor when the marquee action is finished.
  // m_MarqueePickPosition0.Set(-1);
  // m_MarqueePickPosition1.Set(-1);
  SetReadBackProperty(pView, "MarqueeResultActionID", m_uiMarqueeActionID);

  WTempHybridArray<WUInt32, 32> IDs;
  WVariantArray resArray;

  const WUInt32 lowX = WMath::Min(x0, x1);
  const WUInt32 highX = WMath::Max(x0, x1);
  const WUInt32 lowY = WMath::Min(y0, y1);
  const WUInt32 highY = WMath::Max(y0, y1);

  WUInt32 offset = 0;

  for (WUInt32 y = lowY; y < highY; y += 1)
  {
    for (WUInt32 x = lowX + offset; x < highX; x += 2)
    {
      const WUInt32 uiIndex = (y * m_uiWindowWidth) + x;

      const WUInt32 id = m_PickingResultsID[uiIndex];

      // prevent duplicates
      if (IDs.Contains(id))
        continue;

      IDs.PushBack(id);
      resArray.PushBack(id);
    }

    // only evaluate every second pixel, in a checker board pattern
    offset = (offset + 1) % 2;
  }

  SetReadBackProperty(pView, "MarqueeResult", resArray);
}

void WPickingRenderPass::ProcessPickingRenderData(WExtractedRenderData& extractedRenderData)
{
  // copy selection to set for faster checks
  m_SelectionSet.Clear();
  {
    auto renderDataList = extractedRenderData.GetRawRenderDataWithCategory(WDefaultRenderDataCategories::Selection);
    for (auto& sortableRenderData : renderDataList)
    {
      m_SelectionSet.Insert(sortableRenderData.m_pRenderData->m_hOwner);
    }
  }

  auto Filter = [&](WRenderData::Category originalCategory, WRenderData::Category filteredCategory)
  {
    auto renderDataList = extractedRenderData.GetRawRenderDataWithCategory(originalCategory);
    for (auto& sortableRenderData : renderDataList)
    {
      auto pRenderData = sortableRenderData.m_pRenderData;
      if (m_SelectionSet.Contains(pRenderData->m_hOwner) || pRenderData->IsInstanceOf(m_pGridRenderDataType))
        continue;

      extractedRenderData.AddRenderData(pRenderData, filteredCategory);
    }

    WArrayPtr<const WTextureDependency> textureDependencies = extractedRenderData.GetTextureDependenciesWithCategory(originalCategory);
    for (WTextureDependency dependency : textureDependencies)
    {
      dependency.m_uiCategory = filteredCategory.m_uiValue;
      extractedRenderData.AddDependency(dependency);
    }

    WArrayPtr<const WBufferDependency> bufferDependencies = extractedRenderData.GetBufferDependenciesWithCategory(originalCategory);
    for (WBufferDependency dependency : bufferDependencies)
    {
      dependency.m_uiCategory = filteredCategory.m_uiValue;
      extractedRenderData.AddDependency(dependency);
    }
  };

  Filter(WDefaultRenderDataCategories::LitOpaqueStatic, s_LitOpaqueWithoutSelection);
  Filter(WDefaultRenderDataCategories::LitOpaqueDynamic, s_LitOpaqueWithoutSelection);

  Filter(WDefaultRenderDataCategories::LitMaskedStatic, s_LitMaskedWithoutSelection);
  Filter(WDefaultRenderDataCategories::LitMaskedDynamic, s_LitOpaqueWithoutSelection);

  if (m_bPickTransparent)
  {
    Filter(WDefaultRenderDataCategories::LitTransparent, s_LitTransparentWithoutSelection);
    Filter(WDefaultRenderDataCategories::SimpleTransparent, s_SimpleTransparentWithoutSelection);
  }
}
