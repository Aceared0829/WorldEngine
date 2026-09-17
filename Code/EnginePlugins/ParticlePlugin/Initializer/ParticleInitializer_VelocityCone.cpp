#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_VelocityCone.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializerFactory_VelocityCone, 2, WRTTIDefaultAllocator<WParticleInitializerFactory_VelocityCone>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(30)), new WClampValueAttribute(WAngle::MakeFromDegree(1), WAngle::MakeFromDegree(89))),
    W_MEMBER_PROPERTY("Speed", m_Speed),
    W_MEMBER_PROPERTY("SpeedScaleParam", m_sSpeedScaleParameter)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WConeVisualizerAttribute(WBasisAxis::PositiveZ, "Angle", 1.0f, nullptr, WColor::CornflowerBlue)
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializer_VelocityCone, 1, WRTTIDefaultAllocator<WParticleInitializer_VelocityCone>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleInitializerFactory_VelocityCone::WParticleInitializerFactory_VelocityCone()
{
  m_Angle = WAngle::MakeFromDegree(45);
}

const WRTTI* WParticleInitializerFactory_VelocityCone::GetInitializerType() const
{
  return WGetStaticRTTI<WParticleInitializer_VelocityCone>();
}

void WParticleInitializerFactory_VelocityCone::CopyInitializerProperties(WParticleInitializer* pInitializer0, bool bFirstTime) const
{
  WParticleInitializer_VelocityCone* pInitializer = static_cast<WParticleInitializer_VelocityCone*>(pInitializer0);

  pInitializer->m_Angle = WMath::Clamp(m_Angle, WAngle::MakeFromDegree(1), WAngle::MakeFromDegree(89));
  pInitializer->m_Speed = m_Speed;
  pInitializer->m_sSpeedScaleParameter = WTempHashedString(m_sSpeedScaleParameter.GetData());
}

void WParticleInitializerFactory_VelocityCone::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 2;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_Angle;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;

  // Version 2
  inout_stream << m_sSpeedScaleParameter;
}

void WParticleInitializerFactory_VelocityCone::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_Angle;
  inout_stream >> m_Speed.m_Value;
  inout_stream >> m_Speed.m_fVariance;

  if (uiVersion >= 2)
  {
    inout_stream >> m_sSpeedScaleParameter;
  }
}

void WParticleInitializerFactory_VelocityCone::QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_ApplyVelocity>());
}

//////////////////////////////////////////////////////////////////////////

void WParticleInitializer_VelocityCone::CreateRequiredStreams()
{
  CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, true);
}

void WParticleInitializer_VelocityCone::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Velocity Cone");

  const WVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();

  WFloat16Vec4* pVelocity = m_pStreamVelocity->GetWritableData<WFloat16Vec4>();

  WRandom& rng = GetRNG();

  const float fSpeedScale = WMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpeedScaleParameter, 1.0f), 0.0f);

  // const float dist = 1.0f / WMath::Tan(m_Angle);

  for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    const WVec3 dir = WVec3::MakeRandomDeviationZ(rng, m_Angle);
    // dir.z = 0;
    // float len = 0.0f;

    // do
    //{
    //  // random point in a rectangle
    //  dir.x = (float)rng.DoubleMinMax(-1.0, 1.0);
    //  dir.y = (float)rng.DoubleMinMax(-1.0, 1.0);

    //  // discard points outside the circle
    //  len = dir.GetLengthSquared();
    //} while (len > 1.0f);

    // dir.z = dist;
    // dir.Normalize();

    const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance) * fSpeedScale;

    const WVec3 vel = startVel + GetOwnerSystem()->GetTransform().m_qRotation * dir * fSpeed;
    const float fVelLength = vel.GetLength();
    const WVec3 velDir = fVelLength > 0.0f ? vel / fVelLength : WVec3(0, 0, 1);

    pVelocity[i] = WVec4(velDir.x, velDir.y, velDir.z, fVelLength);
  }
}

//////////////////////////////////////////////////////////////////////////

class WParticleInitializerFactory_VelocityCone_1_2 : public WGraphPatch
{
public:
  WParticleInitializerFactory_VelocityCone_1_2()
    : WGraphPatch("WParticleInitializerFactory_VelocityCone", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

WParticleInitializerFactory_VelocityCone_1_2 g_WParticleInitializerFactory_VelocityCone_1_2;

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_VelocityCone);
