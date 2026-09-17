#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomSize.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializerFactory_RandomSize, 3, WRTTIDefaultAllocator<WParticleInitializerFactory_RandomSize>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Size", m_Size)->AddAttributes(new WDefaultValueAttribute(WVarianceTypeFloat(1.0f)), new WClampValueAttribute(0.0f, WVariant())),
    W_RESOURCE_MEMBER_PROPERTY("SizeCurve", m_hCurve)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    W_MEMBER_PROPERTY("SizeScaleParameter", m_sSizeScaleParameter),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializer_RandomSize, 1, WRTTIDefaultAllocator<WParticleInitializer_RandomSize>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WRTTI* WParticleInitializerFactory_RandomSize::GetInitializerType() const
{
  return WGetStaticRTTI<WParticleInitializer_RandomSize>();
}

void WParticleInitializerFactory_RandomSize::CopyInitializerProperties(WParticleInitializer* pInitializer0, bool bFirstTime) const
{
  WParticleInitializer_RandomSize* pInitializer = static_cast<WParticleInitializer_RandomSize*>(pInitializer0);

  pInitializer->m_hCurve = m_hCurve;
  pInitializer->m_Size = m_Size;
  pInitializer->m_sSizeScaleParameter = m_sSizeScaleParameter;
}

void WParticleInitializerFactory_RandomSize::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 3;
  inout_stream << uiVersion;

  inout_stream << m_hCurve;
  inout_stream << m_Size.m_Value;
  inout_stream << m_Size.m_fVariance;
  inout_stream << m_sSizeScaleParameter;
}

void WParticleInitializerFactory_RandomSize::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_hCurve;
  inout_stream >> m_Size.m_Value;
  inout_stream >> m_Size.m_fVariance;

  if (uiVersion >= 3)
  {
    inout_stream >> m_sSizeScaleParameter;
  }
}

void WParticleInitializer_RandomSize::CreateRequiredStreams()
{
  CreateStream("Size", WProcessingStream::DataType::Half, &m_pStreamSize, true);
}

void WParticleInitializer_RandomSize::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Random Size");

  WFloat16* pSize = m_pStreamSize->GetWritableData<WFloat16>();

  WRandom& rng = GetRNG();

  const float fSizeScale = WMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSizeScaleParameter, 1.0f), 0.0f);

  if (!m_hCurve.IsValid())
  {
    for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pSize[i] = rng.FloatVariance(m_Size.m_Value, m_Size.m_fVariance) * fSizeScale;
    }
  }
  else
  {
    WResourceLock<WCurve1DResource> pResource(m_hCurve, WResourceAcquireMode::BlockTillLoaded);

    if (!pResource->GetDescriptor().m_Curves.IsEmpty())
    {
      const WCurve1D& curve = pResource->GetDescriptor().m_Curves[0];

      double fMinX, fMaxX;
      curve.QueryExtents(fMinX, fMaxX);

      // make sure the curve has a length of at least 1
      fMinX = WMath::Min(fMinX, 0.0);
      fMaxX = WMath::Max(fMaxX, 1.0);

      for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
      {
        const double f = rng.DoubleMinMax(fMinX, fMaxX);

        double val = curve.Evaluate(f);
        val = curve.NormalizeValue(val);

        pSize[i] = (float)(val * rng.DoubleVariance(m_Size.m_Value, m_Size.m_fVariance)) * fSizeScale;
      }
    }
    else
    {
      for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
      {
        pSize[i] = fSizeScale;
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class WParticleInitializerFactory_RandomSize_1_2 : public WGraphPatch
{
public:
  WParticleInitializerFactory_RandomSize_1_2()
    : WGraphPatch("WParticleInitializerFactory_RandomSize", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Size").IgnoreResult();
  }
};

WParticleInitializerFactory_RandomSize_1_2 g_WParticleInitializerFactory_RandomSize_1_2;

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomSize);
