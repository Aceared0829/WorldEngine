#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/StateMachine/StateMachineComponent.h>
#include <RendererCore/Components/BlackboardComponent.h>

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgStateMachineStateChanged);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgStateMachineStateChanged, 1, WRTTIDefaultAllocator<WMsgStateMachineStateChanged>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("OldStateName", GetOldStateName, SetOldStateName),
    W_ACCESSOR_PROPERTY("NewStateName", GetNewStateName, SetNewStateName),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineState_SendMsg, 1, WRTTIDefaultAllocator<WStateMachineState_SendMsg>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MessageDelay", m_MessageDelay),
    W_MEMBER_PROPERTY("SendMessageOnEnter", m_bSendMessageOnEnter)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("SendMessageOnExit", m_bSendMessageOnExit),
    W_MEMBER_PROPERTY("LogOnEnter", m_bLogOnEnter),
    W_MEMBER_PROPERTY("LogOnExit", m_bLogOnExit),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineState_SendMsg::WStateMachineState_SendMsg(WStringView sName)
  : WStateMachineState(sName)
{
}

WStateMachineState_SendMsg::~WStateMachineState_SendMsg() = default;

void WStateMachineState_SendMsg::OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const
{
  WHashedString sFromState = (pFromState != nullptr) ? pFromState->GetNameHashed() : WHashedString();

  if (m_bSendMessageOnEnter)
  {
    if (auto pOwner = WDynamicCast<WStateMachineComponent*>(&ref_instance.GetOwner()))
    {
      WMsgStateMachineStateChanged msg;
      msg.m_sOldStateName = sFromState;
      msg.m_sNewStateName = GetNameHashed();

      pOwner->SendStateChangedMsg(msg, m_MessageDelay);
    }
  }

  if (m_bLogOnEnter)
  {
    WLog::Info("State Machine: Entering '{}' State from '{}'", GetNameHashed(), sFromState);
  }
}

void WStateMachineState_SendMsg::OnExit(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pToState) const
{
  WHashedString sToState = (pToState != nullptr) ? pToState->GetNameHashed() : WHashedString();

  if (m_bSendMessageOnExit)
  {
    if (auto pOwner = WDynamicCast<WStateMachineComponent*>(&ref_instance.GetOwner()))
    {
      WMsgStateMachineStateChanged msg;
      msg.m_sOldStateName = GetNameHashed();
      msg.m_sNewStateName = sToState;

      pOwner->SendStateChangedMsg(msg, m_MessageDelay);
    }
  }

  if (m_bLogOnExit)
  {
    WLog::Info("State Machine: Exiting '{}' State to '{}'", GetNameHashed(), sToState);
  }
}

WResult WStateMachineState_SendMsg::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_MessageDelay;
  inout_stream << m_bSendMessageOnEnter;
  inout_stream << m_bSendMessageOnExit;
  inout_stream << m_bLogOnEnter;
  inout_stream << m_bLogOnExit;
  return W_SUCCESS;
}

WResult WStateMachineState_SendMsg::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_MessageDelay;
  inout_stream >> m_bSendMessageOnEnter;
  inout_stream >> m_bSendMessageOnExit;
  inout_stream >> m_bLogOnEnter;
  inout_stream >> m_bLogOnExit;
  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineState_SwitchObject, 1, WRTTIDefaultAllocator<WStateMachineState_SwitchObject>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("PathToGroup", m_sGroupPath),
    W_MEMBER_PROPERTY("ObjectToEnable", m_sObjectToEnable),
    W_MEMBER_PROPERTY("DeactivateOthers", m_bDeactivateOthers)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineState_SwitchObject::WStateMachineState_SwitchObject(WStringView sName)
  : WStateMachineState(sName)
{
}

WStateMachineState_SwitchObject::~WStateMachineState_SwitchObject() = default;

void WStateMachineState_SwitchObject::OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const
{
  if (auto pOwner = WDynamicCast<WStateMachineComponent*>(&ref_instance.GetOwner()))
  {
    if (WGameObject* pOwnerGO = pOwner->GetOwner()->FindChildByPath(m_sGroupPath))
    {
      pOwnerGO->ActivateChildByName(WTempHashedString(m_sObjectToEnable), m_bDeactivateOthers);
    }
  }
}

WResult WStateMachineState_SwitchObject::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sGroupPath;
  inout_stream << m_sObjectToEnable;
  inout_stream << m_bDeactivateOthers;
  return W_SUCCESS;
}

WResult WStateMachineState_SwitchObject::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_sGroupPath;
  inout_stream >> m_sObjectToEnable;
  inout_stream >> m_bDeactivateOthers;
  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

WStateMachineComponentManager::WStateMachineComponentManager(WWorld* pWorld)
  : WComponentManager<ComponentType, WBlockStorageType::Compact>(pWorld)
{
  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WStateMachineComponentManager::ResourceEventHandler, this));
}

WStateMachineComponentManager::~WStateMachineComponentManager()
{
  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WStateMachineComponentManager::ResourceEventHandler, this));
}

