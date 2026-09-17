#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererFoundation/Device/Device.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgSetMeshMaterial);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgSetMeshMaterial, 1, WRTTIDefaultAllocator<WMsgSetMeshMaterial>)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material")),
    W_MEMBER_PROPERTY("MaterialSlot", m_uiMaterialSlot),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WMsgSetMeshMaterial::Serialize(WStreamWriter& inout_stream) const
{
  // has to be stringified for transfer
  inout_stream << GetMaterialFile();
  inout_stream << m_uiMaterialSlot;
}

void WMsgSetMeshMaterial::Deserialize(WStreamReader& inout_stream, WUInt8 uiTypeVersion)
{
  WStringBuilder file;
  inout_stream >> file;
  SetMaterialFile(file);

  inout_stream >> m_uiMaterialSlot;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshRenderData, 1, WRTTIDefaultAllocator<WMeshRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
static_assert(sizeof(WMeshRenderData) == 120);
#else
static_assert(sizeof(WMeshRenderData) == 88);
#endif

void WMeshRenderData::FillSortingKey()
{
  const WUInt32 uiMeshIDHash = WHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
  const WUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? WHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;
  const WUInt32 uiFlipWinding = m_Flags.IsSet(Flags::FlipWinding) ? 1 : 0;

  // Sort by material and then by mesh
  m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + m_uiSubMeshIndex) & 0xFFFE) | uiFlipWinding;
}

bool WMeshRenderData::CanBatch(const WRenderData& other0) const
{
  const auto& other = WStaticCast<const WMeshRenderData&>(other0);

  return m_hCustomInstanceDataBuffer == other.m_hCustomInstanceDataBuffer &&
         m_hMesh == other.m_hMesh && m_uiSubMeshIndex == other.m_uiSubMeshIndex &&
         m_hMaterial == other.m_hMaterial &&
         CanBatchByBaseValues(other);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WMeshComponentBase, 4)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    W_MESSAGE_HANDLER(WMsgSetColor, OnMsgSetColor),
    W_MESSAGE_HANDLER(WMsgSetCustomData, OnMsgSetCustomData),
  } W_END_MESSAGEHANDLERS;
}
W_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

WMeshComponentBase::WMeshComponentBase() = default;
WMeshComponentBase::~WMeshComponentBase() = default;

void WMeshComponentBase::OnDeactivated()
{
  DeleteInstanceData();

  SUPER::OnDeactivated();
}

void WMeshComponentBase::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  // ignore components that have created meshes (?)

  s << m_hMesh;

  s << m_Materials.GetCount();
  for (const auto& mat : m_Materials)
  {
    s << mat;
  }

  s << m_Color;
  s << m_fSortingDepthOffset;
  s << m_vCustomData;
}

void WMeshComponentBase::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  s >> m_hMesh;

  if (uiVersion < 2)
  {
    WUInt32 uiCategory = 0;
    s >> uiCategory;
  }

  WUInt32 uiMaterials = 0;
  s >> uiMaterials;

  m_Materials.SetCount(uiMaterials);

  for (auto& mat : m_Materials)
  {
    s >> mat;
  }

  s >> m_Color;

  if (uiVersion >= 3)
  {
    s >> m_fSortingDepthOffset;
  }

  if (uiVersion >= 4)
  {
    s >> m_vCustomData;
  }
}

WResult WMeshComponentBase::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  if (m_hMesh.IsValid())
  {
    WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::AllowLoadingFallback);
    ref_bounds = pMesh->GetBounds();
    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WMeshComponentBase::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  const WTransform finalTransform = GetFinalGlobalTransform();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, finalTransform, m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color, m_vCustomData);

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
    {
      // Already done in CreateRenderDataForThisFrame but only with the owner's transform. We need to use the final transform here.
      pRenderData->m_vGlobalPosition = finalTransform.m_vPosition;
      pRenderData->m_Flags.AddOrRemove(WRenderData::Flags::FlipWinding, finalTransform.HasMirrorScaling());

      pRenderData->m_fSortingDepthOffset = m_fSortingDepthOffset;
      pRenderData->m_DataOffsets.m_uiCustomInstance = m_CustomInstanceDataOffset.m_uiOffset;
      pRenderData->m_hCustomInstanceDataBuffer = m_hCustomInstanceDataBuffer;

      pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
      pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, m_hMesh, uiMaterialIndex, uiPartIndex);
    }

    bool bDontCacheYet = false;
    WRenderData::Category category = WMaterialResource::GetRenderDataCategory(hMaterial, &bDontCacheYet);

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic);
  }
}

void WMeshComponentBase::DeleteInstanceData()
{
  if (WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>())
  {
    pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);
  }
  else
  {
    W_ASSERT_DEBUG(m_InstanceDataOffset.IsInvalidated(), "Implementation error");
  }
}

