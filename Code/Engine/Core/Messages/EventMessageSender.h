#pragma once

#include <Core/World/World.h>
#include <Foundation/Communication/Message.h>

namespace WInternal
{
  struct W_CORE_DLL EventMessageSenderHelper
  {
    static bool SendEventMessage(WMessage& ref_msg, WComponent* pSenderComponent, WGameObject* pSearchObject, WSmallArray<WComponentHandle, 1>& inout_cachedReceivers);
    static bool SendEventMessage(WMessage& ref_msg, const WComponent* pSenderComponent, const WGameObject* pSearchObject, WSmallArray<WComponentHandle, 1>& inout_cachedReceivers);
    static void PostEventMessage(const WMessage& msg, const WComponent* pSenderComponent, const WGameObject* pSearchObject, WSmallArray<WComponentHandle, 1>& inout_cachedReceivers, WTime delay, WObjectMsgQueueType::Enum queueType);
  };
} // namespace WInternal

/// A message sender that sends all messages to the next component derived from WEventMessageHandlerComponent
///   up in the hierarchy starting with the given search object. If none is found the message is sent to
///   all components registered as global event message handler. The receiver is cached after the first send/post call.
template <typename EventMessageType>
class WEventMessageSender : public WMessageSenderBase<EventMessageType>
{
public:
  W_ALWAYS_INLINE bool SendEventMessage(EventMessageType& inout_msg, WComponent* pSenderComponent, WGameObject* pSearchObject)
  {
    return WInternal::EventMessageSenderHelper::SendEventMessage(inout_msg, pSenderComponent, pSearchObject, m_CachedReceivers);
  }

  W_ALWAYS_INLINE bool SendEventMessage(EventMessageType& inout_msg, const WComponent* pSenderComponent, const WGameObject* pSearchObject) const
  {
    return WInternal::EventMessageSenderHelper::SendEventMessage(inout_msg, pSenderComponent, pSearchObject, m_CachedReceivers);
  }

  W_ALWAYS_INLINE void PostEventMessage(EventMessageType& ref_msg, WComponent* pSenderComponent, WGameObject* pSearchObject,
    WTime delay, WObjectMsgQueueType::Enum queueType = WObjectMsgQueueType::NextFrame)
  {
    WInternal::EventMessageSenderHelper::PostEventMessage(ref_msg, pSenderComponent, pSearchObject, m_CachedReceivers, delay, queueType);
  }

  W_ALWAYS_INLINE void PostEventMessage(EventMessageType& ref_msg, const WComponent* pSenderComponent, const WGameObject* pSearchObject,
    WTime delay, WObjectMsgQueueType::Enum queueType = WObjectMsgQueueType::NextFrame) const
  {
    WInternal::EventMessageSenderHelper::PostEventMessage(ref_msg, pSenderComponent, pSearchObject, m_CachedReceivers, delay, queueType);
  }

  W_ALWAYS_INLINE void Invalidate()
  {
    m_CachedReceivers.Clear();
    m_CachedReceivers.GetUserData<WUInt32>() = 0;
  }

private:
  mutable WSmallArray<WComponentHandle, 1> m_CachedReceivers;
};
