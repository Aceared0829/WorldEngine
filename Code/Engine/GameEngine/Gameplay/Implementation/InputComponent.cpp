#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/InputComponent.h>
#include <RendererCore/Components/BlackboardComponent.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WInputMessageGranularity, 1)
  W_ENUM_CONSTANT(WInputMessageGranularity::PressOnly),
  W_ENUM_CONSTANT(WInputMessageGranularity::PressAndRelease),
  W_ENUM_CONSTANT(WInputMessageGranularity::PressReleaseAndDown),
W_END_STATIC_REFLECTED_ENUM;

W_IMPLEMENT_MESSAGE_TYPE(WMsgInputActionTriggered);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgInputActionTriggered, 1, WRTTIDefaultAllocator<WMsgInputActionTriggered>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("InputAction", GetInputAction, SetInputAction),
    W_MEMBER_PROPERTY("KeyPressValue", m_fKeyPressValue),
    W_ENUM_MEMBER_PROPERTY("TriggerState", WTriggerState, m_TriggerState),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WInputComponent, 3, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("InputSet", m_sInputSet)->AddAttributes(new WDynamicStringEnumAttribute("InputSet")),
    W_ENUM_MEMBER_PROPERTY("Granularity", WInputMessageGranularity, m_Granularity),
    W_MEMBER_PROPERTY("ForwardToBlackboard", m_bForwardToBlackboard),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Input"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGESENDERS
  {
    W_MESSAGE_SENDER(m_InputEventSender)
  }
  W_END_MESSAGESENDERS;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(GetCurrentInputState, In, "InputAction", In, "OnlyKeyPressed"),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WInputComponent::WInputComponent() = default;
WInputComponent::~WInputComponent() = default;

static inline WTriggerState::Enum ToTriggerState(WKeyState::Enum s)
{
  switch (s)
  {
    case WKeyState::Pressed:
      return WTriggerState::Activated;

    case WKeyState::Released:
      return WTriggerState::Deactivated;

    default:
      return WTriggerState::Continuing;
  }
}

void WInputComponent::Update()
{
  if (m_sInputSet.IsEmpty())
    return;

  WTempHybridArray<WString, 32> AllActions;
  WInputManager::GetAllInputActions(m_sInputSet, AllActions);

  WMsgInputActionTriggered msg;

  WBlackboard* pBlackboard = m_bForwardToBlackboard ? WBlackboardComponent::FindBlackboard(*GetOwner()) : nullptr;

  for (const WString& actionName : AllActions)
  {
    float fValue = 0.0f;
    const WKeyState::Enum state = WInputManager::GetInputActionState(m_sInputSet, actionName, &fValue);

    if (pBlackboard)
    {
      pBlackboard->SetEntryValue(actionName, fValue);
    }

    if (state == WKeyState::Up)
      continue;
    if (state == WKeyState::Down && m_Granularity < WInputMessageGranularity::PressReleaseAndDown)
      continue;
    if (state == WKeyState::Released && m_Granularity == WInputMessageGranularity::PressOnly)
      continue;

    msg.m_TriggerState = ToTriggerState(state);
    msg.m_sInputAction.Assign(actionName);
    msg.m_fKeyPressValue = fValue;

    m_InputEventSender.SendEventMessage(msg, this, GetOwner());
  }
}

float WInputComponent::GetCurrentInputState(const char* szInputAction, bool bOnlyKeyPressed /*= false*/) const
{
  if (!IsActiveAndSimulating())
    return 0;

  if (m_sInputSet.IsEmpty())
    return 0;

  float fValue = 0.0f;
  const WKeyState::Enum state = WInputManager::GetInputActionState(m_sInputSet, szInputAction, &fValue);

  if (bOnlyKeyPressed && state != WKeyState::Pressed)
    return 0;

  return fValue;
}

void WInputComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sInputSet;
  s << m_Granularity;

  // version 3
  s << m_bForwardToBlackboard;
}

void WInputComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();


  s >> m_sInputSet;
  s >> m_Granularity;

  if (uiVersion >= 3)
  {
    s >> m_bForwardToBlackboard;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WInputComponentPatch_1_2 : public WGraphPatch
{
public:
  WInputComponentPatch_1_2()
    : WGraphPatch("WInputComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Input Set", "InputSet");
  }
};

WInputComponentPatch_1_2 g_WInputComponentPatch_1_2;



W_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_InputComponent);
