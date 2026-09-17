#pragma once

#include <RendererFoundation/Resources/Texture.h>

struct ID3D11Resource;
struct D3D11_TEXTURE2D_DESC;
struct D3D11_TEXTURE3D_DESC;
struct D3D11_SUBRESOURCE_DATA;
class WGALDeviceDX11;

W_DEFINE_AS_POD_TYPE(D3D11_SUBRESOURCE_DATA);

class WGALTextureDX11 : public WGALTexture
{
public:
  static WResult Create2DDesc(const WGALTextureCreationDescription& description, WGALDeviceDX11* pDXDevice, D3D11_TEXTURE2D_DESC& out_tex2DDesc);
  static WResult Create3DDesc(const WGALTextureCreationDescription& description, WGALDeviceDX11* pDXDevice, D3D11_TEXTURE3D_DESC& out_tex3DDesc);
  static void ConvertInitialData(const WGALTextureCreationDescription& description, WArrayPtr<WGALSystemMemoryDescription> initialData, WHybridArray<D3D11_SUBRESOURCE_DATA, 16>& out_initialData);

public:
  W_ALWAYS_INLINE ID3D11Resource* GetDXTexture() const;
  ID3D11ShaderResourceView* GetSRV(WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat, WEnum<WGALTextureType> overrideViewType) const;
  ID3D11UnorderedAccessView* GetUAV(WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat) const;

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;
  friend class WGALSharedTextureDX11;

  WGALTextureDX11(const WGALTextureCreationDescription& Description);
  ~WGALTextureDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> initialData) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

  WResult InitFromNativeObject(WGALDeviceDX11* pDXDevice);

protected:
  WGALDeviceDX11* m_pDevice = nullptr;
  ID3D11Resource* m_pDXTexture = nullptr;

  struct View : WHashableStruct<View>
  {
    WGALTextureRange m_TextureRange;
    WEnum<WGALResourceFormat> m_OverrideViewFormat;
    WEnum<WGALTextureType> m_OverrideViewType;

    W_ALWAYS_INLINE static WUInt32 Hash(const View& value) { return value.CalculateHash(); }
    W_ALWAYS_INLINE static bool Equal(const View& a, const View& b) { return a == b; }
  };
  mutable WHashTable<View, ID3D11ShaderResourceView*, View> m_SRVs;
  mutable WHashTable<View, ID3D11UnorderedAccessView*, View> m_UAVs;
};



#include <RendererDX11/Resources/Implementation/TextureDX11_inl.h>
