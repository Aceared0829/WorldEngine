#include <RendererVulkan/RendererVulkanPCH.h>

// #define VK_LOG_LAYOUT_CHANGES

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/DispatchContext.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/BarrierUtilsVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

namespace
{
  // ============================================================================================
  // Helpers shared between Synchronization2 and Vulkan 1.1 code paths.
  // ============================================================================================
  vk::PipelineStageFlags2 GetUnsupportedStages2(const WGALDeviceVulkan& device)
  {
    return static_cast<vk::PipelineStageFlags2>(static_cast<VkPipelineStageFlags>(device.GetUnsupportedStages()));
  }

  vk::ImageSubresourceRange CreateSubresourceRange(const WGALTextureVulkan& texture, const WGALTextureBarrier& barrier)
  {
    if (barrier.m_bAllSubresources)
      return texture.GetFullRange();

    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = texture.GetAspectMask();
    subresourceRange.baseMipLevel = barrier.m_Subresource.m_uiMipLevel;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = barrier.m_Subresource.m_uiArraySlice;
    subresourceRange.layerCount = 1;
    return subresourceRange;
  }

  vk::ImageSubresourceRange CreateSubresourceRange(const WTextureBarrierVulkan& barrier)
  {
    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = barrier.m_AspectMask;

    if (barrier.m_bAllSubresources)
    {
      subresourceRange.baseMipLevel = 0;
      subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
      subresourceRange.baseArrayLayer = 0;
      subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    }
    else
    {
      subresourceRange.baseMipLevel = barrier.m_Subresource.m_uiMipLevel;
      subresourceRange.levelCount = 1;
      subresourceRange.baseArrayLayer = barrier.m_Subresource.m_uiArraySlice;
      subresourceRange.layerCount = 1;
    }

    return subresourceRange;
  }

  // ============================================================================================
  // Synchronization2 code path (VK_KHR_synchronization2).
  // ============================================================================================

  namespace Sync2
  {
    vk::PipelineStageFlags2 GetPipelineStages(WBitflags<WGALShaderStageFlags> stages)
    {
      vk::PipelineStageFlags2 res;
      if (stages.IsSet(WGALShaderStageFlags::VertexShader))
        res |= vk::PipelineStageFlagBits2::eVertexShader;
      if (stages.IsSet(WGALShaderStageFlags::HullShader))
        res |= vk::PipelineStageFlagBits2::eTessellationControlShader;
      if (stages.IsSet(WGALShaderStageFlags::DomainShader))
        res |= vk::PipelineStageFlagBits2::eTessellationEvaluationShader;
      if (stages.IsSet(WGALShaderStageFlags::GeometryShader))
        res |= vk::PipelineStageFlagBits2::eGeometryShader;
      if (stages.IsSet(WGALShaderStageFlags::PixelShader))
        res |= vk::PipelineStageFlagBits2::eFragmentShader;
      if (stages.IsSet(WGALShaderStageFlags::ComputeShader))
        res |= vk::PipelineStageFlagBits2::eComputeShader;
      return res;
    }

    void ResolveBarrierSync(
      WBitflags<WGALResourceState> stateBefore,
      WBitflags<WGALResourceState> stateAfter,
      WBitflags<WGALShaderStageFlags> stagesBefore,
      WBitflags<WGALShaderStageFlags> stagesAfter,
      vk::PipelineStageFlags2 unsupportedStages,
      vk::PipelineStageFlags2& out_srcStages,
      vk::PipelineStageFlags2& out_dstStages,
      vk::AccessFlags2& out_srcAccess,
      vk::AccessFlags2& out_dstAccess)
    {
      WConversionUtilsVulkan::ConvertResourceState(stateBefore, out_srcStages, out_srcAccess);
      WConversionUtilsVulkan::ConvertResourceState(stateAfter, out_dstStages, out_dstAccess);

      if (!stagesBefore.IsSet(WGALShaderStageFlags::Auto))
      {
        const vk::PipelineStageFlags2 explicitStages = GetPipelineStages(stagesBefore);
        if (explicitStages != vk::PipelineStageFlags2{} && (explicitStages & out_srcStages) == explicitStages)
          out_srcStages = explicitStages;
      }

      if (!stagesAfter.IsSet(WGALShaderStageFlags::Auto))
      {
        const vk::PipelineStageFlags2 explicitStages = GetPipelineStages(stagesAfter);
        if (explicitStages != vk::PipelineStageFlags2{} && (explicitStages & out_dstStages) == explicitStages)
          out_dstStages = explicitStages;
      }

      out_srcStages &= ~unsupportedStages;
      out_dstStages &= ~unsupportedStages;
    }

