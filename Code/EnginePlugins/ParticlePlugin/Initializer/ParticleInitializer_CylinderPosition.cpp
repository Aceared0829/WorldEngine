#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_CylinderPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializerFactory_CylinderPosition, 2, WRTTIDefaultAllocator<WParticleInitializerFactory_CylinderPosition>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WDefaultValueAttribute(0.25f), new WClampValueAttribute(0.01f, 100.0f)),
    W_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 100.0f)),
    W_MEMBER_PROPERTY("OnSurface", m_bSpawnOnSurface),
    W_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    W_MEMBER_PROPERTY("Speed", m_Speed),
    W_MEMBER_PROPERTY("ScaleRadiusParam", m_sScaleRadiusParameter),
    W_MEMBER_PROPERTY("ScaleHeightParam", m_sScaleHeightParameter),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCylinderVisualizerAttribute(WBasisAxis::PositiveZ, "Height", "Radius", WColor::MediumVioletRed, nullptr, WVisualizerAnchor::Center, WVec3(1.0f), "PositionOffset")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializer_CylinderPosition, 1, WRTTIDefaultAllocator<WParticleInitializer_CylinderPosition>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleInitializerFactory_CylinderPosition::WParticleInitializerFactory_CylinderPosition()
{
  m_vPositionOffset.SetZero();
  m_fRadius = 0.25f;
  m_fHeight = 1.0f;
  m_bSpawnOnSurface = false;
  m_bSetVelocity = false;
}

const WRTTI* WParticleInitializerFactory_CylinderPosition::GetInitializerType() const
{
  return WGetStaticRTTI<WParticleInitializer_CylinderPosition>();
}

void WParticleInitializerFactory_CylinderPosition::CopyInitializerProperties(WParticleInitializer* pInitializer0, bool bFirstTime) const
{
  WParticleInitializer_CylinderPosition* pInitializer = static_cast<WParticleInitializer_CylinderPosition*>(pInitializer0);

  const float fScaleRadius = pInitializer->GetOwnerEffect()->GetFloatParameter(WTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);
  const float fScaleHeight = pInitializer->GetOwnerEffect()->GetFloatParameter(WTempHashedString(m_sScaleHeightParameter.GetData()), 1.0f);

  pInitializer->m_vPositionOffset = m_vPositionOffset;
  pInitializer->m_fRadius = WMath::Max(m_fRadius * fScaleRadius, 0.01f); // prevent 0 radius
  pInitializer->m_fHeight = WMath::Max(m_fHeight * fScaleHeight, 0.0f);
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity = m_bSetVelocity;
  pInitializer->m_Speed = m_Speed;
}

float WParticleInitializerFactory_CylinderPosition::GetSpawnCountMultiplier(const WParticleEffectInstance* pEffect) const
{
  const float fScaleRadius = pEffect->GetFloatParameter(WTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);
  const float fScaleHeight = pEffect->GetFloatParameter(WTempHashedString(m_sScaleHeightParameter.GetData()), 1.0f);

  if (m_bSpawnOnSurface)
  {
    const float s0 = /* 2.0f * WMath::Pi<float>() * m_fRadius **/ m_fRadius + /* 2.0f * WMath::Pi<float>() * m_fRadius **/ m_fHeight;
    const float s1 = /* 2.0f * WMath::Pi<float>() * m_fRadius **/ m_fRadius * fScaleRadius * fScaleRadius +
                     /*2.0f * WMath::Pi<float>() * m_fRadius **/ fScaleRadius * m_fHeight * fScaleHeight;

    return s1 / s0;
  }
  else
  {
    const float v0 = 1.0f /* WMath::Pi<float>() * m_fRadius * m_fRadius*/;
    const float v1 = 1.0f /* WMath::Pi<float>() * m_fRadius * m_fRadius*/ * fScaleRadius * fScaleRadius;

    return v1 / v0;
  }
}

void WParticleInitializerFactory_CylinderPosition::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 3;
  inout_stream << uiVersion;

  inout_stream << m_fRadius;
  inout_stream << m_fHeight;
  inout_stream << m_bSpawnOnSurface;
  inout_stream << m_bSetVelocity;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;

  // version 2
  inout_stream << m_vPositionOffset;

  // version 3
  inout_stream << m_sScaleRadiusParameter;
  inout_stream << m_sScaleHeightParameter;
}

void WParticleInitializerFactory_CylinderPosition::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fRadius;
  inout_stream >> m_fHeight;
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
    inout_stream >> m_sScaleHeightParameter;
  }
}

void WParticleInitializerFactory_CylinderPosition::QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

void WParticleInitializer_CylinderPosition::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, true);
  }
}

void WParticleInitializer_CylinderPosition::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Cylinder Position");

  const WVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();
  const float fStartSpeed = startVel.GetLength();
  const WVec3 startDir = fStartSpeed > 0.0f ? startVel / fStartSpeed : WVec3(0, 0, 1);

  WVec4* pPosition = m_pStreamPosition->GetWritableData<WVec4>();
  WFloat16Vec4* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<WFloat16Vec4>() : nullptr;

  WRandom& rng = GetRNG();

  const float fRadiusSqr = m_fRadius * m_fRadius;
  const float fHalfHeight = m_fHeight * 0.5f;

  const WTransform trans = GetOwnerSystem()->GetTransform();

  for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    WVec3 pos;
    float len = 0.0f;
    pos.z = 0.0f;

    do
    {
      pos.x = (float)rng.DoubleMinMax(-m_fRadius, m_fRadius);
      pos.y = (float)rng.DoubleMinMax(-m_fRadius, m_fRadius);

      len = pos.GetLengthSquared();
    } while (len > fRadiusSqr ||
             len <= 0.000001f); // prevent spawning at the exact center (note: this has to be smaller than the minimum allowed radius sqr)

    WVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * m_fRadius;

    if (m_fHeight > 0)
    {
      pos.z = (float)rng.DoubleMinMax(-fHalfHeight, fHalfHeight);
    }

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

class WParticleInitializerFactory_CylinderPosition_1_2 : public WGraphPatch
{
public:
  WParticleInitializerFactory_CylinderPosition_1_2()
    : WGraphPatch("WParticleInitializerFactory_CylinderPosition", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

WParticleInitializerFactory_CylinderPosition_1_2 g_WParticleInitializerFactory_CylinderPosition_1_2;

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);
