#include <TerrainPlugin/TerrainPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/TagRegistry.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <TerrainPlugin/Components/TerrainVolumeComponent.h>
#include <TerrainPlugin/Rendering/TerrainRenderData.h>
#include <TerrainPlugin/TerrainSystem.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WTerrainVolumeComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("Resolution", WTerrainResolution, GetResolution, SetResolution),
    W_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new WClampValueAttribute(1.0f, 1024.0f), new WDefaultValueAttribute(64.0f)),
    W_RESOURCE_ACCESSOR_PROPERTY("Material", GetMaterial, SetMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material", "Terrain-Voxel"), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("BaseMaterialIndex", GetBaseMaterialIndex, SetBaseMaterialIndex)->AddAttributes(new WClampValueAttribute(0, 15)),
    W_ACCESSOR_PROPERTY("FillHeight", GetFillHeight, SetFillHeight)->AddAttributes(new WDefaultValueAttribute(-0.01f), new WClampValueAttribute(-0.01f, 100.0f), new WMinValueTextAttribute("Off")),
    W_ACCESSOR_PROPERTY("EnableCollider", GetEnableCollider, SetEnableCollider)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("CleanupIterations", GetCleanupIterations, SetCleanupIterations)->AddAttributes(new WDefaultValueAttribute(2), new WClampValueAttribute(0, 4)),
    W_SET_ACCESSOR_PROPERTY("TerrainTags", GetTags, Reflection_SetTag, Reflection_RemoveTag)->AddAttributes(new WTagSetWidgetAttribute("Terrain")),
    W_ARRAY_ACCESSOR_PROPERTY("Surfaces", Surfaces_GetCount, Surfaces_GetValue, Surfaces_SetValue, Surfaces_Insert, Surfaces_Remove)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgTransformChanged, OnMsgTransformChanged),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgExtractGeometry, OnMsgExtractGeometry),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_FUNCTION_PROPERTY(OnObjectCreated),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Terrain"),
    new WBoxVisualizerAttribute("Size", 1.0f, WColorScheme::LightUI(WColorScheme::Green), nullptr, WVisualizerAnchor::NegX | WVisualizerAnchor::NegY | WVisualizerAnchor::NegZ),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WTerrainVolumeComponent::WTerrainVolumeComponent() = default;
WTerrainVolumeComponent::~WTerrainVolumeComponent() = default;

void WTerrainVolumeComponent::SetResolution(WEnum<WTerrainResolution> resolution)
{
  if (m_Resolution == resolution)
    return;

  m_Resolution = resolution;

  if (m_uiVoxelIndex != WInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<WTerrainSystem>())
    {
      pSystem->RemoveVoxelTerrain(m_uiVoxelIndex);
      const float fVoxelSize = m_fSize / static_cast<float>(m_Resolution.GetValue());
      m_uiVoxelIndex = pSystem->CreateVoxelTerrain(m_Resolution.GetValue(), fVoxelSize);
      auto& vol = pSystem->ModifyVoxelTerrain(m_uiVoxelIndex);
      vol.m_GlobalTransform = GetOwner()->GetGlobalTransform();
      vol.m_bInitialSolid = (m_fFillHeight >= 0.0f);
      vol.m_fFillHeight = m_fFillHeight;
      vol.m_uiCleanupIterations = m_uiCleanupIterations;
      vol.m_Tags = m_Tags;
    }
  }

  TriggerLocalBoundsUpdate();
}

void WTerrainVolumeComponent::SetSize(float f)
{
  if (m_fSize == f)
    return;

  m_fSize = f;

  if (m_uiVoxelIndex != WInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<WTerrainSystem>())
    {
      pSystem->ModifyVoxelTerrain(m_uiVoxelIndex).m_fVoxelSize = m_fSize / static_cast<float>(m_Resolution.GetValue());
    }
  }

  TriggerLocalBoundsUpdate();
}

void WTerrainVolumeComponent::SetMaterial(const WMaterialResourceHandle& hMaterial)
{
  m_hMaterial = hMaterial;
  InvalidateCachedRenderData();
}

void WTerrainVolumeComponent::SetBaseMaterialIndex(WUInt8 i)
{
  m_uiBaseMaterialIndex = i;
  InvalidateCachedRenderData();
}

void WTerrainVolumeComponent::SetFillHeight(float f)
{
  if (m_fFillHeight == f)
    return;

  m_fFillHeight = f;

  if (m_uiVoxelIndex != WInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<WTerrainSystem>())
    {
      auto& vol = pSystem->ModifyVoxelTerrain(m_uiVoxelIndex);
      vol.m_fFillHeight = f;
      vol.m_bInitialSolid = (f >= 0.0f);
    }
  }
}

void WTerrainVolumeComponent::SetEnableCollider(bool bEnable)
{
  m_bEnableCollider = bEnable;
}