    vk::ImageMemoryBarrier2 MakeImageBarrier(
      vk::Image image,
      const vk::ImageSubresourceRange& subresourceRange,
      WBitflags<WGALResourceState> stateBefore,
      WBitflags<WGALResourceState> stateAfter,
      WBitflags<WGALShaderStageFlags> stagesBefore,
      WBitflags<WGALShaderStageFlags> stagesAfter,
      vk::PipelineStageFlags2 unsupportedStages,
      bool bDiscard = false)
    {
      vk::ImageMemoryBarrier2 vkBarrier;

      ResolveBarrierSync(stateBefore, stateAfter, stagesBefore, stagesAfter, unsupportedStages,
        vkBarrier.srcStageMask, vkBarrier.dstStageMask, vkBarrier.srcAccessMask, vkBarrier.dstAccessMask);

      vkBarrier.oldLayout = bDiscard ? vk::ImageLayout::eUndefined : WConversionUtilsVulkan::GetTextureLayout(stateBefore);
      vkBarrier.newLayout = WConversionUtilsVulkan::GetTextureLayout(stateAfter);
      vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.image = image;
      vkBarrier.subresourceRange = subresourceRange;

      return vkBarrier;
    }

    vk::BufferMemoryBarrier2 MakeBufferBarrier(
      vk::Buffer buffer,
      WBitflags<WGALResourceState> stateBefore,
      WBitflags<WGALResourceState> stateAfter,
      WBitflags<WGALShaderStageFlags> stagesBefore,
      WBitflags<WGALShaderStageFlags> stagesAfter,
      vk::PipelineStageFlags2 unsupportedStages)
    {
      vk::BufferMemoryBarrier2 vkBarrier;

      ResolveBarrierSync(stateBefore, stateAfter, stagesBefore, stagesAfter, unsupportedStages,
        vkBarrier.srcStageMask, vkBarrier.dstStageMask, vkBarrier.srcAccessMask, vkBarrier.dstAccessMask);

      vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.buffer = buffer;
      vkBarrier.offset = 0;
      vkBarrier.size = VK_WHOLE_SIZE;

      return vkBarrier;
    }

    void SubmitImageBarriers(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, WArrayPtr<const vk::ImageMemoryBarrier2> barriers)
    {
      if (barriers.IsEmpty())
        return;

#ifdef VK_LOG_LAYOUT_CHANGES
      for (const vk::ImageMemoryBarrier2& b : barriers)
      {
        WLog::Info("CommandBuffer: {}, VkBarrier: Image {} | {} -> {}", WArgP(static_cast<VkCommandBuffer>(ref_commandBuffer)), WArgP(static_cast<VkImage>(b.image)), vk::to_string(b.oldLayout).c_str(), vk::to_string(b.newLayout).c_str());
      }
#endif

      vk::DependencyInfo depInfo;
      depInfo.imageMemoryBarrierCount = barriers.GetCount();
      depInfo.pImageMemoryBarriers = barriers.GetPtr();
      ref_commandBuffer.pipelineBarrier2KHR(depInfo, device.GetDispatchContext());
    }

    void SubmitBufferBarriers(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, WArrayPtr<const vk::BufferMemoryBarrier2> barriers)
    {
      if (barriers.IsEmpty())
        return;

      vk::DependencyInfo depInfo;
      depInfo.bufferMemoryBarrierCount = barriers.GetCount();
      depInfo.pBufferMemoryBarriers = barriers.GetPtr();
      ref_commandBuffer.pipelineBarrier2KHR(depInfo, device.GetDispatchContext());
    }

