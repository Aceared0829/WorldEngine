
vk::ShaderModule WGALShaderVulkan::GetShader(WGALShaderStage::Enum stage) const
{
  return m_Shaders[stage];
}