void WTerrainVolumeComponent::SetCleanupIterations(WUInt8 n)
{
  n = WMath::Clamp(n, (WUInt8)0, (WUInt8)4);

  if (m_uiCleanupIterations == n)
    return;

  m_uiCleanupIterations = n;

  if (m_uiVoxelIndex != WInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<WTerrainSystem>())
      pSystem->ModifyVoxelTerrain(m_uiVoxelIndex).m_uiCleanupIterations = m_uiCleanupIterations;
  }
}

void WTerrainVolumeComponent::Reflection_SetTag(const char* szTagName)
{
  if (WStringUtils::IsNullOrEmpty(szTagName))
    return;

  const WTag& tag = WTagRegistry::GetGlobalRegistry().RegisterTag(szTagName);
  if (m_Tags.IsSet(tag))
    return;

  m_Tags.Set(tag);

  if (m_uiVoxelIndex != WInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<WTerrainSystem>())
      pSystem->ModifyVoxelTerrain(m_uiVoxelIndex).m_Tags = m_Tags;
  }
}

void WTerrainVolumeComponent::Reflection_RemoveTag(const char* szTagName)
{
  if (WStringUtils::IsNullOrEmpty(szTagName))
    return;

  if (const WTag* pTag = WTagRegistry::GetGlobalRegistry().GetTagByName(WTempHashedString(szTagName)))
  {
    if (!m_Tags.IsSet(*pTag))
      return;

    m_Tags.Remove(*pTag);

    if (m_uiVoxelIndex != WInvalidIndex)
    {
      if (auto* pSystem = GetWorld()->GetModule<WTerrainSystem>())
        pSystem->ModifyVoxelTerrain(m_uiVoxelIndex).m_Tags = m_Tags;
    }
  }
}

WUInt32 WTerrainVolumeComponent::Surfaces_GetCount() const
{
  return m_Surfaces.GetCount();
}

WString WTerrainVolumeComponent::Surfaces_GetValue(WUInt32 uiIndex) const
{
  if (uiIndex >= m_Surfaces.GetCount() || !m_Surfaces[uiIndex].IsValid())
    return {};

  return m_Surfaces[uiIndex].GetResourceID();
}

void WTerrainVolumeComponent::Surfaces_SetValue(WUInt32 uiIndex, WString sValue)
{
  m_Surfaces.EnsureCount(uiIndex + 1);

  if (!sValue.IsEmpty())
    m_Surfaces[uiIndex] = WResourceManager::LoadResource<WSurfaceResource>(sValue);
  else
    m_Surfaces[uiIndex].Invalidate();
}

void WTerrainVolumeComponent::Surfaces_Insert(WUInt32 uiIndex, WString sValue)
{
  WSurfaceResourceHandle hSurface;

  if (!sValue.IsEmpty())
    hSurface = WResourceManager::LoadResource<WSurfaceResource>(sValue);

  m_Surfaces.InsertAt(uiIndex, hSurface);
}

void WTerrainVolumeComponent::Surfaces_Remove(WUInt32 uiIndex)
{
  m_Surfaces.RemoveAtAndCopy(uiIndex);
}

void WTerrainVolumeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_hMaterial;
  s << m_Resolution.GetValue();
  s << m_fSize;
  s << m_uiBaseMaterialIndex;
  s << m_fFillHeight;
  s << m_bEnableCollider;
  s << m_uiCleanupIterations;
  m_Tags.Save(s);
  s << m_uiStableId;

  const WUInt32 uiNumSurfaces = m_Surfaces.GetCount();
  s << uiNumSurfaces;
  for (const auto& hSurface : m_Surfaces)
    s << hSurface;
}

void WTerrainVolumeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();
  s >> m_hMaterial;
  WUInt16 uiRes = 0;
  s >> uiRes;
  m_Resolution = static_cast<WTerrainResolution::Enum>(uiRes);
  s >> m_fSize;
  s >> m_uiBaseMaterialIndex;
  s >> m_fFillHeight;
  s >> m_bEnableCollider;
  s >> m_uiCleanupIterations;
  m_Tags.Load(s, WTagRegistry::GetGlobalRegistry());
  s >> m_uiStableId;

  WUInt32 uiNumSurfaces = 0;
  s >> uiNumSurfaces;
  m_Surfaces.SetCount(uiNumSurfaces);
  for (auto& hSurface : m_Surfaces)
    s >> hSurface;
}

void WTerrainVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();

  auto* pSystem = GetWorld()->GetOrCreateModule<WTerrainSystem>();
  const float fVoxelSize = m_fSize / static_cast<float>(m_Resolution.GetValue());
  m_uiVoxelIndex = pSystem->CreateVoxelTerrain(m_Resolution.GetValue(), fVoxelSize);
  auto& vol = pSystem->ModifyVoxelTerrain(m_uiVoxelIndex);
  vol.m_GlobalTransform = GetOwner()->GetGlobalTransform();
  vol.m_bInitialSolid = (m_fFillHeight >= 0.0f);
  vol.m_fFillHeight = m_fFillHeight;
  vol.m_uiCleanupIterations = m_uiCleanupIterations;
  vol.m_Tags = m_Tags;

  TriggerLocalBoundsUpdate();
}

void WTerrainVolumeComponent::OnDeactivated()
{
  if (auto* pSystem = GetWorld()->GetModule<WTerrainSystem>())
  {
    pSystem->RemoveVoxelTerrain(m_uiVoxelIndex);
  }

  if (auto* pRDM = GetWorld()->GetModule<WRenderDataManager>())
  {
    pRDM->DeleteInstanceData(m_InstanceDataOffset);
  }

  m_uiVoxelIndex = WInvalidIndex;

  SUPER::OnDeactivated();
}

void WTerrainVolumeComponent::OnObjectCreated(const WAbstractObjectNode& node)
{
  m_uiStableId = WHashingUtils::xxHash64(&node.GetGuid(), sizeof(WUuid));
}

WResult WTerrainVolumeComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromMinMax(WVec3::MakeZero(), WVec3(m_fSize)));
  return W_SUCCESS;
}

void WTerrainVolumeComponent::OnMsgTransformChanged(WMsgTransformChanged& msg)
{
  if (m_uiVoxelIndex != WInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<WTerrainSystem>())
    {
      auto& vol = pSystem->ModifyVoxelTerrain(m_uiVoxelIndex);
      vol.m_GlobalTransform = msg.m_NewGlobalTransform;
    }
  }
}

void WTerrainVolumeComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hMaterial.IsValid() || m_uiVoxelIndex == WInvalidIndex)
    return;

  const auto* pSystem = GetWorld()->GetModule<WTerrainSystem>();
  if (pSystem == nullptr)
    return;

  const WGALBufferHandle hVerts = pSystem->GetVoxelVolumeGpuMeshVertexBuffer(m_uiVoxelIndex);
  const WGALBufferHandle hIdxs = pSystem->GetVoxelVolumeGpuMeshIndexBuffer(m_uiVoxelIndex);
  if (hVerts.IsInvalidated() || hIdxs.IsInvalidated())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(
    *this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset,
    GetUniqueIdForRendering(), WColor::White, WVec4(0, 1, 0, 1));

  WTerrainVoxelRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WTerrainVoxelRenderData>(GetOwner());
  pRenderData->m_uiNumInstances = 1;
  pRenderData->m_DataOffsets.m_uiInstance = m_InstanceDataOffset.m_uiOffset;
  pRenderData->m_hInstanceDataBuffer = hInstanceDataBuffer;
  pRenderData->m_hMaterial = m_hMaterial;
  pRenderData->m_hGpuMeshVertices = hVerts;
  pRenderData->m_hGpuMeshIndices = hIdxs;
  pRenderData->m_hGpuMeshDrawArgs = pSystem->GetVoxelVolumeGpuMeshDrawArgsBuffer(m_uiVoxelIndex);
  pRenderData->m_uiBaseMaterialIndex = m_uiBaseMaterialIndex;
  pRenderData->m_uiSortingKey = 0;

  msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::LitOpaque, WRenderData::Caching::IfStatic);

  // The vertex shader reads the voxel mesh buffers as SRVs and emulates indexed drawing, so declare
  // them as render-graph dependencies for the categories that render this volume.
  const WGALBufferHandle hDrawArgs = pSystem->GetVoxelVolumeGpuMeshDrawArgsBuffer(m_uiVoxelIndex);
  if (!hVerts.IsInvalidated())
    msg.AddDependency(hVerts, WDefaultRenderDataCategories::LitOpaque, WGALResourceState::ShaderResource, WGALShaderStageFlags::VertexShader);
  if (!hIdxs.IsInvalidated())
    msg.AddDependency(hIdxs, WDefaultRenderDataCategories::LitOpaque, WGALResourceState::ShaderResource, WGALShaderStageFlags::VertexShader);
  if (!hDrawArgs.IsInvalidated())
    msg.AddDependency(hDrawArgs, WDefaultRenderDataCategories::LitOpaque, WGALResourceState::DrawIndirect);
}

