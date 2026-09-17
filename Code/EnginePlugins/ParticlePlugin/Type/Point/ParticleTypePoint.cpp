#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypePointFactory, 1, WRTTIDefaultAllocator<WParticleTypePointFactory>)
{
  //W_BEGIN_ATTRIBUTES
  //{
  //  new WHiddenAttribute()
  //}
  //W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypePoint, 1, WRTTIDefaultAllocator<WParticleTypePoint>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WRTTI* WParticleTypePointFactory::GetTypeType() const
{
  return WGetStaticRTTI<WParticleTypePoint>();
}

void WParticleTypePointFactory::CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const
{
  // WParticleTypePoint* pType = static_cast<WParticleTypePoint*>(pObject);
}

enum class TypePointVersion
{
  Version_0 = 0,


  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleTypePointFactory::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)TypePointVersion::Version_Current;
  inout_stream << uiVersion;
}

void WParticleTypePointFactory::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)TypePointVersion::Version_Current, "Invalid version {0}", uiVersion);
}

void WParticleTypePoint::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Color", WProcessingStream::DataType::Half4, &m_pStreamColor, false);
}

void WParticleTypePoint::ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const
{
  W_PROFILE_SCOPE("PFX: Point");

  const WUInt32 numParticles = (WUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != WRenderWorld::GetFrameCounter())
  {
    m_uiLastExtractedFrame = WRenderWorld::GetFrameCounter();

    const WVec4* pPosition = m_pStreamPosition->GetData<WVec4>();
    const WColorLinear16f* pColor = m_pStreamColor->GetData<WColorLinear16f>();

    // this will automatically be deallocated at the end of the frame
    m_BaseParticleData = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WBaseParticleShaderData, numParticles);
    m_BillboardParticleData = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WBillboardQuadParticleShaderData, numParticles);

    for (WUInt32 p = 0; p < numParticles; ++p)
    {
      m_BaseParticleData[p].Color = pColor[p].ToLinearFloat();
      m_BillboardParticleData[p].Position = pPosition[p].GetAsVec3();
    }
  }

  auto pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WParticlePointRenderData>(nullptr);

  pRenderData->m_vGlobalPosition = instanceTransform.m_vPosition;
  pRenderData->m_GlobalTransform = GetOwnerEffect()->NeedsToApplyTransform() ? instanceTransform : WTransform::MakeIdentity();
  pRenderData->m_TotalEffectLifeTime = GetOwnerEffect()->GetTotalEffectLifeTime();
  pRenderData->m_BaseParticleData = m_BaseParticleData;
  pRenderData->m_BillboardParticleData = m_BillboardParticleData;

  ref_msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::LitTransparent, WRenderData::Caching::Never);
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Point_ParticleTypePoint);
