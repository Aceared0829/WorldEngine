
W_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* WGALBlendStateVulkan::GetBlendState() const
{
  return &m_BlendState;
}

W_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* WGALDepthStencilStateVulkan::GetDepthStencilState() const
{
  return &m_DepthStencilState;
}

W_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* WGALRasterizerStateVulkan::GetRasterizerState() const
{
  return &m_RasterizerState;
}

W_ALWAYS_INLINE const vk::DescriptorImageInfo& WGALSamplerStateVulkan::GetImageInfo() const
{
  return m_ResourceImageInfo;
}
