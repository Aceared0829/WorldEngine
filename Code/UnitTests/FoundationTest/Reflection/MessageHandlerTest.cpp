#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Reflection/Reflection.h>

#ifdef GetMessage
#  undef GetMessage
#endif

namespace
{

  struct WMsgTest : public WMessage
  {
    W_DECLARE_MESSAGE_TYPE(WMsgTest, WMessage);
  };

  W_IMPLEMENT_MESSAGE_TYPE(WMsgTest);
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgTest, 1, WRTTIDefaultAllocator<WMsgTest>)
  W_END_DYNAMIC_REFLECTED_TYPE;

  struct AddMessage : public WMsgTest
  {
    W_DECLARE_MESSAGE_TYPE(AddMessage, WMsgTest);

    WInt32 m_iValue;
  };
  W_IMPLEMENT_MESSAGE_TYPE(AddMessage);
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(AddMessage, 1, WRTTIDefaultAllocator<AddMessage>)
  W_END_DYNAMIC_REFLECTED_TYPE;

  struct SubMessage : public WMsgTest
  {
    W_DECLARE_MESSAGE_TYPE(SubMessage, WMsgTest);

    WInt32 m_iValue;
  };
  W_IMPLEMENT_MESSAGE_TYPE(SubMessage);
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(SubMessage, 1, WRTTIDefaultAllocator<SubMessage>)
  W_END_DYNAMIC_REFLECTED_TYPE;

  struct MulMessage : public WMsgTest
  {
    W_DECLARE_MESSAGE_TYPE(MulMessage, WMsgTest);

    WInt32 m_iValue;
  };
  W_IMPLEMENT_MESSAGE_TYPE(MulMessage);
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(MulMessage, 1, WRTTIDefaultAllocator<MulMessage>)
  W_END_DYNAMIC_REFLECTED_TYPE;

  struct GetMessage : public WMsgTest
  {
    W_DECLARE_MESSAGE_TYPE(GetMessage, WMsgTest);

    WInt32 m_iValue;
  };
  W_IMPLEMENT_MESSAGE_TYPE(GetMessage);
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(GetMessage, 1, WRTTIDefaultAllocator<GetMessage>)
  W_END_DYNAMIC_REFLECTED_TYPE;
} // namespace

class BaseHandler : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(BaseHandler, WReflectedClass);

public:
  BaseHandler()

    = default;

  void OnAddMessage(AddMessage& ref_msg) { m_iValue += ref_msg.m_iValue; }

  void OnMulMessage(MulMessage& ref_msg) { m_iValue *= ref_msg.m_iValue; }

  void OnGetMessage(GetMessage& ref_msg) const { ref_msg.m_iValue = m_iValue; }

  WInt32 m_iValue = 0;
};

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(BaseHandler, 1, WRTTINoAllocator)
{
  W_BEGIN_MESSAGEHANDLERS{
      W_MESSAGE_HANDLER(AddMessage, OnAddMessage),
      W_MESSAGE_HANDLER(MulMessage, OnMulMessage),
      W_MESSAGE_HANDLER(GetMessage, OnGetMessage),
  } W_END_MESSAGEHANDLERS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class DerivedHandler : public BaseHandler
{
  W_ADD_DYNAMIC_REFLECTION(DerivedHandler, BaseHandler);

public:
  void OnAddMessage(AddMessage& ref_msg) { m_iValue += ref_msg.m_iValue * 2; }

  void OnSubMessage(SubMessage& ref_msg) { m_iValue -= ref_msg.m_iValue; }
};

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(DerivedHandler, 1, WRTTINoAllocator)
{
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(AddMessage, OnAddMessage),
    W_MESSAGE_HANDLER(SubMessage, OnSubMessage),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

W_CREATE_SIMPLE_TEST(Reflection, MessageHandler)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Simple Dispatch")
  {
    BaseHandler test;
    const WRTTI* pRTTI = test.GetStaticRTTI();

    W_TEST_BOOL(pRTTI->CanHandleMessage<AddMessage>());
    W_TEST_BOOL(!pRTTI->CanHandleMessage<SubMessage>());
    W_TEST_BOOL(pRTTI->CanHandleMessage<MulMessage>());
    W_TEST_BOOL(pRTTI->CanHandleMessage<GetMessage>());

    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled = pRTTI->DispatchMessage(&test, addMsg);
    W_TEST_BOOL(handled);

    W_TEST_INT(test.m_iValue, 4);

    SubMessage subMsg;
    subMsg.m_iValue = 4;
    handled = pRTTI->DispatchMessage(&test, subMsg); // should do nothing
    W_TEST_BOOL(!handled);

    W_TEST_INT(test.m_iValue, 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Simple Dispatch const")
  {
    const BaseHandler test;
    const WRTTI* pRTTI = test.GetStaticRTTI();

    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled = pRTTI->DispatchMessage(&test, addMsg);
    W_TEST_BOOL(!handled); // should do nothing since object is const and the add message handler is non-const

    W_TEST_INT(test.m_iValue, 0);

    GetMessage getMsg;
    getMsg.m_iValue = 12;
    handled = pRTTI->DispatchMessage(&test, getMsg);
    W_TEST_BOOL(handled);
    W_TEST_INT(getMsg.m_iValue, 0);

    W_TEST_INT(test.m_iValue, 0); // object must not be modified
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Dispatch with inheritance")
  {
    DerivedHandler test;
    const WRTTI* pRTTI = test.GetStaticRTTI();

    W_TEST_BOOL(pRTTI->CanHandleMessage<AddMessage>());
    W_TEST_BOOL(pRTTI->CanHandleMessage<SubMessage>());
    W_TEST_BOOL(pRTTI->CanHandleMessage<MulMessage>());

    // message handler overridden by derived class
    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled = pRTTI->DispatchMessage(&test, addMsg);
    W_TEST_BOOL(handled);

    W_TEST_INT(test.m_iValue, 8);

    SubMessage subMsg;
    subMsg.m_iValue = 4;
    handled = pRTTI->DispatchMessage(&test, subMsg);
    W_TEST_BOOL(handled);

    W_TEST_INT(test.m_iValue, 4);

    // message handled by base class
    MulMessage mulMsg;
    mulMsg.m_iValue = 4;
    handled = pRTTI->DispatchMessage(&test, mulMsg);
    W_TEST_BOOL(handled);

    W_TEST_INT(test.m_iValue, 16);
  }
}
