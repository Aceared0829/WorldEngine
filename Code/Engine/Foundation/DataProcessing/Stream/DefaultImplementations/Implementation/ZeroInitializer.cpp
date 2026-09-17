#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/Memory/MemoryUtils.h>

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>

#include <Foundation/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcessingStreamSpawnerZeroInitialized, 1, WRTTIDefaultAllocator<WProcessingStreamSpawnerZeroInitialized>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WProcessingStreamSpawnerZeroInitialized::WProcessingStreamSpawnerZeroInitialized()

  = default;

void WProcessingStreamSpawnerZeroInitialized::SetStreamName(WStringView sStreamName)
{
  m_sStreamName.Assign(sStreamName);
}

WResult WProcessingStreamSpawnerZeroInitialized::UpdateStreamBindings()
{
  W_ASSERT_DEBUG(!m_sStreamName.IsEmpty(), "WProcessingStreamSpawnerZeroInitialized: Stream name has not been configured");

  m_pStream = m_pStreamGroup->GetStreamByName(m_sStreamName);
  return m_pStream ? W_SUCCESS : W_FAILURE;
}


void WProcessingStreamSpawnerZeroInitialized::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  const WUInt64 uiElementSize = m_pStream->GetElementSize();
  const WUInt64 uiElementStride = m_pStream->GetElementStride();

  for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    WMemoryUtils::ZeroFill<WUInt8>(
      static_cast<WUInt8*>(WMemoryUtils::AddByteOffset(m_pStream->GetWritableData(), static_cast<std::ptrdiff_t>(i * uiElementStride))),
      static_cast<size_t>(uiElementSize));
  }
}



W_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_DefaultImplementations_Implementation_ZeroInitializer);
