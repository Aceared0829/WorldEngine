#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
#  include <Foundation/Utilities/Stats.h>
#endif

WGPUResourcePool* WGPUResourcePool::s_pDefaultInstance = nullptr;

WGPUResourcePool::WGPUResourcePool()
{
  m_pDevice = WGALDevice::GetDefaultDevice();

  m_GALDeviceEventSubscriptionID = m_pDevice->s_Events.AddEventHandler(WMakeDelegate(&WGPUResourcePool::GALDeviceEventHandler, this));
}

WGPUResourcePool::~WGPUResourcePool()
{
  m_pDevice->s_Events.RemoveEventHandler(m_GALDeviceEventSubscriptionID);
  if (!m_TexturesInUse.IsEmpty())
  {
    WLog::SeriousWarning("Destructing a GPU resource pool of which textures are still in use!");
  }

  // Free remaining resources
  RunGC(0);
}

WGALTextureHandle WGPUResourcePool::GetRenderTarget(const WGALTextureCreationDescription& textureDesc)
{
  W_LOCK(m_Lock);

  if (!textureDesc.m_TextureFlags.IsAnySet(WGALTextureUsageFlags::UnorderedAccess | WGALTextureUsageFlags::RenderTarget))
  {
    WLog::Error("Texture description for render target usage has not set the UAV or RenderTargetView flag!");
    return WGALTextureHandle();
  }

  const WUInt32 uiTextureDescHash = textureDesc.CalculateHash();

  // Check if there is a fitting texture available
  auto it = m_AvailableTextures.Find(uiTextureDescHash);
  if (it.IsValid())
  {
    WDynamicArray<TextureHandleWithAge>& textures = it.Value();
    if (!textures.IsEmpty())
    {
      WGALTextureHandle hTexture = textures.PeekBack().m_hTexture;
      textures.PopBack();

      W_ASSERT_DEV(m_pDevice->GetTexture(hTexture) != nullptr, "Invalid texture in resource pool");

      m_TexturesInUse.Insert(hTexture);

      return hTexture;
    }
  }

  // Since we found no matching texture we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  WGALTextureHandle hNewTexture = m_pDevice->CreateTexture(textureDesc);

  if (hNewTexture.IsInvalidated())
  {
    WLog::Error("GPU resource pool couldn't create new texture for given desc (size: {0} x {1}, format: {2})", textureDesc.m_uiWidth,
      textureDesc.m_uiHeight, textureDesc.m_Format);
    return WGALTextureHandle();
  }

  // Also track the new created texture
  m_TexturesInUse.Insert(hNewTexture);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForTexture(textureDesc);

  UpdateMemoryStats();

  return hNewTexture;
}

WGALTextureHandle WGPUResourcePool::GetRenderTarget(WUInt32 uiWidth, WUInt32 uiHeight, WGALResourceFormat::Enum format, WGALMSAASampleCount::Enum sampleCount, WUInt32 uiSliceCount, WGALTextureType::Enum textureType)
{
  WGALTextureCreationDescription TextureDesc;
  TextureDesc.m_TextureFlags = WGALTextureUsageFlags::RenderTarget | WGALTextureUsageFlags::ShaderResource;
  TextureDesc.m_Format = format;
  TextureDesc.m_Type = textureType;
  TextureDesc.m_uiWidth = uiWidth;
  TextureDesc.m_uiHeight = uiHeight;
  TextureDesc.m_SampleCount = sampleCount;
  TextureDesc.m_uiArraySize = uiSliceCount;
  TextureDesc.m_Type = WGALTextureType::Texture2DArray;

  return GetRenderTarget(TextureDesc);
}

void WGPUResourcePool::ReturnRenderTarget(WGALTextureHandle hRenderTarget)
{
  W_LOCK(m_Lock);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)

  // First check if this texture actually came from the pool
  if (!m_TexturesInUse.Contains(hRenderTarget))
  {
    WLog::Error("Returning a texture to the GPU resource pool which wasn't created by the pool is not valid!");
    return;
  }

#endif

  m_TexturesInUse.Remove(hRenderTarget);

  if (const WGALTexture* pTexture = m_pDevice->GetTexture(hRenderTarget))
  {
    const WUInt32 uiTextureDescHash = pTexture->GetDescription().CalculateHash();

    auto it = m_AvailableTextures.Find(uiTextureDescHash);
    if (!it.IsValid())
    {
      it = m_AvailableTextures.Insert(uiTextureDescHash, WDynamicArray<TextureHandleWithAge>());
    }

    it.Value().PushBack({hRenderTarget, WRenderWorld::GetFrameCounter()});
  }
}

WGALBufferHandle WGPUResourcePool::GetBuffer(const WGALBufferCreationDescription& bufferDesc)
{
  W_LOCK(m_Lock);

  W_ASSERT_DEBUG(bufferDesc.m_BufferFlags.IsSet(WGALBufferUsageFlags::Transient), "Resource pool buffers must be transient");

  const WUInt32 uiBufferDescHash = bufferDesc.CalculateHash();

  // Check if there is a fitting buffer available
  auto it = m_AvailableBuffers.Find(uiBufferDescHash);
  if (it.IsValid())
  {
    WDynamicArray<BufferHandleWithAge>& buffers = it.Value();
    if (!buffers.IsEmpty())
    {
      WGALBufferHandle hBuffer = buffers.PeekBack().m_hBuffer;
      buffers.PopBack();

      W_ASSERT_DEV(m_pDevice->GetBuffer(hBuffer) != nullptr, "Invalid buffer in resource pool");

      m_BuffersInUse.Insert(hBuffer);

      return hBuffer;
    }
  }

  // Since we found no matching buffer we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  WGALBufferHandle hNewBuffer = m_pDevice->CreateBuffer(bufferDesc);

  if (hNewBuffer.IsInvalidated())
  {
    WLog::Error("GPU resource pool couldn't create new buffer for given desc (size: {0})", bufferDesc.m_uiTotalSize);
    return WGALBufferHandle();
  }

  // Also track the new created buffer
  m_BuffersInUse.Insert(hNewBuffer);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForBuffer(bufferDesc);

  UpdateMemoryStats();

  return hNewBuffer;
}

