

W_ALWAYS_INLINE ID3D11RenderTargetView* WGALRenderTargetViewDX11::GetRenderTargetView() const
{
  return m_pRenderTargetView;
}

W_ALWAYS_INLINE ID3D11DepthStencilView* WGALRenderTargetViewDX11::GetDepthStencilView() const
{
  return m_pDepthStencilView;
}

W_ALWAYS_INLINE ID3D11UnorderedAccessView* WGALRenderTargetViewDX11::GetUnorderedAccessView() const
{
  return m_pUnorderedAccessView;
}