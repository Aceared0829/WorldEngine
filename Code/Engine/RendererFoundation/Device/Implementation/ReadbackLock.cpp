#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/ReadbackLock.h>


WReadbackBufferLock::WReadbackBufferLock(WGALDevice* pDevice, const WGALReadbackBuffer* pBuffer, WArrayPtr<const WUInt8>& out_memory)
  : m_pDevice(pDevice)
  , m_pBuffer(pBuffer)
{
  if (m_pDevice->LockBufferPlatform(m_pBuffer, out_memory).Failed())
  {
    m_pDevice = nullptr;
    m_pBuffer = nullptr;
  }
}


WReadbackBufferLock::~WReadbackBufferLock()
{
  if (m_pDevice)
  {
    m_pDevice->UnlockBufferPlatform(m_pBuffer);
  }
}

void WReadbackBufferLock::operator=(WReadbackBufferLock&& rhs)
{
  if (m_pDevice)
  {
    m_pDevice->UnlockBufferPlatform(m_pBuffer);
  }

  m_pDevice = rhs.m_pDevice;
  rhs.m_pDevice = nullptr;
  m_pBuffer = rhs.m_pBuffer;
  rhs.m_pBuffer = nullptr;
}

//////////////////////////////////////////////////////////////////////////

WReadbackTextureLock::WReadbackTextureLock(WGALDevice* pDevice, const WGALReadbackTexture* pTexture, const WArrayPtr<const WGALTextureSubresource>& subResources, WDynamicArray<WGALSystemMemoryDescription>& out_memory)
  : m_pDevice(pDevice)
  , m_pTexture(pTexture)
  , m_SubResources(subResources)
{
  if (m_pDevice->LockTexturePlatform(m_pTexture, subResources, out_memory).Failed())
  {
    m_pDevice = nullptr;
    m_pTexture = nullptr;
    m_SubResources = {};
  }
}

WReadbackTextureLock::~WReadbackTextureLock()
{
  if (m_pDevice)
  {
    m_pDevice->UnlockTexturePlatform(m_pTexture, m_SubResources);
  }
}

void WReadbackTextureLock::operator=(WReadbackTextureLock&& rhs)
{
  if (m_pDevice)
  {
    m_pDevice->UnlockTexturePlatform(m_pTexture, m_SubResources);
  }

  m_pDevice = rhs.m_pDevice;
  rhs.m_pDevice = nullptr;
  m_pTexture = rhs.m_pTexture;
  rhs.m_pTexture = nullptr;
  m_SubResources = rhs.m_SubResources;
  rhs.m_SubResources = {};
}