void WMeshComponentBase::SetMesh(const WMeshResourceHandle& hMesh)
{
  if (m_hMesh != hMesh)
  {
    m_hMesh = hMesh;

    TriggerLocalBoundsUpdate();
    InvalidateCachedRenderData();
  }
}

void WMeshComponentBase::SetMaterial(WUInt32 uiIndex, const WMaterialResourceHandle& hMaterial)
{
  if (uiIndex >= 1024)
  {
    WLog::Error("Invalid material slot index used to change mesh component material.");
    return;
  }

  m_Materials.EnsureCount(uiIndex + 1);

  if (m_Materials[uiIndex] != hMaterial)
  {
    m_Materials[uiIndex] = hMaterial;

    InvalidateCachedRenderData();
  }
}

WMaterialResourceHandle WMeshComponentBase::GetMaterial(WUInt32 uiIndex) const
{
  if (uiIndex >= m_Materials.GetCount())
    return WMaterialResourceHandle();

  return m_Materials[uiIndex];
}

void WMeshComponentBase::SetColor(const WColor& color)
{
  if (m_Color != color)
  {
    m_Color = color;

    InvalidateCachedRenderData();
  }
}

void WMeshComponentBase::SetCustomData(const WVec4& vData)
{
  // Use a bitwise comparison since some systems store arbitrary data casted to floats which can look like NaNs.
  // These would compare false or trigger the NaN check in WMath when comparing the vectors.
  if (!WMemoryUtils::IsEqual(&m_vCustomData, &vData))
  {
    m_vCustomData = vData;

    InvalidateCachedRenderData();
  }
}

void WMeshComponentBase::SetSortingDepthOffset(float fOffset)
{
  if (m_fSortingDepthOffset != fOffset)
  {
    m_fSortingDepthOffset = fOffset;

    InvalidateCachedRenderData();
  }
}

void WMeshComponentBase::OnMsgSetMeshMaterial(WMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_uiMaterialSlot, ref_msg.m_hMaterial);
}

void WMeshComponentBase::OnMsgSetColor(WMsgSetColor& ref_msg)
{
  WColor newColor = m_Color;
  ref_msg.ModifyColor(newColor);

  if (m_Color != newColor)
  {
    m_Color = newColor;

    InvalidateCachedRenderData();
  }
}

void WMeshComponentBase::OnMsgSetCustomData(WMsgSetCustomData& ref_msg)
{
  // Use a bitwise comparison since some systems store arbitrary data casted to floats which can look like NaNs.
  // These would compare false or trigger the NaN check in WMath when comparing the vectors.
  if (!WMemoryUtils::IsEqual(&m_vCustomData, &ref_msg.m_vData))
  {
    m_vCustomData = ref_msg.m_vData;

    InvalidateCachedRenderData();
  }
}

void WMeshComponentBase::SetCustomInstanceData(WCustomInstanceDataOffset offset, WGALDynamicBufferHandle hBuffer)
{
  if (m_CustomInstanceDataOffset.m_uiOffset != offset.m_uiOffset || m_hCustomInstanceDataBuffer != hBuffer)
  {
    m_CustomInstanceDataOffset = offset;
    m_hCustomInstanceDataBuffer = hBuffer;

    InvalidateCachedRenderData();
  }
}

WTransform WMeshComponentBase::GetFinalGlobalTransform() const
{
  return GetOwner()->GetGlobalTransform();
}

WMeshRenderData* WMeshComponentBase::CreateRenderData(const WRenderDataManager* pRenderDataManager) const
{
  return pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(GetOwner());
}

WUInt32 WMeshComponentBase::Materials_GetCount() const
{
  return m_Materials.GetCount();
}

WString WMeshComponentBase::Materials_GetValue(WUInt32 uiIndex) const
{
  return GetMaterial(uiIndex).GetResourceID();
}

void WMeshComponentBase::Materials_SetValue(WUInt32 uiIndex, WString sValue)
{
  if (sValue.IsEmpty())
    SetMaterial(uiIndex, WMaterialResourceHandle());
  else
  {
    auto hMat = WResourceManager::LoadResource<WMaterialResource>(sValue);
    SetMaterial(uiIndex, hMat);
  }
}

void WMeshComponentBase::Materials_Insert(WUInt32 uiIndex, WString sValue)
{
  WMaterialResourceHandle hMat;

  if (!sValue.IsEmpty())
    hMat = WResourceManager::LoadResource<WMaterialResource>(sValue);

  m_Materials.InsertAt(uiIndex, hMat);

  InvalidateCachedRenderData();
}

void WMeshComponentBase::Materials_Remove(WUInt32 uiIndex)
{
  m_Materials.RemoveAtAndCopy(uiIndex);

  InvalidateCachedRenderData();
}


W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponentBase);
