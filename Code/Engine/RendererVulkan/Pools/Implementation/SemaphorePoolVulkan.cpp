#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>

vk::Device WSemaphorePoolVulkan::s_Device;
WHybridArray<vk::Semaphore, 4> WSemaphorePoolVulkan::s_Semaphores;

void WSemaphorePoolVulkan::Initialize(vk::Device device)
{
  s_Device = device;
}

void WSemaphorePoolVulkan::DeInitialize()
{
  for (vk::Semaphore& semaphore : s_Semaphores)
  {
    s_Device.destroySemaphore(semaphore, nullptr);
  }
  s_Semaphores.Clear();
  s_Semaphores.Compact();

  s_Device = nullptr;
}

vk::Semaphore WSemaphorePoolVulkan::RequestSemaphore()
{
  W_ASSERT_DEBUG(s_Device, "WSemaphorePoolVulkan::Initialize not called");
  if (!s_Semaphores.IsEmpty())
  {
    vk::Semaphore semaphore = s_Semaphores.PeekBack();
    s_Semaphores.PopBack();
    return semaphore;
  }
  else
  {
    vk::Semaphore semaphore;
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    VK_ASSERT_DEV(s_Device.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore));
    return semaphore;
  }
}

void WSemaphorePoolVulkan::ReclaimSemaphore(vk::Semaphore& ref_semaphore)
{
  W_ASSERT_DEBUG(s_Device, "WSemaphorePoolVulkan::Initialize not called");
  s_Semaphores.PushBack(ref_semaphore);
}
