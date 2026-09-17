#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Threading/Mutex.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

struct WGALDeviceEvent;

/// This class serves as a pool for GPU related resources (e.g. buffers and textures required for rendering).
/// Note that the functions creating and returning render targets are thread safe (by using a mutex).
class W_RENDERERCORE_DLL WGPUResourcePool
{
public:
  WGPUResourcePool();
  ~WGPUResourcePool();

  /// Returns a render target handle for the given texture description
  /// Note that you should return the handle to the pool and never destroy it directly with the device.
  WGALTextureHandle GetRenderTarget(const WGALTextureCreationDescription& textureDesc);

  /// Convenience functions which creates a texture description fit for a 2d render target without a mip chains.
  WGALTextureHandle GetRenderTarget(WUInt32 uiWidth, WUInt32 uiHeight, WGALResourceFormat::Enum format,
    WGALMSAASampleCount::Enum sampleCount = WGALMSAASampleCount::None, WUInt32 uiSliceColunt = 1, WGALTextureType::Enum textureType = WGALTextureType::Texture2D);

  /// Returns a render target to the pool so other consumers can use it.
  /// Note that targets which are returned to the pool are susceptible to destruction due to garbage collection.
  void ReturnRenderTarget(WGALTextureHandle hRenderTarget);


  /// Returns a buffer handle for the given buffer description
  WGALBufferHandle GetBuffer(const WGALBufferCreationDescription& bufferDesc);

  /// Returns a buffer to the pool so other consumers can use it.
  void ReturnBuffer(WGALBufferHandle hBuffer);


  /// Tries to free resources which are currently in the pool.
  /// Triggered automatically due to allocation number / size thresholds but can be triggered manually (e.g. after editor window resize)
  ///
  /// \param uiMinimumAge How many frames at least the resource needs to have been unused before it will be GCed.
  void RunGC(WUInt32 uiMinimumAge);


  static WGPUResourcePool* GetDefaultInstance();
  static void SetDefaultInstance(WGPUResourcePool* pDefaultInstance);

protected:
  void CheckAndPotentiallyRunGC();
  void UpdateMemoryStats() const;
  void GALDeviceEventHandler(const WGALDeviceEvent& e);

  struct TextureHandleWithAge
  {
    WGALTextureHandle m_hTexture;
    WUInt64 m_uiLastUsed = 0;
  };

  struct BufferHandleWithAge
  {
    WGALBufferHandle m_hBuffer;
    WUInt64 m_uiLastUsed = 0;
  };

  WEventSubscriptionID m_GALDeviceEventSubscriptionID = 0;
  WUInt64 m_uiMemoryThresholdForGC = 256 * 1024 * 1024;
  WUInt64 m_uiCurrentlyAllocatedMemory = 0;
  WUInt16 m_uiNumAllocationsThresholdForGC = 128;
  WUInt16 m_uiNumAllocationsSinceLastGC = 0;
  WUInt16 m_uiFramesThresholdSinceLastGC = 60; ///< Every 60 frames resources unused for more than 10 frames in a row are GCed.
  WUInt16 m_uiFramesSinceLastGC = 0;

  WMap<WUInt32, WDynamicArray<TextureHandleWithAge>> m_AvailableTextures;
  WSet<WGALTextureHandle> m_TexturesInUse;

  WMap<WUInt32, WDynamicArray<BufferHandleWithAge>> m_AvailableBuffers;
  WSet<WGALBufferHandle> m_BuffersInUse;
  WDynamicArray<WGALBufferHandle> m_BuffersToBeReused;

  WMutex m_Lock;

  WGALDevice* m_pDevice;

private:
  static WGPUResourcePool* s_pDefaultInstance;
};
