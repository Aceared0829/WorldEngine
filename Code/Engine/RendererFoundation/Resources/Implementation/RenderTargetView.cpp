#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/RenderTargetView.h>


WGALRenderTargetView::WGALRenderTargetView(WGALTexture* pTexture, const WGALRenderTargetViewCreationDescription& description)
  : WGALObject(description)
  , m_pTexture(pTexture)
{
  W_ASSERT_DEV(m_pTexture != nullptr, "Texture must not be null");
}

WGALRenderTargetView::~WGALRenderTargetView() = default;
