#include <Core/CorePCH.h>

#include <Core/Messages/EventMessageSender.h>
#include <Core/World/World.h>

namespace WInternal
{
  template <typename World, typename GameObject>
  static void UpdateCachedReceivers(const WMessage& msg, World& ref_world, const WComponent* pSenderComponent, GameObject pSearchObject, WSmallArray<WComponentHandle, 1>& inout_cachedReceivers)
  {
    if (inout_cachedReceivers.GetUserData<WUInt32>() == 0)
    {
      using ComponentType = typename std::conditional<std::is_const<World>::value, const WComponent*, WComponent*>::type;

      WTempHybridArray<ComponentType, 4> eventMsgHandlers;
      ref_world.FindEventMsgHandlers(msg, pSenderComponent, pSearchObject, eventMsgHandlers);

      for (auto pEventMsgHandler : eventMsgHandlers)
      {
        inout_cachedReceivers.PushBack(pEventMsgHandler->GetHandle());
      }

      inout_cachedReceivers.GetUserData<WUInt32>() = 1;
    }
  }

  bool EventMessageSenderHelper::SendEventMessage(WMessage& ref_msg, WComponent* pSenderComponent, WGameObject* pSearchObject, WSmallArray<WComponentHandle, 1>& inout_cachedReceivers)
  {
    W_ASSERT_DEBUG(pSenderComponent != nullptr || pSearchObject != nullptr, "Sender or search object must be valid.");
    WWorld* pWorld = pSenderComponent ? pSenderComponent->GetWorld() : pSearchObject->GetWorld();
    UpdateCachedReceivers(ref_msg, *pWorld, pSenderComponent, pSearchObject, inout_cachedReceivers);

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    bool bResult = false;
    for (auto hReceiver : inout_cachedReceivers)
    {
      WComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        bResult |= pReceiverComponent->SendMessage(ref_msg);
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && ref_msg.GetDebugMessageRouting())
    {
      WLog::Warning("WEventMessageSender::SendMessage: No event message handler found for message of type {0}.", ref_msg.GetId());
    }
#endif

    return bResult;
  }

  bool EventMessageSenderHelper::SendEventMessage(WMessage& ref_msg, const WComponent* pSenderComponent, const WGameObject* pSearchObject, WSmallArray<WComponentHandle, 1>& inout_cachedReceivers)
  {
    W_ASSERT_DEBUG(pSenderComponent != nullptr || pSearchObject != nullptr, "Sender or search object must be valid.");
    const WWorld* pWorld = pSenderComponent ? pSenderComponent->GetWorld() : pSearchObject->GetWorld();
    UpdateCachedReceivers(ref_msg, *pWorld, pSenderComponent, pSearchObject, inout_cachedReceivers);

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    bool bResult = false;
    for (auto hReceiver : inout_cachedReceivers)
    {
      const WComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        bResult |= pReceiverComponent->SendMessage(ref_msg);
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && ref_msg.GetDebugMessageRouting())
    {
      WLog::Warning("WEventMessageSender::SendMessage: No event message handler found for message of type {0}.", ref_msg.GetId());
    }
#endif

    return bResult;
  }

  void EventMessageSenderHelper::PostEventMessage(const WMessage& msg, const WComponent* pSenderComponent, const WGameObject* pSearchObject, WSmallArray<WComponentHandle, 1>& inout_cachedReceivers, WTime delay, WObjectMsgQueueType::Enum queueType)
  {
    W_ASSERT_DEBUG(pSenderComponent != nullptr || pSearchObject != nullptr, "Sender or search object must be valid.");
    const WWorld* pWorld = pSenderComponent ? pSenderComponent->GetWorld() : pSearchObject->GetWorld();
    UpdateCachedReceivers(msg, *pWorld, pSenderComponent, pSearchObject, inout_cachedReceivers);

    if (!inout_cachedReceivers.IsEmpty())
    {
      for (auto hReceiver : inout_cachedReceivers)
      {
        pWorld->PostMessage(hReceiver, msg, delay, queueType);
      }
    }
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    else if (msg.GetDebugMessageRouting())
    {
      WLog::Warning("WEventMessageSender::PostMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

} // namespace WInternal


