
W_FORCE_INLINE bool WComponentManagerBase::IsValidComponent(const WComponentHandle& hComponent) const
{
  return m_Components.Contains(hComponent);
}

W_FORCE_INLINE bool WComponentManagerBase::TryGetComponent(const WComponentHandle& hComponent, WComponent*& out_pComponent)
{
  return m_Components.TryGetValue(hComponent, out_pComponent);
}

W_FORCE_INLINE bool WComponentManagerBase::TryGetComponent(const WComponentHandle& hComponent, const WComponent*& out_pComponent) const
{
  WComponent* pComponent = nullptr;
  bool res = m_Components.TryGetValue(hComponent, pComponent);
  out_pComponent = pComponent;
  return res;
}

W_ALWAYS_INLINE WUInt32 WComponentManagerBase::GetComponentCount() const
{
  return static_cast<WUInt32>(m_Components.GetCount());
}

template <typename ComponentType>
W_ALWAYS_INLINE WTypedComponentHandle<ComponentType> WComponentManagerBase::CreateComponent(WGameObject* pOwnerObject, ComponentType*& out_pComponent)
{
  WComponent* pComponent = nullptr;
  WComponentHandle hComponent = CreateComponentNoInit(pOwnerObject, pComponent);

  if (pComponent != nullptr)
  {
    InitializeComponent(pComponent);
  }

  out_pComponent = WStaticCast<ComponentType*>(pComponent);
  return WTypedComponentHandle<ComponentType>(hComponent);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, WBlockStorageType::Enum StorageType>
WComponentManager<T, StorageType>::WComponentManager(WWorld* pWorld)
  : WComponentManagerBase(pWorld)
  , m_ComponentStorage(GetBlockAllocator(), GetAllocator())
{
  static_assert(W_IS_DERIVED_FROM_STATIC(WComponent, ComponentType), "Not a valid component type");
}

template <typename T, WBlockStorageType::Enum StorageType>
WComponentManager<T, StorageType>::~WComponentManager() = default;

template <typename T, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE bool WComponentManager<T, StorageType>::TryGetComponent(const WComponentHandle& hComponent, ComponentType*& out_pComponent)
{
  W_ASSERT_DEV(ComponentType::TypeId() == hComponent.GetInternalID().m_TypeId,
    "The given component handle is not of the expected type. Expected type id {0}, got type id {1}", ComponentType::TypeId(),
    hComponent.GetInternalID().m_TypeId);
  W_ASSERT_DEV(hComponent.GetInternalID().m_WorldIndex == GetWorldIndex(),
    "Component does not belong to this world. Expected world id {0} got id {1}", GetWorldIndex(), hComponent.GetInternalID().m_WorldIndex);

  WComponent* pComponent = nullptr;
  bool bResult = WComponentManagerBase::TryGetComponent(hComponent, pComponent);
  out_pComponent = static_cast<ComponentType*>(pComponent);
  return bResult;
}

template <typename T, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE bool WComponentManager<T, StorageType>::TryGetComponent(
  const WComponentHandle& hComponent, const ComponentType*& out_pComponent) const
{
  W_ASSERT_DEV(ComponentType::TypeId() == hComponent.GetInternalID().m_TypeId,
    "The given component handle is not of the expected type. Expected type id {0}, got type id {1}", ComponentType::TypeId(),
    hComponent.GetInternalID().m_TypeId);
  W_ASSERT_DEV(hComponent.GetInternalID().m_WorldIndex == GetWorldIndex(),
    "Component does not belong to this world. Expected world id {0} got id {1}", GetWorldIndex(), hComponent.GetInternalID().m_WorldIndex);

  const WComponent* pComponent = nullptr;
  bool bResult = WComponentManagerBase::TryGetComponent(hComponent, pComponent);
  out_pComponent = static_cast<const ComponentType*>(pComponent);
  return bResult;
}

template <typename T, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE typename WBlockStorage<T, WInternal::DEFAULT_BLOCK_SIZE, StorageType>::Iterator WComponentManager<T, StorageType>::GetComponents(WUInt32 uiStartIndex /*= 0*/)
{
  return m_ComponentStorage.GetIterator(uiStartIndex);
}

template <typename T, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE typename WBlockStorage<T, WInternal::DEFAULT_BLOCK_SIZE, StorageType>::ConstIterator
WComponentManager<T, StorageType>::GetComponents(WUInt32 uiStartIndex /*= 0*/) const
{
  return m_ComponentStorage.GetIterator(uiStartIndex);
}

// static
template <typename T, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE WWorldModuleTypeId WComponentManager<T, StorageType>::TypeId()
{
  return T::TypeId();
}

template <typename T, WBlockStorageType::Enum StorageType>
void WComponentManager<T, StorageType>::CollectAllComponents(WDynamicArray<WComponentHandle>& out_allComponents, bool bOnlyActive)
{
  out_allComponents.Reserve(out_allComponents.GetCount() + m_ComponentStorage.GetCount());

  for (auto it = GetComponents(); it.IsValid(); it.Next())
  {
    if (!bOnlyActive || it->IsActive())
    {
      out_allComponents.PushBack(it->GetHandle());
    }
  }
}

template <typename T, WBlockStorageType::Enum StorageType>
void WComponentManager<T, StorageType>::CollectAllComponents(WDynamicArray<WComponent*>& out_allComponents, bool bOnlyActive)
{
  out_allComponents.Reserve(out_allComponents.GetCount() + m_ComponentStorage.GetCount());

  for (auto it = GetComponents(); it.IsValid(); it.Next())
  {
    if (!bOnlyActive || it->IsActive())
    {
      out_allComponents.PushBack(it);
    }
  }
}

template <typename T, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE WComponent* WComponentManager<T, StorageType>::CreateComponentStorage()
{
  return m_ComponentStorage.Create();
}

template <typename T, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE void WComponentManager<T, StorageType>::DeleteComponentStorage(WComponent* pComponent, WComponent*& out_pMovedComponent)
{
  T* pMovedComponent = nullptr;
  m_ComponentStorage.Delete(static_cast<T*>(pComponent), pMovedComponent);
  out_pMovedComponent = pMovedComponent;
}

template <typename T, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE void WComponentManager<T, StorageType>::RegisterUpdateFunction(UpdateFunctionDesc& desc)
{
  // round up to multiple of data block capacity so tasks only have to deal with complete data blocks
  if (desc.m_uiAsyncPhaseBatchSize != 0)
  {
    desc.m_uiAsyncPhaseBatchSize = static_cast<WUInt16>(WMath::RoundUp(static_cast<WInt32>(desc.m_uiAsyncPhaseBatchSize), WDataBlock<ComponentType, WInternal::DEFAULT_BLOCK_SIZE>::CAPACITY));
  }

  WComponentManagerBase::RegisterUpdateFunction(desc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ComponentType, WComponentUpdateType::Enum UpdateType, WBlockStorageType::Enum StorageType, WWorldUpdatePhase::Enum UpdatePhase>
WComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::WComponentManagerSimple(WWorld* pWorld)
  : WComponentManager<ComponentType, StorageType>(pWorld)
{
}

template <typename ComponentType, WComponentUpdateType::Enum UpdateType, WBlockStorageType::Enum StorageType, WWorldUpdatePhase::Enum UpdatePhase>
void WComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::Initialize()
{
  using OwnType = WComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>;

  WStringBuilder functionName;
  SimpleUpdateName(functionName);

  auto desc = WWorldModule::UpdateFunctionDesc(WWorldModule::UpdateFunction(&OwnType::SimpleUpdate, this), functionName);
  desc.m_Phase = UpdatePhase;
  desc.m_bOnlyUpdateWhenSimulating = (UpdateType == WComponentUpdateType::WhenSimulating);

  this->RegisterUpdateFunction(desc);
}

template <typename ComponentType, WComponentUpdateType::Enum UpdateType, WBlockStorageType::Enum StorageType, WWorldUpdatePhase::Enum UpdatePhase>
void WComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::SimpleUpdate(const WWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

// static
template <typename ComponentType, WComponentUpdateType::Enum UpdateType, WBlockStorageType::Enum StorageType, WWorldUpdatePhase::Enum UpdatePhase>
void WComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::SimpleUpdateName(WStringBuilder& out_sName)
{
  WStringView sName(W_SOURCE_FUNCTION);
  const char* szEnd = sName.FindSubString(",");

  if (szEnd != nullptr && sName.StartsWith("WComponentManagerSimple<class "))
  {
    WStringView sChoppedName(sName.GetStartPointer() + WStringUtils::GetStringElementCount("WComponentManagerSimple<class "), szEnd);

    W_ASSERT_DEV(!sChoppedName.IsEmpty(), "Chopped name is empty: '{0}'", sName);

    out_sName = sChoppedName;
    out_sName.Append("::SimpleUpdate");
  }
  else
  {
    out_sName = sName;
  }
}
