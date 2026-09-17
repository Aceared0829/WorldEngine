#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Implementation/RTTI.h>

class WMessage;

/// Base class for all message handlers in the reflection system's message dispatch framework.
///
/// Message handlers allow types to declare methods that can receive and process specific message
/// types through the reflection system. This enables loose coupling and dynamic message routing
/// without requiring direct method calls or explicit dependencies.
///
/// The handler system supports both const and non-const member functions, automatically detecting
/// the appropriate calling convention at compile time. Messages are dispatched through type-safe
/// function pointers with minimal overhead.
///
/// This is typically used internally by the reflection macros and should not be used directly.
/// Instead, use W_BEGIN_MESSAGEHANDLERS / W_MESSAGE_HANDLER / W_END_MESSAGEHANDLERS in
/// your reflected type definitions.
class W_FOUNDATION_DLL WAbstractMessageHandler
{
public:
  virtual ~WAbstractMessageHandler() = default;

  W_ALWAYS_INLINE void operator()(void* pInstance, WMessage& ref_msg) { (*m_DispatchFunc)(this, pInstance, ref_msg); }

  W_FORCE_INLINE void operator()(const void* pInstance, WMessage& ref_msg)
  {
    W_ASSERT_DEV(m_bIsConst, "Calling a non const message handler with a const instance.");
    (*m_ConstDispatchFunc)(this, pInstance, ref_msg);
  }

  W_ALWAYS_INLINE WMessageId GetMessageId() const { return m_Id; }

  W_ALWAYS_INLINE bool IsConst() const { return m_bIsConst; }

protected:
  using DispatchFunc = void (*)(WAbstractMessageHandler* pSelf, void* pInstance, WMessage&);
  using ConstDispatchFunc = void (*)(WAbstractMessageHandler* pSelf, const void* pInstance, WMessage&);

  union
  {
    DispatchFunc m_DispatchFunc = nullptr;
    ConstDispatchFunc m_ConstDispatchFunc;
  };
  WMessageId m_Id = WSmallInvalidIndex;
  bool m_bIsConst = false;
};

struct WMessageSenderInfo
{
  const char* m_szName;
  const WRTTI* m_pMessageType;
};

namespace WInternal
{
  template <typename Class, typename MessageType>
  struct MessageHandlerTraits
  {
    static WCompileTimeTrueType IsConst(void (Class::*)(MessageType&) const);
    static WCompileTimeFalseType IsConst(...);
  };

  template <bool bIsConst>
  struct MessageHandler
  {
    template <typename Class, typename MessageType, void (Class::*Method)(MessageType&)>
    class Impl : public WAbstractMessageHandler
    {
    public:
      Impl()
      {
        m_DispatchFunc = &Dispatch;
        m_Id = MessageType::GetTypeMsgId();
        m_bIsConst = false;
      }

      static void Dispatch(WAbstractMessageHandler* pSelf, void* pInstance, WMessage& ref_msg)
      {
        W_IGNORE_UNUSED(pSelf);
        Class* pTargetInstance = static_cast<Class*>(pInstance);
        (pTargetInstance->*Method)(static_cast<MessageType&>(ref_msg));
      }
    };
  };

  template <>
  struct MessageHandler<true>
  {
    template <typename Class, typename MessageType, void (Class::*Method)(MessageType&) const>
    class Impl : public WAbstractMessageHandler
    {
    public:
      Impl()
      {
        m_ConstDispatchFunc = &Dispatch;
        m_Id = MessageType::GetTypeMsgId();
        m_bIsConst = true;
      }

      /// Casts the given message to the type of this message handler, then passes that to the class instance.
      static void Dispatch(WAbstractMessageHandler* pSelf, const void* pInstance, WMessage& ref_msg)
      {
        W_IGNORE_UNUSED(pSelf);
        const Class* pTargetInstance = static_cast<const Class*>(pInstance);
        (pTargetInstance->*Method)(static_cast<MessageType&>(ref_msg));
      }
    };
  };
} // namespace WInternal

#define W_IS_CONST_MESSAGE_HANDLER(Class, MessageType, Method) \
  (sizeof(WInternal::MessageHandlerTraits<Class, MessageType>::IsConst(Method)) == sizeof(WCompileTimeTrueType))
