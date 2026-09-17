#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStream, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleStreamFactory::WParticleStreamFactory(const char* szStreamName, WProcessingStream::DataType dataType, const WRTTI* pStreamTypeToCreate)
{
  m_szStreamName = szStreamName;
  m_DataType = dataType;
  m_pStreamTypeToCreate = pStreamTypeToCreate;
}

const WRTTI* WParticleStreamFactory::GetParticleStreamType() const
{
  return m_pStreamTypeToCreate;
}

WProcessingStream::DataType WParticleStreamFactory::GetStreamDataType() const
{
  return m_DataType;
}

const char* WParticleStreamFactory::GetStreamName() const
{
  return m_szStreamName;
}

WParticleStream* WParticleStreamFactory::CreateParticleStream(WParticleSystemInstance* pOwner) const
{
  const WRTTI* pRtti = GetParticleStreamType();
  W_ASSERT_DEBUG(pRtti->IsDerivedFrom<WParticleStream>(), "Particle stream factory does not create a valid stream type");

  WParticleStream* pStream = pRtti->GetAllocator()->Allocate<WParticleStream>();

  pOwner->CreateStream(GetStreamName(), GetStreamDataType(), &pStream->m_pStream, pStream->m_StreamBinding, true);
  pStream->Initialize(pOwner);

  return pStream;
}

//////////////////////////////////////////////////////////////////////////

WParticleStream::WParticleStream()
{
  // make sure default stream initializers are run very first
  m_fPriority = -1000.0f;
}

WResult WParticleStream::UpdateStreamBindings()
{
  m_StreamBinding.UpdateBindings(m_pStreamGroup);
  return W_SUCCESS;
}

void WParticleStream::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  const WUInt64 uiElementSize = m_pStream->GetElementSize();
  const WUInt64 uiElementStride = m_pStream->GetElementStride();

  for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    WMemoryUtils::ZeroFill<WUInt8>(
      static_cast<WUInt8*>(WMemoryUtils::AddByteOffset(m_pStream->GetWritableData(), static_cast<ptrdiff_t>(i * uiElementStride))),
      static_cast<size_t>(uiElementSize));
  }
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Streams_ParticleStream);
