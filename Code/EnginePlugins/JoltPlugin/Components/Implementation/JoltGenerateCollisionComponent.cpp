#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Components/JoltGenerateCollisionComponent.h>
#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Components/SplineComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/SplineMeshComponent.h>

class SplineCollisionGenerationTask : public WTask
{
public:
  SplineCollisionGenerationTask(const WComponentHandle& hOwnerComponent, const WStringView sCollisionMeshPath, const WSpline& spline, WArrayMap<float, float> distanceToKey, WArrayPtr<WCpuMeshResourceHandle> meshes,
    WArrayPtr<WVec2> scaleOffsets, float fLocalOffsetY, float fLocalOffsetZ)
    : m_hOwnerComponent(hOwnerComponent)
    , m_sCollisionMeshPath(sCollisionMeshPath)
    , m_Spline(spline)
    , m_DistanceToKey(distanceToKey)
    , m_Meshes(meshes)
    , m_ScaleOffsets(scaleOffsets)
    , m_fLocalOffsetY(fLocalOffsetY)
    , m_fLocalOffsetZ(fLocalOffsetZ)
  {
  }

  virtual void Execute() override
  {
    WTempHybridArray<WCpuMeshResource*, 16> cpuMeshes;

    for (auto& hMeshCpu : m_Meshes)
    {
      WCpuMeshResource* pMeshCpu = WResourceManager::BeginAcquireResource<WCpuMeshResource>(hMeshCpu, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      W_ASSERT_DEV(pMeshCpu != nullptr, "Failed to load cpu mesh resource for spline mesh generation");
      cpuMeshes.PushBack(pMeshCpu);
    }

    W_SCOPE_EXIT(
      for (auto pMeshCpu : cpuMeshes) {
        WResourceManager::EndAcquireResource(pMeshCpu);
      });

    WMeshResourceDescriptor splineMeshDesc;
    if (WSplineMeshComponent::GenerateSplineMeshDesc(m_Spline, m_DistanceToKey, cpuMeshes, m_ScaleOffsets, m_fLocalOffsetY, m_fLocalOffsetZ, splineMeshDesc).Failed())
      return;

    WJoltMeshDesc joltMeshDesc;
    {
      const auto& splineMeshBufferDesc = splineMeshDesc.MeshBufferDesc();

      joltMeshDesc.m_Type = WJoltMeshDesc::Type::Triangle;
      joltMeshDesc.m_Vertices = splineMeshBufferDesc.GetPositionData();

      const WUInt32 uiNumIndices = splineMeshBufferDesc.GetPrimitiveCount() * 3;
      if (splineMeshBufferDesc.Uses32BitIndices())
      {
        WArrayPtr<const WUInt32> indices = WMakeArrayPtr(reinterpret_cast<const WUInt32*>(splineMeshBufferDesc.GetIndexBufferData().GetPtr()), uiNumIndices);
        joltMeshDesc.m_TriangleIndices = indices;
      }
      else
      {
        joltMeshDesc.m_TriangleIndices.SetCountUninitialized(uiNumIndices);
        const WUInt16* pIndices = reinterpret_cast<const WUInt16*>(splineMeshBufferDesc.GetIndexBufferData().GetPtr());

        for (WUInt32 tri = 0; tri < uiNumIndices; ++tri)
        {
          joltMeshDesc.m_TriangleIndices[tri] = pIndices[tri];
        }
      }

      joltMeshDesc.m_TriangleSurfaceID.SetCount(splineMeshDesc.MeshBufferDesc().GetPrimitiveCount());
      for (auto& subMesh : splineMeshDesc.GetSubMeshes())
      {
        const WUInt32 uiLastTriangle = subMesh.m_uiFirstPrimitive + subMesh.m_uiPrimitiveCount;
        const WUInt16 uiSurface = subMesh.m_uiMaterialIndex;

        for (WUInt32 t = subMesh.m_uiFirstPrimitive; t < uiLastTriangle; ++t)
        {
          joltMeshDesc.m_TriangleSurfaceID[t] = uiSurface;
        }
      }

      for (auto& mat : splineMeshDesc.GetMaterials())
      {
        joltMeshDesc.m_Surfaces.PushBack(mat.m_sPath);
      }
    }

    WDeferredFileWriter fileWriter;
    fileWriter.SetOutput(m_sCollisionMeshPath);

    if (WJoltMeshResourceWriter::WriteMeshResource(std::move(joltMeshDesc), fileWriter).Failed())
    {
      WLog::Error("Could not write spline collision mesh file to '{}'", m_sCollisionMeshPath);
      return;
    }

    if (fileWriter.Close().Failed())
    {
      WLog::Error("Could not write spline collision mesh file to '{}'", m_sCollisionMeshPath);
    }

    WMsgComponentInternalTrigger msg;
    msg.m_sMessage.Assign("GenerationDone");

    WWorld::GetWorld(m_hOwnerComponent)->PostMessage(m_hOwnerComponent, msg, WTime::MakeZero());
  }

private:
  WComponentHandle m_hOwnerComponent;

  WString m_sCollisionMeshPath;
  WSpline m_Spline;
  WArrayMap<float, float> m_DistanceToKey;
  WDynamicArray<WCpuMeshResourceHandle> m_Meshes;
  WDynamicArray<WVec2> m_ScaleOffsets;
  float m_fLocalOffsetY = 0;
  float m_fLocalOffsetZ = 0;
};

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WJoltMeshMapping, WNoBase, 1, WRTTIDefaultAllocator<WJoltMeshMapping>)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("RenderMesh", m_hRenderMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Static", WDependencyFlags::None), new WRequiredAttribute()),
    W_RESOURCE_MEMBER_PROPERTY("CollisionMesh", m_hCollisionMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Triangle", WDependencyFlags::None), new WRequiredAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WResult WJoltMeshMapping::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_hRenderMesh;
  inout_stream << m_hCollisionMesh;

  return W_SUCCESS;
}

WResult WJoltMeshMapping::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_hRenderMesh;
  inout_stream >> m_hCollisionMesh;

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltGenerateCollisionComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_ARRAY_ACCESSOR_PROPERTY("MeshMappings", Reflection_GetMeshMappingCount, Reflection_GetMeshMapping, Reflection_SetMeshMapping, Reflection_InsertMeshMapping, Reflection_RemoveMeshMapping),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgGenerateSplineMeshCollision, OnMsgGenerateSplineMeshCollision),
    W_MESSAGE_HANDLER(WMsgComponentInternalTrigger, OnMsgComponentInternalTrigger),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_FUNCTION_PROPERTY(OnObjectCreated),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Physics/Jolt/Misc"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltGenerateCollisionComponent::WJoltGenerateCollisionComponent() = default;
