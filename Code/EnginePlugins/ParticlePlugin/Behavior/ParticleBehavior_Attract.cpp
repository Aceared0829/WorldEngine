#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Attract.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_Attract, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_Attract>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Influence", m_fInfluence)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("AffectVelocity", m_bAffectVelocity)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("MaxAttractors", m_uiMaxAttractors)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(1, 8)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_Attract, 1, WRTTIDefaultAllocator<WParticleBehavior_Attract>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleBehaviorFactory_Attract::WParticleBehaviorFactory_Attract() = default;

const WRTTI* WParticleBehaviorFactory_Attract::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_Attract>();
}

void WParticleBehaviorFactory_Attract::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_Attract* pBehavior = static_cast<WParticleBehavior_Attract*>(pObject);

  pBehavior->m_fInfluence = m_fInfluence;
  pBehavior->m_bAffectVelocity = m_bAffectVelocity;
  pBehavior->m_uiMaxAttractors = m_uiMaxAttractors;
}

void WParticleBehaviorFactory_Attract::QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const
{
  if (m_bAffectVelocity)
  {
    inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_ApplyVelocity>());
  }
}

void WParticleBehaviorFactory_Attract::Save(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  inout_stream << m_fInfluence;
  inout_stream << m_bAffectVelocity;
  inout_stream << m_uiMaxAttractors;
}

void WParticleBehaviorFactory_Attract::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  /*const auto version =*/inout_stream.ReadVersion(1);

  inout_stream >> m_fInfluence;
  inout_stream >> m_bAffectVelocity;
  inout_stream >> m_uiMaxAttractors;
}

void WParticleBehavior_Attract::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);

  if (m_bAffectVelocity)
  {
    CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
  }

  GetOwnerEffect()->RequestAttractorSamples(m_uiMaxAttractors);
}

void WParticleBehavior_Attract::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: AttractToPosition");

  const float tDiff = m_TimeDiff.AsFloatInSeconds();
  if (tDiff <= 0.0f)
    return;

  // Attractor data is pre-computed every frame by FindNearbyAttractors() on the main thread.
  const WArrayPtr<const WParticleAttractorData> attractors = GetOwnerEffect()->GetAttractorData();

  if (attractors.IsEmpty())
    return;

  if (m_bAffectVelocity)
  {
    for (const WParticleAttractorData& a : attractors)
    {
      const WSimdVec4f vAttractorPos = WSimdConversion::ToVec3(a.m_vPosition);
      const WSimdFloat fMaxDistSqr = a.m_fRadius * a.m_fRadius;
      const WSimdFloat fMinDistSqr = a.m_fMinDistance * a.m_fMinDistance;
      const WSimdFloat fRadius = a.m_fRadius;
      const WSimdFloat fInvRange = (a.m_fRadius > a.m_fMinDistance) ? (1.0f / (a.m_fRadius - a.m_fMinDistance)) : 0.0f;
      const WSimdFloat fScaledStrength = a.m_fStrength * m_fInfluence * tDiff;
      const WSimdFloat fKillDistSqr = a.m_fKillDistance * a.m_fKillDistance;

      WProcessingStreamIterator<WSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);
      WProcessingStreamIterator<WFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);
      WUInt32 idx = 0;

      while (!itPosition.HasReachedEnd())
      {
        const WSimdVec4f vPos = itPosition.Current();
        const WSimdVec4f vToTarget = vAttractorPos - vPos;
        const WSimdFloat fDistSqr = vToTarget.GetLengthSquared<3>();

        if (fDistSqr < fKillDistSqr)
        {
          m_pStreamGroup->RemoveElement(idx);
        }
        else if (fDistSqr < fMaxDistSqr && fDistSqr > fMinDistSqr)
        {
          WSimdVec4f vDir = vToTarget;
          const WSimdFloat fDist = vDir.GetLengthAndNormalize<3, WMathAcc::BITS_23>();

          const WSimdFloat fAcceleration = fScaledStrength * ((fRadius - fDist) * fInvRange);

          const WVec4 vel = itVelocity.Current();
          WSimdVec4f vNewVel = WSimdVec4f(vel.x, vel.y, vel.z, 0.0f) * WSimdFloat(vel.w) + vDir * fAcceleration;
          const WSimdFloat fNewSpeed = vNewVel.GetLengthAndNormalize<3, WMathAcc::BITS_23>();

          itVelocity.Current() = WVec4(vNewVel.GetComponent<0>(), vNewVel.GetComponent<1>(), vNewVel.GetComponent<2>(), fNewSpeed);
        }

        ++idx;
        itPosition.Advance();
        itVelocity.Advance();
      }
    }
  }
  else
  {
    for (const WParticleAttractorData& a : attractors)
    {
      const WSimdVec4f vAttractorPos = WSimdConversion::ToVec3(a.m_vPosition);
      const WSimdFloat fMaxDistSqr = a.m_fRadius * a.m_fRadius;
      const WSimdFloat fMinDistSqr = a.m_fMinDistance * a.m_fMinDistance;
      const WSimdFloat fRadius = a.m_fRadius;
      const WSimdFloat fInvRange = (a.m_fRadius > a.m_fMinDistance) ? (1.0f / (a.m_fRadius - a.m_fMinDistance)) : 0.0f;
      const WSimdFloat fScaledStrength = a.m_fStrength * m_fInfluence * tDiff;
      const WSimdFloat fKillDistSqr = a.m_fKillDistance * a.m_fKillDistance;

      WProcessingStreamIterator<WSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);
      WUInt32 idx = 0;

      while (!itPosition.HasReachedEnd())
      {
        const WSimdVec4f vPos = itPosition.Current();
        const WSimdVec4f vToTarget = vAttractorPos - vPos;
        const WSimdFloat fDistSqr = vToTarget.GetLengthSquared<3>();

        if (fDistSqr < fKillDistSqr)
        {
          m_pStreamGroup->RemoveElement(idx);
        }
        else if (fDistSqr < fMaxDistSqr && fDistSqr > fMinDistSqr)
        {
          WSimdVec4f vDir = vToTarget;
          const WSimdFloat fDist = vDir.GetLengthAndNormalize<3, WMathAcc::BITS_23>();

          const WSimdFloat fMoveAmount = fScaledStrength * ((fRadius - fDist) * fInvRange);

          // Clamp move amount so particles don't overshoot the min-distance shell.
          const WSimdFloat fMaxMove = fDist - WSimdFloat(a.m_fMinDistance);
          itPosition.Current() = vPos + vDir * fMoveAmount.Min(fMaxMove);
        }

        ++idx;
        itPosition.Advance();
      }
    }
  }
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Attract);
