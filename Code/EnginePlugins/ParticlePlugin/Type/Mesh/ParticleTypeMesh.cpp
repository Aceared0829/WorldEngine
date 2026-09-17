#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Mesh/ParticleTypeMesh.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypeMeshFactory, 1, WRTTIDefaultAllocator<WParticleTypeMeshFactory>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Mesh", m_sMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Static"), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material")),
    W_MEMBER_PROPERTY("Scale", m_fScale)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypeMesh, 1, WRTTIDefaultAllocator<WParticleTypeMesh>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WRTTI* WParticleTypeMeshFactory::GetTypeType() const
{
  return WGetStaticRTTI<WParticleTypeMesh>();
}

void WParticleTypeMeshFactory::CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const
{
  WParticleTypeMesh* pType = static_cast<WParticleTypeMesh*>(pObject);

  pType->m_hMesh = m_hMesh;
  pType->m_hMaterial = m_hMaterial;
  pType->m_sTintColorParameter = WTempHashedString(m_sTintColorParameter.GetData());
  pType->m_bMaterialOverride = !m_sMaterial.IsEmpty();
  pType->m_fScale = m_fScale;

  pType->m_pRenderDataManager = (WRenderDataManager*)pType->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(WGetStaticRTTI<WRenderDataManager>());
}

enum class TypeMeshVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added material
  Version_3, // added scale

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleTypeMeshFactory::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)TypeMeshVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_sMesh;
  inout_stream << m_sTintColorParameter;

  // Version 2
  inout_stream << m_sMaterial;

  // Version 3
  inout_stream << m_fScale;
}

void WParticleTypeMeshFactory::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)TypeMeshVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sMesh;
  inout_stream >> m_sTintColorParameter;

  if (uiVersion >= 2)
  {
    inout_stream >> m_sMaterial;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_fScale;
  }

  if (!m_sMesh.IsEmpty())
  {
    m_hMesh = WResourceManager::LoadResource<WMeshResource>(m_sMesh);
  }

  if (!m_sMaterial.IsEmpty())
  {
    m_hMaterial = WResourceManager::LoadResource<WMaterialResource>(m_sMaterial);
  }
}

WParticleTypeMesh::WParticleTypeMesh() = default;

WParticleTypeMesh::~WParticleTypeMesh()
{
  if (m_pRenderDataManager != nullptr)
  {
    m_pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);
  }
  else
  {
    W_ASSERT_DEBUG(m_InstanceDataOffset.IsInvalidated(), "Implementation error");
  }
}

void WParticleTypeMesh::CreateRequiredStreams()
{
  QueryMeshAndMaterialInfo();

  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", WProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", WProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", WProcessingStream::DataType::Half, &m_pStreamRotationSpeed, false);
  CreateStream("RotationOffset", WProcessingStream::DataType::Half, &m_pStreamRotationOffset, false);
  CreateStream("Axis", WProcessingStream::DataType::Float3, &m_pStreamAxis, true);

  if (m_uiNumSubMeshes > 1)
  {
    // only create this stream when necessary
    CreateStream("Variation", WProcessingStream::DataType::Int, &m_pStreamVariation, false);
  }
}

void WParticleTypeMesh::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  WVec3* pAxis = m_pStreamAxis->GetWritableData<WVec3>();
  WRandom& rng = GetRNG();

  for (WUInt32 i = 0; i < uiNumElements; ++i)
  {
    const WUInt64 uiElementIdx = uiStartIndex + i;

    pAxis[uiElementIdx] = WVec3::MakeRandomDirection(rng);
  }
}

bool WParticleTypeMesh::QueryMeshAndMaterialInfo() const
{
  if (!m_hMesh.IsValid())
  {
    m_bRenderDataCached = true;
    m_hMaterial.Invalidate();
    m_uiNumSubMeshes = 0;
    m_CachedSubMeshMaterials.Clear();
    return true;
  }

  WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() != WResourceAcquireResult::Final)
    return false;

  m_uiNumSubMeshes = static_cast<WUInt8>(pMesh->GetSubMeshes().GetCount());

  if (!m_hMaterial.IsValid() && !pMesh->GetMaterials().IsEmpty())
  {
    // When no material is specified, we'll use per-submesh materials during rendering
    // For now, use the first material to determine the render category
    m_hMaterial = pMesh->GetMaterials()[0];
  }

  // Cache materials for each submesh to avoid locking the mesh resource during rendering
  if (!m_bMaterialOverride && m_uiNumSubMeshes > 0)
  {
    const auto& subMeshes = pMesh->GetSubMeshes();
    const auto& materials = pMesh->GetMaterials();

    m_CachedSubMeshMaterials.SetCount(m_uiNumSubMeshes);
    for (WUInt32 i = 0; i < m_uiNumSubMeshes; ++i)
    {
      const WUInt32 uiMaterialIdx = subMeshes[i].m_uiMaterialIndex;
      if (uiMaterialIdx < materials.GetCount() && materials[uiMaterialIdx].IsValid())
      {
        m_CachedSubMeshMaterials[i] = materials[uiMaterialIdx];
      }
      else
      {
        m_CachedSubMeshMaterials[i].Invalidate();
      }
    }
  }
  else
  {
    m_CachedSubMeshMaterials.Clear();
  }

  if (!m_hMaterial.IsValid())
  {
    m_bRenderDataCached = true;
    return true;
  }

  WResourceLock<WMaterialResource> pMaterial(m_hMaterial, WResourceAcquireMode::AllowLoadingFallback);
  if (pMaterial.GetAcquireResult() != WResourceAcquireResult::Final)
    return false;

  m_RenderCategory = pMaterial->GetRenderDataCategory();

  m_bRenderDataCached = true;
  return true;
}

