#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/RendererReflection.h>

WTextureViewContext::WTextureViewContext(WTextureContext* pContext)
  : WEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
}

WTextureViewContext::~WTextureViewContext() = default;

WViewHandle WTextureViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Texture Editor - View");
  pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
  pView->GetBlackboard()->SetEntryValue(WMakeHashedString("DepthPrePass.Active"), false);
  pView->GetBlackboard()->SetEntryValue(WMakeHashedString("AOPass.Active"), false);

  return pView->GetHandle();
}

void WTextureViewContext::SetCamera(const WViewRedrawMsgToEngine* pMsg)
{
  // Do not apply render mode here otherwise we would switch to a different pipeline.
  // Also use hard-coded clipping planes so the quad is not culled too early.

  WCameraMode::Enum cameraMode = (WCameraMode::Enum)pMsg->m_iCameraMode;
  m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, 0.0001f, 50.0f);
  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  // Draw some stats
  auto hResource = m_pTextureContext->GetTexture();
  if (hResource.IsValid())
  {
    WResourceLock<WTexture2DResource> pResource(hResource, WResourceAcquireMode::AllowLoadingFallback);
    const WGALResourceFormat::Enum format = pResource->GetFormat();
    const int iMipLevel = m_pTextureContext->GetLodLevel();
    const WUInt32 uiWidth = pResource->GetWidth();
    const WUInt32 uiHeight = pResource->GetHeight();

    WStringBuilder sText;
    if (!WReflectionUtils::EnumerationToString(WGetStaticRTTI<WGALResourceFormat>(), format, sText, WReflectionUtils::EnumConversionMode::ValueNameOnly))
    {
      sText = "Unknown format";
    }

    const WUInt32 uiArraySlices = m_pTextureContext->GetNumArraySlices();
    if (uiArraySlices > 1)
    {
      sText.PrependFormat("{}x{} [{} slices] - ", uiWidth, uiHeight, uiArraySlices);
    }
    else
    {
      sText.PrependFormat("{}x{} - ", uiWidth, uiHeight);
    }
    sText.Append("\nPreview Mip Level: ");

    if (iMipLevel < 0)
    {
      sText.Append("Auto");
    }
    else
    {
      const WUInt32 uiMipWidth = WMath::Max(1u, uiWidth >> WMath::Max(iMipLevel, 0));
      const WUInt32 uiMipHeight = WMath::Max(1u, uiHeight >> WMath::Max(iMipLevel, 0));

      sText.AppendFormat("{} ({}x{})", iMipLevel, uiMipWidth, uiMipHeight);
    }

    WDebugRenderer::DrawInfoText(m_hView, WDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
