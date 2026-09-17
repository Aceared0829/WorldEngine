#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_SpherePosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializerFactory_SpherePosition, 2, WRTTIDefaultAllocator<WParticleInitializerFactory_SpherePosition>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WDefaultValueAttribute(0.25f), new WClampValueAttribute(0.01f, 100.0f)),
    W_MEMBER_PROPERTY("OnSurface", m_bSpawnOnSurface),
    W_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    W_MEMBER_PROPERTY("Speed", m_Speed),
    W_MEMBER_PROPERTY("ScaleRadiusParam", m_sScaleRadiusParameter),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WSphereVisualizerAttribute("Radius", WColor::MediumVioletRed, nullptr, WVisualizerAnchor::Center, WVec3(1.0f), "PositionOffset"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializer_SpherePosition, 1, WRTTIDefaultAllocator<WParticleInitializer_SpherePosition>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleInitializerFactory_SpherePosition::WParticleInitializerFactory_SpherePosition()
{
  m_fRadius = 0.25f;
  m_vPositionOffset.SetZero();
  m_bSpawnOnSurface = false;
  m_bSetVelocity = false;
}

const WRTTI* WParticleInitializerFactory_SpherePosition::GetInitializerType() const
{
  return WGetStaticRTTI<WParticleInitializer_SpherePosition>();
}

void WParticleInitializerFactory_SpherePosition::CopyInitializerProperties(WParticleInitializer* pInitializer0, bool bFirstTime) const
{
  WParticleInitializer_SpherePosition* pInitializer = static_cast<WParticleInitializer_SpherePosition*>(pInitializer0);

  const float fScale = pInitializer->GetOwnerEffect()->GetFloatParameter(WTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);

  pInitializer->m_fRadius = WMath::Max(m_fRadius * fScale, 0.01f); // prevent 0 radius
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity = m_bSetVelocity;
  pInitializer->m_Speed = m_Speed;
  pInitializer->m_vPositionOffset = m_vPositionOffset;
}

float WParticleInitializerFactory_SpherePosition::GetSpawnCountMultiplier(const WParticleEffectInstance* pEffect) const
{
  const float fScale = pEffect->GetFloatParameter(WTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);

  if (m_fRadius != 0.0f && fScale != 1.0f)
  {
    if (m_bSpawnOnSurface)
    {
      // original surface area
      const float s0 = 1.0f; /*4.0f * WMath::Pi<float>() * m_fRadius * m_fRadius; */
      // new surface area
      const float s1 = 1.0f /*4.0f * WMath::Pi<float>() * m_fRadius * m_fRadius */ * fScale * fScale;

      return s1 / s0;
    }
    else
    {
      // original volume
      const float v0 = 1.0f;
      /* 4.0f / 3.0f * WMath::Pi<float>() * m_fRadius* m_fRadius* m_fRadius; */
      // new volume
      const float v1 = 1.0f /* 4.0f / 3.0f * WMath::Pi<float>() * m_fRadius * m_fRadius * m_fRadius*/ * fScale * fScale * fScale;

      return v1 / v0;
    }
  }

  return 1.0f;
}

void WParticleInitializerFactory_SpherePosition::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 3;
  inout_stream << uiVersion;

  inout_stream << m_fRadius;
  inout_stream << m_bSpawnOnSurface;
  inout_stream << m_bSetVelocity;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;

  // version 2
  inout_stream << m_vPositionOffset;

  // version 3
  inout_stream << m_sScaleRadiusParameter;
}

void WParticleInitializerFactory_SpherePosition::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fRadius;
  inout_stream >> m_bSpawnOnSurface;
  inout_stream >> m_bSetVelocity;
  inout_stream >> m_Speed.m_Value;
  inout_stream >> m_Speed.m_fVariance;

  if (uiVersion >= 2)
  {
    inout_stream >> m_vPositionOffset;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_sScaleRadiusParameter;
  }
}

void WParticleInitializerFactory_SpherePosition::QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

void WParticleInitializer_SpherePosition::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, true);
  }
}

void WParticleInitializer_SpherePosition::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Sphere Position");

  const WVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();
  const float fStartSpeed = startVel.GetLength();
  const WVec3 startDir = fStartSpeed > 0.0f ? startVel / fStartSpeed : WVec3(0, 0, 1);

  WVec4* pPosition = m_pStreamPosition->GetWritableData<WVec4>();
  WFloat16Vec4* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<WFloat16Vec4>() : nullptr;

  WRandom& rng = GetRNG();

  const WTransform trans = GetOwnerSystem()->GetTransform();

  for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    WVec3 pos = WVec3::MakeRandomPointInSphere(rng) * m_fRadius;
    WVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * m_fRadius;

    pos += m_vPositionOffset;

    if (m_bSetVelocity)
    {
      const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance);

      const WVec3 vel = startVel + trans.m_qRotation * normalPos * fSpeed;
      const float fVelLength = vel.GetLength();
      const WVec3 velDir = fVelLength > 0.0f ? vel / fVelLength : WVec3(0, 0, 1);

      pVelocity[i] = WVec4(velDir.x, velDir.y, velDir.z, fVelLength);
    }

    pPosition[i] = (trans * pos).GetAsVec4(0);
  }
}

//////////////////////////////////////////////////////////////////////////

class WParticleInitializerFactory_SpherePosition_1_2 : public WGraphPatch
{
public:
  WParticleInitializerFactory_SpherePosition_1_2()
    : WGraphPatch("WParticleInitializerFactory_SpherePosition", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

WParticleInitializerFactory_SpherePosition_1_2 g_WParticleInitializerFactory_SpherePosition_1_2;

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
