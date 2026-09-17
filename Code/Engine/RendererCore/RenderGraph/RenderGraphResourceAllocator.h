#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Utils/ResourceStateTracker.h> // provides WHashHelper<WGALTextureHandle/WGALBufferHandle>

/// Per-graph, single-threaded resource allocator that sits in front of the
/// global WRenderGraphResourcePool.
///
/// Each render graph owns one allocator. The allocator provides a simple
/// AcquireTexture / ReleaseTexture API. Internally it tracks how many resources
/// of each description are currently in use so that the same GPU resource is never
/// handed out twice within a graph, while still sharing resources between graphs
/// that happen to need the same description.
///
/// Released resources stay alive in the allocator for intra-graph recycling.
/// FreeResources (or the destructor) drops all shared pointers, decrementing
/// the ref counts back toward the pool.
class W_RENDERERCORE_DLL WRenderGraphResourceAllocator
{
public:
  explicit WRenderGraphResourceAllocator(WRenderGraphResourcePool* pPool);
  ~WRenderGraphResourceAllocator();

  WRenderGraphResourceAllocator(const WRenderGraphResourceAllocator&) = delete;
  WRenderGraphResourceAllocator& operator=(const WRenderGraphResourceAllocator&) = delete;
  WRenderGraphResourceAllocator(WRenderGraphResourceAllocator&& rhs) noexcept;
  WRenderGraphResourceAllocator& operator=(WRenderGraphResourceAllocator&& rhs) noexcept;

  /// Acquire a texture matching the given description. If a previously released
  /// texture with the same description is available, it is recycled. Otherwise, a
  /// new slot is requested from the pool.
  WGALTextureHandle AcquireTexture(const WGALTextureCreationDescription& desc);

  /// Return a texture for intra-graph recycling. The underlying pooled resource
  /// stays alive in the allocator; a future AcquireTexture with a matching
  /// description may return this same resource.
  void ReleaseTexture(WGALTextureHandle hTexture);

  /// Acquire a buffer matching the given description.
  WGALBufferHandle AcquireBuffer(const WGALBufferCreationDescription& desc);

  /// Return a buffer for intra-graph recycling.
  void ReleaseBuffer(WGALBufferHandle hBuffer);

  /// Drop all shared pointers, decrementing ref counts back toward the pool.
  void FreeResources();

private:
  WRenderGraphResourcePool* m_pPool = nullptr;

  struct TextureGroup
  {
    TextureGroup();
    WUInt32 m_uiNextPoolIndex = 0;
    WHybridArray<WSharedPtr<WPooledRenderTexture>, 1> m_All;
    WHybridArray<WSharedPtr<WPooledRenderTexture>, 1> m_Available;
  };
  WHashTable<WUInt64, TextureGroup> m_TextureGroups; // keyed by desc hash
  WHashTable<WGALTextureHandle, WSharedPtr<WPooledRenderTexture>> m_HandleToTexture;

  struct BufferGroup
  {
    BufferGroup();
    WUInt32 m_uiNextPoolIndex = 0;
    WHybridArray<WSharedPtr<WPooledRenderBuffer>, 1> m_All;
    WHybridArray<WSharedPtr<WPooledRenderBuffer>, 1> m_Available;
  };
  WHashTable<WUInt64, BufferGroup> m_BufferGroups;
  WHashTable<WGALBufferHandle, WSharedPtr<WPooledRenderBuffer>> m_HandleToBuffer;

  static WUInt64 ComputeDescHash(const WGALTextureCreationDescription& desc);
  static WUInt64 ComputeDescHash(const WGALBufferCreationDescription& desc);
};
