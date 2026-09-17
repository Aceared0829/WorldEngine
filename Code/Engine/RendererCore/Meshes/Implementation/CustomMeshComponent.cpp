#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

/////////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCustomMeshRenderData, 1, WRTTIDefaultAllocator<WCustomMeshRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WCustomMeshRenderData::FillSortingKey()
{
  const WUInt32 uiMeshIDHash = WHashingUtils::StringHashTo32(m_hDynamicMeshBuffer.GetResourceIDHash());
  const WUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? WHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;
  const WUInt32 uiFlipWinding = m_Flags.IsSet(Flags::FlipWinding) ? 1 : 0;

  // Sort by material and then by mesh
  m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash) & 0xFFFE) | uiFlipWinding;
}

bool WCustomMeshRenderData::CanBatch(const WRenderData& other0) const
{
  const auto& other = WStaticCast<const WCustomMeshRenderData&>(other0);

  return m_hDynamicMeshBuffer == other.m_hDynamicMeshBuffer &&
         m_hMaterial == other.m_hMaterial &&
         CanBatchByBaseValues(other);
}

/////////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WCustomMeshComponent, 4, WComponentMode::Static)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new WDefaultValueAttribute(WVec4(0, 1, 0, 1))),
    W_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material"), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    W_MESSAGE_HANDLER(WMsgSetColor, OnMsgSetColor),
    W_MESSAGE_HANDLER(WMsgSetCustomData, OnMsgSetCustomData),
  } W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

WAtomicInteger32 s_iCustomMeshResources;

WCustomMeshComponent::WCustomMeshComponent() = default;
WCustomMeshComponent::~WCustomMeshComponent() = default;

void WCustomMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  if (false)
  {
    WGeometry geo;
    geo.AddTorus(1.0f, 1.5f, 32, 16, false);
    geo.TriangulatePolygons();
    geo.ComputeTangents();

    auto hMesh = CreateMeshResource(WGALPrimitiveTopology::Triangles, geo.GetVertices().GetCount(), geo.GetPolygons().GetCount(), WGALIndexType::UInt);

    WResourceLock<WDynamicMeshBufferResource> pMesh(hMesh, WResourceAcquireMode::BlockTillLoaded);

    auto positions = pMesh->AccessPositionData();
    auto ntts = pMesh->AccessNormalTangentTexCoord0Data();
    auto cols = pMesh->AccessColorData();

    for (WUInt32 v = 0; v < positions.GetCount(); ++v)
    {
      positions[v] = geo.GetVertices()[v].m_vPosition;

      ntts[v].EncodeNormal(geo.GetVertices()[v].m_vNormal);
      ntts[v].EncodeTangent(geo.GetVertices()[v].m_vTangent, 1.0f);
      ntts[v].m_vTexCoord.SetZero();

      cols[v] = WColor::CornflowerBlue;
    }

    auto ind = pMesh->AccessIndex32Data();

    for (WUInt32 i = 0; i < geo.GetPolygons().GetCount(); ++i)
    {
      ind[i * 3 + 0] = geo.GetPolygons()[i].m_Vertices[0];
      ind[i * 3 + 1] = geo.GetPolygons()[i].m_Vertices[1];
      ind[i * 3 + 2] = geo.GetPolygons()[i].m_Vertices[2];
    }

    SetBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), 1.5f));
  }
}

void WCustomMeshComponent::OnDeactivated()
{
  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();
}

void WCustomMeshComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_Color;
  s << m_hMaterial;

  s << m_vCustomData;
  s << m_fSortingDepthOffset;
}

void WCustomMeshComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_hMaterial;

  if (uiVersion < 2)
  {
    WUInt32 uiCategory = 0;
    s >> uiCategory;
  }

  if (uiVersion >= 3)
  {
    s >> m_vCustomData;
  }

  if (uiVersion >= 4)
  {
    s >> m_fSortingDepthOffset;
  }
}

