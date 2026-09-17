#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/SplineMeshComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

#include <ProcGenPlugin/Shaders/ProcGen/ProcGenVertexColor.h>

namespace
{
  static WCpuMeshResourceHandle ExtractCpuMeshResource(const WMeshComponentBase& meshComponent)
  {
    WWorldGeoExtractionUtil::MeshObjectList meshObjects(WTempAllocator::Get());

    WMsgExtractGeometry msg;
    msg.m_pMeshObjects = &meshObjects;

    meshComponent.SendMessage(msg);

    if (meshObjects.IsEmpty())
      return WCpuMeshResourceHandle();

    return meshObjects[0].m_hMeshResource;
  }

  W_ALWAYS_INLINE static WCustomInstanceDataOffset EncodeOffset(WCustomInstanceDataOffset offset, WUInt32 uiNumOutputs)
  {
    WCustomInstanceDataOffset res;
    res.m_uiOffset = uiNumOutputs << VERTEX_COLOR_ACCESS_OFFSET_BITS | (offset.m_uiOffset & VERTEX_COLOR_ACCESS_OFFSET_MASK);
    return res;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

using namespace WProcGenInternal;

WProcVertexColorComponentManager::WProcVertexColorComponentManager(WWorld* pWorld)
  : WComponentManager<WProcVertexColorComponent, WBlockStorageType::Compact>(pWorld)
{
}

WProcVertexColorComponentManager::~WProcVertexColorComponentManager() = default;

void WProcVertexColorComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    constexpr WUInt32 uiInitialBufferSize = 1024 * 16;

    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(WColorLinearUB);
    desc.m_uiTotalSize = uiInitialBufferSize * desc.m_uiStructSize;
    desc.m_ResourceAccess.m_bImmutable = false;

    if (WGALDevice::GetDefaultDevice()->GetCapabilities().m_bSupportsTexelBuffer)
    {
      desc.m_BufferFlags = WGALBufferUsageFlags::TexelBuffer | WGALBufferUsageFlags::ShaderResource;
      desc.m_Format = WGALResourceFormat::RGBAUByteNormalized;
    }
    else
    {
      desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
    }

    WRenderDataManager* pRenderDataManager = GetWorld()->GetOrCreateModule<WRenderDataManager>();
    m_uiCustomDataIndex = pRenderDataManager->RegisterCustomInstanceData(desc, "ProcVertexColors",
      // Before upload callback
      [this]()
      {
        WTaskSystem::WaitForGroup(m_UpdateTaskGroupID);
        m_UpdateTaskGroupID.Invalidate();
        m_uiNextTaskIndex = 0;
      });
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WProcVertexColorComponentManager::UpdateVertexColors, this);
    desc.m_Phase = WWorldUpdatePhase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WProcVertexColorComponentManager::OnResourceEvent, this));

  WProcVolumeComponent::GetAreaInvalidatedEvent().AddEventHandler(WMakeDelegate(&WProcVertexColorComponentManager::OnAreaInvalidated, this));
}

void WProcVertexColorComponentManager::Deinitialize()
{
  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WProcVertexColorComponentManager::OnResourceEvent, this));

  WProcVolumeComponent::GetAreaInvalidatedEvent().RemoveEventHandler(WMakeDelegate(&WProcVertexColorComponentManager::OnAreaInvalidated, this));

  SUPER::Deinitialize();
}

