#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/RendererReflection.h>

WTextureCubeViewContext::WTextureCubeViewContext(WTextureCubeContext* pContext)
  : WEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
}

WTextureCubeViewContext::~WTextureCubeViewContext() = default;

WViewHandle WTextureCubeViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Texture Cube Editor - View");
  pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
  pView->GetBlackboard()->SetEntryValue(WMakeHashedString("DepthPrePass.Active"), false);
  pView->GetBlackboard()->SetEntryValue(WMakeHashedString("AOPass.Active"), false);

  return pView->GetHandle();
}

void WTextureCubeViewContext::SetCamera(const WViewRedrawMsgToEngine* pMsg)
{
  // Do not apply render mode here otherwise we would switch to a different pipeline.
  // Also use hard-coded clipping planes so the quad is not culled to early.

  WCameraMode::Enum cameraMode = (WCameraMode::Enum)pMsg->m_iCameraMode;
  m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, 0.0001f, 50.0f);
  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  // Draw some stats
  auto hResource = m_pTextureContext->GetTexture();
  if (hResource.IsValid())
  {
    WResourceLock<WTextureCubeResource> pResource(hResource, WResourceAcquireMode::AllowLoadingFallback);
    WGALResourceFormat::Enum format = pResource->GetFormat();
    WUInt32 uiWidthAndHeight = pResource->GetWidthAndHeight();

    WStringBuilder sText;
    if (!WReflectionUtils::EnumerationToString(WGetStaticRTTI<WGALResourceFormat>(), format, sText, WReflectionUtils::EnumConversionMode::ValueNameOnly))
    {
      sText = "Unknown format";
    }

    sText.PrependFormat("{0}x{1}x6 - ", uiWidthAndHeight, uiWidthAndHeight);

    WDebugRenderer::DrawInfoText(m_hView, WDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
