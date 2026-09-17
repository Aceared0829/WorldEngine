#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Components/JoltVisColMeshComponent.h>
#include <JoltPlugin/Shapes/JoltShapeConvexHullComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltVisColMeshComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("CollisionMesh", GetMesh, SetMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Triangle;CompatibleAsset_Jolt_Colmesh_Convex", WDependencyFlags::Package)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Physics/Jolt/Misc"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltVisColMeshComponent::WJoltVisColMeshComponent() = default;
WJoltVisColMeshComponent::~WJoltVisColMeshComponent() = default;

void WJoltVisColMeshComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hCollisionMesh;
}


void WJoltVisColMeshComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hCollisionMesh;

  GetWorld()->GetOrCreateComponentManager<WJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
}

WResult WJoltVisColMeshComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  // have to assume this isn't thread safe
  // CreateCollisionRenderMesh();

  if (m_hMesh.IsValid())
  {
    WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::BlockTillLoaded);
    ref_bounds = pMesh->GetBounds();
    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WJoltVisColMeshComponent::SetMesh(const WJoltMeshResourceHandle& hMesh)
{
  if (m_hCollisionMesh != hMesh)
  {
    m_hCollisionMesh = hMesh;
    m_hMesh.Invalidate();

    GetWorld()->GetOrCreateComponentManager<WJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void WJoltVisColMeshComponent::CreateCollisionRenderMesh()
{
  if (!m_hCollisionMesh.IsValid())
  {
    WJoltStaticActorComponent* pSibling = nullptr;
    if (GetOwner()->TryGetComponentOfBaseType(pSibling))
    {
      m_hCollisionMesh = pSibling->GetMesh();
    }
  }

  if (!m_hCollisionMesh.IsValid())
  {
    WJoltShapeConvexHullComponent* pSibling = nullptr;
    if (GetOwner()->TryGetComponentOfBaseType(pSibling))
    {
      m_hCollisionMesh = pSibling->GetMesh();
    }
  }

  if (!m_hCollisionMesh.IsValid())
    return;

  WResourceLock<WJoltMeshResource> pMesh(m_hCollisionMesh, WResourceAcquireMode::BlockTillLoaded);

  if (pMesh.GetAcquireResult() == WResourceAcquireResult::MissingFallback)
    return;

  WStringBuilder sColMeshName = pMesh->GetResourceID();
  sColMeshName.AppendFormat("_{0}_JoltVisColMesh",
    pMesh->GetCurrentResourceChangeCounter()); // the change counter allows to react to resource updates

  m_hMesh = WResourceManager::GetExistingResource<WMeshResource>(sColMeshName);

  if (m_hMesh.IsValid())
  {
    TriggerLocalBoundsUpdate();
    return;
  }

  WCpuMeshResourceHandle hCpuMesh = pMesh->ConvertToCpuMesh();

  if (!hCpuMesh.IsValid())
    return;

  WResourceLock<WCpuMeshResource> pCpuMesh(hCpuMesh, WResourceAcquireMode::BlockTillLoaded);

  WMeshResourceDescriptor md = pCpuMesh->GetDescriptor();

  // replace all materials with the preview material
  // actually the existing materials here are the surfaces of the collision mesh
  // so this can't be used for rendering anyway
  for (WUInt32 i = 0; i < md.GetMaterials().GetCount(); ++i)
  {
    md.SetMaterial(i, "Materials/Common/ColMesh.WMaterial");
  }

  m_hMesh = WResourceManager::GetOrCreateResource<WMeshResource>(sColMeshName, std::move(md), "Collision Mesh Visualization");

  TriggerLocalBoundsUpdate();
}

void WJoltVisColMeshComponent::Initialize()
{
  SUPER::Initialize();

  GetWorld()->GetOrCreateComponentManager<WJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
}

void WJoltVisColMeshComponent::OnDeactivated()
{
  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();
}

void WJoltVisColMeshComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering());

  WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::AllowLoadingFallback);
  WArrayPtr<const WMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (WUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const WUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    WMaterialResourceHandle hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    WMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(GetOwner());
    pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
    pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, m_hMesh, uiMaterialIndex, uiPartIndex);

    msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::LitOpaque, WRenderData::Caching::IfStatic);
  }
}

//////////////////////////////////////////////////////////////////////////

void WJoltVisColMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  WWorldModule::UpdateFunctionDesc desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WJoltVisColMeshComponentManager::Update, this);
  desc.m_Phase = WWorldUpdatePhase::PreAsync;

  RegisterUpdateFunction(desc);

  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WJoltVisColMeshComponentManager::ResourceEventHandler, this));
}

void WJoltVisColMeshComponentManager::Deinitialize()
{
  W_LOCK(m_Mutex);

  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WJoltVisColMeshComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void WJoltVisColMeshComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  WDeque<WComponentHandle> requireUpdate;
  m_RequireUpdate.Swap(requireUpdate);

  for (const auto& hComp : requireUpdate)
  {
    WJoltVisColMeshComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp))
      continue;

    pComp->CreateCollisionRenderMesh();
  }
}

void WJoltVisColMeshComponentManager::EnqueueUpdate(WComponentHandle hComponent)
{
  m_RequireUpdate.PushBack(hComponent);
}

void WJoltVisColMeshComponentManager::ResourceEventHandler(const WResourceEvent& e)
{
  if ((e.m_Type == WResourceEvent::Type::ResourceContentUnloading || e.m_Type == WResourceEvent::Type::ResourceContentUpdated) && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<WJoltMeshResource>())
  {
    W_LOCK(m_Mutex);

    WJoltMeshResourceHandle hResource((WJoltMeshResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const WJoltVisColMeshComponent* pComponent = static_cast<WJoltVisColMeshComponent*>(it.Value());

      if (pComponent->GetMesh() == hResource)
      {
        m_RequireUpdate.PushBack(pComponent->GetHandle());
      }
    }
  }
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltVisColMeshComponent);
