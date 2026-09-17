
#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererVulkan/Device/DeclarationsVulkan.h>

class WGALDeviceVulkan;
class WCommandBufferPoolVulkan;
class WStagingBufferPoolVulkan;

/// Thread-safe context for initializing resources. Records a command buffer that transitions all newly created resources into their initial state.
class WInitContextVulkan
{
public:
  WInitContextVulkan(WGALDeviceVulkan* pDevice);
  ~WInitContextVulkan();

  void AfterBeginFrame();

  WMutex& AccessLock() { return m_Lock; }

  /// Returns a finished command buffer of all background loading up to this point.
  ///    The command buffer is already ended and marked to be reclaimed so the only thing done on it should be to submit it.
  vk::CommandBuffer GetFinishedCommandBuffer();

  /// Initializes a texture and moves it into its default state.
  /// \param pTexture The texture to initialize.
  /// \param createInfo The image creation info for the texture. Needed for initial state information.
  /// \param pInitialData The initial data of the texture. If not set, the initial content will be undefined.
  void InitTexture(const WGALTextureVulkan* pTexture, vk::ImageCreateInfo& ref_createInfo, WArrayPtr<WGALSystemMemoryDescription> initialData);

  /// Initializes a buffer with the given data.
  /// \param pBuffer The buffer to initialize.
  /// \param pInitialData The initial data that the buffer should be filled with.
  void InitBuffer(const WGALBufferVulkan* pBuffer, WConstByteArrayPtr initialData);

  /// Updates a texture region
  void UpdateTexture(const WGALTextureVulkan* pTexture, const WGALTextureSubresource& subresource, const WBoundingBoxu32& box, const WGALSystemMemoryDescription& sourceData);

  /// Updates a buffer range
  /// \param pBuffer The buffer to update.
  /// \param uiOffset The offset inside the buffer where the new data should be placed.
  /// \param pSourceData The new data to update the buffer with.
  void UpdateBuffer(const WGALBufferVulkan* pBuffer, WUInt32 uiOffset, WConstByteArrayPtr sourceData);

  /// Used by WUniformBufferPoolVulkan to write the entire uniform scratch pool to the GPU
  /// \param gpuBuffer The device local buffer to update.
  /// \param stagingBuffer The staging buffer that contains the data to be copied to gpuBuffer. If null, buffer is CPU writable and already contains the data.
  /// \param uiOffset Offset in the buffer.
  /// \param uiSize The size of the data to be copied from stagingBuffer to gpuBuffer.
  void UpdateDynamicUniformBuffer(vk::Buffer gpuBuffer, vk::Buffer stagingBuffer, WUInt32 uiOffset, WUInt32 uiSize);

  /// Executes work generates by WGALDeviceVulkan::UpdateBufferForNextFramePlatform and UpdateTextureForNextFramePlatform
  /// \param buffers The pending buffer copies.
  /// \param textures The pending texture copies.
  void ExecutePendingCopies(WArrayPtr<WPendingBufferCopyVulkan> buffers, WArrayPtr<WPendingTextureCopyVulkan> textures);

private:
  void EnsureCommandBufferExists();

  WGALDeviceVulkan* m_pDevice = nullptr;

  WMutex m_Lock;
  WDynamicArray<WUInt8> m_TempData;
  vk::CommandBuffer m_CurrentCommandBuffer;
  WUniquePtr<WCommandBufferPoolVulkan> m_pCommandBufferPool;
  WUniquePtr<WStagingBufferPoolVulkan> m_pStagingBufferPool;
};