void WProcVertexColorComponentManager::UpdateVertexColors(const WWorldModule::UpdateContext& context)
{
  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->CompactCustomInstanceDataBuffer(m_uiCustomDataIndex);

  m_UpdateContexts.SetCount(m_ComponentsToUpdate.GetCount());

  // New allocations first to ensure that the buffer is large enough and the memory is not invalidated by reallocations
  {
    for (WUInt32 i = 0; i < m_ComponentsToUpdate.GetCount(); ++i)
    {
      WProcVertexColorComponent* pComponent = nullptr;
      if (!TryGetComponent(m_ComponentsToUpdate[i], pComponent) || !pComponent->IsActiveAndInitialized())
        continue;

      if (!UpdateComponentOutputs(*pComponent))
        continue;

      auto pMeshComponent = pComponent->GetMeshComponent();
      if (pMeshComponent == nullptr)
        continue;

      auto hCpuMesh = ExtractCpuMeshResource(*pMeshComponent);
      if (hCpuMesh.IsValid() == false)
        continue;

      WResourceLock<WCpuMeshResource> pCpuMesh(hCpuMesh, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pCpuMesh.GetAcquireResult() != WResourceAcquireResult::Final)
        continue;

      const auto& meshBufferDescriptor = pCpuMesh->GetDescriptor().MeshBufferDesc();
      const WUInt32 uiNumOutputs = pComponent->m_Outputs.GetCount();
      const WUInt32 uiVertexColorCount = meshBufferDescriptor.GetVertexCount() * uiNumOutputs;

      auto& offset = pComponent->m_CustomInstanceDataOffset;
      pRenderDataManager->DeleteCustomInstanceData(m_uiCustomDataIndex, offset);

      WGALDynamicBufferHandle hVertexColorsBuffer;
      pRenderDataManager->GetOrCreateCustomInstanceData<WColorLinearUB>(m_uiCustomDataIndex, pComponent, hVertexColorsBuffer, offset, uiVertexColorCount);
      pMeshComponent->SetCustomInstanceData(EncodeOffset(offset, uiNumOutputs), hVertexColorsBuffer);

      auto& updateContext = m_UpdateContexts[i];
      updateContext.m_pComponent = pComponent;
      updateContext.m_hCpuMesh = hCpuMesh;
      updateContext.m_uiVertexColorOffset = offset.m_uiOffset;
    }
  }

  // Update
  if (m_ComponentsToUpdate.IsEmpty() == false)
  {
    m_UpdateTaskGroupID = WTaskSystem::CreateTaskGroup(WTaskPriority::EarlyThisFrame);

    WGALDynamicBuffer* pBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(pRenderDataManager->GetCustomInstanceDataBuffer(m_uiCustomDataIndex));
    W_ASSERT_DEV(pBuffer != nullptr, "Vertex color buffer not found.");

    for (const auto& updateContext : m_UpdateContexts)
    {
      if (updateContext.m_pComponent == nullptr)
        continue;

      UpdateComponentVertexColors(updateContext, *pBuffer);
    }

    m_ComponentsToUpdate.Clear();

    WTaskSystem::StartTaskGroup(m_UpdateTaskGroupID);
  }
}

bool WProcVertexColorComponentManager::UpdateComponentOutputs(WProcVertexColorComponent& component)
{
  component.m_Outputs.Clear();

  {
    WResourceLock<WProcGenGraphResource> pResource(component.m_hResource, WResourceAcquireMode::BlockTillLoaded);
    auto outputs = pResource->GetVertexColorOutputs();

    for (auto& outputDesc : component.m_OutputDescs)
    {
      if (!outputDesc.m_sName.IsEmpty())
      {
        bool bOutputFound = false;
        for (auto& pOutput : outputs)
        {
          if (pOutput->m_sName == outputDesc.m_sName)
          {
            component.m_Outputs.PushBack(pOutput);
            bOutputFound = true;
            break;
          }
        }

        if (!bOutputFound)
        {
          component.m_Outputs.PushBack(nullptr);
          WLog::Error("Vertex Color Output with name '{}' not found in Proc Gen Graph '{}'", outputDesc.m_sName, pResource->GetResourceID());
        }
      }
      else
      {
        component.m_Outputs.PushBack(nullptr);
      }
    }
  }

  return component.HasValidOutputs();
}

