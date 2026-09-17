#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/DescriptorWritePoolVulkan.h>

#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererVulkan/Pools/UniformBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

WDescriptorWritePoolVulkan::WDescriptorWritePoolVulkan(WGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
{
}

void WDescriptorWritePoolVulkan::WriteTransientDescriptor(vk::DescriptorSet descriptorSet, const WGALBindGroupCreationDescription& desc, WUniformBufferPoolVulkan* pUniformBufferPool)
{
  const WGALBindGroupLayoutVulkan* pLayout = static_cast<const WGALBindGroupLayoutVulkan*>(m_pDevice->GetBindGroupLayout(desc.m_hBindGroupLayout));
  WArrayPtr<const WShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
  const WUInt32 uiBindings = bindings.GetCount();

  W_LOCK(m_Mutex);
  for (WUInt32 i = 0; i < uiBindings; ++i)
  {
    const WShaderResourceBinding& binding = bindings[i];
    const WGALBindGroupItem& item = desc.m_BindGroupItems[i];
    vk::WriteDescriptorSet& write = WriteBindGroupItem(descriptorSet, binding, item, pUniformBufferPool);
    if (binding.m_ResourceType == WGALShaderResourceType::ConstantBuffer)
    {
      // Offsets are moved into separate offset array so leave at 0. Dynamic uniform buffers need to be tagged in the layout, so we can't decide whether we use them or not depending on what buffer we have bound. Thus, everything is a dynamic uniform buffer and the offset must be provided via the bindDescriptorSets call for normal constant buffers too.
      // Offset does not need to be stored, as it is handled in WGALCommandEncoderImplVulkan::FindDynamicUniformBuffers.
      const_cast<vk::DescriptorBufferInfo*>(write.pBufferInfo)->offset = 0;
    }
  }
}

void WDescriptorWritePoolVulkan::WriteDescriptor(vk::DescriptorSet descriptorSet, const WGALBindGroupCreationDescription& desc, WDynamicArray<WUInt32>& out_offsets)
{
  out_offsets.Clear();
  const WGALBindGroupLayoutVulkan* pLayout = static_cast<const WGALBindGroupLayoutVulkan*>(m_pDevice->GetBindGroupLayout(desc.m_hBindGroupLayout));
  WArrayPtr<const WShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
  const WUInt32 uiBindings = bindings.GetCount();

  W_LOCK(m_Mutex);
  for (WUInt32 i = 0; i < uiBindings; ++i)
  {
    const WShaderResourceBinding& binding = bindings[i];
    const WGALBindGroupItem& item = desc.m_BindGroupItems[i];
    vk::WriteDescriptorSet& write = WriteBindGroupItem(descriptorSet, binding, item, nullptr);
    if (binding.m_ResourceType == WGALShaderResourceType::ConstantBuffer)
    {
      out_offsets.PushBack(static_cast<WUInt32>(write.pBufferInfo->offset));
      const_cast<vk::DescriptorBufferInfo*>(write.pBufferInfo)->offset = 0;
    }
  }
}

WUInt32 WDescriptorWritePoolVulkan::FlushWrites()
{
  W_LOCK(m_Mutex);
  WUInt32 uiDescriptorWrites = 0;
  if (!m_DescriptorWrites.IsEmpty())
  {
    uiDescriptorWrites = m_DescriptorWrites.GetCount();
    m_pDevice->GetVulkanDevice().updateDescriptorSets(m_DescriptorWrites.GetCount(), m_DescriptorWrites.GetData(), 0, nullptr);
    m_DescriptorWrites.Clear();
    m_DescriptorImageInfos.Clear();
    m_DescriptorBufferInfos.Clear();
    m_DescriptorBufferViews.Clear();
  }
  return uiDescriptorWrites;
}

vk::WriteDescriptorSet& WDescriptorWritePoolVulkan::WriteBindGroupItem(vk::DescriptorSet descriptorSet, const WShaderResourceBinding& binding,
  const WGALBindGroupItem& item, WUniformBufferPoolVulkan* pUniformBufferPool)
{
  vk::WriteDescriptorSet& write = m_DescriptorWrites.ExpandAndGetRef();
  write.dstArrayElement = 0;
  write.descriptorType = WConversionUtilsVulkan::GetDescriptorType(binding.m_ResourceType);
  write.dstBinding = binding.m_iSlot;
  write.dstSet = descriptorSet;
  // A single WGALBindGroupItem describes exactly one resource and the info structs below are allocated one at a time from a deque, so writing more than one element here would read out of bounds.
  W_ASSERT_DEV(binding.m_uiArraySize == 1, "Descriptor arrays are not supported, binding '{}' requests {} elements.", binding.m_sName, binding.m_uiArraySize);
  write.descriptorCount = 1;
  switch (binding.m_ResourceType)
  {
    case WGALShaderResourceType::ConstantBuffer:
    {
      vk::DescriptorBufferInfo& bufferInfo = m_DescriptorBufferInfos.ExpandAndGetRef();
      write.pBufferInfo = &bufferInfo;
      const WGALBufferVulkan* pBuffer = static_cast<const WGALBufferVulkan*>(m_pDevice->GetBuffer(item.m_Buffer.m_hBuffer));
      if (pBuffer->GetDescription().m_BufferFlags.IsSet(WGALBufferUsageFlags::Transient))
      {
        W_ASSERT_DEV(pUniformBufferPool != nullptr, "Bind group resources must not contain transient constant buffer references");
        bufferInfo = *pUniformBufferPool->GetBuffer(pBuffer);
      }
      else
      {
        bufferInfo = pBuffer->GetBufferInfo();
        bufferInfo.range = item.m_Buffer.m_BufferRange.m_uiByteCount;
      }
    }
    break;
    case WGALShaderResourceType::Texture:
    case WGALShaderResourceType::TextureRW:
    case WGALShaderResourceType::TextureAndSampler:
    {
      vk::DescriptorImageInfo& imageInfo = m_DescriptorImageInfos.ExpandAndGetRef();
      write.pImageInfo = &imageInfo;
      const WGALTextureVulkan* pTexture = static_cast<const WGALTextureVulkan*>(m_pDevice->GetTexture(item.m_Texture.m_hTexture));
      imageInfo = pTexture->GetDescriptorImageInfo(item.m_Texture.m_TextureRange, binding.m_ResourceType, binding.m_TextureType, item.m_Texture.m_OverrideViewFormat);
      imageInfo.imageLayout = binding.m_ResourceType == WGALShaderResourceType::TextureRW ? vk::ImageLayout::eGeneral : WConversionUtilsVulkan::GetTextureReadLayout(pTexture->GetImageFormat());

      if (binding.m_ResourceType == WGALShaderResourceType::TextureAndSampler)
      {
        const WGALSamplerStateVulkan* pSampler = static_cast<const WGALSamplerStateVulkan*>(m_pDevice->GetSamplerState(item.m_Texture.m_hSampler));
        imageInfo.sampler = pSampler->GetImageInfo().sampler;
      }
    }
    break;
    case WGALShaderResourceType::TexelBuffer:
    case WGALShaderResourceType::TexelBufferRW:
    {
      vk::BufferView& bufferView = m_DescriptorBufferViews.ExpandAndGetRef();
      write.pTexelBufferView = &bufferView;
      const WGALBufferVulkan* pBuffer = static_cast<const WGALBufferVulkan*>(m_pDevice->GetBuffer(item.m_Buffer.m_hBuffer));
      bufferView = pBuffer->GetTexelBufferView(item.m_Buffer.m_BufferRange, item.m_Buffer.m_OverrideTexelBufferFormat);
    }
    break;
    case WGALShaderResourceType::StructuredBuffer:
    case WGALShaderResourceType::ByteAddressBuffer:
    case WGALShaderResourceType::StructuredBufferRW:
    case WGALShaderResourceType::ByteAddressBufferRW:
    {
      vk::DescriptorBufferInfo& bufferInfo = m_DescriptorBufferInfos.ExpandAndGetRef();
      write.pBufferInfo = &bufferInfo;
      const WGALBufferVulkan* pBuffer = static_cast<const WGALBufferVulkan*>(m_pDevice->GetBuffer(item.m_Buffer.m_hBuffer));
      bufferInfo.buffer = pBuffer->GetBufferInfo().buffer;
      bufferInfo.offset = item.m_Buffer.m_BufferRange.m_uiByteOffset;
      bufferInfo.range = item.m_Buffer.m_BufferRange.m_uiByteCount;
    }
    break;
    case WGALShaderResourceType::Sampler:
    {
      vk::DescriptorImageInfo& imageInfo = m_DescriptorImageInfos.ExpandAndGetRef();
      write.pImageInfo = &imageInfo;
      const WGALSamplerStateVulkan* pSampler = static_cast<const WGALSamplerStateVulkan*>(m_pDevice->GetSamplerState(item.m_Sampler.m_hSampler));
      imageInfo.sampler = pSampler->GetImageInfo().sampler;
    }
    break;
    default:
      break;
  }
  return write;
}