    void TextureBarrier(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, WArrayPtr<const WGALTextureBarrier> barriers)
    {
      WHybridArray<vk::ImageMemoryBarrier2, 16, WTempAllocatorWrapper> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      const vk::PipelineStageFlags2 unsupportedStages = GetUnsupportedStages2(device);

      for (const WGALTextureBarrier& barrier : barriers)
      {
        const WGALTextureVulkan* pTexture = static_cast<const WGALTextureVulkan*>(device.GetTexture(barrier.m_hTexture)->GetParentResource());
        if (pTexture == nullptr)
          continue;

        vkBarriers.PushBack(MakeImageBarrier(
          pTexture->GetImage(),
          CreateSubresourceRange(*pTexture, barrier),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          unsupportedStages,
          barrier.m_bDiscard));
      }

      SubmitImageBarriers(device, ref_commandBuffer, vkBarriers.GetArrayPtr());
    }

    void TextureBarrier(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, WArrayPtr<const WTextureBarrierVulkan> barriers)
    {
      WHybridArray<vk::ImageMemoryBarrier2, 16, WTempAllocatorWrapper> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      const vk::PipelineStageFlags2 unsupportedStages = GetUnsupportedStages2(device);

      for (const WTextureBarrierVulkan& barrier : barriers)
      {
        if (!barrier.m_Image)
          continue;

        vkBarriers.PushBack(MakeImageBarrier(
          barrier.m_Image,
          CreateSubresourceRange(barrier),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          unsupportedStages,
          barrier.m_bDiscard));
      }

      SubmitImageBarriers(device, ref_commandBuffer, vkBarriers.GetArrayPtr());
    }

    void BufferBarrier(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, WArrayPtr<const WGALBufferBarrier> barriers)
    {
      WHybridArray<vk::BufferMemoryBarrier2, 16, WTempAllocatorWrapper> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      const vk::PipelineStageFlags2 unsupportedStages = GetUnsupportedStages2(device);

      for (const WGALBufferBarrier& barrier : barriers)
      {
        const WGALBufferVulkan* pBuffer = static_cast<const WGALBufferVulkan*>(device.GetBuffer(barrier.m_hBuffer));
        if (pBuffer == nullptr)
          continue;

        vkBarriers.PushBack(MakeBufferBarrier(
          pBuffer->GetVkBuffer(),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          unsupportedStages));
      }

      SubmitBufferBarriers(device, ref_commandBuffer, vkBarriers.GetArrayPtr());
    }

    void BufferBarrier(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, WArrayPtr<const WBufferBarrierVulkan> barriers)
    {
      WHybridArray<vk::BufferMemoryBarrier2, 16, WTempAllocatorWrapper> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      const vk::PipelineStageFlags2 unsupportedStages = GetUnsupportedStages2(device);

      for (const WBufferBarrierVulkan& barrier : barriers)
      {
        if (!barrier.m_Buffer)
          continue;

        vkBarriers.PushBack(MakeBufferBarrier(
          barrier.m_Buffer,
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          unsupportedStages));
      }

      SubmitBufferBarriers(device, ref_commandBuffer, vkBarriers.GetArrayPtr());
    }

    void TextureBarrier(
      const WGALDeviceVulkan& device,
      vk::CommandBuffer& ref_commandBuffer,
      vk::Image image,
      const vk::ImageSubresourceRange& subresourceRange,
      WBitflags<WGALResourceState> stateBefore,
      WBitflags<WGALResourceState> stateAfter,
      WBitflags<WGALShaderStageFlags> stagesBefore,
      WBitflags<WGALShaderStageFlags> stagesAfter)
    {
      const vk::ImageMemoryBarrier2 vkBarrier = MakeImageBarrier(image, subresourceRange, stateBefore, stateAfter, stagesBefore, stagesAfter, GetUnsupportedStages2(device));
      SubmitImageBarriers(device, ref_commandBuffer, WMakeArrayPtr(&vkBarrier, 1));
    }

    void BufferBarrier(
      const WGALDeviceVulkan& device,
      vk::CommandBuffer& ref_commandBuffer,
      vk::Buffer buffer,
      WBitflags<WGALResourceState> stateBefore,
      WBitflags<WGALResourceState> stateAfter,
      WBitflags<WGALShaderStageFlags> stagesBefore,
      WBitflags<WGALShaderStageFlags> stagesAfter)
    {
      const vk::BufferMemoryBarrier2 vkBarrier = MakeBufferBarrier(buffer, stateBefore, stateAfter, stagesBefore, stagesAfter, GetUnsupportedStages2(device));
      SubmitBufferBarriers(device, ref_commandBuffer, WMakeArrayPtr(&vkBarrier, 1));
    }
  } // namespace Sync2

