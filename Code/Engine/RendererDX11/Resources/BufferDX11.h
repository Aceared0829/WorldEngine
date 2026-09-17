
#pragma once

#include <RendererFoundation/Resources/Buffer.h>
#include <dxgi.h>

struct ID3D11Buffer;
struct D3D11_BUFFER_DESC;

class W_RENDERERDX11_DLL WGALBufferDX11 : public WGALBuffer
{
public:
  static WResult CreateBufferDesc(const WGALBufferCreationDescription& description, D3D11_BUFFER_DESC& out_bufferDesc, DXGI_FORMAT& out_indexFormat);

public:
  ID3D11Buffer* GetDXBuffer() const;
  DXGI_FORMAT GetIndexFormat() const;
  ID3D11ShaderResourceView* GetSRV(WGALBufferRange bufferRange, WEnum<WGALShaderResourceType> resourceType, WEnum<WGALResourceFormat> overrideTexelBufferFormat) const;
  ID3D11UnorderedAccessView* GetUAV(WGALBufferRange bufferRange, WEnum<WGALShaderResourceType> resourceType, WEnum<WGALResourceFormat> overrideTexelBufferFormat) const;

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  WGALBufferDX11(const WGALBufferCreationDescription& Description);
  virtual ~WGALBufferDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice, WArrayPtr<const WUInt8> pInitialData) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  WGALDeviceDX11* m_pDevice = nullptr;
  ID3D11Buffer* m_pDXBuffer = nullptr;
  DXGI_FORMAT m_IndexFormat = DXGI_FORMAT_UNKNOWN; // Only applicable for index buffers

  // Views
  struct View : WHashableStruct<View>
  {
    WGALBufferRange m_BufferRange;
    WEnum<WGALShaderResourceType> m_ResourceType;
    WEnum<WGALResourceFormat> m_OverrideTexelBufferFormat;

    W_ALWAYS_INLINE static WUInt32 Hash(const View& value) { return value.CalculateHash(); }
    W_ALWAYS_INLINE static bool Equal(const View& a, const View& b) { return a == b; }
  };
  mutable WHashTable<View, ID3D11ShaderResourceView*, View> m_SRVs;
  mutable WHashTable<View, ID3D11UnorderedAccessView*, View> m_UAVs;
};

#include <RendererDX11/Resources/Implementation/BufferDX11_inl.h>
