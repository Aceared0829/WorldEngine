
#pragma once

#include <RendererFoundation/Resources/Texture.h>

class W_RENDERERFOUNDATION_DLL WGALProxyTexture : public WGALTexture
{
public:
  virtual ~WGALProxyTexture();

  virtual const WGALResourceBase* GetParentResource() const override;
  WGALTextureHandle GetParentTextureHandle() const { return m_hParentTexture; }
  WUInt16 GetSlice() const { return m_uiSlice; }

protected:
  friend class WGALDevice;

  WGALProxyTexture(WGALTextureHandle hParentTexture, const WGALTexture& parentTexture, WUInt16 uiSlice);

  virtual WResult InitPlatform(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> pInitialData) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  WGALTextureHandle m_hParentTexture;
  const WGALTexture* m_pParentTexture;
  WUInt16 m_uiSlice = 0;
};
