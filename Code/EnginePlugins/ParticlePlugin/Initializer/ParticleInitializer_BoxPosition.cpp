#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_BoxPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializerFactory_BoxPosition, 1, WRTTIDefaultAllocator<WParticleInitializerFactory_BoxPosition>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    W_MEMBER_PROPERTY("Size", m_vSize)->AddAttributes(new WDefaultValueAttribute(WVec3(0, 0, 0))),
    W_MEMBER_PROPERTY("ScaleXParam", m_sScaleXParameter),
    W_MEMBER_PROPERTY("ScaleYParam", m_sScaleYParameter),
    W_MEMBER_PROPERTY("ScaleZParam", m_sScaleZParameter),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WBoxVisualizerAttribute("Size", 1.0f, WColor::MediumVioletRed, nullptr, WVisualizerAnchor::Center, WVec3(1.0f), "PositionOffset")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializer_BoxPosition, 1, WRTTIDefaultAllocator<WParticleInitializer_BoxPosition>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleInitializerFactory_BoxPosition::WParticleInitializerFactory_BoxPosition()
{
  m_vPositionOffset.SetZero();
  m_vSize.Set(0, 0, 0);
}

const WRTTI* WParticleInitializerFactory_BoxPosition::GetInitializerType() const
{
  return WGetStaticRTTI<WParticleInitializer_BoxPosition>();
}

void WParticleInitializerFactory_BoxPosition::CopyInitializerProperties(WParticleInitializer* pInitializer0, bool bFirstTime) const
{
  WParticleInitializer_BoxPosition* pInitializer = static_cast<WParticleInitializer_BoxPosition*>(pInitializer0);

  const float fScaleX = pInitializer->GetOwnerEffect()->GetFloatParameter(WTempHashedString(m_sScaleXParameter.GetData()), 1.0f);
  const float fScaleY = pInitializer->GetOwnerEffect()->GetFloatParameter(WTempHashedString(m_sScaleYParameter.GetData()), 1.0f);
  const float fScaleZ = pInitializer->GetOwnerEffect()->GetFloatParameter(WTempHashedString(m_sScaleZParameter.GetData()), 1.0f);

  WVec3 vSize = m_vSize;
  vSize.x *= fScaleX;
  vSize.y *= fScaleY;
  vSize.z *= fScaleZ;

  pInitializer->m_vPositionOffset = m_vPositionOffset;
  pInitializer->m_vSize = vSize;
}

float WParticleInitializerFactory_BoxPosition::GetSpawnCountMultiplier(const WParticleEffectInstance* pEffect) const
{
  const float fScaleX = pEffect->GetFloatParameter(WTempHashedString(m_sScaleXParameter.GetData()), 1.0f);
  const float fScaleY = pEffect->GetFloatParameter(WTempHashedString(m_sScaleYParameter.GetData()), 1.0f);
  const float fScaleZ = pEffect->GetFloatParameter(WTempHashedString(m_sScaleZParameter.GetData()), 1.0f);

  float fSpawnMultiplier = 1.0f;

  if (m_vSize.x != 0.0f)
    fSpawnMultiplier *= fScaleX;

  if (m_vSize.y != 0.0f)
    fSpawnMultiplier *= fScaleY;

  if (m_vSize.z != 0.0f)
    fSpawnMultiplier *= fScaleZ;

  return fSpawnMultiplier;
}

void WParticleInitializerFactory_BoxPosition::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 3;
  inout_stream << uiVersion;

  inout_stream << m_vSize;

  // version 2
  inout_stream << m_vPositionOffset;

  // version 3
  inout_stream << m_sScaleXParameter;
  inout_stream << m_sScaleYParameter;
  inout_stream << m_sScaleZParameter;
}

void WParticleInitializerFactory_BoxPosition::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_vSize;

  if (uiVersion >= 2)
  {
    inout_stream >> m_vPositionOffset;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_sScaleXParameter;
    inout_stream >> m_sScaleYParameter;
    inout_stream >> m_sScaleZParameter;
  }
}

void WParticleInitializer_BoxPosition::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, true);
}

void WParticleInitializer_BoxPosition::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Box Position");

  WSimdVec4f* pPosition = m_pStreamPosition->GetWritableData<WSimdVec4f>();

  WRandom& rng = GetRNG();

  if (m_vSize.IsZero())
  {
    WSimdVec4f pos = WSimdConversion::ToVec4((GetOwnerSystem()->GetTransform() * m_vPositionOffset).GetAsVec4(0));

    for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pPosition[i] = pos;
    }
  }
  else
  {
    WSimdVec4f pos;
    WSimdTransform transform = WSimdConversion::ToTransform(GetOwnerSystem()->GetTransform());

    float p0[4];
    p0[3] = 0;

    for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      p0[0] = (float)(rng.DoubleMinMax(-m_vSize.x, m_vSize.x) * 0.5) + m_vPositionOffset.x;
      p0[1] = (float)(rng.DoubleMinMax(-m_vSize.y, m_vSize.y) * 0.5) + m_vPositionOffset.y;
      p0[2] = (float)(rng.DoubleMinMax(-m_vSize.z, m_vSize.z) * 0.5) + m_vPositionOffset.z;

      pos.Load<4>(p0);

      pPosition[i] = transform.TransformPosition(pos);
    }
  }
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_BoxPosition);
