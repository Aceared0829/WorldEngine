
#pragma once

#include <RendererFoundation/State/State.h>


struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11RasterizerState2;
struct ID3D11SamplerState;

class W_RENDERERDX11_DLL WGALBlendStateDX11 : public WGALBlendState
{
public:
  W_ALWAYS_INLINE ID3D11BlendState* GetDXBlendState() const;

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  WGALBlendStateDX11(const WGALBlendStateCreationDescription& Description);

  ~WGALBlendStateDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  ID3D11BlendState* m_pDXBlendState = nullptr;
};

class W_RENDERERDX11_DLL WGALDepthStencilStateDX11 : public WGALDepthStencilState
{
public:
  W_ALWAYS_INLINE ID3D11DepthStencilState* GetDXDepthStencilState() const;

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  WGALDepthStencilStateDX11(const WGALDepthStencilStateCreationDescription& Description);

  ~WGALDepthStencilStateDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  ID3D11DepthStencilState* m_pDXDepthStencilState = nullptr;
};

class W_RENDERERDX11_DLL WGALRasterizerStateDX11 : public WGALRasterizerState
{
public:
  W_ALWAYS_INLINE ID3D11RasterizerState* GetDXRasterizerState() const;

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  WGALRasterizerStateDX11(const WGALRasterizerStateCreationDescription& Description);

  ~WGALRasterizerStateDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  ID3D11RasterizerState* m_pDXRasterizerState = nullptr;
};

class W_RENDERERDX11_DLL WGALSamplerStateDX11 : public WGALSamplerState
{
public:
  W_ALWAYS_INLINE ID3D11SamplerState* GetDXSamplerState() const;

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  WGALSamplerStateDX11(const WGALSamplerStateCreationDescription& Description);

  ~WGALSamplerStateDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  ID3D11SamplerState* m_pDXSamplerState = nullptr;
};


#include <RendererDX11/State/Implementation/StateDX11_inl.h>
