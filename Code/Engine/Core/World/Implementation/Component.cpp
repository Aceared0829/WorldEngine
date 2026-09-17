#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WComponent, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Active", GetActiveFlag, SetActiveFlag)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(IsActive),
    W_SCRIPT_FUNCTION_PROPERTY(IsActiveAndInitialized),
    W_SCRIPT_FUNCTION_PROPERTY(IsActiveAndSimulating),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_GetOwner),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_GetWorld),
    W_SCRIPT_FUNCTION_PROPERTY(GetUniqueID),
    W_SCRIPT_FUNCTION_PROPERTY(SetUniqueID, In, "UniqueID"),
    W_SCRIPT_FUNCTION_PROPERTY(DeleteComponent),
    W_SCRIPT_FUNCTION_PROPERTY(Initialize)->AddAttributes(new WScriptBaseClassFunctionAttribute(WComponent_ScriptBaseClassFunctions::Initialize)),
    W_SCRIPT_FUNCTION_PROPERTY(Deinitialize)->AddAttributes(new WScriptBaseClassFunctionAttribute(WComponent_ScriptBaseClassFunctions::Deinitialize)),
    W_SCRIPT_FUNCTION_PROPERTY(OnActivated)->AddAttributes(new WScriptBaseClassFunctionAttribute(WComponent_ScriptBaseClassFunctions::OnActivated)),
    W_SCRIPT_FUNCTION_PROPERTY(OnDeactivated)->AddAttributes(new WScriptBaseClassFunctionAttribute(WComponent_ScriptBaseClassFunctions::OnDeactivated)),
    W_SCRIPT_FUNCTION_PROPERTY(OnSimulationStarted)->AddAttributes(new WScriptBaseClassFunctionAttribute(WComponent_ScriptBaseClassFunctions::OnSimulationStarted)),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_Update, In, "DeltaTime")->AddAttributes(new WScriptBaseClassFunctionAttribute(WComponent_ScriptBaseClassFunctions::Update)),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WComponent::SetActiveFlag(bool bEnabled)
{
  if (m_ComponentFlags.IsSet(WObjectFlags::ActiveFlag) != bEnabled)
  {
    m_ComponentFlags.AddOrRemove(WObjectFlags::ActiveFlag, bEnabled);

    UpdateActiveState(GetOwner() == nullptr ? true : GetOwner()->IsActive());
  }
}

WWorld* WComponent::GetWorld()
{
  return m_pManager->GetWorld();
}

const WWorld* WComponent::GetWorld() const
{
  return m_pManager->GetWorld();
}

void WComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  W_IGNORE_UNUSED(inout_stream);
}

void WComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  W_IGNORE_UNUSED(inout_stream);
}

void WComponent::EnsureInitialized()
{
  W_ASSERT_DEV(m_pOwner != nullptr, "Owner must not be null");

  if (IsInitializing())
  {
    WLog::Error("Recursive initialize call is ignored.");
    return;
  }

  if (!IsInitialized())
  {
    m_pMessageDispatchType = GetDynamicRTTI();

    m_ComponentFlags.Add(WObjectFlags::Initializing);

    Initialize();

    m_ComponentFlags.Remove(WObjectFlags::Initializing);
    m_ComponentFlags.Add(WObjectFlags::Initialized);
  }
}

void WComponent::EnsureSimulationStarted()
{
  W_ASSERT_DEV(IsActiveAndInitialized(), "Must not be called on uninitialized or inactive components.");
  W_ASSERT_DEV(GetWorld()->GetWorldSimulationEnabled(), "Must not be called when the world is not simulated.");

  if (m_ComponentFlags.IsSet(WObjectFlags::SimulationStarting))
  {
    WLog::Error("Recursive simulation started call is ignored.");
    return;
  }

  if (!IsSimulationStarted())
  {
    m_ComponentFlags.Add(WObjectFlags::SimulationStarting);

    OnSimulationStarted();

    m_ComponentFlags.Remove(WObjectFlags::SimulationStarting);
    m_ComponentFlags.Add(WObjectFlags::SimulationStarted);
  }
}

