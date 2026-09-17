#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/BarrierUtilsVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

WInitContextVulkan::WInitContextVulkan(WGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
{
  WAllocator* pAllocator = m_pDevice->GetAllocator();
  m_pCommandBufferPool = W_NEW(pAllocator, WCommandBufferPoolVulkan, pAllocator);
  m_pCommandBufferPool->Initialize(m_pDevice->GetVulkanDevice(), m_pDevice->GetGraphicsQueue().m_uiQueueFamily);
  m_pStagingBufferPool = W_NEW(pAllocator, WStagingBufferPoolVulkan);
  m_pStagingBufferPool->Initialize(m_pDevice, 50 * 1024 * 1024);
}

WInitContextVulkan::~WInitContextVulkan()
{
  W_ASSERT_DEBUG(!m_CurrentCommandBuffer, "GetFinishedCommandBuffer should have been called before destruction.");

  m_pCommandBufferPool->DeInitialize();
  m_pStagingBufferPool->DeInitialize();
  m_pCommandBufferPool.Clear();
  m_pStagingBufferPool.Clear();
}


void WInitContextVulkan::AfterBeginFrame()
{
  W_LOCK(m_Lock);
  m_pStagingBufferPool->AfterBeginFrame();
}

vk::CommandBuffer WInitContextVulkan::GetFinishedCommandBuffer()
{
  W_LOCK(m_Lock);
  if (m_CurrentCommandBuffer)
  {
    m_pStagingBufferPool->BeforeCommandBufferSubmit();
    if (m_pDevice->GetExtensions().m_bDebugUtilsMarkers)
    {
      m_CurrentCommandBuffer.endDebugUtilsLabelEXT(m_pDevice->GetDispatchContext());
    }
    vk::CommandBuffer res = m_CurrentCommandBuffer;
    res.end();

    m_pDevice->ReclaimLater(m_CurrentCommandBuffer, m_pCommandBufferPool.Borrow());
    return res;
  }
  return nullptr;
}

void WInitContextVulkan::EnsureCommandBufferExists()
{
  if (!m_CurrentCommandBuffer)
  {
    // Restart new command buffer if none is active already.
    m_CurrentCommandBuffer = m_pCommandBufferPool->RequestCommandBuffer();
    vk::CommandBufferBeginInfo beginInfo;
    m_CurrentCommandBuffer.begin(&beginInfo);
    if (m_pDevice->GetExtensions().m_bDebugUtilsMarkers)
    {
      constexpr float markerColor[4] = {0, 0, 0, 0};
      vk::DebugUtilsLabelEXT markerInfo = {};
      WMemoryUtils::Copy(markerInfo.color.data(), markerColor, W_ARRAY_SIZE(markerColor));
      markerInfo.pLabelName = "InitContext";

      m_CurrentCommandBuffer.beginDebugUtilsLabelEXT(markerInfo, m_pDevice->GetDispatchContext());
    }
  }
}


void WInitContextVulkan::InitTexture(const WGALTextureVulkan* pTexture, vk::ImageCreateInfo& ref_createInfo, WArrayPtr<WGALSystemMemoryDescription> initialData)
{
  W_LOCK(m_Lock);

  EnsureCommandBufferExists();

  const WBitflags<WGALResourceState> defaultState = pTexture->GetDescription().GetDefaultState();

  WBarrierUtilsVulkan barriers(*m_pDevice, m_CurrentCommandBuffer);

  if (pTexture->GetDescription().m_pExisitingNativeObject == nullptr)
  {
    if (pTexture->GetDescription().m_SampleCount != WGALMSAASampleCount::None)
    {
      // #TODO_VULKAN how do we clear a MS target to zero?
      // Transition MSAA target from undefined to its default state.
      barriers.TextureBarrier(pTexture->GetImage(), pTexture->GetFullRange(),
        WGALResourceState::Unknown, defaultState);
      return;
    }

    WDynamicArray<WGALSystemMemoryDescription> zeroInitialData;
    if (initialData.IsEmpty())
    {
      // If we don't have any initial data to initialize the texture to zero memory to match DX11 behavior.
      const vk::Format format = pTexture->GetImageFormat();
      const WUInt8 uiBlockSize = vk::blockSize(format);
      const auto blockExtent = vk::blockExtent(format);
      {
        // Compute max size of the temp buffer.
        const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(0);
        const VkExtent3D blockCount = {
          (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
          (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
          (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};
        const WUInt32 uiTotalSize = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
        if (m_TempData.GetCount() < uiTotalSize)
          m_TempData.SetCount(uiTotalSize, 0);
      }

      for (WUInt32 uiLayer = 0; uiLayer < ref_createInfo.arrayLayers; uiLayer++)
      {
        for (WUInt32 uiMipLevel = 0; uiMipLevel < ref_createInfo.mipLevels; uiMipLevel++)
        {
          const WUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * ref_createInfo.mipLevels;
          W_ASSERT_DEBUG(zeroInitialData.GetCount() == uiSubresourceIndex, "");

          const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(uiMipLevel);
          const VkExtent3D blockCount = {
            (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
            (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
            (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};

          WGALSystemMemoryDescription data;
          data.m_pData = m_TempData.GetByteArrayPtr();
          data.m_uiRowPitch = uiBlockSize * blockCount.width;
          data.m_uiSlicePitch = data.m_uiRowPitch * blockCount.height;
          zeroInitialData.PushBack(data);
        }
      }
      initialData = zeroInitialData.GetArrayPtr();
    }

    // Transition entire image from undefined to transfer destination.
    barriers.TextureBarrier(pTexture->GetImage(), pTexture->GetFullRange(),
      WGALResourceState::Unknown, WGALResourceState::CopyDestination);

    for (WUInt32 uiLayer = 0; uiLayer < ref_createInfo.arrayLayers; uiLayer++)
    {
      for (WUInt32 uiMipLevel = 0; uiMipLevel < ref_createInfo.mipLevels; uiMipLevel++)
      {
        const WUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * ref_createInfo.mipLevels;
        W_ASSERT_DEBUG(uiSubresourceIndex < initialData.GetCount(), "Not all data provided in the intial texture data.");
        const WGALSystemMemoryDescription& subResourceData = initialData[uiSubresourceIndex];

        vk::ImageSubresourceLayers subresourceLayers;
        // We do not support stencil uploads right now.
        subresourceLayers.aspectMask = WConversionUtilsVulkan::IsDepthFormat(pTexture->GetImageFormat()) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
        subresourceLayers.mipLevel = uiMipLevel;
        subresourceLayers.baseArrayLayer = uiLayer;
        subresourceLayers.layerCount = 1;

        const vk::Offset3D imageOffset = {0, 0, 0};
        const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(uiMipLevel);

        // Image is already in CopyDestination, so skip before/after image transitions inside UploadTextureStaging.
        m_pDevice->UploadTextureStaging(*m_pDevice, m_pStagingBufferPool.Borrow(), m_CurrentCommandBuffer, pTexture, subresourceLayers, imageOffset, imageExtent, subResourceData);
      }
    }

    // Transition entire image from transfer destination to default state.
    barriers.TextureBarrier(pTexture->GetImage(), pTexture->GetFullRange(),
      WGALResourceState::CopyDestination, defaultState);
  }
  else
  {
    // We don't actually know what the current state is of an existing native object, so switch from Unknown to default.
    // This must not be done for swap chains textures as these are not allowed to be touched until acquired. The initial layout is set for these at the first time of acquire.
    if (!pTexture->GetDescription().m_TextureFlags.IsSet(WGALTextureUsageFlags::Presentable))
    {
      barriers.TextureBarrier(pTexture->GetImage(), pTexture->GetFullRange(), WGALResourceState::Unknown, defaultState);
    }
  }
}

void WInitContextVulkan::InitBuffer(const WGALBufferVulkan* pBuffer, WConstByteArrayPtr initialData)
{
  W_LOCK(m_Lock);

  EnsureCommandBufferExists();

  WBarrierUtilsVulkan barriers(*m_pDevice, m_CurrentCommandBuffer);
  const WBitflags<WGALResourceState> defaultState = pBuffer->GetDescription().GetDefaultState();

  // During initialization, there can't be any read/write hazard with the GPU so we can write to the memory directly if supported, e.g. unified memory.
  const WVulkanAllocationInfo& allocInfo = pBuffer->GetAllocationInfo();
  if (allocInfo.m_pMappedData != nullptr)
  {
    WMemoryUtils::Copy((WUInt8*)allocInfo.m_pMappedData, initialData.GetPtr(), initialData.GetCount());

    barriers.BufferBarrier(pBuffer->GetVkBuffer(),
      WGALResourceState::CpuWrite, defaultState);
  }
  else
  {
    barriers.BufferBarrier(pBuffer->GetVkBuffer(), WGALResourceState::Unknown, WGALResourceState::CopyDestination);

    m_pDevice->UploadBufferStaging(*m_pDevice, m_pStagingBufferPool.Borrow(), m_CurrentCommandBuffer, pBuffer, initialData, 0);

    barriers.BufferBarrier(pBuffer->GetVkBuffer(), WGALResourceState::CopyDestination, defaultState);
  }
}

void WInitContextVulkan::UpdateTexture(const WGALTextureVulkan* pTexture, const WGALTextureSubresource& subresource, const WBoundingBoxu32& box, const WGALSystemMemoryDescription& sourceData)
{
  W_LOCK(m_Lock);

  EnsureCommandBufferExists();

  const WBitflags<WGALResourceState> defaultState = pTexture->GetDescription().GetDefaultState();

  const WVec3U32 boxExtents = box.GetExtents();
  const vk::Offset3D imageOffset = {(WInt32)box.m_vMin.x, (WInt32)box.m_vMin.y, (WInt32)box.m_vMin.z};
  const vk::Extent3D imageExtent = {boxExtents.x, boxExtents.y, boxExtents.z};

  vk::ImageSubresourceLayers subresourceLayers;
  subresourceLayers.aspectMask = WConversionUtilsVulkan::IsDepthFormat(pTexture->GetImageFormat()) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  subresourceLayers.mipLevel = subresource.m_uiMipLevel;
  subresourceLayers.baseArrayLayer = subresource.m_uiArraySlice;
  subresourceLayers.layerCount = 1;

  vk::ImageSubresourceRange subresourceRange = WConversionUtilsVulkan::GetSubresourceRange(subresourceLayers);

  WBarrierUtilsVulkan barriers(*m_pDevice, m_CurrentCommandBuffer);
  barriers.TextureBarrier(pTexture->GetImage(), subresourceRange, defaultState, WGALResourceState::CopyDestination);

  m_pDevice->UploadTextureStaging(*m_pDevice, m_pStagingBufferPool.Borrow(), m_CurrentCommandBuffer, pTexture, subresourceLayers, imageOffset, imageExtent, sourceData);

  barriers.TextureBarrier(pTexture->GetImage(), subresourceRange, WGALResourceState::CopyDestination, defaultState);
}

void WInitContextVulkan::UpdateBuffer(const WGALBufferVulkan* pBuffer, WUInt32 uiOffset, WConstByteArrayPtr sourceData)
{
  W_LOCK(m_Lock);

  EnsureCommandBufferExists();
  const WBitflags<WGALResourceState> defaultState = pBuffer->GetDescription().GetDefaultState();
  WBarrierUtilsVulkan barriers(*m_pDevice, m_CurrentCommandBuffer);
  barriers.BufferBarrier(pBuffer->GetVkBuffer(), defaultState, WGALResourceState::CopyDestination);

  m_pDevice->UploadBufferStaging(*m_pDevice, m_pStagingBufferPool.Borrow(), m_CurrentCommandBuffer, pBuffer, sourceData, uiOffset);

  barriers.BufferBarrier(pBuffer->GetVkBuffer(), WGALResourceState::CopyDestination, defaultState);
}

void WInitContextVulkan::UpdateDynamicUniformBuffer(vk::Buffer gpuBuffer, vk::Buffer stagingBuffer, WUInt32 uiOffset, WUInt32 uiSize)
{
  W_LOCK(m_Lock);

  EnsureCommandBufferExists();

  WBarrierUtilsVulkan barriers(*m_pDevice, m_CurrentCommandBuffer);

  if (stagingBuffer)
  {
    // gpuBuffer can't be accessed on the CPU, so the data is present in stagingBuffer and needs to be copied over.
    barriers.BufferBarrier(stagingBuffer,
      WGALResourceState::CpuWrite, WGALResourceState::CopySource);

    vk::BufferCopy bufferCopy = {};
    bufferCopy.dstOffset = uiOffset;
    bufferCopy.srcOffset = uiOffset;
    bufferCopy.size = uiSize;
    m_CurrentCommandBuffer.copyBuffer(stagingBuffer, gpuBuffer, 1, &bufferCopy);

    barriers.BufferBarrier(gpuBuffer,
      WGALResourceState::CopyDestination, WGALResourceState::ConstantBuffer);
  }
  else
  {
    // gpuBuffer is writable on the CPU and thus we only need to add a barrier.
    barriers.BufferBarrier(gpuBuffer,
      WGALResourceState::CpuWrite, WGALResourceState::ConstantBuffer);
  }
}

void WInitContextVulkan::ExecutePendingCopies(WArrayPtr<WPendingBufferCopyVulkan> buffers, WArrayPtr<WPendingTextureCopyVulkan> textures)
{
  if (buffers.IsEmpty() && textures.IsEmpty())
    return;

  auto getRange = [](const vk::ImageSubresourceLayers& layers) -> vk::ImageSubresourceRange
  {
    vk::ImageSubresourceRange range;
    range.aspectMask = layers.aspectMask;
    range.baseMipLevel = layers.mipLevel;
    range.levelCount = 1;
    range.baseArrayLayer = layers.baseArrayLayer;
    range.layerCount = layers.layerCount;
    return range;
  };

  W_LOCK(m_Lock);
  EnsureCommandBufferExists();

  WBarrierUtilsVulkan barriers(*m_pDevice, m_CurrentCommandBuffer);

  // Transition resources to copy-compatible states.
  for (const WPendingBufferCopyVulkan& bufferCopy : buffers)
  {
    const WBitflags<WGALResourceState> defaultState = bufferCopy.m_pDstBuffer->GetDescription().GetDefaultState();

    barriers.BufferBarrier(bufferCopy.m_SrcBuffer.m_buffer,
      WGALResourceState::CpuWrite, WGALResourceState::CopySource);

    barriers.BufferBarrier(bufferCopy.m_pDstBuffer->GetVkBuffer(),
      defaultState, WGALResourceState::CopyDestination);
  }
  for (const WPendingTextureCopyVulkan& textureCopy : textures)
  {
    const WBitflags<WGALResourceState> defaultState = textureCopy.m_pDstTexture->GetDescription().GetDefaultState();

    barriers.TextureBarrier(textureCopy.m_pDstTexture->GetImage(), getRange(textureCopy.m_Region.imageSubresource),
      defaultState, WGALResourceState::CopyDestination);

    barriers.BufferBarrier(textureCopy.m_SrcBuffer.m_buffer,
      WGALResourceState::CpuWrite, WGALResourceState::CopySource);
  }

  // Execute the actual copies.
  for (const WPendingBufferCopyVulkan& bufferCopy : buffers)
  {
    m_CurrentCommandBuffer.copyBuffer(bufferCopy.m_SrcBuffer.m_buffer, bufferCopy.m_pDstBuffer->GetVkBuffer(), 1, &bufferCopy.m_Region);
  }
  for (const WPendingTextureCopyVulkan& textureCopy : textures)
  {
    m_CurrentCommandBuffer.copyBufferToImage(textureCopy.m_SrcBuffer.m_buffer, textureCopy.m_pDstTexture->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1, &textureCopy.m_Region);
  }

  // Transition resources back to their default states.
  for (const WPendingBufferCopyVulkan& bufferCopy : buffers)
  {
    const WBitflags<WGALResourceState> defaultState = bufferCopy.m_pDstBuffer->GetDescription().GetDefaultState();

    barriers.BufferBarrier(bufferCopy.m_pDstBuffer->GetVkBuffer(),
      WGALResourceState::CopyDestination, defaultState);
  }
  for (const WPendingTextureCopyVulkan& textureCopy : textures)
  {
    const WBitflags<WGALResourceState> defaultState = textureCopy.m_pDstTexture->GetDescription().GetDefaultState();

    barriers.TextureBarrier(textureCopy.m_pDstTexture->GetImage(), getRange(textureCopy.m_Region.imageSubresource),
      WGALResourceState::CopyDestination, defaultState);
  }
}
