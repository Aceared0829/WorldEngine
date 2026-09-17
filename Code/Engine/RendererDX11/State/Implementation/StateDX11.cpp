#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererDX11/State/StateDX11.h>

#include <d3d11.h>
#include <d3d11_3.h>


// Mapping tables to map WGAL constants to DX11 constants
#include <RendererDX11/State/Implementation/StateDX11_MappingTables.inl>

// Blend state

WGALBlendStateDX11::WGALBlendStateDX11(const WGALBlendStateCreationDescription& Description)
  : WGALBlendState(Description)

{
}

WGALBlendStateDX11::~WGALBlendStateDX11() = default;

static D3D11_BLEND_OP ToD3DBlendOp(WGALBlendOp::Enum e)
{
  switch (e)
  {
    case WGALBlendOp::Add:
      return D3D11_BLEND_OP_ADD;
    case WGALBlendOp::Max:
      return D3D11_BLEND_OP_MAX;
    case WGALBlendOp::Min:
      return D3D11_BLEND_OP_MIN;
    case WGALBlendOp::RevSubtract:
      return D3D11_BLEND_OP_REV_SUBTRACT;
    case WGALBlendOp::Subtract:
      return D3D11_BLEND_OP_SUBTRACT;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  return D3D11_BLEND_OP_ADD;
}

static D3D11_BLEND ToD3DBlend(WGALBlend::Enum e)
{
  switch (e)
  {
    case WGALBlend::BlendFactor:
      W_ASSERT_NOT_IMPLEMENTED;
      // if this is used, it also must be implemented in WGALContextDX11::SetBlendStatePlatform
      return D3D11_BLEND_BLEND_FACTOR;
    case WGALBlend::DestAlpha:
      return D3D11_BLEND_DEST_ALPHA;
    case WGALBlend::DestColor:
      return D3D11_BLEND_DEST_COLOR;
    case WGALBlend::InvBlendFactor:
      W_ASSERT_NOT_IMPLEMENTED;
      // if this is used, it also must be implemented in WGALContextDX11::SetBlendStatePlatform
      return D3D11_BLEND_INV_BLEND_FACTOR;
    case WGALBlend::InvDestAlpha:
      return D3D11_BLEND_INV_DEST_ALPHA;
    case WGALBlend::InvDestColor:
      return D3D11_BLEND_INV_DEST_COLOR;
    case WGALBlend::InvSrcAlpha:
      return D3D11_BLEND_INV_SRC_ALPHA;
    case WGALBlend::InvSrcColor:
      return D3D11_BLEND_INV_SRC_COLOR;
    case WGALBlend::One:
      return D3D11_BLEND_ONE;
    case WGALBlend::SrcAlpha:
      return D3D11_BLEND_SRC_ALPHA;
    case WGALBlend::SrcAlphaSaturated:
      return D3D11_BLEND_SRC_ALPHA_SAT;
    case WGALBlend::SrcColor:
      return D3D11_BLEND_SRC_COLOR;
    case WGALBlend::Zero:
      return D3D11_BLEND_ZERO;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  return D3D11_BLEND_ONE;
}

WResult WGALBlendStateDX11::InitPlatform(WGALDevice* pDevice)
{
  D3D11_BLEND_DESC DXDesc;
  DXDesc.AlphaToCoverageEnable = m_Description.m_bAlphaToCoverage;
  DXDesc.IndependentBlendEnable = m_Description.m_bIndependentBlend;

  for (WInt32 i = 0; i < 8; ++i)
  {
    DXDesc.RenderTarget[i].BlendEnable = m_Description.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled;
    DXDesc.RenderTarget[i].BlendOp = ToD3DBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOp);
    DXDesc.RenderTarget[i].BlendOpAlpha = ToD3DBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha);
    DXDesc.RenderTarget[i].DestBlend = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlend);
    DXDesc.RenderTarget[i].DestBlendAlpha = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha);
    DXDesc.RenderTarget[i].SrcBlend = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlend);
    DXDesc.RenderTarget[i].SrcBlendAlpha = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha);
    DXDesc.RenderTarget[i].RenderTargetWriteMask = m_Description.m_RenderTargetBlendDescriptions[i].m_uiWriteMask &
                                                   0x0F; // D3D11: RenderTargetWriteMask can only have the least significant 4 bits set.
  }

  if (FAILED(static_cast<WGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateBlendState(&DXDesc, &m_pDXBlendState)))
  {
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WGALBlendStateDX11::DeInitPlatform(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);

  W_GAL_DX11_RELEASE(m_pDXBlendState);
  return W_SUCCESS;
}