WJoltGenerateCollisionComponent::~WJoltGenerateCollisionComponent() = default;

void WJoltGenerateCollisionComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s.WriteArray(m_MeshMappings).IgnoreResult();
  s << m_uiStableId;
  s << m_uiCollisionLayer;
}

void WJoltGenerateCollisionComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s.ReadArray(m_MeshMappings).IgnoreResult();
  s >> m_uiStableId;

  if (uiVersion >= 2)
  {
    s >> m_uiCollisionLayer;
  }
}

void WJoltGenerateCollisionComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  WTaskSystem::WaitForGroup(m_TaskGroupID);
}

void WJoltGenerateCollisionComponent::Reflection_SetMeshMapping(WUInt32 uiIndex, const WJoltMeshMapping& mapping)
{
  m_MeshMappings.EnsureCount(uiIndex + 1);
  m_MeshMappings[uiIndex] = mapping;
}

void WJoltGenerateCollisionComponent::Reflection_InsertMeshMapping(WUInt32 uiIndex, const WJoltMeshMapping& mapping)
{
  m_MeshMappings.InsertAt(uiIndex, mapping);
}

void WJoltGenerateCollisionComponent::Reflection_RemoveMeshMapping(WUInt32 uiIndex)
{
  m_MeshMappings.RemoveAtAndCopy(uiIndex);
}

WCpuMeshResourceHandle WJoltGenerateCollisionComponent::GetCollisionCpuMeshForRenderMesh(WMeshResourceHandle hRenderMesh) const
{
  for (const auto& mapping : m_MeshMappings)
  {
    if (mapping.m_hRenderMesh == hRenderMesh)
    {
      WResourceLock<WJoltMeshResource> pCollisionMesh(mapping.m_hCollisionMesh, WResourceAcquireMode::BlockTillLoaded);
      if (pCollisionMesh.GetAcquireResult() == WResourceAcquireResult::Final)
      {
        return pCollisionMesh->ConvertToCpuMesh();
      }
    }
  }

  return WCpuMeshResourceHandle();
}

