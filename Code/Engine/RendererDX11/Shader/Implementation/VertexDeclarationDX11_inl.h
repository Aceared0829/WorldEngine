

ID3D11InputLayout* WGALVertexDeclarationDX11::GetDXInputLayout() const
{
  return m_pDXInputLayout;
}

WArrayPtr<const WUInt32> WGALVertexDeclarationDX11::GetVertexBufferStrides() const
{
  return m_VertexBufferStrides;
}