// Depth Stencil state

WGALDepthStencilStateDX11::WGALDepthStencilStateDX11(const WGALDepthStencilStateCreationDescription& Description)
  : WGALDepthStencilState(Description)

{
}

WGALDepthStencilStateDX11::~WGALDepthStencilStateDX11() = default;

WResult WGALDepthStencilStateDX11::InitPlatform(WGALDevice* pDevice)
{
  D3D11_DEPTH_STENCIL_DESC DXDesc;
  DXDesc.DepthEnable = m_Description.m_bDepthEnable;
  DXDesc.DepthWriteMask = m_Description.m_bDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
  DXDesc.DepthFunc = GALCompareFuncToDX11[m_Description.m_DepthTestFunc];
  DXDesc.StencilEnable = m_Description.m_bStencilEnable;
  DXDesc.StencilReadMask = m_Description.m_uiStencilReadMask;
  DXDesc.StencilWriteMask = m_Description.m_uiStencilWriteMask;

  DXDesc.FrontFace.StencilFailOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_FailOp];
  DXDesc.FrontFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_DepthFailOp];
  DXDesc.FrontFace.StencilPassOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_PassOp];
  DXDesc.FrontFace.StencilFunc = GALCompareFuncToDX11[m_Description.m_FrontFaceStencilOp.m_StencilFunc];

  const WGALStencilOpDescription& backFaceStencilOp = m_Description.m_BackFaceStencilOp;
  DXDesc.BackFace.StencilFailOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_FailOp];
  DXDesc.BackFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_DepthFailOp];
  DXDesc.BackFace.StencilPassOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_PassOp];
  DXDesc.BackFace.StencilFunc = GALCompareFuncToDX11[backFaceStencilOp.m_StencilFunc];


  if (FAILED(static_cast<WGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateDepthStencilState(&DXDesc, &m_pDXDepthStencilState)))
  {
    return W_FAILURE;
  }
  else
  {
    return W_SUCCESS;
  }
}

WResult WGALDepthStencilStateDX11::DeInitPlatform(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);

  W_GAL_DX11_RELEASE(m_pDXDepthStencilState);
  return W_SUCCESS;
}


// Rasterizer state

WGALRasterizerStateDX11::WGALRasterizerStateDX11(const WGALRasterizerStateCreationDescription& Description)
  : WGALRasterizerState(Description)

{
}

WGALRasterizerStateDX11::~WGALRasterizerStateDX11() = default;



WResult WGALRasterizerStateDX11::InitPlatform(WGALDevice* pDevice)
{
  const bool NeedsStateDesc2 = m_Description.m_bConservativeRasterization;

  if (NeedsStateDesc2)
  {
    D3D11_RASTERIZER_DESC2 DXDesc2;
    DXDesc2.CullMode = GALCullModeToDX11[m_Description.m_CullMode];
    DXDesc2.DepthBias = m_Description.m_iDepthBias;
    DXDesc2.DepthBiasClamp = m_Description.m_fDepthBiasClamp;
    DXDesc2.DepthClipEnable = TRUE;
    DXDesc2.FillMode = m_Description.m_bWireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    DXDesc2.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
    DXDesc2.MultisampleEnable = TRUE;
    DXDesc2.AntialiasedLineEnable = TRUE;
    DXDesc2.ScissorEnable = m_Description.m_bScissorTest;
    DXDesc2.SlopeScaledDepthBias = m_Description.m_fSlopeScaledDepthBias;
    DXDesc2.ConservativeRaster =
      m_Description.m_bConservativeRasterization ? D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    DXDesc2.ForcedSampleCount = 0;

    if (!pDevice->GetCapabilities().m_bSupportsConservativeRasterization && m_Description.m_bConservativeRasterization)
    {
      WLog::Error("Rasterizer state description enables conservative rasterization which is not available!");
      return W_FAILURE;
    }

    ID3D11RasterizerState2* pDXRasterizerState2 = nullptr;

    if (FAILED(static_cast<WGALDeviceDX11*>(pDevice)->GetDXDevice3()->CreateRasterizerState2(&DXDesc2, &pDXRasterizerState2)))
    {
      return W_FAILURE;
    }
    else
    {
      m_pDXRasterizerState = pDXRasterizerState2;
      return W_SUCCESS;
    }
  }
  else
  {
    D3D11_RASTERIZER_DESC DXDesc;
    DXDesc.CullMode = GALCullModeToDX11[m_Description.m_CullMode];
    DXDesc.DepthBias = m_Description.m_iDepthBias;
    DXDesc.DepthBiasClamp = m_Description.m_fDepthBiasClamp;
    DXDesc.DepthClipEnable = TRUE;
    DXDesc.FillMode = m_Description.m_bWireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    DXDesc.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
    DXDesc.MultisampleEnable = TRUE;
    DXDesc.AntialiasedLineEnable = TRUE;
    DXDesc.ScissorEnable = m_Description.m_bScissorTest;
    DXDesc.SlopeScaledDepthBias = m_Description.m_fSlopeScaledDepthBias;

    if (FAILED(static_cast<WGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateRasterizerState(&DXDesc, &m_pDXRasterizerState)))
    {
      return W_FAILURE;
    }
    else
    {
      return W_SUCCESS;
    }
  }
}


