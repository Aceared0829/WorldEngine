#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/FencePoolVulkan.h>

vk::Device WFencePoolVulkan::s_Device;
WHybridArray<vk::Fence, 4> WFencePoolVulkan::s_Fences;

void WFencePoolVulkan::Initialize(vk::Device device)
{
  s_Device = device;
}

void WFencePoolVulkan::DeInitialize()
{
  for (vk::Fence& fence : s_Fences)
  {
    s_Device.destroyFence(fence, nullptr);
  }
  s_Fences.Clear();
  s_Fences.Compact();

  s_Device = nullptr;
}

vk::Fence WFencePoolVulkan::RequestFence()
{
  W_ASSERT_DEBUG(s_Device, "WFencePoolVulkan::Initialize not called");
  if (!s_Fences.IsEmpty())
  {
    vk::Fence Fence = s_Fences.PeekBack();
    s_Fences.PopBack();
    return Fence;
  }
  else
  {
    vk::Fence fence;
    vk::FenceCreateInfo createInfo = {};
    VK_ASSERT_DEV(s_Device.createFence(&createInfo, nullptr, &fence));
    return fence;
  }
}

void WFencePoolVulkan::ReclaimFence(vk::Fence& ref_fence)
{
  vk::Result fenceStatus = s_Device.getFenceStatus(ref_fence);
  if (fenceStatus == vk::Result::eNotReady)
  {
    // #TODO_VULKAN Workaround for fences that were waited for (and thus signaled) returning VK_NOT_READY if AMDs profiler is active.
    // The fence will simply take another round through the reclaim process and will eventually turn signaled.
    static_cast<WGALDeviceVulkan*>(WGALDevice::GetDefaultDevice())->ReclaimLater(ref_fence);
    return;
  }
  VK_ASSERT_DEV(fenceStatus);
  s_Device.resetFences(1, &ref_fence);
  W_ASSERT_DEBUG(s_Device, "WFencePoolVulkan::Initialize not called");
  s_Fences.PushBack(ref_fence);
}


WFenceQueueVulkan::WFenceQueueVulkan(WGALDeviceVulkan* pDevice)
  : m_Device(pDevice->GetVulkanDevice())
  , m_PendingFences(pDevice->GetAllocator())
{
}

WFenceQueueVulkan::~WFenceQueueVulkan()
{
  while (!m_PendingFences.IsEmpty())
  {
    WaitForNextFence(WTime::MakeFromHours(1));
  }
}

WGALFenceHandle WFenceQueueVulkan::GetCurrentFenceHandle()
{
  return m_uiCurrentFenceCounter;
}

void WFenceQueueVulkan::FenceSubmitted(vk::Fence vkFence)
{
  m_PendingFences.PushBack({vkFence, m_uiCurrentFenceCounter});
  m_uiCurrentFenceCounter++;
}

void WFenceQueueVulkan::FlushReadyFences()
{
  while (!m_PendingFences.IsEmpty())
  {
    if (WaitForNextFence() == WGALAsyncResult::Pending)
      return;
  }
}

WEnum<WGALAsyncResult> WFenceQueueVulkan::GetFenceResult(WGALFenceHandle hFence, WTime timeout /*= WTime::MakeZero()*/)
{
  if (hFence <= m_uiReachedFenceCounter)
    return WGALAsyncResult::Ready;

  W_ASSERT_DEBUG(hFence <= m_uiCurrentFenceCounter, "Invalid fence handle");

  while (!m_PendingFences.IsEmpty() && m_PendingFences[0].m_hFence <= hFence)
  {
    const WTime start = WTime::Now();
    WEnum<WGALAsyncResult> res = WaitForNextFence(timeout);
    if (res == WGALAsyncResult::Pending)
      return res;

    const WTime end = WTime::Now();
    // Clamp timeout to zero as zero is considered no wait.
    timeout = WMath::Max(timeout - (end - start), WTime::MakeZero());
  }

  return hFence <= m_uiReachedFenceCounter ? WGALAsyncResult::Ready : WGALAsyncResult::Pending;
}

WEnum<WGALAsyncResult> WFenceQueueVulkan::WaitForNextFence(WTime timeout /*= WTime::MakeZero()*/)
{
  vk::Result fenceStatus;
  {
    W_PROFILE_SCOPE("getFenceStatus");
    fenceStatus = m_Device.getFenceStatus(m_PendingFences[0].m_vkFence);
  }
  if (fenceStatus == vk::Result::eSuccess)
  {
    m_uiReachedFenceCounter = m_PendingFences[0].m_hFence;
    m_PendingFences.PopFront();
    return WGALAsyncResult::Ready;
  }

  W_ASSERT_DEBUG(fenceStatus == vk::Result::eNotReady, "getFenceStatus returned {}", vk::to_string(fenceStatus).c_str());
  if (fenceStatus == vk::Result::eNotReady && timeout.IsPositive())
  {
    fenceStatus = m_Device.waitForFences(1, &m_PendingFences[0].m_vkFence, true, static_cast<WUInt64>(timeout.GetNanoseconds()));
    if (fenceStatus == vk::Result::eTimeout)
      return WGALAsyncResult::Pending;

    W_ASSERT_DEBUG(fenceStatus == vk::Result::eSuccess, "waitForFences returned {}", vk::to_string(fenceStatus).c_str());

    m_uiReachedFenceCounter = m_PendingFences[0].m_hFence;
    m_PendingFences.PopFront();
    return WGALAsyncResult::Ready;
  }
  return WGALAsyncResult::Pending;
}
