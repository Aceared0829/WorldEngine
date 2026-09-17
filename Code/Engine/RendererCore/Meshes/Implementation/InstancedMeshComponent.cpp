#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/InstancedMeshComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WMeshInstanceData, WNoBase, 1, WRTTIDefaultAllocator<WMeshInstanceData>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    W_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new WDefaultValueAttribute(WVec3(1.0f, 1.0f, 1.0f))),

    W_MEMBER_PROPERTY("Color", m_color)
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WTransformManipulatorAttribute("LocalPosition", "LocalRotation", "LocalScaling"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE
// clang-format on

void WMeshInstanceData::SetLocalPosition(WVec3 vPosition)
{
  m_transform.m_vPosition = vPosition;
}
WVec3 WMeshInstanceData::GetLocalPosition() const
{
  return m_transform.m_vPosition;
}

void WMeshInstanceData::SetLocalRotation(WQuat qRotation)
{
  m_transform.m_qRotation = qRotation;
}

WQuat WMeshInstanceData::GetLocalRotation() const
{
  return m_transform.m_qRotation;
}

void WMeshInstanceData::SetLocalScaling(WVec3 vScaling)
{
  m_transform.m_vScale = vScaling;
}

WVec3 WMeshInstanceData::GetLocalScaling() const
{
  return m_transform.m_vScale;
}

static constexpr WTypeVersion s_MeshInstanceDataVersion = 1;
WResult WMeshInstanceData::Serialize(WStreamWriter& ref_writer) const
{
  ref_writer.WriteVersion(s_MeshInstanceDataVersion);

  ref_writer << m_transform;
  ref_writer << m_color;

  return W_SUCCESS;
}

WResult WMeshInstanceData::Deserialize(WStreamReader& ref_reader)
{
  /*auto version = */ ref_reader.ReadVersion(s_MeshInstanceDataVersion);

  ref_reader >> m_transform;
  ref_reader >> m_color;

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WInstancedMeshComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Static"), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("MainColor", GetColor, SetColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new WDefaultValueAttribute(WVec4(0, 1, 0, 1))),
    W_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material")),

    W_ARRAY_ACCESSOR_PROPERTY("InstanceData", Instances_GetCount, Instances_GetValue, Instances_SetValue, Instances_Insert, Instances_Remove),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractGeometry, OnMsgExtractGeometry),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

WInstancedMeshComponent::WInstancedMeshComponent() = default;
WInstancedMeshComponent::~WInstancedMeshComponent() = default;

void WInstancedMeshComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  inout_stream.GetStream().WriteArray(m_RawInstancedData).IgnoreResult();
}

void WInstancedMeshComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  inout_stream.GetStream().ReadArray(m_RawInstancedData).IgnoreResult();
}

void WInstancedMeshComponent::OnMsgExtractGeometry(WMsgExtractGeometry& ref_msg) {}

WResult WInstancedMeshComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  if (!m_hMesh.IsValid() || m_RawInstancedData.IsEmpty())
    return W_FAILURE;

  WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::AllowLoadingFallback);
  WBoundingBoxSphere singleBounds = pMesh->GetBounds();
  m_fBoundingSphereRadius = singleBounds.m_fSphereRadius;

  for (const auto& instance : m_RawInstancedData)
  {
    auto instanceBounds = singleBounds;
    instanceBounds.Transform(instance.m_transform.GetAsMat4());

    ref_bounds.ExpandToInclude(instanceBounds);
  }

  return W_SUCCESS;
}

void WInstancedMeshComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid() || m_RawInstancedData.IsEmpty())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  WGALDynamicBufferHandle hInstanceDataBuffer;
  auto instanceData = msg.m_pRenderDataManager->GetOrCreateInstanceData(this, bDynamic, hInstanceDataBuffer, m_InstanceDataOffset, m_RawInstancedData.GetCount());

  const WTransform ownerTransform = GetOwner()->GetGlobalTransform();
  const WUInt32 uiUniqueID = GetUniqueIdForRendering();
  const WUInt32 uiRandomSeed = GetOwner()->GetStableRandomSeed();
  for (WUInt32 i = 0; i < m_RawInstancedData.GetCount(); ++i)
  {
    auto& meshInstance = m_RawInstancedData[i];
    const WTransform globalTransform = ownerTransform * meshInstance.m_transform;
    const WColor color = m_Color * meshInstance.m_color;

    WRenderDataManager::FillPerInstanceData(instanceData[i], nullptr, globalTransform, uiUniqueID, color, m_vCustomData, m_fBoundingSphereRadius, uiRandomSeed + i);
  }

  WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::AllowLoadingFallback);
  WArrayPtr<const WMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (WUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const WUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    WMaterialResourceHandle hMaterial;

    // If we have a material override, use that otherwise use the default mesh material.
    if (GetMaterial(uiMaterialIndex).IsValid())
      hMaterial = m_Materials[uiMaterialIndex];
    else
      hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    WMeshRenderData* pRenderData = CreateRenderData(msg.m_pRenderDataManager);
    pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
    pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, m_hMesh, uiMaterialIndex, uiPartIndex, m_RawInstancedData.GetCount());

    bool bDontCacheYet = false;
    WRenderData::Category category = WMaterialResource::GetRenderDataCategory(hMaterial, &bDontCacheYet);

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic);
  }
}

WUInt32 WInstancedMeshComponent::Instances_GetCount() const
{
  return m_RawInstancedData.GetCount();
}

WMeshInstanceData WInstancedMeshComponent::Instances_GetValue(WUInt32 uiIndex) const
{
  return m_RawInstancedData[uiIndex];
}

void WInstancedMeshComponent::Instances_SetValue(WUInt32 uiIndex, WMeshInstanceData value)
{
  m_RawInstancedData[uiIndex] = value;

  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void WInstancedMeshComponent::Instances_Insert(WUInt32 uiIndex, WMeshInstanceData value)
{
  m_RawInstancedData.InsertAt(uiIndex, value);

  TriggerLocalBoundsUpdate();
  DeleteInstanceData();
  InvalidateCachedRenderData();
}

void WInstancedMeshComponent::Instances_Remove(WUInt32 uiIndex)
{
  m_RawInstancedData.RemoveAtAndCopy(uiIndex);

  TriggerLocalBoundsUpdate();
  DeleteInstanceData();
  InvalidateCachedRenderData();
}


W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_InstancedMeshComponent);
