
W_ALWAYS_INLINE ID3D11Device* WGALDeviceDX11::GetDXDevice() const
{
  return m_pDevice;
}

W_ALWAYS_INLINE ID3D11Device3* WGALDeviceDX11::GetDXDevice3() const
{
  return m_pDevice3;
}

W_ALWAYS_INLINE ID3D11DeviceContext* WGALDeviceDX11::GetDXImmediateContext() const
{
  return m_pImmediateContext;
}

W_ALWAYS_INLINE IDXGIFactory1* WGALDeviceDX11::GetDXGIFactory() const
{
  return m_pDXGIFactory;
}

W_ALWAYS_INLINE const WGALFormatLookupTableDX11& WGALDeviceDX11::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}
