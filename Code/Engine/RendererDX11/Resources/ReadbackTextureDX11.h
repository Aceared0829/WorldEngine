#pragma once

#include <RendererFoundation/Resources/ReadbackTexture.h>

struct ID3D11Resource;
struct D3D11_TEXTURE2D_DESC;
struct D3D11_TEXTURE3D_DESC;
struct D3D11_SUBRESOURCE_DATA;
class WGALDeviceDX11;

class WGALReadbackTextureDX11 : public WGALReadbackTexture
{
public:
  W_ALWAYS_INLINE ID3D11Resource* GetDXTexture() const { return m_pDXTexture; }

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  WGALReadbackTextureDX11(const WGALTextureCreationDescription& Description);
  ~WGALReadbackTextureDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  ID3D11Resource* m_pDXTexture = nullptr;
};
