#include <RendererFoundation/Resources/ResourceFormats.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

W_ALWAYS_INLINE vk::AttachmentLoadOp WConversionUtilsVulkan::GetAttachmentLoadOp(WEnum<WGALRenderTargetLoadOp> op)
{
  switch (op)
  {
    case WGALRenderTargetLoadOp::Load:
      return vk::AttachmentLoadOp::eLoad;
    case WGALRenderTargetLoadOp::Clear:
      return vk::AttachmentLoadOp::eClear;
    case WGALRenderTargetLoadOp::DontCare:
      return vk::AttachmentLoadOp::eDontCare;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return vk::AttachmentLoadOp::eLoad;
  }
}

W_ALWAYS_INLINE vk::AttachmentStoreOp WConversionUtilsVulkan::GetAttachmentStoreOp(WEnum<WGALRenderTargetStoreOp> op)
{
  switch (op)
  {
    case WGALRenderTargetStoreOp::Store:
      return vk::AttachmentStoreOp::eStore;
    case WGALRenderTargetStoreOp::Discard:
      return vk::AttachmentStoreOp::eDontCare;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return vk::AttachmentStoreOp::eStore;
  }
}

W_ALWAYS_INLINE vk::VertexInputRate WConversionUtilsVulkan::GetVertexBindingRate(WEnum<WGALVertexBindingRate> rate)
{
  switch (rate)
  {
    case WGALVertexBindingRate::Vertex:
      return vk::VertexInputRate::eVertex;
    case WGALVertexBindingRate::Instance:
      return vk::VertexInputRate::eInstance;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return vk::VertexInputRate::eVertex;
  }
}

W_ALWAYS_INLINE vk::SampleCountFlagBits WConversionUtilsVulkan::GetSamples(WEnum<WGALMSAASampleCount> samples)
{
  switch (samples)
  {
    case WGALMSAASampleCount::None:
      return vk::SampleCountFlagBits::e1;
    case WGALMSAASampleCount::TwoSamples:
      return vk::SampleCountFlagBits::e2;
    case WGALMSAASampleCount::FourSamples:
      return vk::SampleCountFlagBits::e4;
    case WGALMSAASampleCount::EightSamples:
      return vk::SampleCountFlagBits::e8;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return vk::SampleCountFlagBits::e1;
  }
}

W_ALWAYS_INLINE vk::PresentModeKHR WConversionUtilsVulkan::GetPresentMode(WEnum<WGALPresentMode> presentMode, const WDynamicArray<vk::PresentModeKHR>& supportedModes)
{
  switch (presentMode)
  {
    case WGALPresentMode::Immediate:
    {
      if (supportedModes.Contains(vk::PresentModeKHR::eImmediate))
        return vk::PresentModeKHR::eImmediate;
      else if (supportedModes.Contains(vk::PresentModeKHR::eMailbox))
        return vk::PresentModeKHR::eMailbox;
      else
        return vk::PresentModeKHR::eFifo;
    }
    case WGALPresentMode::VSync:
      return vk::PresentModeKHR::eFifo; // FIFO must be supported according to the standard.
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return vk::PresentModeKHR::eFifo;
  }
}

W_FORCE_INLINE vk::ImageSubresourceRange WConversionUtilsVulkan::GetSubresourceRange(const WGALTextureCreationDescription& texDesc, const WGALRenderTargetViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;
  WGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == WGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = WGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  range.setBaseMipLevel(viewDesc.m_uiMipLevel).setLevelCount(1).setBaseArrayLayer(viewDesc.m_uiFirstSlice).setLayerCount(viewDesc.m_uiSliceCount);
  return range;
}

W_ALWAYS_INLINE vk::ImageSubresourceRange WConversionUtilsVulkan::GetSubresourceRange(
  const vk::ImageSubresourceLayers& layers)
{
  vk::ImageSubresourceRange range;
  range.aspectMask = layers.aspectMask;
  range.baseMipLevel = layers.mipLevel;
  range.levelCount = 1;
  range.baseArrayLayer = layers.baseArrayLayer;
  range.layerCount = layers.layerCount;
  return range;
}

