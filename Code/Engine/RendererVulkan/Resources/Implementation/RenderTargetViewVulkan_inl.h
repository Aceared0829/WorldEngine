

W_ALWAYS_INLINE vk::ImageView WGALRenderTargetViewVulkan::GetImageView() const
{
  return m_ImageView;
}

W_ALWAYS_INLINE bool WGALRenderTargetViewVulkan::IsFullRange() const
{
  return m_bBfullRange;
}

W_ALWAYS_INLINE vk::ImageSubresourceRange WGALRenderTargetViewVulkan::GetRange() const
{
  return m_Range;
}
