#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WQuadParticleOrientation, 2)
  W_ENUM_CONSTANTS(WQuadParticleOrientation::Billboard)
  W_ENUM_CONSTANTS(WQuadParticleOrientation::Rotating_OrthoEmitterDir, WQuadParticleOrientation::Rotating_EmitterDir)
  W_ENUM_CONSTANTS(WQuadParticleOrientation::Fixed_EmitterDir, WQuadParticleOrientation::Fixed_RandomDir, WQuadParticleOrientation::Fixed_WorldUp)
  W_ENUM_CONSTANTS(WQuadParticleOrientation::FixedAxis_EmitterDir, WQuadParticleOrientation::FixedAxis_ParticleDir)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypeQuadFactory, 2, WRTTIDefaultAllocator<WParticleTypeQuadFactory>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Orientation", WQuadParticleOrientation, m_Orientation),
    W_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(90))),
    W_ENUM_MEMBER_PROPERTY("RenderMode", WParticleTypeRenderMode, m_RenderMode),
    W_ENUM_MEMBER_PROPERTY("LightingMode", WParticleLightingMode, m_LightingMode),
    W_MEMBER_PROPERTY("NormalCurvature", m_fNormalCurvature)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0, 1)),
    W_MEMBER_PROPERTY("LightDirectionality", m_fLightDirectionality)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0, 1)),
    W_MEMBER_PROPERTY("UseCustomMaterial", m_bUseCustomMaterial),
    W_MEMBER_PROPERTY("CustomMaterial", m_sCustomMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material", "QuadParticle")),
    W_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new WDefaultValueAttribute(WStringView("{ e00262e8-58f5-42f5-880d-569257047201 }"))),// wrap in WStringView to prevent a memory leak report
    W_ENUM_MEMBER_PROPERTY("TextureAtlas", WParticleTextureAtlasType, m_TextureAtlasType),
    W_ENUM_MEMBER_PROPERTY("TextureOrientation", WParticleTextureAtlasOrientation, m_TextureAtlasOrientation),
    W_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(1, 16)),
    W_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(1, 16)),
    W_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    W_MEMBER_PROPERTY("ParticleStretch", m_fStretch)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(-100.0f, 100.0f)),
    W_MEMBER_PROPERTY("GeometryProximityFadeOut", m_fGeometryProximityFadeOut)->AddAttributes(new WDefaultValueAttribute(0.1f), new WClampValueAttribute(0, 100)),
    W_MEMBER_PROPERTY("CameraProximityFadeOut", m_fCameraProximityFadeOut)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0, 100)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypeQuad, 1, WRTTIDefaultAllocator<WParticleTypeQuad>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WRTTI* WParticleTypeQuadFactory::GetTypeType() const
{
  return WGetStaticRTTI<WParticleTypeQuad>();
}

void WParticleTypeQuadFactory::CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const
{
  WParticleTypeQuad* pType = static_cast<WParticleTypeQuad*>(pObject);

  pType->m_Orientation = m_Orientation;
  pType->m_MaxDeviation = m_MaxDeviation;
  pType->m_hTexture.Invalidate();
  pType->m_RenderMode = m_RenderMode;
  pType->m_uiNumSpritesX = m_uiNumSpritesX;
  pType->m_uiNumSpritesY = m_uiNumSpritesY;
  pType->m_sTintColorParameter = WTempHashedString(m_sTintColorParameter.GetData());
  pType->m_TextureAtlasType = m_TextureAtlasType;
  pType->m_TextureAtlasOrientation = m_TextureAtlasOrientation;
  pType->m_fStretch = m_fStretch;
  pType->m_LightingMode = m_LightingMode;
  pType->m_fNormalCurvature = m_fNormalCurvature;
  pType->m_fLightDirectionality = m_fLightDirectionality;
  pType->m_fGeometryProximityFadeOut = m_fGeometryProximityFadeOut;
  pType->m_fCameraProximityFadeOut = m_fCameraProximityFadeOut;
  pType->m_hCustomMaterial.Invalidate();

  if (m_bUseCustomMaterial)
  {
    pType->m_hCustomMaterial = m_hCustomMaterial;
  }
  else
  {
    pType->m_hTexture = m_hTexture;
  }
}