W_ALWAYS_INLINE vk::ImageSubresourceRange WConversionUtilsVulkan::GetSubresourceRange(WGALResourceFormat::Enum format, WGALTextureRange textureRange)
{
  vk::ImageSubresourceRange range;
  range.aspectMask = WGALResourceFormat::IsDepthFormat(format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (format == WGALResourceFormat::D24S8)
  {
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  }
  range.baseMipLevel = textureRange.m_uiBaseMipLevel;
  range.levelCount = textureRange.m_uiMipLevels;
  range.baseArrayLayer = textureRange.m_uiBaseArraySlice;
  range.layerCount = textureRange.m_uiArraySlices;
  return range;
}

W_ALWAYS_INLINE vk::ImageViewType WConversionUtilsVulkan::GetImageViewType(WEnum<WGALTextureType> texType)
{
  switch (texType)
  {
    case WGALTextureType::Texture2D:
    case WGALTextureType::Texture2DShared:
    case WGALTextureType::Texture2DProxy:
      return vk::ImageViewType::e2D;

    case WGALTextureType::Texture2DArray:
      return vk::ImageViewType::e2DArray;

    case WGALTextureType::TextureCube:
      return vk::ImageViewType::eCube;

    case WGALTextureType::TextureCubeArray:
      return vk::ImageViewType::eCubeArray;

    case WGALTextureType::Texture3D:
      return vk::ImageViewType::e3D;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return vk::ImageViewType::e1D;
  }
}

W_ALWAYS_INLINE vk::ImageViewType WConversionUtilsVulkan::GetImageViewType(WEnum<WGALShaderTextureType> texType)
{
  switch (texType)
  {
    case WGALShaderTextureType::Texture2D:
    case WGALShaderTextureType::Texture2DMS:
      return vk::ImageViewType::e2D;
    case WGALShaderTextureType::Texture2DArray:
    case WGALShaderTextureType::Texture2DMSArray:
      return vk::ImageViewType::e2DArray;
    case WGALShaderTextureType::Texture3D:
      return vk::ImageViewType::e3D;
    case WGALShaderTextureType::TextureCube:
      return vk::ImageViewType::eCube;
    case WGALShaderTextureType::TextureCubeArray:
      return vk::ImageViewType::eCubeArray;
    case WGALShaderTextureType::Texture1D:
    case WGALShaderTextureType::Texture1DArray:
    case WGALShaderTextureType::Unknown:
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return vk::ImageViewType::e1D;
  }
}

W_ALWAYS_INLINE vk::ImageViewType WConversionUtilsVulkan::GetImageArrayViewType(WEnum<WGALTextureType> texType)
{
  switch (texType)
  {
    case WGALTextureType::Texture2D:
    case WGALTextureType::Texture2DArray:
    case WGALTextureType::Texture2DProxy:
      return vk::ImageViewType::e2DArray;

    case WGALTextureType::TextureCube:
    case WGALTextureType::TextureCubeArray:
      return vk::ImageViewType::eCubeArray;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return vk::ImageViewType::e1D;
  }
}

W_ALWAYS_INLINE bool WConversionUtilsVulkan::IsDepthFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eD16Unorm:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

W_ALWAYS_INLINE bool WConversionUtilsVulkan::IsStencilFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eS8Uint:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

W_ALWAYS_INLINE vk::ImageLayout WConversionUtilsVulkan::GetTextureReadLayout(vk::Format format)
{
  return IsDepthFormat(format) ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
}

W_ALWAYS_INLINE vk::PrimitiveTopology WConversionUtilsVulkan::GetPrimitiveTopology(WEnum<WGALPrimitiveTopology> topology)
{
  switch (topology)
  {
    case WGALPrimitiveTopology::Points:
      return vk::PrimitiveTopology::ePointList;
    case WGALPrimitiveTopology::Lines:
      return vk::PrimitiveTopology::eLineList;
    case WGALPrimitiveTopology::Triangles:
      return vk::PrimitiveTopology::eTriangleList;
    case WGALPrimitiveTopology::TriangleStrip:
      return vk::PrimitiveTopology::eTriangleStrip;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return vk::PrimitiveTopology::ePointList;
  }
}

W_ALWAYS_INLINE vk::ShaderStageFlagBits WConversionUtilsVulkan::GetShaderStage(WGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case WGALShaderStage::VertexShader:
      return vk::ShaderStageFlagBits::eVertex;
    case WGALShaderStage::HullShader:
      return vk::ShaderStageFlagBits::eTessellationControl;
    case WGALShaderStage::DomainShader:
      return vk::ShaderStageFlagBits::eTessellationEvaluation;
    case WGALShaderStage::GeometryShader:
      return vk::ShaderStageFlagBits::eGeometry;
    case WGALShaderStage::PixelShader:
      return vk::ShaderStageFlagBits::eFragment;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case WGALShaderStage::ComputeShader:
      return vk::ShaderStageFlagBits::eCompute;
  }
}

W_ALWAYS_INLINE vk::PipelineStageFlags WConversionUtilsVulkan::GetPipelineStage(WGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case WGALShaderStage::VertexShader:
      return vk::PipelineStageFlagBits::eVertexShader;
    case WGALShaderStage::HullShader:
      return vk::PipelineStageFlagBits::eTessellationControlShader;
    case WGALShaderStage::DomainShader:
      return vk::PipelineStageFlagBits::eTessellationEvaluationShader;
    case WGALShaderStage::GeometryShader:
      return vk::PipelineStageFlagBits::eGeometryShader;
    case WGALShaderStage::PixelShader:
      return vk::PipelineStageFlagBits::eFragmentShader;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case WGALShaderStage::ComputeShader:
      return vk::PipelineStageFlagBits::eComputeShader;
  }
}