void WProcVertexColorComponentManager::UpdateComponentVertexColors(const UpdateContext& context, WGALDynamicBuffer& buffer)
{
  auto pComponent = context.m_pComponent;
  if (pComponent->HasValidOutputs() == false)
    return;

  WResourceLock<WCpuMeshResource> pCpuMesh(context.m_hCpuMesh, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCpuMesh.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  if (m_uiNextTaskIndex >= m_UpdateTasks.GetCount())
  {
    m_UpdateTasks.PushBack(W_DEFAULT_NEW(WProcGenInternal::VertexColorTask));
  }

  auto& pUpdateTask = m_UpdateTasks[m_uiNextTaskIndex];

  WStringBuilder taskName = "VertexColor ";
  taskName.Append(pCpuMesh->GetResourceIdOrDescription());
  pUpdateTask->ConfigureTask(taskName, WTaskNesting::Never);

  const auto& meshBufferDescriptor = pCpuMesh->GetDescriptor().MeshBufferDesc();
  const auto meshBBox = pCpuMesh->GetDescriptor().GetBounds().GetBox();
  auto vertexColorData = buffer.MapForWriting<WColorLinearUB>(context.m_uiVertexColorOffset);

  WTempHybridArray<WProcVertexColorMapping, 2> outputMappings;
  for (auto& outputDesc : pComponent->m_OutputDescs)
  {
    outputMappings.PushBack(outputDesc.m_Mapping);
  }

  pUpdateTask->Prepare(*GetWorld(), meshBufferDescriptor, pComponent->GetOwner()->GetGlobalTransform(), meshBBox, pComponent->m_Outputs, outputMappings, vertexColorData);

  WTaskSystem::AddTaskToGroup(m_UpdateTaskGroupID, pUpdateTask);

  ++m_uiNextTaskIndex;
}

void WProcVertexColorComponentManager::EnqueueUpdate(WProcVertexColorComponent& component)
{
  if (!component.IsActiveAndInitialized() || !component.GetResource().IsValid())
    return;

  if (!m_ComponentsToUpdate.Contains(component.GetHandle()))
  {
    m_ComponentsToUpdate.PushBack(component.GetHandle());
  }
}

void WProcVertexColorComponentManager::RemoveComponent(WProcVertexColorComponent& component)
{
  m_ComponentsToUpdate.RemoveAndSwap(component.GetHandle());

  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteCustomInstanceData(m_uiCustomDataIndex, component.m_CustomInstanceDataOffset);
}

void WProcVertexColorComponentManager::OnResourceEvent(const WResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != WResourceEvent::Type::ResourceContentUnloading || resourceEvent.m_pResource->GetReferenceCount() == 0)
    return;

  if (auto pResource = WDynamicCast<WProcGenGraphResource*>(resourceEvent.m_pResource))
  {
    WProcGenGraphResourceHandle hResource(pResource);

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource)
      {
        EnqueueUpdate(*it);
      }
    }
  }
  else if (auto pResource = WDynamicCast<WMeshResource*>(resourceEvent.m_pResource))
  {
    WMeshResourceHandle hMeshResource(pResource);

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      auto pMeshComponent = it->GetMeshComponent();
      if (pMeshComponent != nullptr && pMeshComponent->GetMesh() == hMeshResource)
      {
        EnqueueUpdate(*it);
      }
    }
  }
}

void WProcVertexColorComponentManager::OnAreaInvalidated(const WProcGenInternal::InvalidatedArea& area)
{
  if (area.m_pWorld != GetWorld())
    return;

  WSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = WDefaultSpatialDataCategories::RenderStatic.GetBitmask() | WDefaultSpatialDataCategories::RenderDynamic.GetBitmask();

  GetWorld()->GetSpatialSystem()->FindObjectsInBox(area.m_Box, queryParams,
    [this](WGameObject* pObject)
    {
      WTempHybridArray<WProcVertexColorComponent*, 4> components;
      pObject->TryGetComponentsOfBaseType(components);

      for (auto pComponent : components)
      {
        EnqueueUpdate(*pComponent);
      }

      return WVisitorExecution::Continue;
    });
}

WGALDynamicBufferHandle WProcVertexColorComponentManager::GetVertexColorBuffer()
{
  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  return pRenderDataManager->GetCustomInstanceDataBuffer(m_uiCustomDataIndex);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WProcVertexColorOutputDesc, WNoBase, 1, WRTTIDefaultAllocator<WProcVertexColorOutputDesc>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new WDynamicStringEnumAttribute("ProcGenOutputNameEnum")),
    W_MEMBER_PROPERTY("Mapping", m_Mapping),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

static WTypeVersion s_ProcVertexColorOutputDescVersion = 1;
WResult WProcVertexColorOutputDesc::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_ProcVertexColorOutputDescVersion);
  inout_stream << m_sName;
  W_SUCCEED_OR_RETURN(m_Mapping.Serialize(inout_stream));

  return W_SUCCESS;
}

WResult WProcVertexColorOutputDesc::Deserialize(WStreamReader& inout_stream)
{
  /*WTypeVersion version =*/inout_stream.ReadVersion(s_ProcVertexColorOutputDescVersion);
  inout_stream >> m_sName;
  W_SUCCEED_OR_RETURN(m_Mapping.Deserialize(inout_stream));

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WProcVertexColorComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_ProcGen_Graph"), new WRequiredAttribute()),
    W_ARRAY_ACCESSOR_PROPERTY("OutputDescs", OutputDescs_GetCount, GetOutputDesc, SetOutputDesc, OutputDescs_Insert, OutputDescs_Remove),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgTransformChanged, OnMsgTransformChanged),
    W_MESSAGE_HANDLER(WMsgCustomInstanceDataOffsetChanged, OnMsgCustomInstanceDataOffsetChanged),
    W_MESSAGE_HANDLER(WMsgGenerateSplineMeshCollision, OnMsgGenerateSplineMeshCollision),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Construction/Procedural Generation"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WProcVertexColorComponent::WProcVertexColorComponent() = default;
WProcVertexColorComponent::~WProcVertexColorComponent() = default;