  // ============================================================================================
  // Vulkan 1.1 fallback code path (vkCmdPipelineBarrier).
  //
  // Unlike Synchronization2, the legacy API takes a single src/dst stage mask per pipelineBarrier
  // call rather than per-barrier. The helpers therefore accumulate the combined stage mask while
  // building the barrier list and pass it to the submit function.
  // ============================================================================================

  namespace Sync1
  {
    vk::PipelineStageFlags GetPipelineStages(WBitflags<WGALShaderStageFlags> stages)
    {
      vk::PipelineStageFlags res;
      if (stages.IsSet(WGALShaderStageFlags::VertexShader))
        res |= vk::PipelineStageFlagBits::eVertexShader;
      if (stages.IsSet(WGALShaderStageFlags::HullShader))
        res |= vk::PipelineStageFlagBits::eTessellationControlShader;
      if (stages.IsSet(WGALShaderStageFlags::DomainShader))
        res |= vk::PipelineStageFlagBits::eTessellationEvaluationShader;
      if (stages.IsSet(WGALShaderStageFlags::GeometryShader))
        res |= vk::PipelineStageFlagBits::eGeometryShader;
      if (stages.IsSet(WGALShaderStageFlags::PixelShader))
        res |= vk::PipelineStageFlagBits::eFragmentShader;
      if (stages.IsSet(WGALShaderStageFlags::ComputeShader))
        res |= vk::PipelineStageFlagBits::eComputeShader;
      return res;
    }

    void ResolveBarrierSync(
      WBitflags<WGALResourceState> stateBefore,
      WBitflags<WGALResourceState> stateAfter,
      WBitflags<WGALShaderStageFlags> stagesBefore,
      WBitflags<WGALShaderStageFlags> stagesAfter,
      vk::PipelineStageFlags unsupportedStages,
      vk::PipelineStageFlags& out_srcStages,
      vk::PipelineStageFlags& out_dstStages,
      vk::AccessFlags& out_srcAccess,
      vk::AccessFlags& out_dstAccess)
    {
      WConversionUtilsVulkan::ConvertResourceState(stateBefore, out_srcStages, out_srcAccess);
      WConversionUtilsVulkan::ConvertResourceState(stateAfter, out_dstStages, out_dstAccess);

      if (!stagesBefore.IsSet(WGALShaderStageFlags::Auto))
      {
        const vk::PipelineStageFlags explicitStages = GetPipelineStages(stagesBefore);
        if (explicitStages != vk::PipelineStageFlags{} && (explicitStages & out_srcStages) == explicitStages)
          out_srcStages = explicitStages;
      }

      if (!stagesAfter.IsSet(WGALShaderStageFlags::Auto))
      {
        const vk::PipelineStageFlags explicitStages = GetPipelineStages(stagesAfter);
        if (explicitStages != vk::PipelineStageFlags{} && (explicitStages & out_dstStages) == explicitStages)
          out_dstStages = explicitStages;
      }

      out_srcStages &= ~unsupportedStages;
      out_dstStages &= ~unsupportedStages;

      // Unlike Synchronization2 there is no eNone, and a zero stage mask is invalid.
      if (!out_srcStages)
        out_srcStages = vk::PipelineStageFlagBits::eTopOfPipe;
      if (!out_dstStages)
        out_dstStages = vk::PipelineStageFlagBits::eBottomOfPipe;
    }

