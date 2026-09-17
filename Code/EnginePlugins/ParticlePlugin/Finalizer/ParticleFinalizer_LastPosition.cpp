#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleFinalizerFactory_LastPosition, 1, WRTTIDefaultAllocator<WParticleFinalizerFactory_LastPosition>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleFinalizer_LastPosition, 1, WRTTIDefaultAllocator<WParticleFinalizer_LastPosition>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleFinalizerFactory_LastPosition::WParticleFinalizerFactory_LastPosition() = default;

const WRTTI* WParticleFinalizerFactory_LastPosition::GetFinalizerType() const
{
  return WGetStaticRTTI<WParticleFinalizer_LastPosition>();
}

void WParticleFinalizerFactory_LastPosition::CopyFinalizerProperties(WParticleFinalizer* pObject, bool bFirstTime) const
{
  WParticleFinalizer_LastPosition* pFinalizer = static_cast<WParticleFinalizer_LastPosition*>(pObject);
}

//////////////////////////////////////////////////////////////////////////

WParticleFinalizer_LastPosition::WParticleFinalizer_LastPosition()
{
  // do this at the start of the frame, but after the initializers
  m_fPriority = -499.0f;
}

WParticleFinalizer_LastPosition::~WParticleFinalizer_LastPosition() = default;

void WParticleFinalizer_LastPosition::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("LastPosition", WProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
}

void WParticleFinalizer_LastPosition::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: LastPosition");

  WProcessingStreamIterator<WVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  WProcessingStreamIterator<WVec3> itLastPosition(m_pStreamLastPosition, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    WVec3 curPos = itPosition.Current().GetAsVec3();
    WVec3& lastPos = itLastPosition.Current();

    lastPos = curPos;

    itPosition.Advance();
    itLastPosition.Advance();
  }
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_LastPosition);
