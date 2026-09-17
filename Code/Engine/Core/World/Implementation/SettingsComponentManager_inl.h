
template <typename ComponentType>
WSettingsComponentManager<ComponentType>::WSettingsComponentManager(WWorld* pWorld)
  : WComponentManagerBase(pWorld)
{
}

template <typename ComponentType>
WSettingsComponentManager<ComponentType>::~WSettingsComponentManager()
{
  for (auto& component : m_Components)
  {
    DeinitializeComponent(component.Borrow());
  }
}

template <typename ComponentType>
W_ALWAYS_INLINE ComponentType* WSettingsComponentManager<ComponentType>::GetSingletonComponent()
{
  for (const auto& pComponent : m_Components)
  {
    // retrieve the first component that is active
    if (pComponent->IsActive())
      return pComponent.Borrow();
  }

  return nullptr;
}

template <typename ComponentType>
W_ALWAYS_INLINE const ComponentType* WSettingsComponentManager<ComponentType>::GetSingletonComponent() const
{
  for (const auto& pComponent : m_Components)
  {
    // retrieve the first component that is active
    if (pComponent->IsActive())
      return pComponent.Borrow();
  }

  return nullptr;
}

// static
template <typename ComponentType>
W_ALWAYS_INLINE WWorldModuleTypeId WSettingsComponentManager<ComponentType>::TypeId()
{
  return ComponentType::TypeId();
}

template <typename ComponentType>
void WSettingsComponentManager<ComponentType>::CollectAllComponents(WDynamicArray<WComponentHandle>& out_allComponents, bool bOnlyActive)
{
  for (auto& component : m_Components)
  {
    if (!bOnlyActive || component->IsActive())
    {
      out_allComponents.PushBack(component->GetHandle());
    }
  }
}

template <typename ComponentType>
void WSettingsComponentManager<ComponentType>::CollectAllComponents(WDynamicArray<WComponent*>& out_allComponents, bool bOnlyActive)
{
  for (auto& component : m_Components)
  {
    if (!bOnlyActive || component->IsActive())
    {
      out_allComponents.PushBack(component.Borrow());
    }
  }
}

template <typename ComponentType>
WComponent* WSettingsComponentManager<ComponentType>::CreateComponentStorage()
{
  if (!m_Components.IsEmpty())
  {
    WLog::Warning("A component of type '{0}' is already present in this world. Having more than one is not allowed.", WGetStaticRTTI<ComponentType>()->GetTypeName());
  }

  m_Components.PushBack(W_NEW(GetAllocator(), ComponentType));
  return m_Components.PeekBack().Borrow();
}

template <typename ComponentType>
void WSettingsComponentManager<ComponentType>::DeleteComponentStorage(WComponent* pComponent, WComponent*& out_pMovedComponent)
{
  out_pMovedComponent = pComponent;

  for (WUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    if (m_Components[i].Borrow() == pComponent)
    {
      m_Components.RemoveAtAndCopy(i);
      break;
    }
  }
}