    vk::ImageMemoryBarrier MakeImageBarrier(
      vk::Image image,
      const vk::ImageSubresourceRange& subresourceRange,
      WBitflags<WGALResourceState> stateBefore,
      WBitflags<WGALResourceState> stateAfter,
      WBitflags<WGALShaderStageFlags> stagesBefore,
      WBitflags<WGALShaderStageFlags> stagesAfter,
      vk::PipelineStageFlags unsupportedStages,
      vk::PipelineStageFlags& inout_srcStages,
      vk::PipelineStageFlags& inout_dstStages,
      bool bDiscard = false)
    {
      vk::ImageMemoryBarrier vkBarrier;
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      ResolveBarrierSync(stateBefore, stateAfter, stagesBefore, stagesAfter, unsupportedStages,
        srcStages, dstStages, vkBarrier.srcAccessMask, vkBarrier.dstAccessMask);

      inout_srcStages |= srcStages;
      inout_dstStages |= dstStages;

      vkBarrier.oldLayout = bDiscard ? vk::ImageLayout::eUndefined : WConversionUtilsVulkan::GetTextureLayout(stateBefore);
      vkBarrier.newLayout = WConversionUtilsVulkan::GetTextureLayout(stateAfter);
      vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.image = image;
      vkBarrier.subresourceRange = subresourceRange;

      return vkBarrier;
    }

    vk::BufferMemoryBarrier MakeBufferBarrier(
      vk::Buffer buffer,
      WBitflags<WGALResourceState> stateBefore,
      WBitflags<WGALResourceState> stateAfter,
      WBitflags<WGALShaderStageFlags> stagesBefore,
      WBitflags<WGALShaderStageFlags> stagesAfter,
      vk::PipelineStageFlags unsupportedStages,
      vk::PipelineStageFlags& inout_srcStages,
      vk::PipelineStageFlags& inout_dstStages)
    {
      vk::BufferMemoryBarrier vkBarrier;
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      ResolveBarrierSync(stateBefore, stateAfter, stagesBefore, stagesAfter, unsupportedStages,
        srcStages, dstStages, vkBarrier.srcAccessMask, vkBarrier.dstAccessMask);

      inout_srcStages |= srcStages;
      inout_dstStages |= dstStages;

      vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.buffer = buffer;
      vkBarrier.offset = 0;
      vkBarrier.size = VK_WHOLE_SIZE;

      return vkBarrier;
    }

    void SubmitImageBarriers(
      vk::CommandBuffer& ref_commandBuffer,
      vk::PipelineStageFlags srcStages,
      vk::PipelineStageFlags dstStages,
      WArrayPtr<const vk::ImageMemoryBarrier> barriers)
    {
      if (barriers.IsEmpty())
        return;

#ifdef VK_LOG_LAYOUT_CHANGES
      for (const vk::ImageMemoryBarrier& b : barriers)
      {
        WLog::Info("CommandBuffer: {}, VkBarrier: Image {} | {} -> {}", WArgP(static_cast<VkCommandBuffer>(ref_commandBuffer)), WArgP(static_cast<VkImage>(b.image)), vk::to_string(b.oldLayout).c_str(), vk::to_string(b.newLayout).c_str());
      }
#endif

      ref_commandBuffer.pipelineBarrier(srcStages, dstStages, vk::DependencyFlags(),
        0, nullptr,
        0, nullptr,
        barriers.GetCount(), barriers.GetPtr());
    }

    void SubmitBufferBarriers(
      vk::CommandBuffer& ref_commandBuffer,
      vk::PipelineStageFlags srcStages,
      vk::PipelineStageFlags dstStages,
      WArrayPtr<const vk::BufferMemoryBarrier> barriers)
    {
      if (barriers.IsEmpty())
        return;

      ref_commandBuffer.pipelineBarrier(srcStages, dstStages, vk::DependencyFlags(),
        0, nullptr,
        barriers.GetCount(), barriers.GetPtr(),
        0, nullptr);
    }

    void TextureBarrier(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, WArrayPtr<const WGALTextureBarrier> barriers)
    {
      WHybridArray<vk::ImageMemoryBarrier, 16, WTempAllocatorWrapper> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      const vk::PipelineStageFlags unsupportedStages = device.GetUnsupportedStages();
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      for (const WGALTextureBarrier& barrier : barriers)
      {
        const WGALTextureVulkan* pTexture = static_cast<const WGALTextureVulkan*>(device.GetTexture(barrier.m_hTexture)->GetParentResource());
        if (pTexture == nullptr)
          continue;

        vkBarriers.PushBack(MakeImageBarrier(
          pTexture->GetImage(),
          CreateSubresourceRange(*pTexture, barrier),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          unsupportedStages,
          srcStages,
          dstStages,
          barrier.m_bDiscard));
      }

      SubmitImageBarriers(ref_commandBuffer, srcStages, dstStages, vkBarriers.GetArrayPtr());
    }

