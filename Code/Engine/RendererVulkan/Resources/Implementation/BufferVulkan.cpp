#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

WGALBufferVulkan::WGALBufferVulkan(const WGALBufferCreationDescription& Description)
  : WGALBuffer(Description)
{
}

WGALBufferVulkan::~WGALBufferVulkan() = default;

WResult WGALBufferVulkan::InitPlatform(WGALDevice* pDevice, WArrayPtr<const WUInt8> pInitialData)
{
  m_pDeviceVulkan = static_cast<WGALDeviceVulkan*>(pDevice);
  m_Device = m_pDeviceVulkan->GetVulkanDevice();

  // Derive stages and access from the GAL default state, then mask to supported stages.
  const WBitflags<WGALResourceState> defaultState = m_Description.GetDefaultState();
  WConversionUtilsVulkan::ConvertResourceState(defaultState, m_Stages, m_Access);
  m_Stages &= m_pDeviceVulkan->GetSupportedStages();

  const bool bSRV = m_Description.m_BufferFlags.IsSet(WGALBufferUsageFlags::ShaderResource);
  const bool bUAV = m_Description.m_BufferFlags.IsSet(WGALBufferUsageFlags::UnorderedAccess);
  for (WGALBufferUsageFlags::Enum flag : m_Description.m_BufferFlags)
  {
    switch (flag)
    {
      case WGALBufferUsageFlags::VertexBuffer:
        m_Usage |= vk::BufferUsageFlagBits::eVertexBuffer;
        // W_ASSERT_DEBUG(!bSRV && !bUAV, "Not implemented");
        break;
      case WGALBufferUsageFlags::IndexBuffer:
        m_Usage |= vk::BufferUsageFlagBits::eIndexBuffer;
        m_IndexType = m_Description.m_uiStructSize == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
        // W_ASSERT_DEBUG(!bSRV && !bUAV, "Not implemented");
        break;
      case WGALBufferUsageFlags::ConstantBuffer:
        m_Usage |= vk::BufferUsageFlagBits::eUniformBuffer;
        break;
      case WGALBufferUsageFlags::TexelBuffer:
        if (bSRV)
          m_Usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
        if (bUAV)
          m_Usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
        break;
      case WGALBufferUsageFlags::StructuredBuffer:
      case WGALBufferUsageFlags::ByteAddressBuffer:
        m_Usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        break;
      case WGALBufferUsageFlags::ShaderResource:
        break;
      case WGALBufferUsageFlags::UnorderedAccess:
        break;
      case WGALBufferUsageFlags::DrawIndirect:
        m_Usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
        break;
      case WGALBufferUsageFlags::Transient:
        break;
      default:
        WLog::Error("Unknown buffer type supplied to CreateBuffer()!");
        return W_FAILURE;
    }
  }

  m_Usage |= vk::BufferUsageFlagBits::eTransferSrc;
  m_Usage |= vk::BufferUsageFlagBits::eTransferDst;

  W_ASSERT_DEBUG(pInitialData.GetCount() <= m_Description.m_uiTotalSize, "Initial data is bigger than target buffer.");
  vk::DeviceSize alignment = GetAlignment(m_pDeviceVulkan, m_Usage);
  m_Size = WMemoryUtils::AlignSize((vk::DeviceSize)m_Description.m_uiTotalSize, alignment);

  m_ResourceBufferInfo.offset = 0;
  m_ResourceBufferInfo.range = m_Size;

  // No buffer needed if we use transient memory for constant buffers.
  if (m_Description.m_BufferFlags.AreAllSet(WGALBufferUsageFlags::Transient | WGALBufferUsageFlags::ConstantBuffer))
    return W_SUCCESS;

  W_SUCCEED_OR_RETURN(CreateBuffer());

  if (!pInitialData.IsEmpty())
  {
    m_pDeviceVulkan->GetInitContext().InitBuffer(this, pInitialData);
  }
  return W_SUCCESS;
}

