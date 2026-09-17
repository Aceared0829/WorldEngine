#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Flies.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_Flies, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_Flies>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("FlySpeed", m_fSpeed)->AddAttributes(new WDefaultValueAttribute(0.2f), new WClampValueAttribute(0.0f, 1000.0f)),
    W_MEMBER_PROPERTY("PathLength", m_fPathLength)->AddAttributes(new WDefaultValueAttribute(0.2f), new WClampValueAttribute(0.0f, 100.0f)),
    W_MEMBER_PROPERTY("MaxEmitterDistance", m_fMaxEmitterDistance)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 100.0f)),
    W_MEMBER_PROPERTY("MaxSteeringAngle", m_MaxSteeringAngle)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(30)), new WClampValueAttribute(WAngle::MakeFromDegree(1.0f), WAngle::MakeFromDegree(180.0f))),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_Flies, 1, WRTTIDefaultAllocator<WParticleBehavior_Flies>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleBehaviorFactory_Flies::WParticleBehaviorFactory_Flies() = default;
WParticleBehaviorFactory_Flies::~WParticleBehaviorFactory_Flies() = default;

const WRTTI* WParticleBehaviorFactory_Flies::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_Flies>();
}

void WParticleBehaviorFactory_Flies::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_Flies* pBehavior = static_cast<WParticleBehavior_Flies*>(pObject);

  pBehavior->m_fSpeed = m_fSpeed;
  pBehavior->m_fPathLength = m_fPathLength;
  pBehavior->m_fMaxEmitterDistance = m_fMaxEmitterDistance;
  pBehavior->m_MaxSteeringAngle = m_MaxSteeringAngle;
}

void WParticleBehaviorFactory_Flies::QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_ApplyVelocity>());
}

enum class BehaviorFliesVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleBehaviorFactory_Flies::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)BehaviorFliesVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fSpeed;
  inout_stream << m_fPathLength;
  inout_stream << m_fMaxEmitterDistance;
  inout_stream << m_MaxSteeringAngle;
}

void WParticleBehaviorFactory_Flies::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)BehaviorFliesVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fSpeed;
  inout_stream >> m_fPathLength;
  inout_stream >> m_fMaxEmitterDistance;
  inout_stream >> m_MaxSteeringAngle;
}

void WParticleBehavior_Flies::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, false);

  m_TimeToChangeDir = WTime::MakeZero();
}

void WParticleBehavior_Flies::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Flies");

  const WTime tCur = GetOwnerEffect()->GetTotalEffectLifeTime();
  const bool bChangeDirection = tCur >= m_TimeToChangeDir;

  if (!bChangeDirection)
    return;

  m_TimeToChangeDir = tCur + WTime::MakeFromSeconds(m_fPathLength / m_fSpeed);

  const WVec3 vEmitterPos = GetOwnerSystem()->GetTransform().m_vPosition;
  const float fMaxDistanceToEmitterSquared = WMath::Square(m_fMaxEmitterDistance);

  WProcessingStreamIterator<WVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  WProcessingStreamIterator<WFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  WQuat qRot;

  while (!itPosition.HasReachedEnd())
  {
    // if (pLifeArray[i] == pMaxLifeArray[i])

    const WVec3 vPartToEm = vEmitterPos - itPosition.Current().GetAsVec3();
    const float fDist = vPartToEm.GetLengthSquared();

    const WVec4 vel = itVelocity.Current();
    const WVec3 vDir(vel.x, vel.y, vel.z);
    const float fSpeed = vel.w;
    const WVec3 vVelocity = vDir * fSpeed;

    if (fDist > fMaxDistanceToEmitterSquared)
    {
      WVec3 vPivot;
      vPivot = vDir.CrossRH(vPartToEm);
      vPivot.NormalizeIfNotZero().IgnoreResult();

      qRot = WQuat::MakeFromAxisAndAngle(vPivot, m_MaxSteeringAngle);

      const WVec3 newVel = qRot * vVelocity;
      const float newSpeed = newVel.GetLength();
      const WVec3 newDir = newSpeed > 0.0f ? newVel / newSpeed : WVec3(0, 0, 1);

      itVelocity.Current() = WVec4(newDir.x, newDir.y, newDir.z, newSpeed);
    }
    else
    {
      const WVec3 newDir = WVec3::MakeRandomDeviation(GetRNG(), m_MaxSteeringAngle, vDir);

      itVelocity.Current() = WVec4(newDir.x, newDir.y, newDir.z, m_fSpeed);
    }

    itPosition.Advance();
    itVelocity.Advance();
  }
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Flies);
