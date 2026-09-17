#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomRotationSpeed.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializerFactory_RandomRotationSpeed, 2, WRTTIDefaultAllocator<WParticleInitializerFactory_RandomRotationSpeed>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("RandomStartAngle", m_bRandomStartAngle),
    W_MEMBER_PROPERTY("DegreesPerSecond", m_RotationSpeed)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(90)), new WClampValueAttribute(WAngle::MakeFromDegree(0), WVariant())),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializer_RandomRotationSpeed, 1, WRTTIDefaultAllocator<WParticleInitializer_RandomRotationSpeed>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WRTTI* WParticleInitializerFactory_RandomRotationSpeed::GetInitializerType() const
{
  return WGetStaticRTTI<WParticleInitializer_RandomRotationSpeed>();
}

void WParticleInitializerFactory_RandomRotationSpeed::CopyInitializerProperties(WParticleInitializer* pInitializer0, bool bFirstTime) const
{
  WParticleInitializer_RandomRotationSpeed* pInitializer = static_cast<WParticleInitializer_RandomRotationSpeed*>(pInitializer0);

  pInitializer->m_RotationSpeed = m_RotationSpeed;
  pInitializer->m_bRandomStartAngle = m_bRandomStartAngle;
}

enum class InitializerRandomRotationVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added start offset

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleInitializerFactory_RandomRotationSpeed::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)InitializerRandomRotationVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_RotationSpeed.m_Value;
  inout_stream << m_RotationSpeed.m_fVariance;

  // Version 2
  inout_stream << m_bRandomStartAngle;
}

void WParticleInitializerFactory_RandomRotationSpeed::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_RotationSpeed.m_Value;
  inout_stream >> m_RotationSpeed.m_fVariance;

  if (uiVersion >= 2)
  {
    inout_stream >> m_bRandomStartAngle;
  }
}


void WParticleInitializer_RandomRotationSpeed::CreateRequiredStreams()
{
  CreateStream("RotationSpeed", WProcessingStream::DataType::Half, &m_pStreamRotationSpeed, true);
  CreateStream("RotationOffset", WProcessingStream::DataType::Half, &m_pStreamRotationOffset, true);
}

void WParticleInitializer_RandomRotationSpeed::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Random Rotation");

  WFloat16* pSpeed = m_pStreamRotationSpeed->GetWritableData<WFloat16>();

  // speed
  if (m_RotationSpeed.m_Value != WAngle::MakeFromRadian(0))
  {
    WRandom& rng = GetRNG();

    for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float value = (float)rng.DoubleVariance(m_RotationSpeed.m_Value.GetRadian(), m_RotationSpeed.m_fVariance);

      pSpeed[i] = m_bPositiveSign ? value : -value;
      m_bPositiveSign = !m_bPositiveSign;
    }
  }
  else
  {
    for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pSpeed[i] = 0;
    }
  }

  // offset
  if (m_bRandomStartAngle)
  {
    WFloat16* pOffset = m_pStreamRotationOffset->GetWritableData<WFloat16>();

    WRandom& rng = GetRNG();

    for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pOffset[i] = (float)rng.DoubleMinMax(-WMath::Pi<double>(), +WMath::Pi<double>());
    }
  }
  else
  {
    WFloat16* pOffset = m_pStreamRotationOffset->GetWritableData<WFloat16>();

    for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pOffset[i] = 0;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class WParticleInitializerFactory_RandomRotationSpeed_1_2 : public WGraphPatch
{
public:
  WParticleInitializerFactory_RandomRotationSpeed_1_2()
    : WGraphPatch("WParticleInitializerFactory_RandomRotationSpeed", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("DegreesPerSecond").IgnoreResult();
  }
};

WParticleInitializerFactory_RandomRotationSpeed_1_2 g_WParticleInitializerFactory_RandomRotationSpeed_1_2;

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomRotationSpeed);
