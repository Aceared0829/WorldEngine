#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Descriptors/Enumerations.h>

class WGALDeviceVulkan;

/// Describes a texture barrier for a layout/state transition using a raw Vulkan image.
struct WTextureBarrierVulkan
{
  W_DECLARE_POD_TYPE();
  vk::Image m_Image;
  vk::ImageAspectFlags m_AspectMask = vk::ImageAspectFlagBits::eColor;
  WBitflags<WGALResourceState> m_StateBefore;
  WBitflags<WGALResourceState> m_StateAfter;
  WBitflags<WGALShaderStageFlags> m_StagesBefore;
  WBitflags<WGALShaderStageFlags> m_StagesAfter;
  WGALTextureSubresource m_Subresource = {};
  bool m_bAllSubresources = true; ///< If true, the barrier applies to all subresources and m_Subresource is ignored.
  bool m_bDiscard = false;
};

/// Describes a buffer barrier for a state transition using a raw Vulkan buffer.
struct WBufferBarrierVulkan
{
  W_DECLARE_POD_TYPE();
  vk::Buffer m_Buffer;
  WBitflags<WGALResourceState> m_StateBefore;
  WBitflags<WGALResourceState> m_StateAfter;
  WBitflags<WGALShaderStageFlags> m_StagesBefore;
  WBitflags<WGALShaderStageFlags> m_StagesAfter;
};

/// Translates GAL barrier descriptions into Vulkan barriers and submits them.
///
/// Uses VK_KHR_synchronization2 (vkCmdPipelineBarrier2) when the extension is available on the device, otherwise falls back to the core Vulkan 1.1 vkCmdPipelineBarrier path.
class W_RENDERERVULKAN_DLL WBarrierUtilsVulkan
{
public:
  WBarrierUtilsVulkan(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer);

  /// Submits image memory barriers for the given texture barrier descriptions.
  void TextureBarrier(WArrayPtr<const WGALTextureBarrier> barriers);

  /// Submits image memory barriers for the given Vulkan texture barrier descriptions.
  void TextureBarrier(WArrayPtr<const WTextureBarrierVulkan> barriers);

  /// Submits buffer memory barriers for the given buffer barrier descriptions.
  void BufferBarrier(WArrayPtr<const WGALBufferBarrier> barriers);

  /// Submits buffer memory barriers for the given Vulkan buffer barrier descriptions.
  void BufferBarrier(WArrayPtr<const WBufferBarrierVulkan> barriers);

  /// Submits a single image memory barrier.
  ///
  /// If stagesBefore or stagesAfter are set to Auto, the pipeline stages are inferred from the resource state.
  void TextureBarrier(
    WGALTextureHandle hTexture,
    WBitflags<WGALResourceState> stateBefore,
    WBitflags<WGALResourceState> stateAfter,
    WBitflags<WGALShaderStageFlags> stagesBefore = WGALShaderStageFlags::Auto,
    WBitflags<WGALShaderStageFlags> stagesAfter = WGALShaderStageFlags::Auto);

  /// Submits a single image memory barrier for a raw Vulkan image.
  ///
  /// If stagesBefore or stagesAfter are set to Auto, the pipeline stages are inferred from the resource state.
  void TextureBarrier(
    vk::Image image,
    const vk::ImageSubresourceRange& subresourceRange,
    WBitflags<WGALResourceState> stateBefore,
    WBitflags<WGALResourceState> stateAfter,
    WBitflags<WGALShaderStageFlags> stagesBefore = WGALShaderStageFlags::Auto,
    WBitflags<WGALShaderStageFlags> stagesAfter = WGALShaderStageFlags::Auto);

  /// Submits a single buffer memory barrier.
  ///
  /// If stagesBefore or stagesAfter are set to Auto, the pipeline stages are inferred from the resource state.
  void BufferBarrier(
    WGALBufferHandle hBuffer,
    WBitflags<WGALResourceState> stateBefore,
    WBitflags<WGALResourceState> stateAfter,
    WBitflags<WGALShaderStageFlags> stagesBefore = WGALShaderStageFlags::Auto,
    WBitflags<WGALShaderStageFlags> stagesAfter = WGALShaderStageFlags::Auto);

  /// Submits a single buffer memory barrier for a raw Vulkan buffer.
  ///
  /// If stagesBefore or stagesAfter are set to Auto, the pipeline stages are inferred from the resource state.
  void BufferBarrier(
    vk::Buffer buffer,
    WBitflags<WGALResourceState> stateBefore,
    WBitflags<WGALResourceState> stateAfter,
    WBitflags<WGALShaderStageFlags> stagesBefore = WGALShaderStageFlags::Auto,
    WBitflags<WGALShaderStageFlags> stagesAfter = WGALShaderStageFlags::Auto);

private:
  const WGALDeviceVulkan& m_Device;
  vk::CommandBuffer& m_CommandBuffer;
};
