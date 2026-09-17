#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/BufferPool.h>

WAtomicInteger32 WGALBufferPool::s_iNumber = 0;

WGALBufferPool::~WGALBufferPool()
{
  Deinitialize();
}

void WGALBufferPool::Initialize(const WGALBufferCreationDescription& desc, WStringView sDebugName)
{
  m_iUniqueID = s_iNumber.Increment();
  W_ASSERT_DEBUG(m_EventSubscriptionID == 0, "WGALBufferPool already initialized");
  m_EventSubscriptionID = WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WGALBufferPool::GALStaticDeviceEventHandler, this));
  m_Desc = desc;
  m_sDebugName = sDebugName;
}

void WGALBufferPool::Deinitialize()
{
  if (m_EventSubscriptionID != 0)
  {
    WGALDevice::s_Events.RemoveEventHandler(m_EventSubscriptionID);
    m_EventSubscriptionID = {};
    for (WGALBufferHandle hBuffer : m_UsedBuffers)
    {
      WGALDevice::GetDefaultDevice()->DestroyBuffer(hBuffer);
    }
    m_UsedBuffers.Clear();
    m_UsedBuffers.Compact();
    for (WGALBufferHandle hBuffer : m_FreeBuffers)
    {
      WGALDevice::GetDefaultDevice()->DestroyBuffer(hBuffer);
    }
    m_FreeBuffers.Clear();
    m_FreeBuffers.Compact();
  }
}


WGALBufferHandle WGALBufferPool::GetNewBuffer() const
{
  WGALBufferHandle buffer = {};
  if (!m_FreeBuffers.IsEmpty())
  {
    buffer = m_FreeBuffers.PeekBack();
    m_FreeBuffers.PopBack();
    m_UsedBuffers.PushBack(buffer);
    return buffer;
  }

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  buffer = pDevice->CreateBuffer(m_Desc);
  if (!m_sDebugName.IsEmpty())
  {

    WStringBuilder sTmp;
    sTmp.SetFormat("{}-{}#{}", m_sDebugName, m_iUniqueID, m_UsedBuffers.GetCount() + m_FreeBuffers.GetCount());
    pDevice->GetBuffer(buffer)->SetDebugName(sTmp);
  }
  W_ASSERT_DEV(!buffer.IsInvalidated(), "Failed to create pool buffer");
  m_UsedBuffers.PushBack(buffer);
  return buffer;
}

WGALBufferHandle WGALBufferPool::GetCurrentBuffer() const
{
  if (m_UsedBuffers.IsEmpty())
    return WGALBufferHandle();
  return m_UsedBuffers.PeekBack();
}

void WGALBufferPool::GALStaticDeviceEventHandler(const WGALDeviceEvent& e)
{
  if (e.m_Type == WGALDeviceEvent::AfterEndFrame)
  {
    WGALBufferHandle hKeepAcrossFrames = {};
    if (!m_Desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::Transient) && !m_UsedBuffers.IsEmpty())
    {
      hKeepAcrossFrames = m_UsedBuffers.PeekBack();
      m_UsedBuffers.PopBack();
    }
    while (!m_UsedBuffers.IsEmpty())
    {
      m_FreeBuffers.PushBack(m_UsedBuffers.PeekBack());
      m_UsedBuffers.PopBack();
    }
    if (!hKeepAcrossFrames.IsInvalidated())
    {
      m_UsedBuffers.PushBack(hKeepAcrossFrames);
    }
  }
}
