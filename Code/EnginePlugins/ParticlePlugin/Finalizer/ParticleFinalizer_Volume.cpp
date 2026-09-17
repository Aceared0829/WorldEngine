#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Volume.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleFinalizerFactory_Volume, 1, WRTTIDefaultAllocator<WParticleFinalizerFactory_Volume>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleFinalizer_Volume, 1, WRTTIDefaultAllocator<WParticleFinalizer_Volume>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleFinalizerFactory_Volume::WParticleFinalizerFactory_Volume() = default;
WParticleFinalizerFactory_Volume::~WParticleFinalizerFactory_Volume() = default;

const WRTTI* WParticleFinalizerFactory_Volume::GetFinalizerType() const
{
  return WGetStaticRTTI<WParticleFinalizer_Volume>();
}

void WParticleFinalizerFactory_Volume::CopyFinalizerProperties(WParticleFinalizer* pObject, bool bFirstTime) const
{
  WParticleFinalizer_Volume* pFinalizer = static_cast<WParticleFinalizer_Volume*>(pObject);
}

WParticleFinalizer_Volume::WParticleFinalizer_Volume() = default;
WParticleFinalizer_Volume::~WParticleFinalizer_Volume() = default;

void WParticleFinalizer_Volume::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  m_pStreamSize = nullptr;
}

void WParticleFinalizer_Volume::QueryOptionalStreams()
{
  m_pStreamSize = GetOwnerSystem()->QueryStream("Size", WProcessingStream::DataType::Half);
}

void WParticleFinalizer_Volume::Process(WUInt64 uiNumElements)
{
  if (uiNumElements == 0)
    return;

  W_PROFILE_SCOPE("PFX: Volume");

  const WSimdVec4f* pPosition = m_pStreamPosition->GetData<WSimdVec4f>();

  const WSimdBBoxSphere volume = WSimdBBoxSphere::MakeFromPoints(pPosition, static_cast<WUInt32>(uiNumElements));

  float fMaxSize = 0;

  if (m_pStreamSize != nullptr)
  {
    const WFloat16* pSize = m_pStreamSize->GetData<WFloat16>();

    WSimdVec4f vMax;
    vMax.SetZero();

    constexpr WUInt32 uiElementsPerLoop = 4;
    for (WUInt64 i = 0; i < uiNumElements; i += uiElementsPerLoop)
    {
      const float x = pSize[i + 0];
      const float y = pSize[i + 1];
      const float z = pSize[i + 2];
      const float w = pSize[i + 3];

      vMax = vMax.CompMax(WSimdVec4f(x, y, z, w));
    }

    for (WUInt64 i = (uiNumElements / uiElementsPerLoop) * uiElementsPerLoop; i < uiNumElements; ++i)
    {
      fMaxSize = WMath::Max(fMaxSize, (float)pSize[i]);
    }

    fMaxSize = WMath::Max(fMaxSize, (float)vMax.HorizontalMax<4>());
  }

  GetOwnerSystem()->SetBoundingVolume(WSimdConversion::ToBBoxSphere(volume), fMaxSize);
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_Volume);