WResult WGALRasterizerStateDX11::DeInitPlatform(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);

  W_GAL_DX11_RELEASE(m_pDXRasterizerState);
  return W_SUCCESS;
}


// Sampler state

WGALSamplerStateDX11::WGALSamplerStateDX11(const WGALSamplerStateCreationDescription& Description)
  : WGALSamplerState(Description)

{
}

WGALSamplerStateDX11::~WGALSamplerStateDX11() = default;

/*
 */

WResult WGALSamplerStateDX11::InitPlatform(WGALDevice* pDevice)
{
  WGALSamplerStateCreationDescription desc = this->GetDescription();
  pDevice->AdjustSamplerStateDescription(desc);

  D3D11_SAMPLER_DESC DXDesc;
  DXDesc.AddressU = GALTextureAddressModeToDX11[desc.m_AddressU];
  DXDesc.AddressV = GALTextureAddressModeToDX11[desc.m_AddressV];
  DXDesc.AddressW = GALTextureAddressModeToDX11[desc.m_AddressW];
  DXDesc.BorderColor[0] = desc.m_BorderColor.r;
  DXDesc.BorderColor[1] = desc.m_BorderColor.g;
  DXDesc.BorderColor[2] = desc.m_BorderColor.b;
  DXDesc.BorderColor[3] = desc.m_BorderColor.a;
  DXDesc.ComparisonFunc = GALCompareFuncToDX11[desc.m_SampleCompareFunc];

  if (desc.m_MagFilter == WGALTextureFilterMode::Anisotropic || desc.m_MinFilter == WGALTextureFilterMode::Anisotropic ||
      desc.m_MipFilter == WGALTextureFilterMode::Anisotropic)
  {
    if (desc.m_SampleCompareFunc == WGALCompareFunc::Never)
      DXDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    else
      DXDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
  }
  else
  {
    WUInt32 uiTableIndex = 0;

    if (desc.m_MipFilter == WGALTextureFilterMode::Linear)
      uiTableIndex |= 1;

    if (desc.m_MagFilter == WGALTextureFilterMode::Linear)
      uiTableIndex |= 2;

    if (desc.m_MinFilter == WGALTextureFilterMode::Linear)
      uiTableIndex |= 4;

    if (desc.m_SampleCompareFunc != WGALCompareFunc::Never)
      uiTableIndex |= 8;

    DXDesc.Filter = GALFilterTableIndexToDX11[uiTableIndex];
  }

  DXDesc.MaxAnisotropy = desc.m_uiMaxAnisotropy;
  DXDesc.MaxLOD = desc.m_fMaxMip;
  DXDesc.MinLOD = desc.m_fMinMip;
  DXDesc.MipLODBias = desc.m_fMipLodBias;

  if (FAILED(static_cast<WGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateSamplerState(&DXDesc, &m_pDXSamplerState)))
  {
    return W_FAILURE;
  }
  else
  {
    return W_SUCCESS;
  }
}


WResult WGALSamplerStateDX11::DeInitPlatform(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);

  W_GAL_DX11_RELEASE(m_pDXSamplerState);
  return W_SUCCESS;
}
