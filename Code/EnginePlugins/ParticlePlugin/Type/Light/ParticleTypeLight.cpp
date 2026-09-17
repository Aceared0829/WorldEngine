#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Light/ParticleTypeLight.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypeLightFactory, 1, WRTTIDefaultAllocator<WParticleTypeLightFactory>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("SizeFactor", m_fSizeFactor)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.0f, 1000.0f)),
    W_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.0f, 100000.0f)),
    W_MEMBER_PROPERTY("Percentage", m_uiPercentage)->AddAttributes(new WDefaultValueAttribute(50), new WClampValueAttribute(1, 100)),
    W_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    W_MEMBER_PROPERTY("IntensityScaleParam", m_sIntensityParameter),
    W_MEMBER_PROPERTY("SizeScaleParam", m_sSizeScaleParameter),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypeLight, 1, WRTTIDefaultAllocator<WParticleTypeLight>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleTypeLightFactory::WParticleTypeLightFactory()
{
  m_fSizeFactor = 5.0f;
  m_fIntensity = 10.0f;
  m_uiPercentage = 50;
}


const WRTTI* WParticleTypeLightFactory::GetTypeType() const
{
  return WGetStaticRTTI<WParticleTypeLight>();
}

void WParticleTypeLightFactory::CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const
{
  WParticleTypeLight* pType = static_cast<WParticleTypeLight*>(pObject);

  pType->m_fSizeFactor = m_fSizeFactor;
  pType->m_fIntensity = m_fIntensity;
  pType->m_uiPercentage = m_uiPercentage;
  pType->m_sTintColorParameter = WTempHashedString(m_sTintColorParameter.GetData());
  pType->m_sIntensityParameter = WTempHashedString(m_sIntensityParameter.GetData());
  pType->m_sSizeScaleParameter = WTempHashedString(m_sSizeScaleParameter.GetData());
}

enum class TypeLightVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added tint color and intensity parameter

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleTypeLightFactory::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)TypeLightVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fSizeFactor;
  inout_stream << m_fIntensity;
  inout_stream << m_uiPercentage;

  // Version 2
  inout_stream << m_sTintColorParameter;
  inout_stream << m_sIntensityParameter;
  inout_stream << m_sSizeScaleParameter;
}

void WParticleTypeLightFactory::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)TypeLightVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fSizeFactor;
  inout_stream >> m_fIntensity;
  inout_stream >> m_uiPercentage;

  if (uiVersion >= 2)
  {
    inout_stream >> m_sTintColorParameter;
    inout_stream >> m_sIntensityParameter;
    inout_stream >> m_sSizeScaleParameter;
  }
}

void WParticleTypeLight::CreateRequiredStreams()
{
  m_pStreamOnOff = nullptr;

  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", WProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", WProcessingStream::DataType::Half4, &m_pStreamColor, false);

  if (m_uiPercentage < 100)
  {
    CreateStream("OnOff", WProcessingStream::DataType::Byte, &m_pStreamOnOff, false); /// \todo Initialize (instead of during extraction)
  }
}


void WParticleTypeLight::ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const
{
  W_PROFILE_SCOPE("PFX: Light");

  const WVec4* pPosition = m_pStreamPosition->GetData<WVec4>();
  const WFloat16* pSize = m_pStreamSize->GetData<WFloat16>();
  const WColorLinear16f* pColor = m_pStreamColor->GetData<WColorLinear16f>();

  if (pPosition == nullptr || pSize == nullptr || pColor == nullptr)
    return;

  WInt8* pOnOff = nullptr;

  if (m_pStreamOnOff)
  {
    pOnOff = m_pStreamOnOff->GetWritableData<WInt8>();

    if (pOnOff == nullptr)
      return;
  }

  WRandom& rng = GetRNG();

  const WUInt32 uiNumParticles = (WUInt32)GetOwnerSystem()->GetNumActiveParticles();

  const WUInt32 uiBatchId = 1; // no shadows

  const WColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, WColor::White);
  const float intensityScale = GetOwnerEffect()->GetFloatParameter(m_sIntensityParameter, 1.0f);
  const float sizeScale = GetOwnerEffect()->GetFloatParameter(m_sSizeScaleParameter, 1.0f);

  const float sizeFactor = m_fSizeFactor * sizeScale;
  const float intensity = intensityScale * m_fIntensity;

  WTransform transform;

  if (this->GetOwnerEffect()->IsSimulatedInLocalSpace())
    transform = instanceTransform;
  else
    transform.SetIdentity();

  for (WUInt32 i = 0; i < uiNumParticles; ++i)
  {
    if (pOnOff)
    {
      if (pOnOff[i] == 0)
      {
        if ((WUInt32)rng.IntMinMax(0, 100) <= m_uiPercentage)
          pOnOff[i] = 1;
        else
          pOnOff[i] = -1;
      }

      if (pOnOff[i] < 0)
        continue;
    }

    auto pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WPointLightRenderData>(nullptr);

    pRenderData->m_vGlobalPosition = transform * pPosition[i].GetAsVec3();
    pRenderData->m_LightColor = tintColor * pColor[i].ToLinearFloat();
    pRenderData->m_fIntensity = intensity;
    pRenderData->m_fSpecularMultiplier = 1.0f;
    pRenderData->m_fRadius = 0.0f;
    pRenderData->m_fRange = WMath::Max(0.01f, pSize[i] * sizeFactor);
    pRenderData->m_uiShadowDataOffsetAndFadeOut = 0;
    pRenderData->m_qGlobalRotation.SetIdentity();
    pRenderData->m_fLength = 0.0f;

    float fScreenSpaceSize = WLightComponent::CalculateScreenSpaceSize(WBoundingSphere::MakeFromCenterAndRadius(pRenderData->m_vGlobalPosition, pRenderData->m_fRange * 0.5f), *ref_msg.m_pView->GetCullingCamera());
    pRenderData->FillSortingKey(fScreenSpaceSize);

    ref_msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::Light, WRenderData::Caching::Never);
  }
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Light_ParticleTypeLight);
