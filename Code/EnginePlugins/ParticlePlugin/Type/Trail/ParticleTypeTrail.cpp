#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypeTrailFactory, 1, WRTTIDefaultAllocator<WParticleTypeTrailFactory>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("RenderMode", WParticleTypeRenderMode, m_RenderMode),
    W_ENUM_MEMBER_PROPERTY("LightingMode", WParticleLightingMode, m_LightingMode),
    W_MEMBER_PROPERTY("NormalCurvature", m_fNormalCurvature)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0, 1)),
    W_MEMBER_PROPERTY("LightDirectionality", m_fLightDirectionality)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0, 1)),
    W_MEMBER_PROPERTY("UseCustomMaterial", m_bUseCustomMaterial),
    W_MEMBER_PROPERTY("CustomMaterial", m_sCustomMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material", "TrailParticle")),
    W_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new WDefaultValueAttribute(WStringView("{ e00262e8-58f5-42f5-880d-569257047201 }"))),// wrap in WStringView to prevent a memory leak report
    W_MEMBER_PROPERTY("Segments", m_uiMaxPoints)->AddAttributes(new WDefaultValueAttribute(6), new WClampValueAttribute(3, 64)),
    W_ENUM_MEMBER_PROPERTY("TextureAtlas", WParticleTextureAtlasType, m_TextureAtlasType),
    W_ENUM_MEMBER_PROPERTY("TextureOrientation", WParticleTextureAtlasOrientation, m_TextureAtlasOrientation),
    W_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(1, 16)),
    W_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(1, 16)),
    W_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypeTrail, 1, WRTTIDefaultAllocator<WParticleTypeTrail>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WRTTI* WParticleTypeTrailFactory::GetTypeType() const
{
  return WGetStaticRTTI<WParticleTypeTrail>();
}

void WParticleTypeTrailFactory::CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const
{
  WParticleTypeTrail* pType = static_cast<WParticleTypeTrail*>(pObject);

  pType->m_RenderMode = m_RenderMode;
  pType->m_uiMaxPoints = m_uiMaxPoints;
  pType->m_hTexture.Invalidate();
  pType->m_TextureAtlasType = m_TextureAtlasType;
  pType->m_TextureAtlasOrientation = m_TextureAtlasOrientation;
  pType->m_uiNumSpritesX = m_uiNumSpritesX;
  pType->m_uiNumSpritesY = m_uiNumSpritesY;
  pType->m_sTintColorParameter = WTempHashedString(m_sTintColorParameter.GetData());
  pType->m_LightingMode = m_LightingMode;
  pType->m_fNormalCurvature = m_fNormalCurvature;
  pType->m_fLightDirectionality = m_fLightDirectionality;
  pType->m_hCustomMaterial.Invalidate();

  if (m_bUseCustomMaterial)
  {
    pType->m_hCustomMaterial = m_hCustomMaterial;
  }
  else
  {
    pType->m_hTexture = m_hTexture;
  }

  // fixed 25 FPS for the update rate
  pType->m_UpdateDiff = WTime::MakeFromSeconds(1.0 / 25.0); // m_UpdateDiff;

  if (bFirstTime)
  {
    pType->GetOwnerSystem()->AddParticleDeathEventHandler(WMakeDelegate(&WParticleTypeTrail::OnParticleDeath, pType));

    pType->m_LastSnapshot = pType->GetOwnerEffect()->GetTotalEffectLifeTime();
  }

  // m_uiMaxPoints = WMath::Min<WUInt16>(8, m_uiMaxPoints);

  // clamp the number of points to the maximum possible count
  pType->m_uiMaxPoints = WMath::Min<WUInt16>(pType->m_uiMaxPoints, pType->ComputeTrailPointBucketSize(pType->m_uiMaxPoints));

  pType->m_uiCurFirstIndex = 1;
}

enum class TypeTrailVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added render mode
  Version_3, // added texture atlas support
  Version_4, // added tint color
  Version_5, // added distortion mode
  Version_6, // added particle lighting
  Version_7, // added custom material support
  Version_8, // added texture atlas orientation

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleTypeTrailFactory::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)TypeTrailVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_sTexture;
  inout_stream << m_uiMaxPoints;
  inout_stream << m_UpdateDiff;
  inout_stream << m_RenderMode;

  // version 3
  inout_stream << m_TextureAtlasType;
  inout_stream << m_uiNumSpritesX;
  inout_stream << m_uiNumSpritesY;

  // version 4
  inout_stream << m_sTintColorParameter;

  // version 5
  WString sDistortionTexture;
  float fDistortionStrength = 0;
  inout_stream << sDistortionTexture;
  inout_stream << fDistortionStrength;

  // Version 6
  inout_stream << m_LightingMode;
  inout_stream << m_fNormalCurvature;
  inout_stream << m_fLightDirectionality;

  // Version 7
  inout_stream << m_bUseCustomMaterial;
  inout_stream << m_sCustomMaterial;

  // Version 8
  inout_stream << m_TextureAtlasOrientation;
}

