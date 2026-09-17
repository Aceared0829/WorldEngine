#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_BoundsSphere.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WParticleSphereOutOfBoundsMode, 1)
  W_ENUM_CONSTANT(WParticleSphereOutOfBoundsMode::Kill),
  W_ENUM_CONSTANT(WParticleSphereOutOfBoundsMode::Constrain),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_BoundsSphere, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_BoundsSphere>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("CenterOffset", m_vCenterOffset),
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WDefaultValueAttribute(3.0f), new WClampValueAttribute(0.01f, {})),
    W_ENUM_MEMBER_PROPERTY("OutOfBoundsMode", WParticleSphereOutOfBoundsMode, m_OutOfBoundsMode),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WSphereVisualizerAttribute("Radius", WColor::LightGreen, nullptr, WVisualizerAnchor::Center, WVec3::MakeZero(), "CenterOffset")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_BoundsSphere, 1, WRTTIDefaultAllocator<WParticleBehavior_BoundsSphere>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleBehaviorFactory_BoundsSphere::WParticleBehaviorFactory_BoundsSphere() = default;

const WRTTI* WParticleBehaviorFactory_BoundsSphere::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_BoundsSphere>();
}

void WParticleBehaviorFactory_BoundsSphere::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_BoundsSphere* pBehavior = static_cast<WParticleBehavior_BoundsSphere*>(pObject);

  pBehavior->m_vCenterOffset = m_vCenterOffset;
  pBehavior->m_fRadius = m_fRadius;
  pBehavior->m_OutOfBoundsMode = m_OutOfBoundsMode;
}

void WParticleBehaviorFactory_BoundsSphere::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_vCenterOffset;
  inout_stream << m_fRadius;
  inout_stream << m_OutOfBoundsMode;
}

void WParticleBehaviorFactory_BoundsSphere::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= 1, "Invalid version {0}", uiVersion);

  inout_stream >> m_vCenterOffset;
  inout_stream >> m_fRadius;
  inout_stream >> m_OutOfBoundsMode;
}

void WParticleBehavior_BoundsSphere::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
}

void WParticleBehavior_BoundsSphere::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: BoundsSphere");

  const WSimdTransform trans = WSimdConversion::ToTransform(GetOwnerSystem()->GetTransform());
  const WSimdVec4f center = trans.TransformPosition(WSimdConversion::ToVec3(m_vCenterOffset));
  const float fRadiusSqr = m_fRadius * m_fRadius;

  WProcessingStreamIterator<WSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);

  if (m_OutOfBoundsMode == WParticleSphereOutOfBoundsMode::Kill)
  {
    WUInt32 idx = 0;

    while (!itPosition.HasReachedEnd())
    {
      const WSimdVec4f pos = itPosition.Current();
      const WSimdVec4f diff = pos - center;
      const float distSqr = diff.Dot<3>(diff);

      if (distSqr > fRadiusSqr)
      {
        m_pStreamGroup->RemoveElement(idx);
      }

      ++idx;
      itPosition.Advance();
    }
  }
  else // Constrain
  {
    while (!itPosition.HasReachedEnd())
    {
      const WSimdVec4f pos = itPosition.Current();
      const WSimdVec4f diff = pos - center;
      const float distSqr = diff.Dot<3>(diff);

      if (distSqr > fRadiusSqr)
      {
        // Push particle back to the sphere surface
        const float dist = WMath::Sqrt(distSqr);
        const WSimdVec4f normalized = diff / WSimdFloat(dist);
        itPosition.Current() = center + normalized * WSimdFloat(m_fRadius);
      }

      itPosition.Advance();
    }
  }
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_BoundsSphere);
