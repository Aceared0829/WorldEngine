#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <Core/Utils/Blackboard.h>
#include <EnginePluginScene/RenderPipeline/EditorSelectedObjectsExtractor.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/SwapChain.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorSelectedObjectsExtractor, 1, WRTTIDefaultAllocator<WEditorSelectedObjectsExtractor>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WEditorSelectedObjectsExtractor::WEditorSelectedObjectsExtractor()
{
  m_pSceneContext = nullptr;
}

WEditorSelectedObjectsExtractor::~WEditorSelectedObjectsExtractor()
{
  WRenderWorld::DeleteView(m_hRenderTargetView);
}

const WDeque<WGameObjectHandle>* WEditorSelectedObjectsExtractor::GetSelection()
{
  if (m_pSceneContext == nullptr)
    return nullptr;

  return &m_pSceneContext->GetSelectionWithChildren();
}

void WEditorSelectedObjectsExtractor::Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData)
{
  const bool bShowCameraOverlays = view.GetCameraUsageHint() == WCameraUsageHint::EditorView;

  if (bShowCameraOverlays && m_pSceneContext && m_pSceneContext->GetRenderSelectionBoxes())
  {
    const WDeque<WGameObjectHandle>* pSelection = GetSelection();
    if (pSelection == nullptr)
      return;

    const WCameraComponent* pCamComp = nullptr;

    CreateRenderTargetTexture(view);

    W_LOCK(view.GetWorld()->GetReadMarker());

    for (const auto& hObj : *pSelection)
    {
      const WGameObject* pObject = nullptr;
      if (!view.GetWorld()->TryGetObject(hObj, pObject))
        continue;

      if (FilterByViewTags(view, pObject))
        continue;

      if (pObject->TryGetComponentOfBaseType(pCamComp))
      {
        UpdateRenderTargetCamera(pCamComp);

        WResourceLock<WRenderToTexture2DResource> pRT(m_hRenderTarget, WResourceAcquireMode::AllowLoadingFallback);
        if (pRT.GetAcquireResult() == WResourceAcquireResult::Final)
        {
          const float fAspect = 9.0f / 16.0f;

          // TODO: use aspect ratio of camera render target, if available
          WDebugRenderer::Draw2DRectangle(view.GetHandle(), WRectFloat(20, 20, 256, 256 * fAspect), 0, WColor::White, m_hRenderTarget);

          // TODO: if the camera renders to a texture anyway, use its view + render target instead

          WRenderWorld::AddViewToRender(m_hRenderTargetView);

          // The consumer view will sample this render target as a texture.
          WRenderWorld::AddViewDependency(view, pRT->GetGALTexture(), WGALResourceState::ShaderResource);
        }

        break;
      }
    }
  }

  WSelectedObjectsExtractorBase::Extract(view, visibleObjects, ref_extractedRenderData);
}

WResult WEditorSelectedObjectsExtractor::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return W_SUCCESS;
}

WResult WEditorSelectedObjectsExtractor::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  return W_SUCCESS;
}

void WEditorSelectedObjectsExtractor::CreateRenderTargetTexture(const WView& view)
{
  if (m_hRenderTarget.IsValid())
    return;

  m_hRenderTarget = WResourceManager::GetExistingResource<WRenderToTexture2DResource>("EditorCameraRT");

  if (!m_hRenderTarget.IsValid())
  {
    const float fAspect = 9.0f / 16.0f;
    const WUInt32 uiWidth = 256;

    WRenderToTexture2DResourceDescriptor d;
    d.m_Format = WGALResourceFormat::RGBAUByteNormalizedsRGB;
    d.m_uiWidth = uiWidth;
    d.m_uiHeight = (WUInt32)(uiWidth * fAspect);

    m_hRenderTarget = WResourceManager::GetOrCreateResource<WRenderToTexture2DResource>("EditorCameraRT", std::move(d));
  }

  CreateRenderTargetView(view);
}

void WEditorSelectedObjectsExtractor::CreateRenderTargetView(const WView& view)
{
  W_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  WResourceLock<WRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, WResourceAcquireMode::BlockTillLoaded);

  WStringBuilder name("EditorCameraRT");

  WView* pRenderTargetView = nullptr;
  m_hRenderTargetView = WRenderWorld::CreateView(name, pRenderTargetView);

  // MainRenderPipeline.WRenderPipelineAsset
  auto hRenderPipeline = WResourceManager::LoadResource<WRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
  pRenderTargetView->SetRenderPipelineResource(hRenderPipeline);

  // TODO: get rid of const cast ?
  pRenderTargetView->SetWorld(const_cast<WWorld*>(view.GetWorld()));
  pRenderTargetView->SetCamera(&m_RenderTargetCamera);

  m_RenderTargetCamera.SetCameraMode(WCameraMode::PerspectiveFixedFovY, 45, 0.1f, 100.0f);

  WGALRenderTargets renderTargets;
  renderTargets.m_hRTs[0] = pRenderTarget->GetGALTexture();
  pRenderTargetView->SetRenderTargets(renderTargets);

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  pRenderTargetView->SetViewport(WRectFloat(0, 0, resX, resY));
}

void WEditorSelectedObjectsExtractor::UpdateRenderTargetCamera(const WCameraComponent* pCamComp)
{
  float fFarPlane = WMath::Max(pCamComp->GetNearPlane() + 0.00001f, pCamComp->GetFarPlane());
  switch (pCamComp->GetCameraMode())
  {
    case WCameraMode::OrthoFixedHeight:
    case WCameraMode::OrthoFixedWidth:
      m_RenderTargetCamera.SetCameraMode(pCamComp->GetCameraMode(), pCamComp->GetOrthoDimension(), pCamComp->GetNearPlane(), fFarPlane);
      break;
    case WCameraMode::PerspectiveFixedFovX:
    case WCameraMode::PerspectiveFixedFovY:
      m_RenderTargetCamera.SetCameraMode(pCamComp->GetCameraMode(), pCamComp->GetFieldOfView(), pCamComp->GetNearPlane(), fFarPlane);
      break;
    case WCameraMode::Stereo:
      m_RenderTargetCamera.SetCameraMode(WCameraMode::PerspectiveFixedFovY, 45, pCamComp->GetNearPlane(), fFarPlane);
      break;
    default:
      break;
  }


  WView* pRenderTargetView = nullptr;
  if (!WRenderWorld::TryGetView(m_hRenderTargetView, pRenderTargetView))
    return;

  pRenderTargetView->m_IncludeTags = pCamComp->m_IncludeTags;
  pRenderTargetView->m_ExcludeTags = pCamComp->m_ExcludeTags;
  pRenderTargetView->m_ExcludeTags.SetByName("Editor");

  if (pCamComp->GetRenderPipeline().IsValid())
  {
    pRenderTargetView->SetRenderPipelineResource(pCamComp->GetRenderPipeline());
  }
  else
  {
    // MainRenderPipeline.WRenderPipelineAsset
    auto hRenderPipeline = WResourceManager::LoadResource<WRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
    pRenderTargetView->SetRenderPipelineResource(hRenderPipeline);
  }

  pRenderTargetView->SetBlackboard(pCamComp->GetBlackboard());

  const WVec3 pos = pCamComp->GetOwner()->GetGlobalPosition();
  const WVec3 dir = pCamComp->GetOwner()->GetGlobalDirForwards();
  const WVec3 up = pCamComp->GetOwner()->GetGlobalDirUp();

  m_RenderTargetCamera.LookAt(pos, pos + dir, up);
  m_RenderTargetCamera.SetExposure(pCamComp->GetExposure());
}
