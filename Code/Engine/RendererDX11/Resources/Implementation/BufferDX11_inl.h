
W_ALWAYS_INLINE ID3D11Buffer* WGALBufferDX11::GetDXBuffer() const
{
  return m_pDXBuffer;
}

W_ALWAYS_INLINE DXGI_FORMAT WGALBufferDX11::GetIndexFormat() const
{
  return m_IndexFormat;
}
