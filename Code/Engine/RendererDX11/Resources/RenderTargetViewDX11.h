
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class WGALRenderTargetViewDX11 : public WGALRenderTargetView
{
public:
  W_ALWAYS_INLINE ID3D11RenderTargetView* GetRenderTargetView() const;

  W_ALWAYS_INLINE ID3D11DepthStencilView* GetDepthStencilView() const;

  W_ALWAYS_INLINE ID3D11UnorderedAccessView* GetUnorderedAccessView() const;

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  WGALRenderTargetViewDX11(WGALTexture* pTexture, const WGALRenderTargetViewCreationDescription& Description);

  virtual ~WGALRenderTargetViewDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  ID3D11RenderTargetView* m_pRenderTargetView = nullptr;

  ID3D11DepthStencilView* m_pDepthStencilView = nullptr;

  ID3D11UnorderedAccessView* m_pUnorderedAccessView = nullptr;
};

#include <RendererDX11/Resources/Implementation/RenderTargetViewDX11_inl.h>