void WComponent::PostMessage(const WMessage& msg, WTime delay, WObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessage(GetHandle(), msg, delay, queueType);
}

bool WComponent::HandlesMessage(const WMessage& msg) const
{
  return m_pMessageDispatchType->CanHandleMessage(msg.GetId());
}

void WComponent::SetUserFlag(WUInt8 uiFlagIndex, bool bSet)
{
  W_ASSERT_DEBUG(uiFlagIndex < 8, "Flag index {0} is out of the valid range [0 - 7]", uiFlagIndex);

  m_ComponentFlags.AddOrRemove(static_cast<WObjectFlags::Enum>(WObjectFlags::UserFlag0 << uiFlagIndex), bSet);
}

bool WComponent::GetUserFlag(WUInt8 uiFlagIndex) const
{
  W_ASSERT_DEBUG(uiFlagIndex < 8, "Flag index {0} is out of the valid range [0 - 7]", uiFlagIndex);

  return m_ComponentFlags.IsSet(static_cast<WObjectFlags::Enum>(WObjectFlags::UserFlag0 << uiFlagIndex));
}

void WComponent::DeleteComponent()
{
  GetOwningManager()->DeleteComponent(this);
}

void WComponent::Initialize() {}

void WComponent::Deinitialize()
{
  W_ASSERT_DEV(m_pOwner != nullptr, "Owner must still be valid");

  SetActiveFlag(false);
}

void WComponent::OnActivated() {}

void WComponent::OnDeactivated() {}

void WComponent::OnSimulationStarted() {}

void WComponent::EnableUnhandledMessageHandler(bool enable)
{
  m_ComponentFlags.AddOrRemove(WObjectFlags::UnhandledMessageHandler, enable);
}

bool WComponent::OnUnhandledMessage(WMessage& msg, bool bWasPostedMsg)
{
  W_IGNORE_UNUSED(msg);
  W_IGNORE_UNUSED(bWasPostedMsg);
  return false;
}

bool WComponent::OnUnhandledMessage(WMessage& msg, bool bWasPostedMsg) const
{
  W_IGNORE_UNUSED(msg);
  W_IGNORE_UNUSED(bWasPostedMsg);
  return false;
}

void WComponent::UpdateActiveState(bool bOwnerActive)
{
  const bool bSelfActive = bOwnerActive && m_ComponentFlags.IsSet(WObjectFlags::ActiveFlag);

  if (m_ComponentFlags.IsSet(WObjectFlags::ActiveState) != bSelfActive)
  {
    m_ComponentFlags.AddOrRemove(WObjectFlags::ActiveState, bSelfActive);

    if (IsInitialized())
    {
      if (bSelfActive)
      {
        // Don't call OnActivated & EnsureSimulationStarted here since there might be other components
        // that are needed in the OnSimulation callback but are activated right after this component.
        // Instead add the component to the initialization batch again.
        // There initialization will be skipped since the component is already initialized.
        GetWorld()->AddComponentToInitialize(GetHandle());
      }
      else
      {
        OnDeactivated();

        m_ComponentFlags.Remove(WObjectFlags::SimulationStarted);
      }
    }
  }
}

WGameObject* WComponent::Reflection_GetOwner() const
{
  return m_pOwner;
}

WWorld* WComponent::Reflection_GetWorld() const
{
  return m_pManager->GetWorld();
}

void WComponent::Reflection_Update(WTime deltaTime)
{
  W_IGNORE_UNUSED(deltaTime);
  // This is just a dummy function for the scripting reflection
}

bool WComponent::SendMessageInternal(WMessage& msg, bool bWasPostedMsg)
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      WLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment", msg.GetId(),
        GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(WObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg, bWasPostedMsg))
    return true;

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    WLog::Warning("Component type '{0}' does not have a message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

  return false;
}

bool WComponent::SendMessageInternal(WMessage& msg, bool bWasPostedMsg) const
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      WLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment", msg.GetId(),
        GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(WObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg, bWasPostedMsg))
    return true;

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    WLog::Warning(
      "(const) Component type '{0}' does not have a CONST message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

  return false;
}


W_STATICLINK_FILE(Core, Core_World_Implementation_Component);