void WJoltGenerateCollisionComponent::OnObjectCreated(const WAbstractObjectNode& node)
{
  m_uiStableId = WHashingUtils::xxHash64(&node.GetGuid(), sizeof(WUuid));
}

void WJoltGenerateCollisionComponent::OnMsgGenerateSplineMeshCollision(WMsgGenerateSplineMeshCollision& ref_msg)
{
  // Only generate in the editor
  if (GetUniqueID() == WInvalidIndex)
    return;

  m_sCollisionMeshPath.Clear();

  WTempHybridArray<WCpuMeshResourceHandle, 16> cpuMeshes;
  WTempHybridArray<WVec2, 16> scaleOffsets;
  for (WUInt32 i = 0; i < ref_msg.m_RenderMeshes.GetCount(); ++i)
  {
    auto hCpuMesh = GetCollisionCpuMeshForRenderMesh(ref_msg.m_RenderMeshes[i]);
    if (hCpuMesh.IsValid())
    {
      cpuMeshes.PushBack(hCpuMesh);
      scaleOffsets.PushBack(ref_msg.m_ScaleOffsets[i]);
    }
  }

  if (cpuMeshes.IsEmpty())
    return;

  const WSplineComponent* pSplineComponent = nullptr;
  if (!GetWorld()->TryGetComponent(ref_msg.m_hSplineComponent, pSplineComponent))
    return;

  const WUInt64 uiStableSplineId = WHashingUtils::xxHash64(&pSplineComponent->GetUuid(), sizeof(WUuid));

  WStringBuilder sb;
  sb.SetFormat(":project/AssetCache/Generated/GenCol_{}_{}.WJoltMesh", WArgU(m_uiStableId, 16, true, 16, true), WArgU(uiStableSplineId, 16, true, 16, true));
  m_sCollisionMeshPath.Assign(sb);

  auto pTask = W_DEFAULT_NEW(SplineCollisionGenerationTask, GetHandle(), sb, pSplineComponent->GetSpline(), pSplineComponent->GetDistanceToKeyRemapping(), cpuMeshes, scaleOffsets, ref_msg.m_fLocalOffsetY, ref_msg.m_fLocalOffsetZ);
  pTask->ConfigureTask("Generate Spline Collision Mesh", WTaskNesting::Maybe);

  StartGenerateTask(pTask);
}

void WJoltGenerateCollisionComponent::OnMsgComponentInternalTrigger(WMsgComponentInternalTrigger& ref_msg)
{
  if (ref_msg.m_sMessage == "GenerationDone")
  {
    m_pGenerationTask = nullptr;

    if (m_pNextGenerationTask != nullptr)
    {
      m_pGenerationTask = std::move(m_pNextGenerationTask);
      m_TaskGroupID = WTaskSystem::StartSingleTask(m_pGenerationTask, WTaskPriority::LongRunning);
      m_pNextGenerationTask = nullptr;
    }
  }
}

void WJoltGenerateCollisionComponent::StartGenerateTask(WSharedPtr<WTask>&& pTask)
{
  if (m_pGenerationTask != nullptr)
  {
    m_pNextGenerationTask = pTask;
    return;
  }

  m_pGenerationTask = pTask;
  m_TaskGroupID = WTaskSystem::StartSingleTask(m_pGenerationTask, WTaskPriority::LongRunning);
}

void WJoltGenerateCollisionComponent::FinalizeGeneration()
{
  if (m_sCollisionMeshPath.IsEmpty())
    return;

  WTaskSystem::WaitForGroup(m_TaskGroupID);

  WGameObject* pObject = GetOwner();
  while (pObject->WasCreatedByPrefab())
  {
    pObject = pObject->GetParent();
  }

  WJoltStaticActorComponent* pStaticActorComponent = nullptr;
  if (!pObject->TryGetComponentOfBaseType(pStaticActorComponent))
  {
    WJoltStaticActorComponent::CreateComponent(pObject, pStaticActorComponent);
  }

  W_ASSERT_DEV(!pStaticActorComponent->WasCreatedByPrefab(), "Should have been handled above");
  WJoltMeshResourceHandle hCollisionMesh = WResourceManager::LoadResource<WJoltMeshResource>(m_sCollisionMeshPath);
  pStaticActorComponent->SetMesh(hCollisionMesh);
  pStaticActorComponent->m_uiCollisionLayer = m_uiCollisionLayer;
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltGenerateCollisionComponent);
