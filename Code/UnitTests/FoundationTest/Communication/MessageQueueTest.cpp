#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/MessageQueue.h>

namespace
{
  struct WMsgTest : public WMessage
  {
    W_DECLARE_MESSAGE_TYPE(WMsgTest, WMessage);
  };

  W_IMPLEMENT_MESSAGE_TYPE(WMsgTest);
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgTest, 1, WRTTIDefaultAllocator<WMsgTest>)
  W_END_DYNAMIC_REFLECTED_TYPE;

  struct TestMessage : public WMsgTest
  {
    W_DECLARE_MESSAGE_TYPE(TestMessage, WMsgTest);

    int x;
    int y;
  };

  struct MetaData
  {
    int receiver;
  };

  using TestMessageQueue = WMessageQueue<MetaData>;

  W_IMPLEMENT_MESSAGE_TYPE(TestMessage);
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage, 1, WRTTIDefaultAllocator<TestMessage>)
  W_END_DYNAMIC_REFLECTED_TYPE;
} // namespace

W_CREATE_SIMPLE_TEST(Communication, MessageQueue)
{
  {
    TestMessage msg;
    W_TEST_INT(msg.GetSize(), sizeof(TestMessage));
  }

  TestMessageQueue q;

  W_TEST_BLOCK(WTestBlock::Enabled, "Enqueue")
  {
    for (WUInt32 i = 0; i < 100; ++i)
    {
      TestMessage* pMsg = W_DEFAULT_NEW(TestMessage);
      pMsg->x = rand();
      pMsg->y = rand();

      MetaData md;
      md.receiver = rand() % 10;

      q.Enqueue(pMsg, md);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Sorting")
  {
    struct MessageComparer
    {
      bool Less(const TestMessageQueue::Entry& a, const TestMessageQueue::Entry& b) const
      {
        if (a.m_MetaData.receiver != b.m_MetaData.receiver)
          return a.m_MetaData.receiver < b.m_MetaData.receiver;

        return a.m_pMessage->GetHash() < b.m_pMessage->GetHash();
      }
    };

    q.Sort(MessageComparer());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator[]")
  {
    W_LOCK(q);

    WMessage* pLastMsg = q[0].m_pMessage;
    MetaData lastMd = q[0].m_MetaData;

    for (WUInt32 i = 1; i < q.GetCount(); ++i)
    {
      WMessage* pMsg = q[i].m_pMessage;
      MetaData md = q[i].m_MetaData;

      if (md.receiver == lastMd.receiver)
      {
        W_TEST_BOOL(pMsg->GetHash() >= pLastMsg->GetHash());
      }
      else
      {
        W_TEST_BOOL(md.receiver >= lastMd.receiver);
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Dequeue")
  {
    WMessage* pMsg = nullptr;
    MetaData md;

    while (q.TryDequeue(pMsg, md))
    {
      W_DEFAULT_DELETE(pMsg);
    }
  }
}
