
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>

struct ID3D11InputLayout;

class WGALVertexDeclarationDX11 : public WGALVertexDeclaration
{
public:
  W_ALWAYS_INLINE ID3D11InputLayout* GetDXInputLayout() const;
  W_ALWAYS_INLINE WArrayPtr<const WUInt32> GetVertexBufferStrides() const;

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  virtual WResult InitPlatform(WGALDevice* pDevice) override;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  WGALVertexDeclarationDX11(const WGALVertexDeclarationCreationDescription& Description);

  virtual ~WGALVertexDeclarationDX11();

  ID3D11InputLayout* m_pDXInputLayout = nullptr;
  WHybridArray<WUInt32, W_GAL_MAX_VERTEX_BUFFER_COUNT> m_VertexBufferStrides;
};

#include <RendererDX11/Shader/Implementation/VertexDeclarationDX11_inl.h>
