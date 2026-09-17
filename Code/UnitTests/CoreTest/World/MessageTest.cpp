#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Time/Clock.h>

namespace
{
  struct WMsgTest : public WMessage
  {
    W_DECLARE_MESSAGE_TYPE(WMsgTest, WMessage);
  };

  // clang-format off
  W_IMPLEMENT_MESSAGE_TYPE(WMsgTest);
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgTest, 1, WRTTIDefaultAllocator<WMsgTest>)
  W_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  struct TestMessage1 : public WMsgTest
  {
    W_DECLARE_MESSAGE_TYPE(TestMessage1, WMsgTest);

    int m_iValue;
  };

  struct TestMessage2 : public WMsgTest
  {
    W_DECLARE_MESSAGE_TYPE(TestMessage2, WMsgTest);

    virtual WInt32 GetSortingKey() const override { return 2; }

    int m_iValue;
  };

  // clang-format off
  W_IMPLEMENT_MESSAGE_TYPE(TestMessage1);
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage1, 1, WRTTIDefaultAllocator<TestMessage1>)
  W_END_DYNAMIC_REFLECTED_TYPE;

  W_IMPLEMENT_MESSAGE_TYPE(TestMessage2);
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage2, 1, WRTTIDefaultAllocator<TestMessage2>)
  W_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  class TestComponentMsg;
  using TestComponentMsgManager = WComponentManager<TestComponentMsg, WBlockStorageType::FreeList>;

  class TestComponentMsg : public WComponent
  {
    W_DECLARE_COMPONENT_TYPE(TestComponentMsg, WComponent, TestComponentMsgManager);

  public:
    TestComponentMsg()

      = default;
    ~TestComponentMsg() = default;

    virtual void SerializeComponent(WWorldWriter& inout_stream) const override {}
    virtual void DeserializeComponent(WWorldReader& inout_stream) override {}

    void OnTestMessage(TestMessage1& ref_msg) { m_iSomeData += ref_msg.m_iValue; }

    void OnTestMessage2(TestMessage2& ref_msg) { m_iSomeData2 += 2 * ref_msg.m_iValue; }

    WInt32 m_iSomeData = 1;
    WInt32 m_iSomeData2 = 2;
  };

  // clang-format off
  W_BEGIN_COMPONENT_TYPE(TestComponentMsg, 1, WComponentMode::Static)
  {
    W_BEGIN_MESSAGEHANDLERS
    {
      W_MESSAGE_HANDLER(TestMessage1, OnTestMessage),
      W_MESSAGE_HANDLER(TestMessage2, OnTestMessage2),
    }
    W_END_MESSAGEHANDLERS;
  }
  W_END_COMPONENT_TYPE;
  // clang-format on

  void ResetComponents(WGameObject& ref_object)
  {
    TestComponentMsg* pComponent = nullptr;
    if (ref_object.TryGetComponentOfBaseType(pComponent))
    {
      pComponent->m_iSomeData = 1;
      pComponent->m_iSomeData2 = 2;
    }

    for (auto it = ref_object.GetChildren(); it.IsValid(); ++it)
    {
      ResetComponents(*it);
    }
  }
} // namespace