void WGPUResourcePool::ReturnBuffer(WGALBufferHandle hBuffer)
{
  W_LOCK(m_Lock);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)

  // First check if this texture actually came from the pool
  if (!m_BuffersInUse.Contains(hBuffer))
  {
    WLog::Error("Returning a buffer to the GPU resource pool which wasn't created by the pool is not valid!");
    return;
  }

#endif

  m_BuffersInUse.Remove(hBuffer);
  m_BuffersToBeReused.PushBack(hBuffer);
}

void WGPUResourcePool::RunGC(WUInt32 uiMinimumAge)
{
  W_LOCK(m_Lock);

  W_PROFILE_SCOPE("RunGC");
  WUInt64 uiCurrentFrame = WRenderWorld::GetFrameCounter();
  // Destroy all available textures older than uiMinimumAge frames
  {
    for (auto it = m_AvailableTextures.GetIterator(); it.IsValid();)
    {
      auto& textures = it.Value();
      for (WInt32 i = (WInt32)textures.GetCount() - 1; i >= 0; i--)
      {
        TextureHandleWithAge& texture = textures[i];
        if (texture.m_uiLastUsed + uiMinimumAge <= uiCurrentFrame)
        {
          if (const WGALTexture* pTexture = m_pDevice->GetTexture(texture.m_hTexture))
          {
            m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForTexture(pTexture->GetDescription());
          }

          m_pDevice->DestroyTexture(texture.m_hTexture);
          textures.RemoveAtAndCopy(i);
        }
        else
        {
          // The available textures are used as a stack. Thus they are ordered by last used.
          break;
        }
      }
      if (textures.IsEmpty())
      {
        auto itCopy = it;
        ++it;
        m_AvailableTextures.Remove(itCopy);
      }
      else
      {
        ++it;
      }
    }
  }

  // Destroy all available buffers older than uiMinimumAge frames
  {
    for (auto it = m_AvailableBuffers.GetIterator(); it.IsValid();)
    {
      auto& buffers = it.Value();
      for (WInt32 i = (WInt32)buffers.GetCount() - 1; i >= 0; i--)
      {
        BufferHandleWithAge& buffer = buffers[i];
        if (buffer.m_uiLastUsed + uiMinimumAge <= uiCurrentFrame)
        {
          if (const WGALBuffer* pBuffer = m_pDevice->GetBuffer(buffer.m_hBuffer))
          {
            m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForBuffer(pBuffer->GetDescription());
          }

          m_pDevice->DestroyBuffer(buffer.m_hBuffer);
          buffers.RemoveAtAndCopy(i);
        }
        else
        {
          // The available buffers are used as a stack. Thus they are ordered by last used.
          break;
        }
      }
      if (buffers.IsEmpty())
      {
        auto itCopy = it;
        ++it;
        m_AvailableBuffers.Remove(itCopy);
      }
      else
      {
        ++it;
      }
    }
  }

  m_uiNumAllocationsSinceLastGC = 0;

  UpdateMemoryStats();
}



WGPUResourcePool* WGPUResourcePool::GetDefaultInstance()
{
  return s_pDefaultInstance;
}

void WGPUResourcePool::SetDefaultInstance(WGPUResourcePool* pDefaultInstance)
{
  W_DEFAULT_DELETE(s_pDefaultInstance);
  s_pDefaultInstance = pDefaultInstance;
}


void WGPUResourcePool::CheckAndPotentiallyRunGC()
{
  if ((m_uiNumAllocationsSinceLastGC >= m_uiNumAllocationsThresholdForGC) || (m_uiCurrentlyAllocatedMemory >= m_uiMemoryThresholdForGC))
  {
    // Only try to collect resources unused for 3 or more frames. Using a smaller number will result in constant memory thrashing.
    RunGC(3);
  }
}

void WGPUResourcePool::UpdateMemoryStats() const
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  float fMegaBytes = float(m_uiCurrentlyAllocatedMemory) / (1024.0f * 1024.0f);
  WStats::SetStat("GPU Resource Pool/Memory Consumption (MB)", fMegaBytes);
#endif
}

void WGPUResourcePool::GALDeviceEventHandler(const WGALDeviceEvent& e)
{
  if (e.m_Type == WGALDeviceEvent::AfterEndFrame)
  {
    for (WGALBufferHandle hBuffer : m_BuffersToBeReused)
    {
      if (const WGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer))
      {
        const WUInt32 uiBufferDescHash = pBuffer->GetDescription().CalculateHash();

        auto it = m_AvailableBuffers.Find(uiBufferDescHash);
        if (!it.IsValid())
        {
          it = m_AvailableBuffers.Insert(uiBufferDescHash, WDynamicArray<BufferHandleWithAge>());
        }

        it.Value().PushBack({hBuffer, WRenderWorld::GetFrameCounter()});
      }
    }
    m_BuffersToBeReused.Clear();

    ++m_uiFramesSinceLastGC;
    if (m_uiFramesSinceLastGC >= m_uiFramesThresholdSinceLastGC)
    {
      m_uiFramesSinceLastGC = 0;
      RunGC(10);
    }
  }
}