void WParticleTypeTrailFactory::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)TypeTrailVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sTexture;
  inout_stream >> m_uiMaxPoints;
  inout_stream >> m_UpdateDiff;

  if (uiVersion >= 2)
  {
    inout_stream >> m_RenderMode;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_TextureAtlasType;
    inout_stream >> m_uiNumSpritesX;
    inout_stream >> m_uiNumSpritesY;

    if (m_TextureAtlasType == WParticleTextureAtlasType::None)
    {
      m_uiNumSpritesX = 1;
      m_uiNumSpritesY = 1;
    }
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_sTintColorParameter;
  }

  if (uiVersion >= 5)
  {
    WString sDistortionTexture;
    float fDistortionStrength;
    inout_stream >> sDistortionTexture;
    inout_stream >> fDistortionStrength;
  }

  if (uiVersion >= 6)
  {
    inout_stream >> m_LightingMode;
    inout_stream >> m_fNormalCurvature;
    inout_stream >> m_fLightDirectionality;
  }

  if (uiVersion >= 7)
  {
    inout_stream >> m_bUseCustomMaterial;
    inout_stream >> m_sCustomMaterial;
  }

  if (uiVersion >= 8)
  {
    inout_stream >> m_TextureAtlasOrientation;
  }

  if (m_bUseCustomMaterial && !m_sCustomMaterial.IsEmpty())
  {
    m_hCustomMaterial = WResourceManager::LoadResource<WMaterialResource>(m_sCustomMaterial);
  }
  else if (!m_sTexture.IsEmpty())
  {
    m_hTexture = WResourceManager::LoadResource<WTexture2DResource>(m_sTexture);
  }
}

//////////////////////////////////////////////////////////////////////////

WParticleTypeTrail::WParticleTypeTrail() = default;

WParticleTypeTrail::~WParticleTypeTrail()
{
  if (m_pStreamPosition != nullptr)
  {
    GetOwnerSystem()->RemoveParticleDeathEventHandler(WMakeDelegate(&WParticleTypeTrail::OnParticleDeath, this));
  }
}