W_ALWAYS_INLINE vk::PipelineStageFlags WConversionUtilsVulkan::GetPipelineStage(vk::ShaderStageFlags flags)
{
  vk::PipelineStageFlags res;
  if (flags & vk::ShaderStageFlagBits::eVertex)
    res |= vk::PipelineStageFlagBits::eVertexShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationControl)
    res |= vk::PipelineStageFlagBits::eTessellationControlShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationEvaluation)
    res |= vk::PipelineStageFlagBits::eTessellationEvaluationShader;
  if (flags & vk::ShaderStageFlagBits::eGeometry)
    res |= vk::PipelineStageFlagBits::eGeometryShader;
  if (flags & vk::ShaderStageFlagBits::eFragment)
    res |= vk::PipelineStageFlagBits::eFragmentShader;
  if (flags & vk::ShaderStageFlagBits::eCompute)
    res |= vk::PipelineStageFlagBits::eComputeShader;

  return res;
}

W_ALWAYS_INLINE vk::DescriptorType WConversionUtilsVulkan::GetDescriptorType(WGALShaderResourceType::Enum type)
{
  switch (type)
  {
    case WGALShaderResourceType::Unknown:
      W_REPORT_FAILURE("Unknown descriptor type");
      break;
    case WGALShaderResourceType::PushConstants:
      W_REPORT_FAILURE("Push constants should never appear as shader resources");
      break;
    case WGALShaderResourceType::Sampler:
      return vk::DescriptorType::eSampler;
    case WGALShaderResourceType::ConstantBuffer:
      return vk::DescriptorType::eUniformBufferDynamic;
    case WGALShaderResourceType::Texture:
      return vk::DescriptorType::eSampledImage;
    case WGALShaderResourceType::TextureAndSampler:
      return vk::DescriptorType::eCombinedImageSampler;
    case WGALShaderResourceType::TexelBuffer:
      return vk::DescriptorType::eUniformTexelBuffer;
    case WGALShaderResourceType::StructuredBuffer:
      return vk::DescriptorType::eStorageBuffer;
    case WGALShaderResourceType::ByteAddressBuffer:
      return vk::DescriptorType::eStorageBuffer;
    case WGALShaderResourceType::TextureRW:
      return vk::DescriptorType::eStorageImage;
    case WGALShaderResourceType::TexelBufferRW:
      return vk::DescriptorType::eStorageTexelBuffer;
    case WGALShaderResourceType::StructuredBufferRW:
      return vk::DescriptorType::eStorageBuffer;
    case WGALShaderResourceType::ByteAddressBufferRW:
      return vk::DescriptorType::eStorageBuffer;
    case WGALShaderResourceType::COUNT:
      W_REPORT_FAILURE("COUNT is not a valid resource type");
      break;
  }
  W_REPORT_FAILURE("Unknown resource type: {}", (int)type);
  return vk::DescriptorType::eMutableVALVE;
}

W_ALWAYS_INLINE vk::ShaderStageFlagBits WConversionUtilsVulkan::GetShaderStages(WBitflags<WGALShaderStageFlags> stages)
{
  return (vk::ShaderStageFlagBits)stages.GetValue();
}

W_ALWAYS_INLINE vk::PipelineStageFlags WConversionUtilsVulkan::GetPipelineStages(WBitflags<WGALShaderStageFlags> stages)
{
  vk::PipelineStageFlags res;
  for (int i = 0; i < WGALShaderStage::ENUM_COUNT; ++i)
  {
    WGALShaderStageFlags::Enum flag = WGALShaderStageFlags::MakeFromShaderStage((WGALShaderStage::Enum)i);
    if (stages.IsSet(flag))
    {
      res |= GetPipelineStage((WGALShaderStage::Enum)i);
    }
  }
  return res;
}

static_assert((WUInt32)vk::ShaderStageFlagBits::eVertex == (WUInt32)WGALShaderStageFlags::VertexShader);
static_assert((WUInt32)vk::ShaderStageFlagBits::eTessellationControl == (WUInt32)WGALShaderStageFlags::HullShader);
static_assert((WUInt32)vk::ShaderStageFlagBits::eTessellationEvaluation == (WUInt32)WGALShaderStageFlags::DomainShader);
static_assert((WUInt32)vk::ShaderStageFlagBits::eGeometry == (WUInt32)WGALShaderStageFlags::GeometryShader);
static_assert((WUInt32)vk::ShaderStageFlagBits::eFragment == (WUInt32)WGALShaderStageFlags::PixelShader);
static_assert((WUInt32)vk::ShaderStageFlagBits::eCompute == (WUInt32)WGALShaderStageFlags::ComputeShader);
