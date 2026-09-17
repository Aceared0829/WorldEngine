#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>

#include <d3d11.h>

WGALShaderDX11::WGALShaderDX11(const WGALShaderCreationDescription& Description)
  : WGALShader(Description)

{
}

WGALShaderDX11::~WGALShaderDX11() = default;

void WGALShaderDX11::SetDebugName(WStringView sName) const
{
  const char* szName = sName.GetStartPointer();
  const WUInt32 uiLength = sName.GetElementCount();

  if (m_pVertexShader != nullptr)
  {
    m_pVertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pHullShader != nullptr)
  {
    m_pHullShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pDomainShader != nullptr)
  {
    m_pDomainShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pGeometryShader != nullptr)
  {
    m_pGeometryShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pPixelShader != nullptr)
  {
    m_pPixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pComputeShader != nullptr)
  {
    m_pComputeShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

WResult WGALShaderDX11::InitPlatform(WGALDevice* pDevice)
{
  m_pDevice = pDevice;
  W_SUCCEED_OR_RETURN(CreateBindingMapping(true));
  W_SUCCEED_OR_RETURN(CreateLayouts(pDevice, false));

  WGALDeviceDX11* pDXDevice = static_cast<WGALDeviceDX11*>(pDevice);
  ID3D11Device* pD3D11Device = pDXDevice->GetDXDevice();

  if (m_Description.HasByteCodeForStage(WGALShaderStage::VertexShader))
  {
    if (FAILED(pD3D11Device->CreateVertexShader(m_Description.m_ByteCodes[WGALShaderStage::VertexShader]->GetByteCode(),
          m_Description.m_ByteCodes[WGALShaderStage::VertexShader]->GetSize(), nullptr, &m_pVertexShader)))
    {
      WLog::Error("Couldn't create native vertex shader from bytecode!");
      return W_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(WGALShaderStage::HullShader))
  {
    if (FAILED(pD3D11Device->CreateHullShader(m_Description.m_ByteCodes[WGALShaderStage::HullShader]->GetByteCode(),
          m_Description.m_ByteCodes[WGALShaderStage::HullShader]->GetSize(), nullptr, &m_pHullShader)))
    {
      WLog::Error("Couldn't create native hull shader from bytecode!");
      return W_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(WGALShaderStage::DomainShader))
  {
    if (FAILED(pD3D11Device->CreateDomainShader(m_Description.m_ByteCodes[WGALShaderStage::DomainShader]->GetByteCode(),
          m_Description.m_ByteCodes[WGALShaderStage::DomainShader]->GetSize(), nullptr, &m_pDomainShader)))
    {
      WLog::Error("Couldn't create native domain shader from bytecode!");
      return W_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(WGALShaderStage::GeometryShader))
  {
    if (FAILED(pD3D11Device->CreateGeometryShader(m_Description.m_ByteCodes[WGALShaderStage::GeometryShader]->GetByteCode(),
          m_Description.m_ByteCodes[WGALShaderStage::GeometryShader]->GetSize(), nullptr, &m_pGeometryShader)))
    {
      WLog::Error("Couldn't create native geometry shader from bytecode!");
      return W_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(WGALShaderStage::PixelShader))
  {
    if (FAILED(pD3D11Device->CreatePixelShader(m_Description.m_ByteCodes[WGALShaderStage::PixelShader]->GetByteCode(),
          m_Description.m_ByteCodes[WGALShaderStage::PixelShader]->GetSize(), nullptr, &m_pPixelShader)))
    {
      WLog::Error("Couldn't create native pixel shader from bytecode!");
      return W_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(WGALShaderStage::ComputeShader))
  {
    if (FAILED(pD3D11Device->CreateComputeShader(m_Description.m_ByteCodes[WGALShaderStage::ComputeShader]->GetByteCode(),
          m_Description.m_ByteCodes[WGALShaderStage::ComputeShader]->GetSize(), nullptr, &m_pComputeShader)))
    {
      WLog::Error("Couldn't create native compute shader from bytecode!");
      return W_FAILURE;
    }
  }


  return W_SUCCESS;
}

WResult WGALShaderDX11::DeInitPlatform(WGALDevice* pDevice)
{
  DestroyBindingMapping();
  DestroyLayouts(pDevice);

  W_GAL_DX11_RELEASE(m_pVertexShader);
  W_GAL_DX11_RELEASE(m_pHullShader);
  W_GAL_DX11_RELEASE(m_pDomainShader);
  W_GAL_DX11_RELEASE(m_pGeometryShader);
  W_GAL_DX11_RELEASE(m_pPixelShader);
  W_GAL_DX11_RELEASE(m_pComputeShader);

  return W_SUCCESS;
}