void WParticleTypeTrail::CreateRequiredStreams()
{
  CreateStream("LifeTime", WProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", WProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", WProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("TrailData", WProcessingStream::DataType::Short2, &m_pStreamTrailData, true);

  m_pStreamVariation = nullptr;

  if (m_TextureAtlasType == WParticleTextureAtlasType::RandomVariations || m_TextureAtlasType == WParticleTextureAtlasType::RandomYAnimatedX || m_TextureAtlasType == WParticleTextureAtlasType::RandomXAnimatedY)
  {
    CreateStream("Variation", WProcessingStream::DataType::Int, &m_pStreamVariation, false);
  }
}

void WParticleTypeTrail::ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const
{
  W_PROFILE_SCOPE("PFX: Trail");

  if (!m_hTexture.IsValid() && !m_hCustomMaterial.IsValid())
    return;

  const WUInt32 numActiveParticles = (WUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numActiveParticles == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != WRenderWorld::GetFrameCounter())
  {
    m_uiLastExtractedFrame = WRenderWorld::GetFrameCounter();

    const WColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, WColor::White);

    const WFloat16* pSize = m_pStreamSize->GetData<WFloat16>();
    const WColorLinear16f* pColor = m_pStreamColor->GetData<WColorLinear16f>();
    const TrailData* pTrailData = m_pStreamTrailData->GetData<TrailData>();
    const WFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetData<WFloat16Vec2>();
    const WUInt32* pVariation = m_pStreamVariation ? m_pStreamVariation->GetData<WUInt32>() : nullptr;

    const WUInt32 uiBucketSize = ComputeTrailPointBucketSize(m_uiMaxPoints);

    // this will automatically be deallocated at the end of the frame
    m_BaseParticleData =
      W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WBaseParticleShaderData, (WUInt32)GetOwnerSystem()->GetNumActiveParticles());
    m_TrailPointsShared =
      W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WVec4, (WUInt32)GetOwnerSystem()->GetNumActiveParticles() * uiBucketSize);
    m_TrailParticleData =
      W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WTrailParticleShaderData, (WUInt32)GetOwnerSystem()->GetNumActiveParticles());

    for (WUInt32 p = 0; p < numActiveParticles; ++p)
    {
      m_BaseParticleData[p].Size = pSize[p];
      m_BaseParticleData[p].Color = pColor[p].ToLinearFloat() * tintColor;
      m_BaseParticleData[p].Life = pLifeTime[p].x * pLifeTime[p].y;
      m_BaseParticleData[p].Variation = (pVariation != nullptr) ? pVariation[p] : 0;

      m_TrailParticleData[p].NumPoints = pTrailData[p].m_uiNumPoints;
    }

    for (WUInt32 p = 0; p < numActiveParticles; ++p)
    {
      const WVec4* pTrailPositions = GetTrailPointsPositions(pTrailData[p].m_uiIndexForTrailPoints);

      WVec4* pRenderPositions = &m_TrailPointsShared[p * uiBucketSize];

      /// \todo This loop could be done without a condition
      for (WUInt32 i = 0; i < m_uiMaxPoints; ++i)
      {
        if (i > m_uiCurFirstIndex)
        {
          pRenderPositions[i] = pTrailPositions[m_uiCurFirstIndex + m_uiMaxPoints - i];
        }
        else
        {
          pRenderPositions[i] = pTrailPositions[m_uiCurFirstIndex - i];
        }
      }
    }
  }

  auto pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WParticleTrailRenderData>(nullptr);

  if (m_hCustomMaterial.IsValid())
  {
    pRenderData->m_uiSortingKey = ComputeSortingKey(m_RenderMode, m_hCustomMaterial.GetResourceIDHash(), 0);
  }
  else
  {
    pRenderData->m_uiSortingKey = ComputeSortingKey(m_RenderMode, m_hTexture.GetResourceIDHash(), 0);
  }

  pRenderData->m_vGlobalPosition = instanceTransform.m_vPosition;

  pRenderData->m_GlobalTransform = GetOwnerEffect()->NeedsToApplyTransform() ? instanceTransform : WTransform::MakeIdentity();
  pRenderData->m_TotalEffectLifeTime = GetOwnerEffect()->GetTotalEffectLifeTime();
  pRenderData->m_RenderMode = m_RenderMode;
  pRenderData->m_uiMaxTrailPoints = m_uiMaxPoints;
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_BaseParticleData = m_BaseParticleData;
  pRenderData->m_TrailParticleData = m_TrailParticleData;
  pRenderData->m_TrailPointsShared = m_TrailPointsShared;
  pRenderData->m_fSnapshotFraction = m_fSnapshotFraction;
  pRenderData->m_LightingMode = m_LightingMode;
  pRenderData->m_TextureAtlasOrientation = m_TextureAtlasOrientation;
  pRenderData->m_fNormalCurvature = m_fNormalCurvature;
  pRenderData->m_fLightDirectionality = m_fLightDirectionality;
  pRenderData->m_hCustomMaterial = m_hCustomMaterial;

  pRenderData->m_uiNumVariationsX = 1;
  pRenderData->m_uiNumVariationsY = 1;
  pRenderData->m_uiNumFlipbookAnimationsX = 1;
  pRenderData->m_uiNumFlipbookAnimationsY = 1;

  switch (m_TextureAtlasType)
  {
    case WParticleTextureAtlasType::None:
      break;

    case WParticleTextureAtlasType::RandomVariations:
      pRenderData->m_uiNumVariationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumVariationsY = m_uiNumSpritesY;
      break;

    case WParticleTextureAtlasType::FlipbookAnimation:
      pRenderData->m_uiNumFlipbookAnimationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumFlipbookAnimationsY = m_uiNumSpritesY;
      break;

    case WParticleTextureAtlasType::RandomYAnimatedX:
      pRenderData->m_uiNumFlipbookAnimationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumVariationsY = m_uiNumSpritesY;
      break;

    case WParticleTextureAtlasType::RandomXAnimatedY:
      pRenderData->m_uiNumVariationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumFlipbookAnimationsY = m_uiNumSpritesY;
      break;
  }

  ref_msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::LitTransparent, WRenderData::Caching::Never);
}

void WParticleTypeTrail::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  TrailData* pTrailData = m_pStreamTrailData->GetWritableData<TrailData>() + uiStartIndex;

  const WVec4* pPosData = m_pStreamPosition->GetData<WVec4>() + uiStartIndex;

  const WUInt32 uiPrevIndex = (m_uiCurFirstIndex > 0) ? (m_uiCurFirstIndex - 1) : (m_uiMaxPoints - 1);
  const WUInt32 uiPrevIndex2 = (uiPrevIndex > 0) ? (uiPrevIndex - 1) : (m_uiMaxPoints - 1);

  for (WUInt64 i = 0; i < uiNumElements; ++i)
  {
    const WVec4 vStartPos = pPosData[i];

    TrailData& td = pTrailData[i];
    td.m_uiNumPoints = 2;
    td.m_uiIndexForTrailPoints = GetIndexForTrailPoints();

    WVec4* pPos = GetTrailPointsPositions(td.m_uiIndexForTrailPoints);
    pPos[m_uiCurFirstIndex] = vStartPos;
    pPos[uiPrevIndex] = vStartPos;
    pPos[uiPrevIndex2] = vStartPos;
  }
}


