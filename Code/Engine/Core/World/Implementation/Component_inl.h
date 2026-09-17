#include <Foundation/Logging/Log.h>

W_ALWAYS_INLINE WComponent::WComponent() = default;

W_ALWAYS_INLINE WComponent::~WComponent()
{
  m_pMessageDispatchType = nullptr;
  m_pManager = nullptr;
  m_pOwner = nullptr;
  m_InternalId.Invalidate();
}

W_ALWAYS_INLINE bool WComponent::IsDynamic() const
{
  return m_ComponentFlags.IsSet(WObjectFlags::Dynamic);
}

W_ALWAYS_INLINE bool WComponent::GetActiveFlag() const
{
  return m_ComponentFlags.IsSet(WObjectFlags::ActiveFlag);
}

W_ALWAYS_INLINE bool WComponent::IsActive() const
{
  return m_ComponentFlags.IsSet(WObjectFlags::ActiveState);
}

W_ALWAYS_INLINE bool WComponent::IsActiveAndInitialized() const
{
  return m_ComponentFlags.AreAllSet(WObjectFlags::ActiveState | WObjectFlags::Initialized);
}

W_ALWAYS_INLINE WComponentManagerBase* WComponent::GetOwningManager()
{
  return m_pManager;
}

W_ALWAYS_INLINE const WComponentManagerBase* WComponent::GetOwningManager() const
{
  return m_pManager;
}

W_ALWAYS_INLINE WGameObject* WComponent::GetOwner()
{
  return m_pOwner;
}

W_ALWAYS_INLINE const WGameObject* WComponent::GetOwner() const
{
  return m_pOwner;
}

W_ALWAYS_INLINE WComponentHandle WComponent::GetHandle() const
{
  return WComponentHandle(m_InternalId);
}

W_ALWAYS_INLINE WUInt32 WComponent::GetUniqueID() const
{
  return m_uiUniqueID;
}

W_ALWAYS_INLINE void WComponent::SetUniqueID(WUInt32 uiUniqueID)
{
  m_uiUniqueID = uiUniqueID;
}

W_ALWAYS_INLINE bool WComponent::IsInitialized() const
{
  return m_ComponentFlags.IsSet(WObjectFlags::Initialized);
}

W_ALWAYS_INLINE bool WComponent::IsInitializing() const
{
  return m_ComponentFlags.IsSet(WObjectFlags::Initializing);
}

W_ALWAYS_INLINE bool WComponent::IsSimulationStarted() const
{
  return m_ComponentFlags.IsSet(WObjectFlags::SimulationStarted);
}

W_ALWAYS_INLINE bool WComponent::IsActiveAndSimulating() const
{
  return m_ComponentFlags.AreAllSet(WObjectFlags::Initialized | WObjectFlags::ActiveState) &&
         m_ComponentFlags.IsAnySet(WObjectFlags::SimulationStarting | WObjectFlags::SimulationStarted);
}