    void TextureBarrier(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, WArrayPtr<const WTextureBarrierVulkan> barriers)
    {
      WHybridArray<vk::ImageMemoryBarrier, 16, WTempAllocatorWrapper> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      const vk::PipelineStageFlags unsupportedStages = device.GetUnsupportedStages();
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      for (const WTextureBarrierVulkan& barrier : barriers)
      {
        if (!barrier.m_Image)
          continue;

        vkBarriers.PushBack(MakeImageBarrier(
          barrier.m_Image,
          CreateSubresourceRange(barrier),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          unsupportedStages,
          srcStages,
          dstStages,
          barrier.m_bDiscard));
      }

      SubmitImageBarriers(ref_commandBuffer, srcStages, dstStages, vkBarriers.GetArrayPtr());
    }

    void BufferBarrier(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, WArrayPtr<const WGALBufferBarrier> barriers)
    {
      WHybridArray<vk::BufferMemoryBarrier, 16, WTempAllocatorWrapper> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      const vk::PipelineStageFlags unsupportedStages = device.GetUnsupportedStages();
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      for (const WGALBufferBarrier& barrier : barriers)
      {
        const WGALBufferVulkan* pBuffer = static_cast<const WGALBufferVulkan*>(device.GetBuffer(barrier.m_hBuffer));
        if (pBuffer == nullptr)
          continue;

        vkBarriers.PushBack(MakeBufferBarrier(
          pBuffer->GetVkBuffer(),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          unsupportedStages,
          srcStages,
          dstStages));
      }

      SubmitBufferBarriers(ref_commandBuffer, srcStages, dstStages, vkBarriers.GetArrayPtr());
    }

    void BufferBarrier(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, WArrayPtr<const WBufferBarrierVulkan> barriers)
    {
      WHybridArray<vk::BufferMemoryBarrier, 16, WTempAllocatorWrapper> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      const vk::PipelineStageFlags unsupportedStages = device.GetUnsupportedStages();
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      for (const WBufferBarrierVulkan& barrier : barriers)
      {
        if (!barrier.m_Buffer)
          continue;

        vkBarriers.PushBack(MakeBufferBarrier(
          barrier.m_Buffer,
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          unsupportedStages,
          srcStages,
          dstStages));
      }

      SubmitBufferBarriers(ref_commandBuffer, srcStages, dstStages, vkBarriers.GetArrayPtr());
    }

    void TextureBarrier(
      const WGALDeviceVulkan& device,
      vk::CommandBuffer& ref_commandBuffer,
      vk::Image image,
      const vk::ImageSubresourceRange& subresourceRange,
      WBitflags<WGALResourceState> stateBefore,
      WBitflags<WGALResourceState> stateAfter,
      WBitflags<WGALShaderStageFlags> stagesBefore,
      WBitflags<WGALShaderStageFlags> stagesAfter)
    {
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;
      const vk::ImageMemoryBarrier vkBarrier = MakeImageBarrier(image, subresourceRange, stateBefore, stateAfter, stagesBefore, stagesAfter, device.GetUnsupportedStages(), srcStages, dstStages);
      SubmitImageBarriers(ref_commandBuffer, srcStages, dstStages, WMakeArrayPtr(&vkBarrier, 1));
    }

    void BufferBarrier(
      const WGALDeviceVulkan& device,
      vk::CommandBuffer& ref_commandBuffer,
      vk::Buffer buffer,
      WBitflags<WGALResourceState> stateBefore,
      WBitflags<WGALResourceState> stateAfter,
      WBitflags<WGALShaderStageFlags> stagesBefore,
      WBitflags<WGALShaderStageFlags> stagesAfter)
    {
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;
      const vk::BufferMemoryBarrier vkBarrier = MakeBufferBarrier(buffer, stateBefore, stateAfter, stagesBefore, stagesAfter, device.GetUnsupportedStages(), srcStages, dstStages);
      SubmitBufferBarriers(ref_commandBuffer, srcStages, dstStages, WMakeArrayPtr(&vkBarrier, 1));
    }
  } // namespace Sync1
} // namespace

WBarrierUtilsVulkan::WBarrierUtilsVulkan(const WGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer)
  : m_Device(device)
  , m_CommandBuffer(ref_commandBuffer)
{
}

