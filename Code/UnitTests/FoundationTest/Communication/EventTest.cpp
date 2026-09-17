#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/Event.h>

W_CREATE_SIMPLE_TEST_GROUP(Communication);

namespace
{
  struct Test
  {
    void DoStuff(WInt32* pEventData) { *pEventData += m_iData; }

    WInt32 m_iData;
  };

  struct TestRecursion
  {
    TestRecursion() { m_uiRecursionCount = 0; }
    void DoStuff(WUInt32 uiRecursions)
    {
      if (m_uiRecursionCount < uiRecursions)
      {
        m_uiRecursionCount++;
        m_Event.Broadcast(uiRecursions, 10);
      }
    }

    using Event = WEvent<WUInt32>;
    Event m_Event;
    WUInt32 m_uiRecursionCount;
  };
} // namespace

W_CREATE_SIMPLE_TEST(Communication, Event)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Basics")
  {
    using TestEvent = WEvent<WInt32*>;
    TestEvent e;

    Test test1;
    test1.m_iData = 3;

    Test test2;
    test2.m_iData = 5;

    WInt32 iResult = 0;

    e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    W_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    iResult = 0;
    e.Broadcast(&iResult);

    W_TEST_INT(iResult, 3);

    e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    W_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    iResult = 0;
    e.Broadcast(&iResult);

    W_TEST_INT(iResult, 8);

    e.RemoveEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    W_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    iResult = 0;
    e.Broadcast(&iResult);

    W_TEST_INT(iResult, 5);

    e.RemoveEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    W_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    iResult = 0;
    e.Broadcast(&iResult);

    W_TEST_INT(iResult, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Unsubscribing via ID")
  {
    using TestEvent = WEvent<WInt32*>;
    TestEvent e;

    Test test1;
    Test test2;

    auto subId1 = e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    W_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    auto subId2 = e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    W_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    e.RemoveEventHandler(subId1);
    W_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    e.RemoveEventHandler(subId2);
    W_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Unsubscribing via Unsubscriber")
  {
    using TestEvent = WEvent<WInt32*>;
    TestEvent e;

    Test test1;
    Test test2;

    {
      TestEvent::Unsubscriber unsub1;

      {
        TestEvent::Unsubscriber unsub2;

        e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1), unsub1);
        W_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

        e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2), unsub2);
        W_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
      }

      W_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
    }

    W_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Recursion")
  {
    for (WUInt32 i = 0; i < 10; i++)
    {
      TestRecursion test;
      test.m_Event.AddEventHandler(TestRecursion::Event::Handler(&TestRecursion::DoStuff, &test));
      test.m_Event.Broadcast(i, 10);
      W_TEST_INT(test.m_uiRecursionCount, i);
      test.m_Event.RemoveEventHandler(TestRecursion::Event::Handler(&TestRecursion::DoStuff, &test));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Remove while iterate")
  {
    using TestEvent = WEvent<int, WMutex, WDefaultAllocatorWrapper, WEventType::CopyOnBroadcast>;
    TestEvent e;

    WUInt32 callMap = 0;

    WEventSubscriptionID subscriptions[4] = {};

    subscriptions[0] = e.AddEventHandler(TestEvent::Handler([&](int i)
      { callMap |= W_BIT(0); }));

    subscriptions[1] = e.AddEventHandler(TestEvent::Handler([&](int i)
      {
      callMap |= W_BIT(1);
      e.RemoveEventHandler(subscriptions[1]); }));

    subscriptions[2] = e.AddEventHandler(TestEvent::Handler([&](int i)
      {
      callMap |= W_BIT(2);
      e.RemoveEventHandler(subscriptions[2]);
      e.RemoveEventHandler(subscriptions[3]); }));

    subscriptions[3] = e.AddEventHandler(TestEvent::Handler([&](int i)
      { callMap |= W_BIT(3); }));

    e.Broadcast(0);

    W_TEST_BOOL(callMap == (W_BIT(0) | W_BIT(1) | W_BIT(2) | W_BIT(3)));

    callMap = 0;
    e.Broadcast(0);
    W_TEST_BOOL(callMap == W_BIT(0));

    e.RemoveEventHandler(subscriptions[0]);
  }
}
