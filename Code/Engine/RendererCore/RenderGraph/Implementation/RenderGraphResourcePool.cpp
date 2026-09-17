#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
#  include <Foundation/Utilities/Stats.h>
#endif

namespace
{
  WUInt64 ComputeDescHash(const WGALTextureCreationDescription& desc)
  {
    return WHashingUtils::xxHash64(&desc, sizeof(desc));
  }

  WUInt64 ComputeDescHash(const WGALBufferCreationDescription& desc)
  {
    return WHashingUtils::xxHash64(&desc, sizeof(desc));
  }
} // namespace

WRenderGraphResourcePool::WRenderGraphResourcePool(WGALDevice* pDevice)
  : m_pDevice(pDevice)
{
}

WRenderGraphResourcePool::~WRenderGraphResourcePool()
{
  Clear();
}

// --- Private: called by WRenderGraphResourceAllocator ---

WSharedPtr<WPooledRenderTexture> WRenderGraphResourcePool::AcquireTexture(const WGALTextureCreationDescription& desc, WUInt32 uiIndex)
{
  W_PROFILE_SCOPE("AcquireTexture");
  W_LOCK(m_Mutex);

  const WUInt64 uiHash = ComputeDescHash(desc);
  auto& slots = m_Textures[uiHash]; // inserts empty array if not present

  // Grow the slot array if needed.
  while (slots.GetCount() <= uiIndex)
  {
    slots.PushBack(nullptr);
  }

  WSharedPtr<WPooledRenderTexture>& pSlot = slots[uiIndex];
  if (pSlot == nullptr)
  {
    // Create the GPU resource.
    WGALTextureHandle hTexture = m_pDevice->CreateTexture(desc);
    W_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create transient texture ({}x{}, format {})",
      desc.m_uiWidth, desc.m_uiHeight, (int)desc.m_Format.GetValue());

    pSlot = W_DEFAULT_NEW(WPooledRenderTexture);
    pSlot->m_hTexture = hTexture;
    pSlot->m_Desc = desc;

    m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForTexture(desc);
    UpdateMemoryStats();
  }

  pSlot->m_uiUsedCounter = 0;
  return pSlot;
}

WSharedPtr<WPooledRenderBuffer> WRenderGraphResourcePool::AcquireBuffer(const WGALBufferCreationDescription& desc, WUInt32 uiIndex)
{
  W_PROFILE_SCOPE("AcquireBuffer");
  W_LOCK(m_Mutex);

  const WUInt64 uiHash = ComputeDescHash(desc);
  auto& slots = m_Buffers[uiHash];

  while (slots.GetCount() <= uiIndex)
  {
    slots.PushBack(nullptr);
  }

  WSharedPtr<WPooledRenderBuffer>& pSlot = slots[uiIndex];
  if (pSlot == nullptr)
  {
    WGALBufferHandle hBuffer = m_pDevice->CreateBuffer(desc);
    W_ASSERT_DEV(!hBuffer.IsInvalidated(), "Failed to create transient buffer (size {})", desc.m_uiTotalSize);

    pSlot = W_DEFAULT_NEW(WPooledRenderBuffer);
    pSlot->m_hBuffer = hBuffer;
    pSlot->m_Desc = desc;

    m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForBuffer(desc);
    UpdateMemoryStats();
  }

  pSlot->m_uiUsedCounter = 0;
  return pSlot;
}

// --- GC & Cleanup ---

void WRenderGraphResourcePool::RunGC(WUInt32 uiMinimumAge)
{
  W_PROFILE_SCOPE("RunGC");
  W_LOCK(m_Mutex);

  for (auto it = m_Textures.GetIterator(); it.IsValid(); ++it)
  {
    auto& slots = it.Value();
    for (WInt32 i = (WInt32)slots.GetCount() - 1; i >= 0; --i)
    {
      WSharedPtr<WPooledRenderTexture>& pTex = slots[i];
      if (pTex == nullptr)
        continue;

      // Only destroy if the pool is the sole owner and it's old enough.
      if (pTex->GetRefCount() > 1)
        continue;
      if (pTex->m_uiUsedCounter < uiMinimumAge)
      {
        pTex->m_uiUsedCounter++;
        continue;
      }
      m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForTexture(pTex->m_Desc);
      m_pDevice->DestroyTexture(pTex->m_hTexture);
      pTex = nullptr;
    }

    // Trim trailing nullptrs from the slot array.
    while (!slots.IsEmpty() && slots.PeekBack() == nullptr)
    {
      slots.PopBack();
    }
  }

  for (auto it = m_Buffers.GetIterator(); it.IsValid(); ++it)
  {
    auto& slots = it.Value();
    for (WInt32 i = (WInt32)slots.GetCount() - 1; i >= 0; --i)
    {
      WSharedPtr<WPooledRenderBuffer>& pBuf = slots[i];
      if (pBuf == nullptr)
        continue;

      if (pBuf->GetRefCount() > 1)
        continue;
      if (pBuf->m_uiUsedCounter < uiMinimumAge)
      {
        pBuf->m_uiUsedCounter++;
        continue;
      }

      m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForBuffer(pBuf->m_Desc);
      m_pDevice->DestroyBuffer(pBuf->m_hBuffer);
      pBuf = nullptr;
    }

    while (!slots.IsEmpty() && slots.PeekBack() == nullptr)
    {
      slots.PopBack();
    }
  }

  UpdateMemoryStats();
}

void WRenderGraphResourcePool::EndFrame()
{
  ++m_uiFrameCounter;
  ++m_uiFramesSinceLastGC;
  if (m_uiFramesSinceLastGC >= s_uiFramesThresholdForGC)
  {
    m_uiFramesSinceLastGC = 0;
    RunGC(s_uiMinimumAgeForGC);
  }
}

void WRenderGraphResourcePool::Clear()
{
  W_LOCK(m_Mutex);

  for (auto it = m_Textures.GetIterator(); it.IsValid(); ++it)
  {
    for (auto& pTex : it.Value())
    {
      if (pTex != nullptr)
      {
        m_pDevice->DestroyTexture(pTex->m_hTexture);
        pTex = nullptr;
      }
    }
  }

  for (auto it = m_Buffers.GetIterator(); it.IsValid(); ++it)
  {
    for (auto& pBuf : it.Value())
    {
      if (pBuf != nullptr)
      {
        m_pDevice->DestroyBuffer(pBuf->m_hBuffer);
        pBuf = nullptr;
      }
    }
  }

  m_Textures.Clear();
  m_Buffers.Clear();
  m_uiCurrentlyAllocatedMemory = 0;
  UpdateMemoryStats();
}

void WRenderGraphResourcePool::UpdateMemoryStats() const
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  float fMegaBytes = float(m_uiCurrentlyAllocatedMemory) / (1024.0f * 1024.0f);
  WStats::SetStat("RenderGraph Resource Pool/Memory (MB)", fMegaBytes);

  WUInt32 uiTextures = 0;
  for (auto it = m_Textures.GetIterator(); it.IsValid(); ++it)
  {
    for (auto& pTex : it.Value())
    {
      if (pTex != nullptr)
        ++uiTextures;
    }
  }

  WUInt32 uiBuffers = 0;
  for (auto it = m_Buffers.GetIterator(); it.IsValid(); ++it)
  {
    for (auto& pBuf : it.Value())
    {
      if (pBuf != nullptr)
        ++uiBuffers;
    }
  }

  WStats::SetStat("RenderGraph Resource Pool/Textures", (double)uiTextures);
  WStats::SetStat("RenderGraph Resource Pool/Buffers", (double)uiBuffers);
#endif
}
