#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Time/Timestamp.h>

/// Simple pool for fences
///
/// Do not call ReclaimFence manually, instead call WGALDeviceVulkan::ReclaimLater which will make sure to reclaim the fence once it is no longer in use.
/// Fences are reclaimed once the frame in WGALDeviceVulkan is reused (currently 4 frames are in rotation). Do not call resetFences, this is already done by ReclaimFence.
/// Usage:
/// \code{.cpp}
///   vk::Fence f = WFencePoolVulkan::RequestFence();
///   <insert fence somewhere>
///   <wait for fence>
///   WGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(f);
/// \endcode
class W_RENDERERVULKAN_DLL WFencePoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();

  static vk::Fence RequestFence();
  static void ReclaimFence(vk::Fence& ref_fence);

private:
  static WHybridArray<vk::Fence, 4> s_Fences;
  static vk::Device s_Device;
};

// #TODO_VULKAN extend to support multiple queues.
class W_RENDERERVULKAN_DLL WFenceQueueVulkan
{
public:
  WFenceQueueVulkan(WGALDeviceVulkan* pDevice);
  ~WFenceQueueVulkan();

  WGALFenceHandle GetCurrentFenceHandle();
  void FenceSubmitted(vk::Fence vkFence);
  void FlushReadyFences();
  WEnum<WGALAsyncResult> GetFenceResult(WGALFenceHandle hFence, WTime timeout = WTime::MakeZero());

private:
  WEnum<WGALAsyncResult> WaitForNextFence(WTime timeout = WTime::MakeZero());

private:
  struct PendingFence
  {
    vk::Fence m_vkFence;
    WGALFenceHandle m_hFence;
  };
  WDeque<PendingFence> m_PendingFences;
  WUInt64 m_uiCurrentFenceCounter = 1;
  WUInt64 m_uiReachedFenceCounter = 0;
  vk::Device m_Device;
};