W_CREATE_SIMPLE_TEST(World, Messaging)
{
  WWorldDesc worldDesc("Test");
  WWorld world(worldDesc);
  W_LOCK(world.GetWriteMarker());

  TestComponentMsgManager* pManager = world.GetOrCreateComponentManager<TestComponentMsgManager>();

  WGameObjectDesc desc;
  desc.m_sName.Assign("Root");
  WGameObject* pRoot = nullptr;
  world.CreateObject(desc, pRoot);
  TestComponentMsg* pComponent = nullptr;
  pManager->CreateComponent(pRoot, pComponent);

  WGameObject* pParents[2];
  desc.m_hParent = pRoot->GetHandle();
  desc.m_sName.Assign("Parent1");
  world.CreateObject(desc, pParents[0]);
  pManager->CreateComponent(pParents[0], pComponent);

  desc.m_sName.Assign("Parent2");
  world.CreateObject(desc, pParents[1]);
  pManager->CreateComponent(pParents[1], pComponent);

  for (WUInt32 i = 0; i < 2; ++i)
  {
    desc.m_hParent = pParents[i]->GetHandle();
    for (WUInt32 j = 0; j < 4; ++j)
    {
      WStringBuilder sb;
      sb.AppendFormat("Parent{0}_Child{1}", i + 1, j + 1);
      desc.m_sName.Assign(sb.GetData());

      WGameObject* pObject = nullptr;
      world.CreateObject(desc, pObject);
      pManager->CreateComponent(pObject, pComponent);
    }
  }

  // one update step so components are initialized
  world.Update();

  W_TEST_BLOCK(WTestBlock::Enabled, "Direct Routing")
  {
    ResetComponents(*pRoot);

    TestMessage1 msg;
    msg.m_iValue = 4;
    pParents[0]->SendMessage(msg);

    TestMessage2 msg2;
    msg2.m_iValue = 4;
    pParents[0]->SendMessage(msg2);

    TestComponentMsg* pComponent2 = nullptr;
    W_TEST_BOOL(pParents[0]->TryGetComponentOfBaseType(pComponent2));
    W_TEST_INT(pComponent2->m_iSomeData, 5);
    W_TEST_INT(pComponent2->m_iSomeData2, 10);

    // siblings, parent and children should not be affected
    W_TEST_BOOL(pParents[1]->TryGetComponentOfBaseType(pComponent2));
    W_TEST_INT(pComponent2->m_iSomeData, 1);
    W_TEST_INT(pComponent2->m_iSomeData2, 2);

    W_TEST_BOOL(pRoot->TryGetComponentOfBaseType(pComponent2));
    W_TEST_INT(pComponent2->m_iSomeData, 1);
    W_TEST_INT(pComponent2->m_iSomeData2, 2);

    for (auto it = pParents[0]->GetChildren(); it.IsValid(); ++it)
    {
      W_TEST_BOOL(it->TryGetComponentOfBaseType(pComponent2));
      W_TEST_INT(pComponent2->m_iSomeData, 1);
      W_TEST_INT(pComponent2->m_iSomeData2, 2);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Queuing")
  {
    ResetComponents(*pRoot);

    for (WUInt32 i = 0; i < 10; ++i)
    {
      TestMessage1 msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, WTime::MakeZero(), WObjectMsgQueueType::NextFrame);

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, WTime::MakeZero(), WObjectMsgQueueType::NextFrame);
    }

    world.Update();

    TestComponentMsg* pComponent2 = nullptr;
    W_TEST_BOOL(pRoot->TryGetComponentOfBaseType(pComponent2));
    W_TEST_INT(pComponent2->m_iSomeData, 46);
    W_TEST_INT(pComponent2->m_iSomeData2, 92);

    WFrameAllocator::Reset();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Queuing with delay")
  {
    ResetComponents(*pRoot);

    for (WUInt32 i = 0; i < 10; ++i)
    {
      TestMessage1 msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, WTime::MakeFromSeconds(i + 1));

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, WTime::MakeFromSeconds(i + 1));
    }

    world.GetClock().SetFixedTimeStep(WTime::MakeFromSeconds(1.001f));

    int iDesiredValue = 1;
    int iDesiredValue2 = 2;

    for (WUInt32 i = 0; i < 10; ++i)
    {
      iDesiredValue += i;
      iDesiredValue2 += i * 2;

      world.Update();

      TestComponentMsg* pComponent2 = nullptr;
      W_TEST_BOOL(pRoot->TryGetComponentOfBaseType(pComponent2));
      W_TEST_INT(pComponent2->m_iSomeData, iDesiredValue);
      W_TEST_INT(pComponent2->m_iSomeData2, iDesiredValue2);
    }

    WFrameAllocator::Reset();
  }
}
