
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;

class W_RENDERERDX11_DLL WGALShaderDX11 : public WGALShader
{
public:
  void SetDebugName(WStringView sName) const override;

  W_ALWAYS_INLINE ID3D11VertexShader* GetDXVertexShader() const;

  W_ALWAYS_INLINE ID3D11HullShader* GetDXHullShader() const;

  W_ALWAYS_INLINE ID3D11DomainShader* GetDXDomainShader() const;

  W_ALWAYS_INLINE ID3D11GeometryShader* GetDXGeometryShader() const;

  W_ALWAYS_INLINE ID3D11PixelShader* GetDXPixelShader() const;

  W_ALWAYS_INLINE ID3D11ComputeShader* GetDXComputeShader() const;

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  WGALShaderDX11(const WGALShaderCreationDescription& description);

  virtual ~WGALShaderDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  ID3D11VertexShader* m_pVertexShader = nullptr;
  ID3D11HullShader* m_pHullShader = nullptr;
  ID3D11DomainShader* m_pDomainShader = nullptr;
  ID3D11GeometryShader* m_pGeometryShader = nullptr;
  ID3D11PixelShader* m_pPixelShader = nullptr;
  ID3D11ComputeShader* m_pComputeShader = nullptr;
};

#include <RendererDX11/Shader/Implementation/ShaderDX11_inl.h>