void WParticleTypeMesh::RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<WRenderDataManager>();
}

void WParticleTypeMesh::ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const
{
  if (!m_bRenderDataCached)
  {
    // check if we now know how to render this thing
    if (!QueryMeshAndMaterialInfo())
      return;
  }

  if (m_uiNumSubMeshes == 0 || !m_hMaterial.IsValid())
    return;

  if (m_RenderCategory.m_uiValue == 0xFFFF)
  {
    m_bRenderDataCached = false;
    return;
  }

  const WUInt32 numParticles = (WUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  W_PROFILE_SCOPE("PFX: Mesh");

  const WTime tCur = GetOwnerEffect()->GetTotalEffectLifeTime();
  const WColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, WColor::White);

  const WVec4* pPosition = m_pStreamPosition->GetData<WVec4>();
  const WFloat16* pSize = m_pStreamSize->GetData<WFloat16>();
  const WColorLinear16f* pColor = m_pStreamColor->GetData<WColorLinear16f>();
  const WFloat16* pRotationSpeed = m_pStreamRotationSpeed->GetData<WFloat16>();
  const WFloat16* pRotationOffset = m_pStreamRotationOffset->GetData<WFloat16>();
  const WVec3* pAxis = m_pStreamAxis->GetData<WVec3>();
  const WInt32* pVariation = (m_pStreamVariation != nullptr) ? m_pStreamVariation->GetData<WInt32>() : nullptr;

  const bool bIsOpaque = m_RenderCategory == WDefaultRenderDataCategories::LitOpaque ||
                         m_RenderCategory == WDefaultRenderDataCategories::LitMasked ||
                         m_RenderCategory == WDefaultRenderDataCategories::SimpleOpaque;

  const WUInt8 uiNumSubMeshes = m_uiNumSubMeshes;

  {
    const bool bDynamic = true;
    WGALDynamicBufferHandle hInstanceDataBuffer;
    const WUInt32 uiMaxNumParticles = (WUInt32)GetOwnerSystem()->GetMaxParticles();
    auto instanceData = ref_msg.m_pRenderDataManager->GetOrCreateInstanceData(nullptr, bDynamic, hInstanceDataBuffer, m_InstanceDataOffset, uiMaxNumParticles);

    // Opaque particles with a single submesh can be batched into one render data
    if (bIsOpaque && uiNumSubMeshes == 1)
    {
      for (WUInt32 p = 0; p < numParticles; ++p)
      {
        const WUInt32 idx = p;

        WTransform trans;
        trans.m_qRotation = WQuat::MakeFromAxisAndAngle(pAxis[p], WAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[idx]) + pRotationOffset[idx]));
        trans.m_vPosition = pPosition[idx].GetAsVec3();
        trans.m_vScale.Set(pSize[idx] * m_fScale);

        WRenderDataManager::FillPerInstanceData(instanceData[p], nullptr, trans, WInvalidIndex, pColor[idx].ToLinearFloat() * tintColor);
      }

      WMeshRenderData* pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(nullptr);
      pRenderData->m_vGlobalPosition = GetOwnerSystem()->GetTransform().m_vPosition;
      pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, m_hMaterial, m_hMesh, 0, 0, numParticles);

      ref_msg.AddRenderData(pRenderData, m_RenderCategory, WRenderData::Caching::Never);
    }
    else
    {
      W_ASSERT_DEBUG(pVariation != nullptr, "Variation stream should be set up");

      // Non-opaque particles or multiple submeshes require per-particle render data
      for (WUInt32 p = 0; p < numParticles; ++p)
      {
        const WUInt32 idx = p;

        WTransform trans;
        trans.m_qRotation = WQuat::MakeFromAxisAndAngle(pAxis[p], WAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[idx]) + pRotationOffset[idx]));
        trans.m_vPosition = pPosition[idx].GetAsVec3();
        trans.m_vScale.Set(pSize[idx] * m_fScale);

        WRenderDataManager::FillPerInstanceData(instanceData[p], nullptr, trans, WInvalidIndex, pColor[idx].ToLinearFloat() * tintColor);

        // Determine submesh index from variation
        const WUInt32 uiSubMeshIdx = static_cast<WUInt32>(WMath::Abs(pVariation[idx])) % uiNumSubMeshes;

        // Determine material for this submesh
        WMaterialResourceHandle hMaterial = m_hMaterial;
        if (!m_bMaterialOverride && uiSubMeshIdx < m_CachedSubMeshMaterials.GetCount())
        {
          hMaterial = m_CachedSubMeshMaterials[uiSubMeshIdx];
        }

        WInstanceDataOffset perParticleOffset = m_InstanceDataOffset;
        perParticleOffset.m_uiOffset += p;

        WMeshRenderData* pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(nullptr);
        pRenderData->m_vGlobalPosition = trans.m_vPosition;
        pRenderData->Fill(perParticleOffset, hInstanceDataBuffer, hMaterial, m_hMesh, 0, uiSubMeshIdx);

        ref_msg.AddRenderData(pRenderData, m_RenderCategory, WRenderData::Caching::Never);
      }
    }
  }
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Mesh_ParticleTypeMesh);
