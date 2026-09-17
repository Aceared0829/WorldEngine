#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/GameState/ForwardEventsToGameStateComponent.h>
#include <Core/GameState/GameStateBase.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WForwardEventsToGameStateComponent, 1 /* version */, WComponentMode::Static)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WForwardEventsToGameStateComponent::WForwardEventsToGameStateComponent() = default;
WForwardEventsToGameStateComponent::~WForwardEventsToGameStateComponent() = default;

bool WForwardEventsToGameStateComponent::HandlesMessage(const WMessage& msg) const
{
  // check whether there is any active game state
  // if so, test whether it would handle this type of message
  if (WGameStateBase* pGameState = WGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->CanHandleMessage(msg.GetId());
  }

  return false;
}

bool WForwardEventsToGameStateComponent::OnUnhandledMessage(WMessage& msg, bool bWasPostedMsg)
{
  W_IGNORE_UNUSED(bWasPostedMsg);

  // if we have an active game state, forward the message to it
  if (WGameStateBase* pGameState = WGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

bool WForwardEventsToGameStateComponent::OnUnhandledMessage(WMessage& msg, bool bWasPostedMsg) const
{
  W_IGNORE_UNUSED(bWasPostedMsg);

  // if we have an active game state, forward the message to it
  if (const WGameStateBase* pGameState = WGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

void WForwardEventsToGameStateComponent::Initialize()
{
  SUPER::Initialize();

  EnableUnhandledMessageHandler(true);
}


W_STATICLINK_FILE(Core, Core_GameState_Implementation_ForwardEventsToGameStateComponent);
