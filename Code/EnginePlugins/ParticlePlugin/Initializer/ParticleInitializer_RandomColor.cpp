#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomColor.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializerFactory_RandomColor, 3, WRTTIDefaultAllocator<WParticleInitializerFactory_RandomColor>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("GradientSource", WGradientSource, m_GradientSource),
    W_MEMBER_PROPERTY("Gradient", m_Gradient),
    W_RESOURCE_MEMBER_PROPERTY("SharedGradient", m_hSharedGradient)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    W_MEMBER_PROPERTY("Color1", m_Color1)->AddAttributes(new WDefaultValueAttribute(WColor::White), new WExposeColorAlphaAttribute()),
    W_MEMBER_PROPERTY("Color2", m_Color2)->AddAttributes(new WDefaultValueAttribute(WColor::White), new WExposeColorAlphaAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleInitializer_RandomColor, 1, WRTTIDefaultAllocator<WParticleInitializer_RandomColor>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WRTTI* WParticleInitializerFactory_RandomColor::GetInitializerType() const
{
  return WGetStaticRTTI<WParticleInitializer_RandomColor>();
}

void WParticleInitializerFactory_RandomColor::CopyInitializerProperties(WParticleInitializer* pInitializer0, bool bFirstTime) const
{
  WParticleInitializer_RandomColor* pInitializer = static_cast<WParticleInitializer_RandomColor*>(pInitializer0);

  pInitializer->m_pGradient = &m_Gradient;
  pInitializer->m_Color1 = m_Color1;
  pInitializer->m_Color2 = m_Color2;
}

void WParticleInitializerFactory_RandomColor::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 2;
  inout_stream << uiVersion;

  inout_stream << m_Color1;
  inout_stream << m_Color2;
  inout_stream << m_GradientSource;
  inout_stream << m_hSharedGradient;
  m_Gradient.Save(inout_stream);
}

void WParticleInitializerFactory_RandomColor::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion == 1)
  {
    // Old version: read the gradient handle
    WColorGradientResourceHandle hGradient;
    inout_stream >> hGradient;

    // Convert to new format using shared gradient
    m_GradientSource = WGradientSource::SharedGradient;
    m_hSharedGradient = hGradient;
  }

  inout_stream >> m_Color1;
  inout_stream >> m_Color2;

  if (uiVersion >= 2)
  {
    inout_stream >> m_GradientSource;
    inout_stream >> m_hSharedGradient;
    m_Gradient.Load(inout_stream);
  }

  if (m_GradientSource == WGradientSource::SharedGradient && m_hSharedGradient.IsValid())
  {
    WResourceLock<WColorGradientResource> pGradientResource(m_hSharedGradient, WResourceAcquireMode::BlockTillLoaded);
    if (pGradientResource.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      m_Gradient = pGradientResource->GetDescriptor().m_Gradient;
    }
  }
}


void WParticleInitializer_RandomColor::CreateRequiredStreams()
{
  CreateStream("Color", WProcessingStream::DataType::Half4, &m_pStreamColor, true);
}

void WParticleInitializer_RandomColor::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Random Color");

  WColorLinear16f* pColor = m_pStreamColor->GetWritableData<WColorLinear16f>();

  WRandom& rng = GetRNG();

  if (m_pGradient == nullptr || m_pGradient->IsEmpty())
  {
    for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float f = (float)rng.DoubleZeroToOneInclusive();
      pColor[i] = WMath::Lerp(m_Color1, m_Color2, f);
    }
  }
  else
  {
    WColorGammaUB color;
    float intensity;

    const bool bMulColor = (m_Color1 != WColor::White) || (m_Color2 != WColor::White);

    for (WUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      m_pGradient->Evaluate(rng.DoubleZeroToOneInclusive(), color, intensity);

      WColor result = color;
      result.ScaleRGB(intensity);

      if (bMulColor)
      {
        const float f2 = (float)rng.DoubleZeroToOneInclusive();
        result *= WMath::Lerp(m_Color1, m_Color2, f2);
      }

      pColor[i] = result;
    }
  }
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomColor);
