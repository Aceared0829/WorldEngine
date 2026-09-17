#pragma once

#include <RendererFoundation/Resources/ReadbackBuffer.h>

struct ID3D11Buffer;

class WGALReadbackBufferDX11 : public WGALReadbackBuffer
{
public:
  W_ALWAYS_INLINE ID3D11Buffer* GetDXBuffer() const { return m_pDXBuffer; }

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  WGALReadbackBufferDX11(const WGALBufferCreationDescription& Description);
  ~WGALReadbackBufferDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  ID3D11Buffer* m_pDXBuffer = nullptr;
  DXGI_FORMAT m_IndexFormat = DXGI_FORMAT_UNKNOWN; // Only applicable for index buffers
};
