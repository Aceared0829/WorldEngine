#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

W_DEFINE_AS_POD_TYPE(vk::PresentModeKHR);

/// Helper functions to convert and extract Vulkan objects from W objects.
class W_RENDERERVULKAN_DLL WConversionUtilsVulkan
{
public:
  /// Helper function to hash vk enums.
  template <typename T, typename R = typename std::underlying_type<T>::type>
  static R GetUnderlyingValue(T value)
  {
    return static_cast<typename std::underlying_type<T>::type>(value);
  }

  /// Helper function to hash vk flags.
  template <typename T>
  static auto GetUnderlyingFlagsValue(T value)
  {
    return static_cast<typename T::MaskType>(value);
  }

  static vk::AttachmentLoadOp GetAttachmentLoadOp(WEnum<WGALRenderTargetLoadOp> op);
  static vk::AttachmentStoreOp GetAttachmentStoreOp(WEnum<WGALRenderTargetStoreOp> op);
  static vk::VertexInputRate GetVertexBindingRate(WEnum<WGALVertexBindingRate> rate);
  static vk::SampleCountFlagBits GetSamples(WEnum<WGALMSAASampleCount> samples);
  static vk::PresentModeKHR GetPresentMode(WEnum<WGALPresentMode> presentMode, const WDynamicArray<vk::PresentModeKHR>& supportedModes);
  static vk::ImageSubresourceRange GetSubresourceRange(const WGALTextureCreationDescription& texDesc, const WGALRenderTargetViewCreationDescription& desc);
  static vk::ImageSubresourceRange GetSubresourceRange(const vk::ImageSubresourceLayers& layers);
  static vk::ImageSubresourceRange GetSubresourceRange(WGALResourceFormat::Enum format, WGALTextureRange textureRange);
  static vk::ImageViewType GetImageViewType(WEnum<WGALTextureType> texType);
  static vk::ImageViewType GetImageViewType(WEnum<WGALShaderTextureType> texType);
  static vk::ImageViewType GetImageArrayViewType(WEnum<WGALTextureType> texType);

  static bool IsDepthFormat(vk::Format format);
  static bool IsStencilFormat(vk::Format format);
  static vk::ImageLayout GetTextureReadLayout(vk::Format format);
  static vk::PrimitiveTopology GetPrimitiveTopology(WEnum<WGALPrimitiveTopology> topology);
  static vk::ShaderStageFlagBits GetShaderStage(WGALShaderStage::Enum stage);
  static vk::ShaderStageFlagBits GetShaderStages(WBitflags<WGALShaderStageFlags> stages);
  static vk::PipelineStageFlags GetPipelineStage(WGALShaderStage::Enum stage);
  static vk::PipelineStageFlags GetPipelineStage(vk::ShaderStageFlags flags);
  static vk::PipelineStageFlags GetPipelineStages(WBitflags<WGALShaderStageFlags> stages);
  static vk::DescriptorType GetDescriptorType(WGALShaderResourceType::Enum type);

  /// Converts an WGALResourceState bitmask to the equivalent Vulkan pipeline stages and access flags.
  static void ConvertResourceState(WBitflags<WGALResourceState> state, vk::PipelineStageFlags& out_stages, vk::AccessFlags& out_access);

  /// Converts an WGALResourceState bitmask to Synchronization2 pipeline stages and access flags.
  static void ConvertResourceState(WBitflags<WGALResourceState> state, vk::PipelineStageFlags2& out_stages, vk::AccessFlags2& out_access);

  /// Returns the Vulkan image layout that corresponds to the given resource state.
  static vk::ImageLayout GetTextureLayout(WBitflags<WGALResourceState> state);
};

#include <RendererVulkan/Utils/Implementation/ConversionUtilsVulkan.inl.h>