WResult WCustomMeshComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  if (m_Bounds.IsValid())
  {
    ref_bounds = m_Bounds;
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WDynamicMeshBufferResourceHandle WCustomMeshComponent::CreateMeshResource(WGALPrimitiveTopology::Enum topology, WUInt32 uiMaxVertices, WUInt32 uiMaxPrimitives, WGALIndexType::Enum indexType)
{
  WDynamicMeshBufferResourceDescriptor desc;
  desc.m_Topology = topology;
  desc.m_uiMaxVertices = uiMaxVertices;
  desc.m_uiMaxPrimitives = uiMaxPrimitives;
  desc.m_IndexType = indexType;
  desc.m_bColorStream = true;

  WStringBuilder sGuid;
  sGuid.SetFormat("CustomMesh_{}", s_iCustomMeshResources.Increment());

  m_hDynamicMesh = WResourceManager::CreateResource<WDynamicMeshBufferResource>(sGuid, std::move(desc));

  InvalidateCachedRenderData();

  return m_hDynamicMesh;
}

void WCustomMeshComponent::SetMeshResource(const WDynamicMeshBufferResourceHandle& hMesh)
{
  m_hDynamicMesh = hMesh;
  InvalidateCachedRenderData();
}

void WCustomMeshComponent::SetBounds(const WBoundingBoxSphere& bounds)
{
  m_Bounds = bounds;
  TriggerLocalBoundsUpdate();
}

void WCustomMeshComponent::SetMaterial(const WMaterialResourceHandle& hMaterial)
{
  m_hMaterial = hMaterial;
  InvalidateCachedRenderData();
}

WMaterialResourceHandle WCustomMeshComponent::GetMaterial() const
{
  return m_hMaterial;
}

void WCustomMeshComponent::SetColor(const WColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const WColor& WCustomMeshComponent::GetColor() const
{
  return m_Color;
}

void WCustomMeshComponent::SetCustomData(const WVec4& vData)
{
  m_vCustomData = vData;

  InvalidateCachedRenderData();
}

const WVec4& WCustomMeshComponent::GetCustomData() const
{
  return m_vCustomData;
}

void WCustomMeshComponent::SetSortingDepthOffset(float fOffset)
{
  if (m_fSortingDepthOffset != fOffset)
  {
    m_fSortingDepthOffset = fOffset;

    InvalidateCachedRenderData();
  }
}

void WCustomMeshComponent::OnMsgSetMeshMaterial(WMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_hMaterial);
}

void WCustomMeshComponent::OnMsgSetColor(WMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

void WCustomMeshComponent::OnMsgSetCustomData(WMsgSetCustomData& ref_msg)
{
  m_vCustomData = ref_msg.m_vData;

  InvalidateCachedRenderData();
}

void WCustomMeshComponent::SetUsePrimitiveRange(WUInt32 uiFirstPrimitive /*= 0*/, WUInt32 uiNumPrimitives /*= WMath::MaxValue<WUInt32>()*/)
{
  m_uiFirstPrimitive = uiFirstPrimitive;
  m_uiNumPrimitives = uiNumPrimitives;

  InvalidateCachedRenderData();
}

void WCustomMeshComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hDynamicMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color, m_vCustomData);

  WResourceLock<WDynamicMeshBufferResource> pMesh(m_hDynamicMesh, WResourceAcquireMode::BlockTillLoaded);

  WCustomMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WCustomMeshRenderData>(GetOwner());
  {
    pRenderData->m_uiNumInstances = 1;
    pRenderData->m_DataOffsets.m_uiInstance = m_InstanceDataOffset.m_uiOffset;
    pRenderData->m_hInstanceDataBuffer = hInstanceDataBuffer;
    pRenderData->m_fSortingDepthOffset = m_fSortingDepthOffset;

    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_hDynamicMeshBuffer = m_hDynamicMesh;
    pRenderData->m_uiFirstPrimitive = WMath::Min(m_uiFirstPrimitive, pMesh->GetDescriptor().m_uiMaxPrimitives);
    pRenderData->m_uiNumPrimitives = WMath::Min(m_uiNumPrimitives, pMesh->GetDescriptor().m_uiMaxPrimitives - pRenderData->m_uiFirstPrimitive);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    pRenderData->m_FallbackGlobalBBox = GetOwner()->GetGlobalBounds().GetBox();
#endif

    pRenderData->FillSortingKey();
  }

  WResourceLock<WMaterialResource> pMaterial(m_hMaterial, WResourceAcquireMode::AllowLoadingFallback);
  WRenderData::Category category = pMaterial->GetRenderDataCategory();
  bool bDontCacheYet = pMaterial.GetAcquireResult() == WResourceAcquireResult::LoadingFallback;

  msg.AddRenderData(pRenderData, category, bDontCacheYet ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic);
}

W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_CustomMeshComponent);