enum class TypeQuadVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // sprite deviation
  Version_3, // distortion
  Version_4, // added texture atlas type
  Version_5, // added particle stretch
  Version_6, // added particle lighting
  Version_7, // added custom material support
  Version_8, // added proximity fade out parameters
  Version_9, // added texture atlas orientation

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleTypeQuadFactory::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)TypeQuadVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_Orientation;
  inout_stream << m_RenderMode;
  inout_stream << m_sTexture;
  inout_stream << m_uiNumSpritesX;
  inout_stream << m_uiNumSpritesY;
  inout_stream << m_sTintColorParameter;
  inout_stream << m_MaxDeviation;

  WString sDistortionTexture;
  float fDistortionStrength = 0;
  inout_stream << sDistortionTexture;
  inout_stream << fDistortionStrength;
  inout_stream << m_TextureAtlasType;

  // Version 5
  inout_stream << m_fStretch;

  // Version 6
  inout_stream << m_LightingMode;
  inout_stream << m_fNormalCurvature;
  inout_stream << m_fLightDirectionality;

  // Version 7
  inout_stream << m_bUseCustomMaterial;
  inout_stream << m_sCustomMaterial;

  // Version 8
  inout_stream << m_fGeometryProximityFadeOut;
  inout_stream << m_fCameraProximityFadeOut;

  // Version 9
  inout_stream << m_TextureAtlasOrientation;
}

void WParticleTypeQuadFactory::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)TypeQuadVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_Orientation;
  inout_stream >> m_RenderMode;
  inout_stream >> m_sTexture;
  inout_stream >> m_uiNumSpritesX;
  inout_stream >> m_uiNumSpritesY;
  inout_stream >> m_sTintColorParameter;

  if (uiVersion >= 2)
  {
    inout_stream >> m_MaxDeviation;
  }

  if (uiVersion >= 3)
  {
    WString sDistortionTexture;
    float fDistortionStrength;
    inout_stream >> sDistortionTexture;
    inout_stream >> fDistortionStrength;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_TextureAtlasType;

    if (m_TextureAtlasType == WParticleTextureAtlasType::None)
    {
      m_uiNumSpritesX = 1;
      m_uiNumSpritesY = 1;
    }
  }

  if (uiVersion >= 5)
  {
    inout_stream >> m_fStretch;
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
    inout_stream >> m_fGeometryProximityFadeOut;
    inout_stream >> m_fCameraProximityFadeOut;
  }

  if (uiVersion >= 9)
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

void WParticleTypeQuadFactory::QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const
{
  if (m_Orientation == WQuadParticleOrientation::FixedAxis_ParticleDir)
  {
    inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_LastPosition>());
  }
}

WParticleTypeQuad::WParticleTypeQuad() = default;
WParticleTypeQuad::~WParticleTypeQuad() = default;

