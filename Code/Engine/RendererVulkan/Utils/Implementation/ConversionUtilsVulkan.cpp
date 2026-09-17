#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

void WConversionUtilsVulkan::ConvertResourceState(WBitflags<WGALResourceState> state, vk::PipelineStageFlags& out_stages, vk::AccessFlags& out_access)
{
  out_stages = {};
  out_access = {};

  // All shader stages that can access resources. Unsupported stages must be masked
  // out by the caller using WGALDeviceVulkan::GetSupportedStages().
  constexpr vk::PipelineStageFlags allShaderStages =
    vk::PipelineStageFlagBits::eVertexShader |
    vk::PipelineStageFlagBits::eTessellationControlShader |
    vk::PipelineStageFlagBits::eTessellationEvaluationShader |
    vk::PipelineStageFlagBits::eGeometryShader |
    vk::PipelineStageFlagBits::eFragmentShader |
    vk::PipelineStageFlagBits::eComputeShader;

  for (auto flag : state)
  {
    switch (flag)
    {
      case WGALResourceState::ShaderResource:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits::eShaderRead;
        break;

      case WGALResourceState::ConstantBuffer:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits::eUniformRead;
        break;

      case WGALResourceState::VertexBuffer:
        out_stages |= vk::PipelineStageFlagBits::eVertexInput;
        out_access |= vk::AccessFlagBits::eVertexAttributeRead;
        break;

      case WGALResourceState::IndexBuffer:
        out_stages |= vk::PipelineStageFlagBits::eVertexInput;
        out_access |= vk::AccessFlagBits::eIndexRead;
        break;

      case WGALResourceState::DrawIndirect:
        out_stages |= vk::PipelineStageFlagBits::eDrawIndirect;
        out_access |= vk::AccessFlagBits::eIndirectCommandRead;
        break;

      case WGALResourceState::DepthStencilRead:
        out_stages |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
        out_access |= vk::AccessFlagBits::eDepthStencilAttachmentRead;
        break;

      case WGALResourceState::CopySource:
        out_stages |= vk::PipelineStageFlagBits::eTransfer;
        out_access |= vk::AccessFlagBits::eTransferRead;
        break;

      case WGALResourceState::ResolveSource:
        out_stages |= vk::PipelineStageFlagBits::eTransfer;
        out_access |= vk::AccessFlagBits::eTransferRead;
        break;

      case WGALResourceState::UnorderedAccess:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
        break;

      case WGALResourceState::RenderTarget:
        out_stages |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
        out_access |= vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        break;

      case WGALResourceState::DepthStencilWrite:
        out_stages |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
        out_access |= vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;

      case WGALResourceState::CopyDestination:
        out_stages |= vk::PipelineStageFlagBits::eTransfer;
        out_access |= vk::AccessFlagBits::eTransferWrite;
        break;

      case WGALResourceState::ResolveDestination:
        out_stages |= vk::PipelineStageFlagBits::eTransfer;
        out_access |= vk::AccessFlagBits::eTransferWrite;
        break;

      case WGALResourceState::Present:
        // No stage/access needed — presentation is handled outside the render pipeline.
        break;

      case WGALResourceState::CpuRead:
        out_stages |= vk::PipelineStageFlagBits::eHost;
        out_access |= vk::AccessFlagBits::eHostRead;
        break;

      case WGALResourceState::CpuWrite:
        out_stages |= vk::PipelineStageFlagBits::eHost;
        out_access |= vk::AccessFlagBits::eHostWrite;
        break;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }

  // If no stages were determined, default to top/bottom of pipe.
  if (!out_stages)
    out_stages = vk::PipelineStageFlagBits::eTopOfPipe;
}

void WConversionUtilsVulkan::ConvertResourceState(WBitflags<WGALResourceState> state, vk::PipelineStageFlags2& out_stages, vk::AccessFlags2& out_access)
{
  out_stages = {};
  out_access = {};

  constexpr vk::PipelineStageFlags2 allShaderStages =
    vk::PipelineStageFlagBits2::eVertexShader |
    vk::PipelineStageFlagBits2::eTessellationControlShader |
    vk::PipelineStageFlagBits2::eTessellationEvaluationShader |
    vk::PipelineStageFlagBits2::eGeometryShader |
    vk::PipelineStageFlagBits2::eFragmentShader |
    vk::PipelineStageFlagBits2::eComputeShader;

  for (auto flag : state)
  {
    switch (flag)
    {
      case WGALResourceState::ShaderResource:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits2::eShaderRead;
        break;

      case WGALResourceState::ConstantBuffer:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits2::eUniformRead;
        break;

      case WGALResourceState::VertexBuffer:
        out_stages |= vk::PipelineStageFlagBits2::eVertexAttributeInput;
        out_access |= vk::AccessFlagBits2::eVertexAttributeRead;
        break;

      case WGALResourceState::IndexBuffer:
        out_stages |= vk::PipelineStageFlagBits2::eIndexInput;
        out_access |= vk::AccessFlagBits2::eIndexRead;
        break;

      case WGALResourceState::DrawIndirect:
        out_stages |= vk::PipelineStageFlagBits2::eDrawIndirect;
        out_access |= vk::AccessFlagBits2::eIndirectCommandRead;
        break;

      case WGALResourceState::DepthStencilRead:
        out_stages |= vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        out_access |= vk::AccessFlagBits2::eDepthStencilAttachmentRead;
        break;

      case WGALResourceState::CopySource:
        out_stages |= vk::PipelineStageFlagBits2::eCopy;
        out_access |= vk::AccessFlagBits2::eTransferRead;
        break;

      case WGALResourceState::ResolveSource:
        out_stages |= vk::PipelineStageFlagBits2::eResolve;
        out_access |= vk::AccessFlagBits2::eTransferRead;
        break;

      case WGALResourceState::UnorderedAccess:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
        break;

      case WGALResourceState::RenderTarget:
        out_stages |= vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        out_access |= vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
        break;

      case WGALResourceState::DepthStencilWrite:
        out_stages |= vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        out_access |= vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        break;

      case WGALResourceState::CopyDestination:
        out_stages |= vk::PipelineStageFlagBits2::eCopy;
        out_access |= vk::AccessFlagBits2::eTransferWrite;
        break;

      case WGALResourceState::ResolveDestination:
        out_stages |= vk::PipelineStageFlagBits2::eResolve;
        out_access |= vk::AccessFlagBits2::eTransferWrite;
        break;

      case WGALResourceState::Present:
        break;

      case WGALResourceState::CpuRead:
        out_stages |= vk::PipelineStageFlagBits2::eHost;
        out_access |= vk::AccessFlagBits2::eHostRead;
        break;

      case WGALResourceState::CpuWrite:
        out_stages |= vk::PipelineStageFlagBits2::eHost;
        out_access |= vk::AccessFlagBits2::eHostWrite;
        break;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }

  if (!out_stages)
    out_stages = vk::PipelineStageFlagBits2::eNone;
}

vk::ImageLayout WConversionUtilsVulkan::GetTextureLayout(WBitflags<WGALResourceState> state)
{
  // Write states require a specific layout.
  if (state.IsSet(WGALResourceState::RenderTarget))
    return vk::ImageLayout::eColorAttachmentOptimal;

  if (state.IsSet(WGALResourceState::DepthStencilWrite))
    return vk::ImageLayout::eDepthStencilAttachmentOptimal;

  if (state.IsSet(WGALResourceState::UnorderedAccess))
    return vk::ImageLayout::eGeneral;

  if (state.IsSet(WGALResourceState::CopyDestination))
    return vk::ImageLayout::eTransferDstOptimal;

  if (state.IsSet(WGALResourceState::ResolveDestination))
    return vk::ImageLayout::eTransferDstOptimal;

  // Read-only states.
  // If multiple read flags are set, different layouts would conflict — fall back to eGeneral.
  const WBitflags<WGALResourceState> readStates = state & WGALResourceState::AllReadStates;
  if (readStates.GetValue() & (readStates.GetValue() - 1))
    return vk::ImageLayout::eGeneral;

  if (state.IsSet(WGALResourceState::DepthStencilRead))
    return vk::ImageLayout::eDepthStencilReadOnlyOptimal;

  if (state.IsSet(WGALResourceState::ShaderResource))
    return vk::ImageLayout::eShaderReadOnlyOptimal;

  if (state.IsSet(WGALResourceState::CopySource))
    return vk::ImageLayout::eTransferSrcOptimal;

  if (state.IsSet(WGALResourceState::ResolveSource))
    return vk::ImageLayout::eTransferSrcOptimal;

  // Special states.
  if (state.IsSet(WGALResourceState::Present))
    return vk::ImageLayout::ePresentSrcKHR;

  // Unknown or no state.
  return vk::ImageLayout::eUndefined;
}