WResult WGALBufferVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  if (m_Buffer)
  {
    m_pDeviceVulkan->DeleteLater(m_Buffer, m_pAlloc);
    m_AllocInfo = {};
  }

  for (auto it : m_TexelBufferViews)
  {
    m_pDeviceVulkan->DeleteLater(it.Value());
  }
  m_TexelBufferViews.Clear();

  m_ResourceBufferInfo = vk::DescriptorBufferInfo();

  m_Stages = {};
  m_Access = {};
  m_IndexType = vk::IndexType::eUint16;
  m_Usage = {};
  m_Size = 0;

  m_pDeviceVulkan = nullptr;
  m_Device = nullptr;

  return W_SUCCESS;
}

const vk::DescriptorBufferInfo& WGALBufferVulkan::GetBufferInfo() const
{
  return m_ResourceBufferInfo;
}

WResult WGALBufferVulkan::CreateBuffer()
{
  vk::BufferCreateInfo bufferCreateInfo;
  bufferCreateInfo.usage = m_Usage;
  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
  bufferCreateInfo.size = m_Size;

  WVulkanAllocationCreateInfo allocCreateInfo;
  allocCreateInfo.m_usage = WVulkanMemoryUsage::Auto;

  VK_SUCCEED_OR_RETURN_W_FAILURE(WMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_Buffer, m_pAlloc, &m_AllocInfo));
  m_ResourceBufferInfo.buffer = m_Buffer;
  return W_SUCCESS;
}

void WGALBufferVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_sDebugName = szName;
  m_pDeviceVulkan->SetDebugName(szName, m_Buffer, m_pAlloc);
}

vk::BufferView WGALBufferVulkan::GetTexelBufferView(WGALBufferRange bufferRange, WEnum<WGALResourceFormat> overrideTexelBufferFormat) const
{
  vk::BufferView bufferView;

  View view;
  view.m_BufferRange = bufferRange;
  view.m_OverrideTexelBufferFormat = overrideTexelBufferFormat;

  if (!m_TexelBufferViews.TryGetValue(view, bufferView))
  {
    const WGALResourceFormat::Enum viewFormat = overrideTexelBufferFormat == WGALResourceFormat::Invalid ? m_Description.m_Format : overrideTexelBufferFormat;

    vk::BufferViewCreateInfo viewCreateInfo;
    viewCreateInfo.buffer = m_Buffer;
    viewCreateInfo.offset = bufferRange.m_uiByteOffset;
    viewCreateInfo.range = bufferRange.m_uiByteCount;
    if (viewCreateInfo.range == W_GAL_WHOLE_SIZE)
      viewCreateInfo.range = VK_WHOLE_SIZE;
    viewCreateInfo.format = m_pDeviceVulkan->GetFormatLookupTable().GetFormatInfo(viewFormat).m_format;
    VK_ASSERT_DEV(m_Device.createBufferView(&viewCreateInfo, nullptr, &bufferView));
    m_TexelBufferViews.Insert(view, bufferView);
  }

  return bufferView;
}

vk::DeviceSize WGALBufferVulkan::GetAlignment(const WGALDeviceVulkan* pDevice, vk::BufferUsageFlags usage)
{
  const vk::PhysicalDeviceProperties& properties = pDevice->GetPhysicalDeviceProperties();

  vk::DeviceSize alignment = WMath::Max<vk::DeviceSize>(4, properties.limits.nonCoherentAtomSize);

  if (usage & vk::BufferUsageFlagBits::eUniformBuffer)
    alignment = WMath::Max(alignment, properties.limits.minUniformBufferOffsetAlignment);

  if (usage & vk::BufferUsageFlagBits::eStorageBuffer)
    alignment = WMath::Max(alignment, properties.limits.minStorageBufferOffsetAlignment);

  if (usage & (vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer))
    alignment = WMath::Max(alignment, properties.limits.minTexelBufferOffsetAlignment);

  if (usage & (vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndirectBuffer))
    alignment = WMath::Max(alignment, VkDeviceSize(16)); // If no cache line aligned perf will suffer.

  if (usage & (vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst))
    alignment = WMath::Max(alignment, properties.limits.optimalBufferCopyOffsetAlignment);

  return alignment;
}