void WParticleTypeQuad::CreateRequiredStreams()
{
  CreateStream("LifeTime", WProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", WProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", WProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", WProcessingStream::DataType::Half, &m_pStreamRotationSpeed, false);
  CreateStream("RotationOffset", WProcessingStream::DataType::Half, &m_pStreamRotationOffset, false);

  m_pStreamAxis = nullptr;
  m_pStreamVariation = nullptr;
  m_pStreamLastPosition = nullptr;

  if (m_Orientation == WQuadParticleOrientation::Fixed_RandomDir || m_Orientation == WQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == WQuadParticleOrientation::Fixed_WorldUp)
  {
    CreateStream("Axis", WProcessingStream::DataType::Float3, &m_pStreamAxis, true);
  }

  if (m_TextureAtlasType == WParticleTextureAtlasType::RandomVariations || m_TextureAtlasType == WParticleTextureAtlasType::RandomYAnimatedX || m_TextureAtlasType == WParticleTextureAtlasType::RandomXAnimatedY)
  {
    CreateStream("Variation", WProcessingStream::DataType::Int, &m_pStreamVariation, false);
  }

  if (m_Orientation == WQuadParticleOrientation::FixedAxis_ParticleDir)
  {
    CreateStream("LastPosition", WProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
  }
}

struct sodComparer
{
  // sort farther particles to the front, so that they get rendered first (back to front)
  W_ALWAYS_INLINE bool Less(const WParticleTypeQuad::sod& a, const WParticleTypeQuad::sod& b) const { return a.dist > b.dist; }
  W_ALWAYS_INLINE bool Equal(const WParticleTypeQuad::sod& a, const WParticleTypeQuad::sod& b) const { return a.dist == b.dist; }
};

void WParticleTypeQuad::ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const
{
  if ((!m_hTexture.IsValid() && !m_hCustomMaterial))
    return;

  const WUInt32 numParticles = (WUInt32)GetOwnerSystem()->GetNumActiveParticles();
  if (numParticles == 0)
    return;

  const bool bNeedsSorting = (m_RenderMode == WParticleTypeRenderMode::Blended) || (m_RenderMode == WParticleTypeRenderMode::BlendedForeground) || (m_RenderMode == WParticleTypeRenderMode::BlendedBackground);

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if ((m_uiLastExtractedFrame != WRenderWorld::GetFrameCounter())
    /*&& !bNeedsSorting*/) // TODO: in theory every shared instance has to sort the Quads, in practice this maybe should be an option
  {
    m_uiLastExtractedFrame = WRenderWorld::GetFrameCounter();

    if (bNeedsSorting)
    {
      // TODO: Using the frame allocator this way results in memory corruptions.
      // Not sure, whether this is supposed to work.
      WTempHybridArray<sod, 64> sorted; // (WFrameAllocator::GetCurrentAllocator());
      sorted.SetCountUninitialized(numParticles);

      const WVec3 vCameraPos = ref_msg.m_pView->GetCullingCamera()->GetCenterPosition();
      const WVec4* pPosition = m_pStreamPosition->GetData<WVec4>();

      for (WUInt32 p = 0; p < numParticles; ++p)
      {
        sorted[p].dist = (pPosition[p].GetAsVec3() - vCameraPos).GetLengthSquared();
        sorted[p].index = p;
      }

      sorted.Sort(sodComparer());

      CreateExtractedData(&sorted);
    }
    else
    {
      CreateExtractedData(nullptr);
    }
  }

  AddParticleRenderData(ref_msg, instanceTransform);
}

W_ALWAYS_INLINE WUInt32 noRedirect(WUInt32 uiIdx, const WHybridArray<WParticleTypeQuad::sod, 64>* pSorted)
{
  return uiIdx;
}

W_ALWAYS_INLINE WUInt32 sortedRedirect(WUInt32 uiIdx, const WHybridArray<WParticleTypeQuad::sod, 64>* pSorted)
{
  return (*pSorted)[uiIdx].index;
}

void WParticleTypeQuad::CreateExtractedData(const WHybridArray<sod, 64>* pSorted) const
{
  auto redirect = (pSorted != nullptr) ? sortedRedirect : noRedirect;

  const WUInt32 numParticles = (WUInt32)GetOwnerSystem()->GetNumActiveParticles();

  const bool bNeedsBillboardData = m_Orientation == WQuadParticleOrientation::Billboard;
  const bool bNeedsTangentData = !bNeedsBillboardData;

  const WVec3 vEmitterPos = GetOwnerSystem()->GetTransform().m_vPosition;
  const WVec3 vEmitterDir = GetOwnerSystem()->GetTransform().m_qRotation * WVec3(0, 0, 1); // Z axis
  const WVec3 vEmitterDirOrtho = vEmitterDir.GetOrthogonalVector();

  const WTime tCur = GetOwnerEffect()->GetTotalEffectLifeTime();
  const WColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, WColor::White);

  const WFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetData<WFloat16Vec2>();
  const WVec4* pPosition = m_pStreamPosition->GetData<WVec4>();
  const WFloat16* pSize = m_pStreamSize->GetData<WFloat16>();
  const WColorLinear16f* pColor = m_pStreamColor->GetData<WColorLinear16f>();
  const WFloat16* pRotationSpeed = m_pStreamRotationSpeed->GetData<WFloat16>();
  const WFloat16* pRotationOffset = m_pStreamRotationOffset->GetData<WFloat16>();
  const WVec3* pAxis = m_pStreamAxis ? m_pStreamAxis->GetData<WVec3>() : nullptr;
  const WUInt32* pVariation = m_pStreamVariation ? m_pStreamVariation->GetData<WUInt32>() : nullptr;
  const WVec3* pLastPosition = m_pStreamLastPosition ? m_pStreamLastPosition->GetData<WVec3>() : nullptr;

  // this will automatically be deallocated at the end of the frame
  m_BaseParticleData = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WBaseParticleShaderData, numParticles);

  AllocateParticleData(numParticles, bNeedsBillboardData, bNeedsTangentData);

  auto SetBaseData = [&](WUInt32 uiDstIdx, WUInt32 uiSrcIdx)
  {
    m_BaseParticleData[uiDstIdx].Size = pSize[uiSrcIdx];
    m_BaseParticleData[uiDstIdx].Color = pColor[uiSrcIdx].ToLinearFloat() * tintColor;
    m_BaseParticleData[uiDstIdx].Life = pLifeTime[uiSrcIdx].x * pLifeTime[uiSrcIdx].y;
    m_BaseParticleData[uiDstIdx].Variation = (pVariation != nullptr) ? pVariation[uiSrcIdx] : 0;
  };

  auto SetBillboardData = [&](WUInt32 uiDstIdx, WUInt32 uiSrcIdx)
  {
    m_BillboardParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    m_BillboardParticleData[uiDstIdx].RotationOffset = pRotationOffset[uiSrcIdx];
    m_BillboardParticleData[uiDstIdx].RotationSpeed = pRotationSpeed[uiSrcIdx];
  };

  auto SetTangentDataEmitterDir = [&](WUInt32 uiDstIdx, WUInt32 uiSrcIdx)
  {
    WMat3 mRotation = WMat3::MakeAxisRotation(vEmitterDir, WAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[uiSrcIdx]) + pRotationOffset[uiSrcIdx]));

    m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    m_TangentParticleData[uiDstIdx].TangentX = mRotation * vEmitterDirOrtho;
    m_TangentParticleData[uiDstIdx].TangentZ = vEmitterDir;
  };

  auto SetTangentDataEmitterDirOrtho = [&](WUInt32 uiDstIdx, WUInt32 uiSrcIdx)
  {
    const WVec3 vDirToParticle = (pPosition[uiSrcIdx].GetAsVec3() - vEmitterPos);
    WVec3 vOrthoDir = vEmitterDir.CrossRH(vDirToParticle);
    vOrthoDir.NormalizeIfNotZero(WVec3(1, 0, 0)).IgnoreResult();

    WMat3 mRotation = WMat3::MakeAxisRotation(vOrthoDir, WAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[uiSrcIdx]) + pRotationOffset[uiSrcIdx]));

    m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    m_TangentParticleData[uiDstIdx].TangentX = vOrthoDir;
    m_TangentParticleData[uiDstIdx].TangentZ = mRotation * vEmitterDir;
  };

  auto SetTangentDataFromAxis = [&](WUInt32 uiDstIdx, WUInt32 uiSrcIdx)
  {
    W_ASSERT_DEBUG(pAxis != nullptr, "Axis must be valid");
    WVec3 vNormal = pAxis[uiSrcIdx];
    vNormal.Normalize();

    const WVec3 vTangentStart = vNormal.GetOrthogonalVector().GetNormalized();

    WMat3 mRotation = WMat3::MakeAxisRotation(vNormal, WAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[uiSrcIdx]) + pRotationOffset[uiSrcIdx]));

    const WVec3 vTangentX = mRotation * vTangentStart;

    m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    m_TangentParticleData[uiDstIdx].TangentX = vTangentX;
    m_TangentParticleData[uiDstIdx].TangentZ = vTangentX.CrossRH(vNormal);
  };

  auto SetTangentDataAligned_Emitter = [&](WUInt32 uiDstIdx, WUInt32 uiSrcIdx)
  {
    m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    m_TangentParticleData[uiDstIdx].TangentX = vEmitterDir;
    m_TangentParticleData[uiDstIdx].TangentZ.x = m_fStretch;
  };

  auto SetTangentDataAligned_ParticleDir = [&](WUInt32 uiDstIdx, WUInt32 uiSrcIdx)
  {
    const WVec3 vCurPos = pPosition[uiSrcIdx].GetAsVec3();
    const WVec3 vLastPos = pLastPosition[uiSrcIdx];
    const WVec3 vDir = vCurPos - vLastPos;
    m_TangentParticleData[uiDstIdx].Position = vCurPos;
    m_TangentParticleData[uiDstIdx].TangentX = vDir;
    m_TangentParticleData[uiDstIdx].TangentZ.x = m_fStretch;
  };

  for (WUInt32 p = 0; p < numParticles; ++p)
  {
    SetBaseData(p, redirect(p, pSorted));
  }

  if (bNeedsBillboardData)
  {
    for (WUInt32 p = 0; p < numParticles; ++p)
    {
      SetBillboardData(p, redirect(p, pSorted));
    }
  }

  if (bNeedsTangentData)
  {
    if (m_Orientation == WQuadParticleOrientation::Rotating_EmitterDir)
    {
      for (WUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataEmitterDir(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == WQuadParticleOrientation::Rotating_OrthoEmitterDir)
    {
      for (WUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataEmitterDirOrtho(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == WQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == WQuadParticleOrientation::Fixed_RandomDir || m_Orientation == WQuadParticleOrientation::Fixed_WorldUp)
    {
      for (WUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataFromAxis(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == WQuadParticleOrientation::FixedAxis_EmitterDir)
    {
      for (WUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataAligned_Emitter(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == WQuadParticleOrientation::FixedAxis_ParticleDir)
    {
      W_ASSERT_DEBUG(pLastPosition != nullptr, "FixedAxis_ParticleDir needs the last position attribute");
      for (WUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataAligned_ParticleDir(p, redirect(p, pSorted));
      }
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void WParticleTypeQuad::AddParticleRenderData(WMsgExtractRenderData& msg, const WTransform& instanceTransform) const
{
  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WParticleQuadRenderData>(nullptr);

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
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_BaseParticleData = m_BaseParticleData;
  pRenderData->m_BillboardParticleData = m_BillboardParticleData;
  pRenderData->m_TangentParticleData = m_TangentParticleData;
  pRenderData->m_uiNumVariationsX = 1;
  pRenderData->m_uiNumVariationsY = 1;
  pRenderData->m_uiNumFlipbookAnimationsX = 1;
  pRenderData->m_uiNumFlipbookAnimationsY = 1;
  pRenderData->m_LightingMode = m_LightingMode;
  pRenderData->m_TextureAtlasOrientation = m_TextureAtlasOrientation;
  pRenderData->m_fNormalCurvature = m_fNormalCurvature;
  pRenderData->m_fLightDirectionality = m_fLightDirectionality;
  pRenderData->m_fGeometryProximityFadeOut = m_fGeometryProximityFadeOut;
  pRenderData->m_fCameraProximityFadeOut = m_fCameraProximityFadeOut;
  pRenderData->m_hCustomMaterial = m_hCustomMaterial;

  switch (m_Orientation)
  {
    case WQuadParticleOrientation::Billboard:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_BILLBOARD";
      break;
    case WQuadParticleOrientation::Rotating_OrthoEmitterDir:
    case WQuadParticleOrientation::Rotating_EmitterDir:
    case WQuadParticleOrientation::Fixed_EmitterDir:
    case WQuadParticleOrientation::Fixed_WorldUp:
    case WQuadParticleOrientation::Fixed_RandomDir:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_TANGENTS";
      break;
    case WQuadParticleOrientation::FixedAxis_EmitterDir:
    case WQuadParticleOrientation::FixedAxis_ParticleDir:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_AXIS_ALIGNED";
      break;
  }

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

  msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::LitTransparent, WRenderData::Caching::Never);
}

void WParticleTypeQuad::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  if (m_pStreamAxis != nullptr)
  {
    WVec3* pAxis = m_pStreamAxis->GetWritableData<WVec3>();
    WRandom& rng = GetRNG();

    if (m_Orientation == WQuadParticleOrientation::Fixed_RandomDir)
    {
      W_PROFILE_SCOPE("PFX: Init Quad Axis Random");

      for (WUInt32 i = 0; i < uiNumElements; ++i)
      {
        const WUInt64 uiElementIdx = uiStartIndex + i;

        pAxis[uiElementIdx] = WVec3::MakeRandomDirection(rng);
      }
    }
    else if (m_Orientation == WQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == WQuadParticleOrientation::Fixed_WorldUp)
    {
      W_PROFILE_SCOPE("PFX: Init Quad Axis");

      WVec3 vNormal;

      if (m_Orientation == WQuadParticleOrientation::Fixed_EmitterDir)
      {
        vNormal = GetOwnerSystem()->GetTransform().m_qRotation * WVec3(0, 0, 1); // Z axis
      }
      else if (m_Orientation == WQuadParticleOrientation::Fixed_WorldUp)
      {
        WCoordinateSystem coord;
        GetOwnerSystem()->GetWorld()->GetCoordinateSystem(GetOwnerSystem()->GetTransform().m_vPosition, coord);

        vNormal = coord.m_vUpDir;
      }

      if (m_MaxDeviation > WAngle::MakeFromDegree(1.0f))
      {
        // how to get from the X axis to the desired normal
        WQuat qRotToDir = WQuat::MakeShortestRotation(WVec3(1, 0, 0), vNormal);

        for (WUInt32 i = 0; i < uiNumElements; ++i)
        {
          const WUInt64 uiElementIdx = uiStartIndex + i;
          const WVec3 vRandomX = WVec3::MakeRandomDeviationX(rng, m_MaxDeviation);

          pAxis[uiElementIdx] = qRotToDir * vRandomX;
        }
      }
      else
      {
        for (WUInt32 i = 0; i < uiNumElements; ++i)
        {
          const WUInt64 uiElementIdx = uiStartIndex + i;
          pAxis[uiElementIdx] = vNormal;
        }
      }
    }
  }
}

void WParticleTypeQuad::AllocateParticleData(const WUInt32 numParticles, const bool bNeedsBillboardData, const bool bNeedsTangentData) const
{
  m_BillboardParticleData = nullptr;
  if (bNeedsBillboardData)
  {
    m_BillboardParticleData = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WBillboardQuadParticleShaderData, numParticles);
  }

  m_TangentParticleData = nullptr;
  if (bNeedsTangentData)
  {
    m_TangentParticleData = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WTangentQuadParticleShaderData, numParticles);
  }
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WQuadParticleOrientationPatch_1_2 final : public WGraphPatch
{
public:
  WQuadParticleOrientationPatch_1_2()
    : WGraphPatch("WQuadParticleOrientation", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // TODO: this type of patch does not work

    pNode->RenameProperty("FragmentOrthogonalEmitterDirection", "Rotating_OrthoEmitterDir");
    pNode->RenameProperty("FragmentEmitterDirection", "Rotating_EmitterDir");

    pNode->RenameProperty("SpriteEmitterDirection", "Fixed_EmitterDir");
    pNode->RenameProperty("SpriteRandom", "Fixed_RandomDir");
    pNode->RenameProperty("SpriteWorldUp", "Fixed_WorldUp");

    pNode->RenameProperty("AxisAligned_Emitter", "FixedAxis_EmitterDir");
  }
};

WQuadParticleOrientationPatch_1_2 g_WQuadParticleOrientationPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class WParticleTypeQuadFactory_1_2 final : public WGraphPatch
{
public:
  WParticleTypeQuadFactory_1_2()
    : WGraphPatch("WParticleTypeQuadFactory", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    WAbstractObjectNode::Property* pProp = pNode->FindProperty("Orientation");
    const WStringBuilder sOri = pProp->m_Value.Get<WString>();

    if (sOri == "WQuadParticleOrientation::FragmentOrthogonalEmitterDirection")
      pProp->m_Value = "WQuadParticleOrientation::Rotating_OrthoEmitterDir";

    if (sOri == "WQuadParticleOrientation::FragmentEmitterDirection")
      pProp->m_Value = "WQuadParticleOrientation::Rotating_EmitterDir";

    if (sOri == "WQuadParticleOrientation::SpriteEmitterDirection")
      pProp->m_Value = "WQuadParticleOrientation::Fixed_EmitterDir";

    if (sOri == "WQuadParticleOrientation::SpriteRandom")
      pProp->m_Value = "WQuadParticleOrientation::Fixed_RandomDir";

    if (sOri == "WQuadParticleOrientation::SpriteWorldUp")
      pProp->m_Value = "WQuadParticleOrientation::Fixed_WorldUp";

    if (sOri == "WQuadParticleOrientation::AxisAligned_Emitter")
      pProp->m_Value = "WQuadParticleOrientation::FixedAxis_EmitterDir";
  }
};

WParticleTypeQuadFactory_1_2 g_WParticleTypeQuadFactory_1_2;


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Quad_ParticleTypeQuad);
