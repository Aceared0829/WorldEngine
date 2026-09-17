#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <RendererCore/RenderGraph/RenderGraphResourceAllocator.h>

WRenderGraphResourceAllocator::WRenderGraphResourceAllocator(WRenderGraphResourcePool* pPool)
  : m_pPool(pPool)
{
  W_ASSERT_DEBUG(pPool != nullptr, "Pool must not be null");
}

WRenderGraphResourceAllocator::~WRenderGraphResourceAllocator()
{
  FreeResources();
}

WRenderGraphResourceAllocator::TextureGroup::TextureGroup()
  : m_All(WFrameAllocator::GetCurrentAllocator())
  , m_Available(WFrameAllocator::GetCurrentAllocator())
{
}

WRenderGraphResourceAllocator::BufferGroup::BufferGroup()
  : m_All(WFrameAllocator::GetCurrentAllocator())
  , m_Available(WFrameAllocator::GetCurrentAllocator())
{
}

WRenderGraphResourceAllocator::WRenderGraphResourceAllocator(WRenderGraphResourceAllocator&& rhs) noexcept
  : m_pPool(rhs.m_pPool)
  , m_TextureGroups(std::move(rhs.m_TextureGroups))
  , m_BufferGroups(std::move(rhs.m_BufferGroups))
{
  rhs.m_pPool = nullptr;
}

WRenderGraphResourceAllocator& WRenderGraphResourceAllocator::operator=(WRenderGraphResourceAllocator&& rhs) noexcept
{
  if (this != &rhs)
  {
    FreeResources();
    m_pPool = rhs.m_pPool;
    m_TextureGroups = std::move(rhs.m_TextureGroups);
    m_BufferGroups = std::move(rhs.m_BufferGroups);
    rhs.m_pPool = nullptr;
  }
  return *this;
}

// --- Textures ---

WGALTextureHandle WRenderGraphResourceAllocator::AcquireTexture(const WGALTextureCreationDescription& desc)
{
  const WUInt64 uiHash = ComputeDescHash(desc);
  TextureGroup& group = m_TextureGroups[uiHash];

  // Try to recycle a previously released texture.
  if (!group.m_Available.IsEmpty())
  {
    WSharedPtr<WPooledRenderTexture> pTex = std::move(group.m_Available.PeekBack());
    group.m_Available.PopBack();
    WGALTextureHandle hTexture = pTex->GetHandle();
    m_HandleToTexture[hTexture] = std::move(pTex);
    return hTexture;
  }

  // Request from the global pool at the next sequential index.
  WSharedPtr<WPooledRenderTexture> pTex = m_pPool->AcquireTexture(desc, group.m_uiNextPoolIndex);
  ++group.m_uiNextPoolIndex;
  group.m_All.PushBack(pTex);
  WGALTextureHandle hTexture = pTex->GetHandle();
  m_HandleToTexture[hTexture] = std::move(pTex);
  return hTexture;
}

void WRenderGraphResourceAllocator::ReleaseTexture(WGALTextureHandle hTexture)
{
  WSharedPtr<WPooledRenderTexture> pTex;
  if (!m_HandleToTexture.Remove(hTexture, &pTex))
  {
    W_ASSERT_DEBUG(false, "ReleaseTexture: handle not found in allocator");
    return;
  }

  const WUInt64 uiHash = ComputeDescHash(pTex->GetDescription());
  TextureGroup& group = m_TextureGroups[uiHash];
  group.m_Available.PushBack(std::move(pTex));
}

// --- Buffers ---

WGALBufferHandle WRenderGraphResourceAllocator::AcquireBuffer(const WGALBufferCreationDescription& desc)
{
  const WUInt64 uiHash = ComputeDescHash(desc);
  BufferGroup& group = m_BufferGroups[uiHash];

  if (!group.m_Available.IsEmpty())
  {
    WSharedPtr<WPooledRenderBuffer> pBuf = std::move(group.m_Available.PeekBack());
    group.m_Available.PopBack();
    WGALBufferHandle hBuffer = pBuf->GetHandle();
    m_HandleToBuffer[hBuffer] = std::move(pBuf);
    return hBuffer;
  }

  WSharedPtr<WPooledRenderBuffer> pBuf = m_pPool->AcquireBuffer(desc, group.m_uiNextPoolIndex);
  ++group.m_uiNextPoolIndex;
  group.m_All.PushBack(pBuf);
  WGALBufferHandle hBuffer = pBuf->GetHandle();
  m_HandleToBuffer[hBuffer] = std::move(pBuf);
  return hBuffer;
}

void WRenderGraphResourceAllocator::ReleaseBuffer(WGALBufferHandle hBuffer)
{
  WSharedPtr<WPooledRenderBuffer> pBuf;
  if (!m_HandleToBuffer.Remove(hBuffer, &pBuf))
  {
    W_ASSERT_DEBUG(false, "ReleaseBuffer: handle not found in allocator");
    return;
  }

  const WUInt64 uiHash = ComputeDescHash(pBuf->GetDescription());
  BufferGroup& group = m_BufferGroups[uiHash];
  group.m_Available.PushBack(std::move(pBuf));
}

// --- Cleanup ---

void WRenderGraphResourceAllocator::FreeResources()
{
  m_HandleToTexture.Clear();
  m_HandleToBuffer.Clear();
  m_TextureGroups.Clear();
  m_BufferGroups.Clear();
}

// --- Hashing ---

WUInt64 WRenderGraphResourceAllocator::ComputeDescHash(const WGALTextureCreationDescription& desc)
{
  return WHashingUtils::xxHash64(&desc, sizeof(desc));
}

WUInt64 WRenderGraphResourceAllocator::ComputeDescHash(const WGALBufferCreationDescription& desc)
{
  return WHashingUtils::xxHash64(&desc, sizeof(desc));
}
