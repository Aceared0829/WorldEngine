#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WGALDevice;

/// Ref-counted wrapper around a pooled GPU texture.
class W_RENDERERCORE_DLL WPooledRenderTexture : public WRefCounted
{
  W_DISALLOW_COPY_AND_ASSIGN(WPooledRenderTexture);
  friend class WRenderGraphResourcePool;

public:
  WGALTextureHandle GetHandle() const { return m_hTexture; }
  const WGALTextureCreationDescription& GetDescription() const { return m_Desc; }
  ~WPooledRenderTexture() = default;

private:
  WPooledRenderTexture() = default;

  WGALTextureHandle m_hTexture;
  WGALTextureCreationDescription m_Desc;
  WUInt64 m_uiUsedCounter = 0;
};

/// Ref-counted wrapper around a pooled GPU buffer.
class W_RENDERERCORE_DLL WPooledRenderBuffer : public WRefCounted
{
  W_DISALLOW_COPY_AND_ASSIGN(WPooledRenderBuffer);
  friend class WRenderGraphResourcePool;

public:
  WGALBufferHandle GetHandle() const { return m_hBuffer; }
  const WGALBufferCreationDescription& GetDescription() const { return m_Desc; }
  ~WPooledRenderBuffer() = default;

private:
  WPooledRenderBuffer() = default;

  WGALBufferHandle m_hBuffer;
  WGALBufferCreationDescription m_Desc;
  WUInt64 m_uiUsedCounter = 0;
};

/// Global, thread-safe transient resource pool for render graphs.
///
/// Resources are indexed by (descHash, index) pairs. The pool owns a shared pointer
/// to every resource it creates. Allocators (WRenderGraphResourceAllocator) also hold
/// shared pointers, keeping the resources alive as long as any graph references them.
///
/// GC destroys GPU resources whose ref count is 1 (only the pool holds them)
/// and that haven't been used for a configurable number of frames.
///
/// Users don't call into the pool directly. Instead, they create an
/// WRenderGraphResourceAllocator which manages acquire/release semantics.
class W_RENDERERCORE_DLL WRenderGraphResourcePool
{
  friend class WRenderGraphResourceAllocator;

public:
  WRenderGraphResourcePool(WGALDevice* pDevice);
  ~WRenderGraphResourcePool();

  /// End-of-frame housekeeping: increments frame counter and runs periodic GC.
  void EndFrame();

  /// Run GC: destroy GPU resources whose ref count is 1 (only pool holds them)
  /// and that haven't been used for uiMinimumAge frames.
  void RunGC(WUInt32 uiMinimumAge);

  /// Destroy all pooled GPU resources immediately.
  void Clear();

private:
  /// Thread-safe. Returns the texture at (descHash, uiIndex), creating the GPU
  /// resource on first access. Bumps m_uiLastUsedFrame.
  WSharedPtr<WPooledRenderTexture> AcquireTexture(const WGALTextureCreationDescription& desc, WUInt32 uiIndex);

  /// Thread-safe. Returns the buffer at (descHash, uiIndex), creating the GPU
  /// resource on first access. Bumps m_uiLastUsedFrame.
  WSharedPtr<WPooledRenderBuffer> AcquireBuffer(const WGALBufferCreationDescription& desc, WUInt32 uiIndex);

  void UpdateMemoryStats() const;

  WGALDevice* m_pDevice = nullptr;
  WMutex m_Mutex;
  WUInt64 m_uiFrameCounter = 0;

  // descHash -> array of pooled resources. Array index = the uiIndex parameter.
  WHashTable<WUInt64, WDynamicArray<WSharedPtr<WPooledRenderTexture>>> m_Textures;
  WHashTable<WUInt64, WDynamicArray<WSharedPtr<WPooledRenderBuffer>>> m_Buffers;

  WUInt64 m_uiCurrentlyAllocatedMemory = 0;
  WUInt16 m_uiFramesSinceLastGC = 0;
  static constexpr WUInt16 s_uiFramesThresholdForGC = 60;
  static constexpr WUInt32 s_uiMinimumAgeForGC = 10;
};