void WStateMachineComponentManager::Initialize()
{
  auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WStateMachineComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void WStateMachineComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  // reload
  {
    for (auto hComponent : m_ComponentsToReload)
    {
      WStateMachineComponent* pComponent = nullptr;
      if (TryGetComponent(hComponent, pComponent) && pComponent->IsActive())
      {
        pComponent->InstantiateStateMachine();
      }
    }
    m_ComponentsToReload.Clear();
  }

  // update
  if (GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      ComponentType* pComponent = it;
      if (pComponent->IsActiveAndSimulating())
      {
        pComponent->Update();
      }
    }
  }
}

void WStateMachineComponentManager::ResourceEventHandler(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<WStateMachineResource>())
  {
    WStateMachineResourceHandle hResource((WStateMachineResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource)
      {
        m_ComponentsToReload.Insert(it->GetHandle());
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WStateMachineComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Resource", GetResource, SetResource)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_StateMachine", WDependencyFlags::Package), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("InitialState", GetInitialState, SetInitialState),
    W_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardNamesEnum")),
  }
  W_END_PROPERTIES;

  W_BEGIN_MESSAGESENDERS
  {
    W_MESSAGE_SENDER(m_StateChangedSender)
  }
  W_END_MESSAGESENDERS;

  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetState, In, "Name"),
    W_SCRIPT_FUNCTION_PROPERTY(GetCurrentState),
    W_SCRIPT_FUNCTION_PROPERTY(FireTransitionEvent, In, "Name"),
  }
  W_END_FUNCTIONS;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
  }
  W_END_ATTRIBUTES;
}

W_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

WStateMachineComponent::WStateMachineComponent() = default;
WStateMachineComponent::WStateMachineComponent(WStateMachineComponent&& other) = default;
WStateMachineComponent::~WStateMachineComponent() = default;
WStateMachineComponent& WStateMachineComponent::operator=(WStateMachineComponent&& other) = default;

void WStateMachineComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s << m_sInitialState;
  s << m_sBlackboardName;
}

void WStateMachineComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  s >> m_sInitialState;

  if (uiVersion >= 2)
  {
    s >> m_sBlackboardName;
  }
}

void WStateMachineComponent::OnActivated()
{
  SUPER::OnActivated();

  InstantiateStateMachine();
}

void WStateMachineComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  m_pStateMachineInstance = nullptr;
}

void WStateMachineComponent::SetResource(const WStateMachineResourceHandle& hResource)
{
  if (m_hResource == hResource)
    return;

  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    InstantiateStateMachine();
  }
}

void WStateMachineComponent::SetInitialState(const char* szName)
{
  WHashedString sInitialState;
  sInitialState.Assign(szName);

  if (m_sInitialState == sInitialState)
    return;

  m_sInitialState = std::move(sInitialState);

  if (IsActiveAndInitialized())
  {
    InstantiateStateMachine();
  }
}

void WStateMachineComponent::SetBlackboardName(const char* szName)
{
  WHashedString sBlackboardName;
  sBlackboardName.Assign(szName);

  if (m_sBlackboardName == sBlackboardName)
    return;

  m_sBlackboardName = std::move(sBlackboardName);

  if (IsActiveAndInitialized())
  {
    InstantiateStateMachine();
  }
}

bool WStateMachineComponent::SetState(WStringView sName)
{
  if (m_pStateMachineInstance != nullptr)
  {
    WHashedString sStateName;
    sStateName.Assign(sName);

    return m_pStateMachineInstance->SetState(sStateName).Succeeded();
  }

  return false;
}

WStringView WStateMachineComponent::GetCurrentState() const
{
  if (m_pStateMachineInstance != nullptr && m_pStateMachineInstance->GetCurrentState())
  {
    return m_pStateMachineInstance->GetCurrentState()->GetName();
  }

  return {};
}

void WStateMachineComponent::FireTransitionEvent(WStringView sEvent)
{
  if (m_pStateMachineInstance != nullptr)
  {
    m_pStateMachineInstance->FireTransitionEvent(sEvent);
  }
}

void WStateMachineComponent::SendStateChangedMsg(WMsgStateMachineStateChanged& msg, WTime delay)
{
  if (delay > WTime::MakeZero())
  {
    m_StateChangedSender.PostEventMessage(msg, this, GetOwner(), delay, WObjectMsgQueueType::NextFrame);
  }
  else
  {
    m_StateChangedSender.SendEventMessage(msg, this, GetOwner());
  }
}

void WStateMachineComponent::InstantiateStateMachine()
{
  m_pStateMachineInstance = nullptr;

  if (m_hResource.IsValid() == false)
    return;

  WResourceLock<WStateMachineResource> pStateMachineResource(m_hResource, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pStateMachineResource.GetAcquireResult() != WResourceAcquireResult::Final)
  {
    WLog::Error("Failed to load state machine '{}'", GetResource().GetResourceID());
    return;
  }

  m_pStateMachineInstance = pStateMachineResource->CreateInstance(*this);
  m_pStateMachineInstance->SetBlackboard(WBlackboardComponent::FindBlackboard(*GetOwner(), m_sBlackboardName.GetView()));
  m_pStateMachineInstance->SetStateOrFallback(m_sInitialState).IgnoreResult();
}

void WStateMachineComponent::Update()
{
  if (m_pStateMachineInstance != nullptr)
  {
    m_pStateMachineInstance->Update(GetWorld()->GetClock().GetTimeDiff());
  }
}


W_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineComponent);
