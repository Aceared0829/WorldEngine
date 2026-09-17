
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class W_RENDERERFOUNDATION_DLL WGALRenderTargetView : public WGALObject<WGALRenderTargetViewCreationDescription>
{
public:
  W_ALWAYS_INLINE WGALTexture* GetTexture() const { return m_pTexture; }

protected:
  friend class WGALDevice;

  WGALRenderTargetView(WGALTexture* pTexture, const WGALRenderTargetViewCreationDescription& description);

  virtual ~WGALRenderTargetView();

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;

  WGALTexture* m_pTexture;
};
