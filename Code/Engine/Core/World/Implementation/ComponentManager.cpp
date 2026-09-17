#include <Core/CorePCH.h>

#include <Core/World/World.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WComponentManagerBase, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WComponentManagerBase::WComponentManagerBase(WWorld* pWorld)
  : WWorldModule(pWorld)
  , m_Components(pWorld->GetAllocator())
{
}

WComponentManagerBase::~WComponentManagerBase() = default;

WComponentHandle WComponentManagerBase::CreateComponent(WGameObject* pOwnerObject)
{
  WComponent* pDummy;
  return CreateComponent(pOwnerObject, pDummy);
}

void WComponentManagerBase::DeleteComponent(const WComponentHandle& hComponent)
{
  WComponent* pComponent = nullptr;
  if (!m_Components.TryGetValue(hComponent, pComponent))
    return;

  DeleteComponent(pComponent);
}

void WComponentManagerBase::DeleteComponent(WComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  DeinitializeComponent(pComponent);

  m_Components.Remove(pComponent->m_InternalId);

  pComponent->m_InternalId.Invalidate();
  pComponent->m_ComponentFlags.Remove(WObjectFlags::ActiveFlag | WObjectFlags::ActiveState);

  GetWorld()->m_Data.m_DeadComponents.Insert(pComponent);
}

void WComponentManagerBase::Deinitialize()
{
  for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
  {
    DeinitializeComponent(it.Value());
  }

  SUPER::Deinitialize();
}

WComponentHandle WComponentManagerBase::CreateComponentNoInit(WGameObject* pOwnerObject, WComponent*& out_pComponent)
{
  W_ASSERT_DEV(m_Components.GetCount() < WWorld::GetMaxNumComponentsPerType(), "Max number of components per type reached: {}",
    WWorld::GetMaxNumComponentsPerType());

  WComponent* pComponent = CreateComponentStorage();
  if (pComponent == nullptr)
  {
    return WComponentHandle();
  }

  WComponentId newId = m_Components.Insert(pComponent);
  newId.m_WorldIndex = GetWorldIndex();
  newId.m_TypeId = pComponent->GetTypeId();

  pComponent->m_pManager = this;
  pComponent->m_InternalId = newId;
  pComponent->m_ComponentFlags.AddOrRemove(WObjectFlags::Dynamic, pComponent->GetMode() == WComponentMode::Dynamic);

  // In Editor we add components via reflection so it is fine to have a nullptr here.
  // We check for a valid owner before the Initialize() callback.
  if (pOwnerObject != nullptr)
  {
    // AddComponent will update the active state internally
    pOwnerObject->AddComponent(pComponent);
  }
  else
  {
    pComponent->UpdateActiveState(true);
  }

  out_pComponent = pComponent;
  return pComponent->GetHandle();
}

void WComponentManagerBase::InitializeComponent(WComponent* pComponent)
{
  GetWorld()->AddComponentToInitialize(pComponent->GetHandle());
}

void WComponentManagerBase::DeinitializeComponent(WComponent* pComponent)
{
  if (pComponent->IsInitialized())
  {
    pComponent->Deinitialize();
    pComponent->m_ComponentFlags.Remove(WObjectFlags::Initialized);
  }

  if (WGameObject* pOwner = pComponent->GetOwner())
  {
    pOwner->RemoveComponent(pComponent);
  }
}

void WComponentManagerBase::PatchIdTable(WComponent* pComponent)
{
  WComponentId id = pComponent->m_InternalId;
  if (id.m_InstanceIndex != WComponentId::INVALID_INSTANCE_INDEX)
    m_Components[id] = pComponent;
}

W_STATICLINK_FILE(Core, Core_World_Implementation_ComponentManager);