void WProcVertexColorComponent::OnActivated()
{
  SUPER::OnActivated();

  auto pManager = static_cast<WProcVertexColorComponentManager*>(GetOwningManager());
  pManager->EnqueueUpdate(*this);

  if (GetUniqueID() != WInvalidIndex)
  {
    GetOwner()->EnableStaticTransformChangesNotifications();
  }
}

void WProcVertexColorComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  auto pManager = static_cast<WProcVertexColorComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(*this);

  // Don't disable notifications as other components attached to the owner game object might need them too.
  // GetOwner()->DisableStaticTransformChangesNotifications();
}

void WProcVertexColorComponent::SetResourceFile(WStringView sFile)
{
  WProcGenGraphResourceHandle hResource;

  if (!sFile.IsEmpty())
  {
    hResource = WResourceManager::LoadResource<WProcGenGraphResource>(sFile);
    WResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

WStringView WProcVertexColorComponent::GetResourceFile() const
{
  return m_hResource.GetResourceID();
}

void WProcVertexColorComponent::SetResource(const WProcGenGraphResourceHandle& hResource)
{
  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<WProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(*this);
  }
}

const WProcVertexColorOutputDesc& WProcVertexColorComponent::GetOutputDesc(WUInt32 uiIndex) const
{
  return m_OutputDescs[uiIndex];
}

void WProcVertexColorComponent::SetOutputDesc(WUInt32 uiIndex, const WProcVertexColorOutputDesc& outputDesc)
{
  m_OutputDescs.EnsureCount(uiIndex + 1);
  m_OutputDescs[uiIndex] = outputDesc;

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<WProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(*this);
  }
}

void WProcVertexColorComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_OutputDescs).IgnoreResult();
}

void WProcVertexColorComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  if (uiVersion >= 2)
  {
    s.ReadArray(m_OutputDescs).IgnoreResult();
  }
  else
  {
    WTempHybridArray<WHashedString, 2> outputNames;
    s.ReadArray(outputNames).IgnoreResult();

    for (auto& outputName : outputNames)
    {
      auto& outputDesc = m_OutputDescs.ExpandAndGetRef();
      outputDesc.m_sName = outputName;
    }
  }
}

void WProcVertexColorComponent::OnMsgTransformChanged(WMsgTransformChanged& ref_msg)
{
  auto pManager = static_cast<WProcVertexColorComponentManager*>(GetOwningManager());
  pManager->EnqueueUpdate(*this);
}

void WProcVertexColorComponent::OnMsgCustomInstanceDataOffsetChanged(WMsgCustomInstanceDataOffsetChanged& ref_msg)
{
  m_CustomInstanceDataOffset = ref_msg.m_NewOffset;

  if (WMeshComponentBase* pMeshComponent = GetMeshComponent())
  {
    auto pManager = static_cast<WProcVertexColorComponentManager*>(GetOwningManager());

    const WUInt32 uiNumOutputs = m_Outputs.GetCount();
    pMeshComponent->SetCustomInstanceData(EncodeOffset(ref_msg.m_NewOffset, uiNumOutputs), pManager->GetVertexColorBuffer());
  }
}

void WProcVertexColorComponent::OnMsgGenerateSplineMeshCollision(WMsgGenerateSplineMeshCollision& ref_msg)
{
  // Although we don't generate any collision meshes here, we use this as a signal that a spline mesh has changed and we need to update our vertex colors.
  auto pManager = static_cast<WProcVertexColorComponentManager*>(GetOwningManager());
  pManager->EnqueueUpdate(*this);
}

WUInt32 WProcVertexColorComponent::OutputDescs_GetCount() const
{
  return m_OutputDescs.GetCount();
}

void WProcVertexColorComponent::OutputDescs_Insert(WUInt32 uiIndex, const WProcVertexColorOutputDesc& outputDesc)
{
  m_OutputDescs.InsertAt(uiIndex, outputDesc);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<WProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(*this);
  }
}

void WProcVertexColorComponent::OutputDescs_Remove(WUInt32 uiIndex)
{
  m_OutputDescs.RemoveAtAndCopy(uiIndex);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<WProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(*this);
  }
}

bool WProcVertexColorComponent::HasValidOutputs() const
{
  for (auto& pOutput : m_Outputs)
  {
    if (pOutput != nullptr && pOutput->m_pByteCode != nullptr)
    {
      return true;
    }
  }

  return false;
}

WMeshComponentBase* WProcVertexColorComponent::GetMeshComponent()
{
  WMeshComponentBase* pMeshComponent = nullptr;
  bool _ = GetOwner()->TryGetComponentOfBaseType(pMeshComponent);
  return pMeshComponent;
}


W_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Components_Implementation_ProcVertexColorComponent);
