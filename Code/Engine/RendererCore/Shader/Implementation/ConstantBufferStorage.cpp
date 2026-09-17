#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>

WConstantBufferStorageBase::WConstantBufferStorageBase(WUInt32 uiSizeInBytes)
{
  m_Data = WMakeArrayPtr(static_cast<WUInt8*>(WFoundation::GetAlignedAllocator()->Allocate(uiSizeInBytes, 16)), uiSizeInBytes);
  WMemoryUtils::ZeroFill(m_Data.GetPtr(), m_Data.GetCount());

  m_hGALConstantBuffer = WGALDevice::GetDefaultDevice()->CreateConstantBuffer(uiSizeInBytes);
}

WConstantBufferStorageBase::~WConstantBufferStorageBase()
{
  WGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGALConstantBuffer);

  WFoundation::GetAlignedAllocator()->Deallocate(m_Data.GetPtr());
  m_Data.Clear();
}

WArrayPtr<WUInt8> WConstantBufferStorageBase::GetRawDataForWriting()
{
  m_bHasBeenModified = true;
  WRenderContext::MarktConstantBufferStorageModified(this);
  return m_Data;
}

WArrayPtr<const WUInt8> WConstantBufferStorageBase::GetRawDataForReading() const
{
  return m_Data;
}

void WConstantBufferStorageBase::UploadData(WGALCommandEncoder* pCommandEncoder)
{
  if (!m_bHasBeenModified && !m_bStartOfFrame)
    return;

  m_bHasBeenModified = false;

  WUInt32 uiNewHash = WHashingUtils::xxHash32(m_Data.GetPtr(), m_Data.GetCount());
  if (m_uiLastHash != uiNewHash || m_bStartOfFrame)
  {
    pCommandEncoder->UpdateBuffer(m_hGALConstantBuffer, 0, m_Data, WGALUpdateMode::TransientConstantBuffer);
    m_uiLastHash = uiNewHash;
  }
  m_bStartOfFrame = false;
}