WCpuMeshResourceHandle WTerrainVolumeComponent::GenerateCpuMesh() const
{
  if (m_uiVoxelIndex == WInvalidIndex)
    return WCpuMeshResourceHandle();

  // Geometry extraction only holds a read lock on the world, so the module must not be created here.
  // Reading the mesh back does mutate the terrain system, hence the const_cast.
  const WTerrainSystem* pConstTerrain = GetWorld()->GetModule<WTerrainSystem>();
  if (pConstTerrain == nullptr)
    return WCpuMeshResourceHandle();

  WTerrainSystem* pTerrain = const_cast<WTerrainSystem*>(pConstTerrain);

  // The hash covers everything that changes the shape of the mesh, so as long as it matches, the
  // cached mesh is still the right one. Without this check, edits to the volume would go unnoticed.
  const WUInt64 uiContentHash = ComputeColliderContentHash(pTerrain->GetVoxelBrushOverlapHash(m_uiVoxelIndex));

  if (m_hCpuMesh.IsValid() && m_uiCpuMeshHash == uiContentHash)
    return m_hCpuMesh;

  m_hCpuMesh.Invalidate();
  m_uiCpuMeshHash = uiContentHash;

  WStringBuilder sResourceName;
  sResourceName.SetFormat("TerrainVolumeCpuMesh:{}-{}", WArgU(m_uiStableId, 16, true, 16, true), uiContentHash);

  m_hCpuMesh = WResourceManager::GetExistingResource<WCpuMeshResource>(sResourceName);
  if (m_hCpuMesh.IsValid())
    return m_hCpuMesh;

  // Reading back from the GPU blocks, so this is deliberately only done on demand.
  WTempArray<VoxelGpuVertex> verts;
  WTempArray<WUInt32> indices;
  WUInt32 uiVertexCount = 0;
  WUInt32 uiTriangleCount = 0;

  if (pTerrain->ReadbackVoxelData(m_uiVoxelIndex, verts, indices, uiVertexCount, uiTriangleCount).Failed())
  {
    WLog::Warning("WTerrainVolumeComponent: could not read back the voxel mesh, the volume provides no geometry.");
    return WCpuMeshResourceHandle();
  }

  // An empty volume is the normal case for one that no brush overlaps.
  if (uiVertexCount == 0 || uiTriangleCount == 0)
    return WCpuMeshResourceHandle();

  if (verts.GetCount() < uiVertexCount || indices.GetCount() < uiTriangleCount * 3)
    return WCpuMeshResourceHandle();

  WMeshResourceDescriptor desc;
  desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Data/Base/Materials/Common/Pattern.WMaterialAsset
  desc.MeshBufferDesc().AddCommonStreams();

  auto& mb = desc.MeshBufferDesc();
  mb.AllocateStreams(uiVertexCount, WGALPrimitiveTopology::Triangles, uiTriangleCount);

  // Only positions are filled in; normals and texture coordinates carry no information that a
  // consumer of this mesh (navmesh generation, geometry export) would use.
  auto positionData = mb.GetPositionData();

  WBoundingBox bounds = WBoundingBox::MakeInvalid();

  for (WUInt32 i = 0; i < uiVertexCount; ++i)
  {
    positionData.GetPtr()[i] = verts[i].Position;
    bounds.ExpandToInclude(verts[i].Position);
  }

  for (WUInt32 uiTriangle = 0; uiTriangle < uiTriangleCount; ++uiTriangle)
  {
    mb.SetTriangleIndices(uiTriangle, indices[uiTriangle * 3 + 0], indices[uiTriangle * 3 + 1], indices[uiTriangle * 3 + 2]);
  }

  desc.SetBounds(WBoundingBoxSphere::MakeFromBox(bounds));
  desc.AddSubMesh(mb.GetPrimitiveCount(), 0, 0);

  m_hCpuMesh = WResourceManager::GetOrCreateResource<WCpuMeshResource>(sResourceName, std::move(desc), sResourceName);
  return m_hCpuMesh;
}

void WTerrainVolumeComponent::OnMsgExtractGeometry(WMsgExtractGeometry& msg) const
{
  // A volume without a collider is not part of the world's physical representation, so it stays out of
  // navmeshes and collision exports. It is still included when the render geometry is what's wanted.
  if (msg.m_Mode == WWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && !m_bEnableCollider)
    return;

  WCpuMeshResourceHandle hMesh = GenerateCpuMesh();

  if (!hMesh.IsValid())
    return;

  // The voxel vertices are in the volume's own space, the same space the baked Jolt collider uses.
  msg.AddMeshObject(GetOwner()->GetGlobalTransform(), hMesh);
}

WUInt64 WTerrainVolumeComponent::ComputeColliderContentHash(WUInt64 uiBrushOverlapHash) const
{
  const WUInt8 uiVersion = 1;

  WHashStreamWriter64 hashWriter;
  hashWriter << uiVersion;
  hashWriter << uiBrushOverlapHash;
  hashWriter << m_Resolution;
  hashWriter << m_fSize;
  hashWriter << m_fFillHeight;

  for (WUInt32 s = 0; s < m_Surfaces.GetCount(); ++s)
    hashWriter << m_Surfaces[s];

  return hashWriter.GetHashValue();
}
