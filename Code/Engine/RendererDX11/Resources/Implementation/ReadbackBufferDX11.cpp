#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/ReadbackBufferDX11.h>

#include <d3d11.h>

WGALReadbackBufferDX11::WGALReadbackBufferDX11(const WGALBufferCreationDescription& Description)
  : WGALReadbackBuffer(Description)
{
}

WGALReadbackBufferDX11::~WGALReadbackBufferDX11() = default;

WResult WGALReadbackBufferDX11::InitPlatform(WGALDevice* pDevice)
{
  WGALDeviceDX11* pDXDevice = static_cast<WGALDeviceDX11*>(pDevice);

  D3D11_BUFFER_DESC BufferDesc = {};
  W_SUCCEED_OR_RETURN(WGALBufferDX11::CreateBufferDesc(m_Description, BufferDesc, m_IndexFormat));
  BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  BufferDesc.Usage = D3D11_USAGE_STAGING;

  if (SUCCEEDED(pDXDevice->GetDXDevice()->CreateBuffer(&BufferDesc, nullptr, &m_pDXBuffer)))
  {
    return W_SUCCESS;
  }
  else
  {
    WLog::Error("Creation of native DirectX buffer failed!");
    return W_FAILURE;
  }
}

WResult WGALReadbackBufferDX11::DeInitPlatform(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);
  W_GAL_DX11_RELEASE(m_pDXBuffer);
  return W_SUCCESS;
}

void WGALReadbackBufferDX11::SetDebugNamePlatform(const char* szName) const
{
  WUInt32 uiLength = WStringUtils::GetStringElementCount(szName);

  if (m_pDXBuffer != nullptr)
  {
    m_pDXBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}