void WParticleTypeTrail::Process(WUInt64 uiNumElements)
{
  const WTime tNow = GetOwnerEffect()->GetTotalEffectLifeTime();

  TrailData* pTrailData = m_pStreamTrailData->GetWritableData<TrailData>();
  const WVec4* pPosData = m_pStreamPosition->GetData<WVec4>();

  if (tNow - m_LastSnapshot >= m_UpdateDiff)
  {
    m_LastSnapshot = tNow;

    m_uiCurFirstIndex = (m_uiCurFirstIndex + 1) == m_uiMaxPoints ? 0 : (m_uiCurFirstIndex + 1);

    for (WUInt64 i = 0; i < uiNumElements; ++i)
    {
      pTrailData[i].m_uiNumPoints = WMath::Min<WUInt16>(pTrailData[i].m_uiNumPoints + 1, m_uiMaxPoints);
    }
  }

  m_fSnapshotFraction = 1.0f - (float)((tNow - m_LastSnapshot).GetSeconds() / m_UpdateDiff.GetSeconds());

  for (WUInt64 i = 0; i < uiNumElements; ++i)
  {
    WVec4* pPositions = GetTrailPointsPositions(pTrailData[i].m_uiIndexForTrailPoints);
    pPositions[m_uiCurFirstIndex] = pPosData[i];
  }
}

WUInt16 WParticleTypeTrail::GetIndexForTrailPoints()
{
  WUInt16 res = 0;

  if (!m_FreeTrailData.IsEmpty())
  {
    res = m_FreeTrailData.PeekBack();
    m_FreeTrailData.PopBack();
  }
  else
  {
    // expand the proper array

    // if (m_uiMaxPoints > 32)
    //{
    res = static_cast<WUInt16>(m_TrailPoints64.GetCount());
    m_TrailPoints64.ExpandAndGetRef();
    //}
    // else if (m_uiMaxPoints > 16)
    //{
    //  res = m_TrailData32.GetCount() & 0xFFFF;
    //  m_TrailData64.ExpandAndGetRef();
    //}
    // else if (m_uiMaxPoints > 8)
    //{
    //  res = m_TrailData16.GetCount() & 0xFFFF;
    //  m_TrailData64.ExpandAndGetRef();
    //}
    // else
    //{
    //  res = m_TrailData8.GetCount() & 0xFFFF;
    //  m_TrailData8.ExpandAndGetRef();
    //}
  }

  return res;
}

WVec4* WParticleTypeTrail::GetTrailPointsPositions(WUInt32 index)
{
  // if (m_uiMaxPoints > 32)
  {
    return &m_TrailPoints64[index].Positions[0];
  }
  // else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailPoints32[index].Positions[0];
  //}
  // else if (m_uiMaxPoints > 8)
  //{
  //  return &m_TrailPoints16[index].Positions[0];
  //}
  // else
  //{
  //  return &m_TrailPoints8[index].Positions[0];
  //}
}

const WVec4* WParticleTypeTrail::GetTrailPointsPositions(WUInt32 index) const
{
  // if (m_uiMaxPoints > 32)
  {
    return &m_TrailPoints64[index].Positions[0];
  }
  // else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailPoints32[index].Positions[0];
  //}
  // else if (m_uiMaxPoints > 8)
  //{
  //  return &m_TrailPoints16[index].Positions[0];
  //}
  // else
  //{
  //  return &m_TrailPoints8[index].Positions[0];
  //}
}


WUInt16 WParticleTypeTrail::ComputeTrailPointBucketSize(WUInt16 uiMaxTrailPoints)
{
  if (uiMaxTrailPoints > 32)
  {
    return 64;
  }
  else if (uiMaxTrailPoints > 16)
  {
    return 32;
  }
  else if (uiMaxTrailPoints > 8)
  {
    return 16;
  }
  else
  {
    return 8;
  }
}

void WParticleTypeTrail::OnParticleDeath(const WStreamGroupElementRemovedEvent& e)
{
  const TrailData* pTrailData = m_pStreamTrailData->GetData<TrailData>();

  // return the trail data to the list of free elements
  m_FreeTrailData.PushBack(pTrailData[e.m_uiElementIndex].m_uiIndexForTrailPoints);
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Trail_ParticleTypeTrail);
