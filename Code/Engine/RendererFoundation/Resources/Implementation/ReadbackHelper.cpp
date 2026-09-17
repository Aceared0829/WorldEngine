#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ReadbackBuffer.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>
#include <RendererFoundation/Resources/ReadbackTexture.h>
#include <RendererFoundation/Resources/Texture.h>

WEnum<WGALAsyncResult> WGALReadbackHelper::GetReadbackResult(WTime timeout) const
{
  if (m_hFence == 0 || m_pDevice == nullptr)
    return WGALAsyncResult::Expired;

  return m_pDevice->GetFenceResult(m_hFence, timeout);
}

WGALReadbackBufferHelper::~WGALReadbackBufferHelper()
{
  Reset();
}

void WGALReadbackBufferHelper::Reset()
{
  if (m_pDevice)
  {
    m_pDevice->DestroyReadbackBuffer(m_hReadbackBuffer);
  }

  m_hFence = 0;
  m_pDevice = nullptr;
}

WGALFenceHandle WGALReadbackBufferHelper::ReadbackBuffer(WGALCommandEncoder& ref_encoder, WGALBufferHandle hBuffer)
{
  W_ASSERT_DEV(!ref_encoder.IsInRenderingScope(), "Readback is only supported outside rendering scope");
  m_pDevice = &ref_encoder.GetDevice();
  const WGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
  W_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle passed in for readback");
  const WGALBufferCreationDescription& desc = pBuffer->GetDescription();

  if (!m_hReadbackBuffer.IsInvalidated())
  {
    const WGALReadbackBuffer* pReadbackBuffer = m_pDevice->GetReadbackBuffer(m_hReadbackBuffer);
    const WGALBufferCreationDescription& readbackDesc = pReadbackBuffer->GetDescription();
    if (desc.m_uiTotalSize != readbackDesc.m_uiTotalSize)
    {
      m_pDevice->DestroyReadbackBuffer(m_hReadbackBuffer);
      m_hReadbackBuffer = {};
    }
  }

  if (m_hReadbackBuffer.IsInvalidated())
  {
    WGALBufferCreationDescription readbackDesc;
    readbackDesc.m_uiTotalSize = desc.m_uiTotalSize;
    readbackDesc.m_ResourceAccess.m_bImmutable = false;
    m_hReadbackBuffer = m_pDevice->CreateReadbackBuffer(readbackDesc);
  }

  ref_encoder.ReadbackBuffer(m_hReadbackBuffer, hBuffer);
  m_hFence = ref_encoder.InsertFence();
  return m_hFence;
}

WReadbackBufferLock WGALReadbackBufferHelper::LockBuffer(WArrayPtr<const WUInt8>& out_memory)
{
  if (m_pDevice == nullptr || m_hFence == 0 || m_pDevice->GetFenceResult(m_hFence) != WGALAsyncResult::Ready)
    return {};

  return m_pDevice->LockBuffer(m_hReadbackBuffer, out_memory);
}

//////////////////////////////////////////////////////////////////////////

WGALReadbackTextureHelper::~WGALReadbackTextureHelper()
{
  Reset();
}

void WGALReadbackTextureHelper::Reset()
{
  if (m_pDevice)
  {
    m_pDevice->DestroyReadbackTexture(m_hReadbackTexture);
  }

  m_hFence = 0;
  m_pDevice = nullptr;
}

WGALFenceHandle WGALReadbackTextureHelper::ReadbackTexture(WGALCommandEncoder& ref_encoder, WGALTextureHandle hTexture)
{
  W_ASSERT_DEV(!ref_encoder.IsInRenderingScope(), "Readback is only supported outside rendering scope");
  m_pDevice = &ref_encoder.GetDevice();
  const WGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
  W_ASSERT_DEV(pTexture != nullptr, "Invalid texture handle passed in for readback");
  WGALTextureCreationDescription desc = pTexture->GetDescription();
  // Reset properties that have no influence on the readback texture.
  desc.m_pExisitingNativeObject = nullptr;
  desc.m_ResourceAccess.m_bImmutable = false;
  desc.m_TextureFlags = {};


  if (!m_hReadbackTexture.IsInvalidated())
  {
    const WGALReadbackTexture* pReadbackTexture = m_pDevice->GetReadbackTexture(m_hReadbackTexture);
    const WGALTextureCreationDescription& readbackDesc = pReadbackTexture->GetDescription();
    if (desc.CalculateHash() != readbackDesc.CalculateHash())
    {
      m_pDevice->DestroyReadbackTexture(m_hReadbackTexture);
      m_hReadbackTexture = {};
    }
  }

  if (m_hReadbackTexture.IsInvalidated())
  {
    W_ASSERT_DEV(desc.m_SampleCount == WGALMSAASampleCount::None, "Readback of Multi-sampled images is unsupported");
    m_hReadbackTexture = m_pDevice->CreateReadbackTexture(desc);
  }
  ref_encoder.ReadbackTexture(m_hReadbackTexture, hTexture);
  m_hFence = ref_encoder.InsertFence();
  return m_hFence;
}

WReadbackTextureLock WGALReadbackTextureHelper::LockTexture(const WArrayPtr<const WGALTextureSubresource>& subResources, WDynamicArray<WGALSystemMemoryDescription>& out_memory)
{
  if (m_pDevice == nullptr || m_hFence == 0 || m_pDevice->GetFenceResult(m_hFence) != WGALAsyncResult::Ready)
    return {};

  return m_pDevice->LockTexture(m_hReadbackTexture, subResources, out_memory);
}