void WBarrierUtilsVulkan::TextureBarrier(WArrayPtr<const WGALTextureBarrier> barriers)
{
  if (barriers.IsEmpty())
    return;

  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::TextureBarrier(m_Device, m_CommandBuffer, barriers);
  else
    Sync1::TextureBarrier(m_Device, m_CommandBuffer, barriers);
}

void WBarrierUtilsVulkan::TextureBarrier(WArrayPtr<const WTextureBarrierVulkan> barriers)
{
  if (barriers.IsEmpty())
    return;

  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::TextureBarrier(m_Device, m_CommandBuffer, barriers);
  else
    Sync1::TextureBarrier(m_Device, m_CommandBuffer, barriers);
}

void WBarrierUtilsVulkan::BufferBarrier(WArrayPtr<const WGALBufferBarrier> barriers)
{
  if (barriers.IsEmpty())
    return;

  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::BufferBarrier(m_Device, m_CommandBuffer, barriers);
  else
    Sync1::BufferBarrier(m_Device, m_CommandBuffer, barriers);
}

void WBarrierUtilsVulkan::BufferBarrier(WArrayPtr<const WBufferBarrierVulkan> barriers)
{
  if (barriers.IsEmpty())
    return;

  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::BufferBarrier(m_Device, m_CommandBuffer, barriers);
  else
    Sync1::BufferBarrier(m_Device, m_CommandBuffer, barriers);
}

void WBarrierUtilsVulkan::TextureBarrier(
  WGALTextureHandle hTexture,
  WBitflags<WGALResourceState> stateBefore,
  WBitflags<WGALResourceState> stateAfter,
  WBitflags<WGALShaderStageFlags> stagesBefore,
  WBitflags<WGALShaderStageFlags> stagesAfter)
{
  const WGALTextureVulkan* pTexture = static_cast<const WGALTextureVulkan*>(m_Device.GetTexture(hTexture)->GetParentResource());
  W_ASSERT_DEV(pTexture != nullptr, "Invalid texture handle.");

  TextureBarrier(pTexture->GetImage(), pTexture->GetFullRange(), stateBefore, stateAfter, stagesBefore, stagesAfter);
}

void WBarrierUtilsVulkan::TextureBarrier(
  vk::Image image,
  const vk::ImageSubresourceRange& subresourceRange,
  WBitflags<WGALResourceState> stateBefore,
  WBitflags<WGALResourceState> stateAfter,
  WBitflags<WGALShaderStageFlags> stagesBefore,
  WBitflags<WGALShaderStageFlags> stagesAfter)
{
  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::TextureBarrier(m_Device, m_CommandBuffer, image, subresourceRange, stateBefore, stateAfter, stagesBefore, stagesAfter);
  else
    Sync1::TextureBarrier(m_Device, m_CommandBuffer, image, subresourceRange, stateBefore, stateAfter, stagesBefore, stagesAfter);
}

void WBarrierUtilsVulkan::BufferBarrier(
  WGALBufferHandle hBuffer,
  WBitflags<WGALResourceState> stateBefore,
  WBitflags<WGALResourceState> stateAfter,
  WBitflags<WGALShaderStageFlags> stagesBefore,
  WBitflags<WGALShaderStageFlags> stagesAfter)
{
  const WGALBufferVulkan* pBuffer = static_cast<const WGALBufferVulkan*>(m_Device.GetBuffer(hBuffer));
  W_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle.");

  BufferBarrier(pBuffer->GetVkBuffer(), stateBefore, stateAfter, stagesBefore, stagesAfter);
}

void WBarrierUtilsVulkan::BufferBarrier(
  vk::Buffer buffer,
  WBitflags<WGALResourceState> stateBefore,
  WBitflags<WGALResourceState> stateAfter,
  WBitflags<WGALShaderStageFlags> stagesBefore,
  WBitflags<WGALShaderStageFlags> stagesAfter)
{
  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::BufferBarrier(m_Device, m_CommandBuffer, buffer, stateBefore, stateAfter, stagesBefore, stagesAfter);
  else
    Sync1::BufferBarrier(m_Device, m_CommandBuffer, buffer, stateBefore, stateAfter, stagesBefore, stagesAfter);
}
