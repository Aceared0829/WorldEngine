#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>

WCommandBufferPoolVulkan::WCommandBufferPoolVulkan(WAllocator* pAllocator)
  : m_CommandBuffers(pAllocator)
{
}

WCommandBufferPoolVulkan::~WCommandBufferPoolVulkan()
{
  W_ASSERT_DEBUG(m_CommandBuffers.IsEmpty(), "Either DeInitialize was not called or ReclaimCommandBuffer was called after DeInitialize");
  W_ASSERT_DEBUG(!m_Device, "DeInitialize was not called");
}

void WCommandBufferPoolVulkan::Initialize(vk::Device device, WUInt32 uiGraphicsFamilyIndex)
{
  m_Device = device;

  // Command buffer
  vk::CommandPoolCreateInfo commandPoolCreateInfo = {};
  commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  commandPoolCreateInfo.queueFamilyIndex = uiGraphicsFamilyIndex;

  m_CommandPool = m_Device.createCommandPool(commandPoolCreateInfo);
}

void WCommandBufferPoolVulkan::DeInitialize()
{
  for (vk::CommandBuffer& commandBuffer : m_CommandBuffers)
  {
    m_Device.freeCommandBuffers(m_CommandPool, 1, &commandBuffer);
  }
  m_CommandBuffers.Clear();
  m_CommandBuffers.Compact();

  m_Device.destroyCommandPool(m_CommandPool);
  m_CommandPool = nullptr;

  m_Device = nullptr;
}

vk::CommandBuffer WCommandBufferPoolVulkan::RequestCommandBuffer()
{
  W_ASSERT_DEBUG(m_Device, "WCommandBufferPoolVulkan::Initialize not called");
  if (!m_CommandBuffers.IsEmpty())
  {
    vk::CommandBuffer CommandBuffer = m_CommandBuffers.PeekBack();
    m_CommandBuffers.PopBack();
    return CommandBuffer;
  }
  else
  {
    vk::CommandBuffer commandBuffer;

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.commandPool = m_CommandPool;
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;

    VK_ASSERT_DEV(m_Device.allocateCommandBuffers(&commandBufferAllocateInfo, &commandBuffer));
    return commandBuffer;
  }
}

void WCommandBufferPoolVulkan::ReclaimCommandBuffer(vk::CommandBuffer& ref_commandBuffer)
{
  W_ASSERT_DEBUG(m_Device, "WCommandBufferPoolVulkan::Initialize not called");
  ref_commandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
  m_CommandBuffers.PushBack(ref_commandBuffer);
}
