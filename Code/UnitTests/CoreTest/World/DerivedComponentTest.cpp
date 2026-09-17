#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>

namespace
{
  using TestComponentBaseManager = WComponentManagerSimple<class TestComponentBase, WComponentUpdateType::Always>;

  class TestComponentBase : public WComponent
  {
    W_DECLARE_COMPONENT_TYPE(TestComponentBase, WComponent, TestComponentBaseManager);

  public:
    void Update() { ++s_iUpdateCounter; }

    static int s_iUpdateCounter;
  };

  int TestComponentBase::s_iUpdateCounter = 0;

  W_BEGIN_COMPONENT_TYPE(TestComponentBase, 1, WComponentMode::Static)
  W_END_COMPONENT_TYPE

  //////////////////////////////////////////////////////////////////////////

  using TestComponentDerived1Manager = WComponentManagerSimple<class TestComponentDerived1, WComponentUpdateType::Always>;

  class TestComponentDerived1 : public TestComponentBase
  {
    W_DECLARE_COMPONENT_TYPE(TestComponentDerived1, TestComponentBase, TestComponentDerived1Manager);

  public:
    void Update() { ++s_iUpdateCounter; }

    static int s_iUpdateCounter;
  };

  int TestComponentDerived1::s_iUpdateCounter = 0;

  W_BEGIN_COMPONENT_TYPE(TestComponentDerived1, 1, WComponentMode::Static)
  W_END_COMPONENT_TYPE
} // namespace


W_CREATE_SIMPLE_TEST(World, DerivedComponents)
{
  WWorldDesc worldDesc("Test");
  WWorld world(worldDesc);
  W_LOCK(world.GetWriteMarker());

  TestComponentBaseManager* pManagerBase = world.GetOrCreateComponentManager<TestComponentBaseManager>();
  TestComponentDerived1Manager* pManagerDerived1 = world.GetOrCreateComponentManager<TestComponentDerived1Manager>();

  WGameObjectDesc desc;
  WGameObject* pObject;
  WGameObjectHandle hObject = world.CreateObject(desc, pObject);
  W_TEST_BOOL(!hObject.IsInvalidated());

  WGameObject* pObject2;
  world.CreateObject(desc, pObject2);

  TestComponentBase::s_iUpdateCounter = 0;
  TestComponentDerived1::s_iUpdateCounter = 0;

  W_TEST_BLOCK(WTestBlock::Enabled, "Derived Component Update")
  {
    TestComponentBase* pComponentBase = nullptr;
    WTypedComponentHandle<TestComponentBase> hComponentBase = TestComponentBase::CreateComponent(pObject, pComponentBase);

    TestComponentBase* pTestBase = nullptr;
    W_TEST_BOOL(world.TryGetComponent(hComponentBase, pTestBase));
    W_TEST_BOOL(pTestBase == pComponentBase);
    W_TEST_BOOL(pComponentBase->GetHandle() == hComponentBase);
    W_TEST_BOOL(pComponentBase->GetOwningManager() == pManagerBase);

    TestComponentDerived1* pComponentDerived1 = nullptr;
    WTypedComponentHandle<TestComponentDerived1> hComponentDerived1 = TestComponentDerived1::CreateComponent(pObject2, pComponentDerived1);

    TestComponentDerived1* pTestDerived1 = nullptr;
    W_TEST_BOOL(world.TryGetComponent(hComponentDerived1, pTestDerived1));
    W_TEST_BOOL(pTestDerived1 == pComponentDerived1);
    W_TEST_BOOL(pComponentDerived1->GetHandle() == hComponentDerived1);
    W_TEST_BOOL(pComponentDerived1->GetOwningManager() == pManagerDerived1);

    world.Update();

    W_TEST_INT(TestComponentBase::s_iUpdateCounter, 1);
    W_TEST_INT(TestComponentDerived1::s_iUpdateCounter, 1);

    // Get component manager via rtti
    W_TEST_BOOL(world.GetManagerForComponentType(WGetStaticRTTI<TestComponentBase>()) == pManagerBase);
    W_TEST_BOOL(world.GetManagerForComponentType(WGetStaticRTTI<TestComponentDerived1>()) == pManagerDerived1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Derived Component TypedComponentHandle Assign")
  {
    TestComponentDerived1* pComponentDerived1 = nullptr;
    WTypedComponentHandle<TestComponentBase> hComponentBase(TestComponentDerived1::CreateComponent(pObject2, pComponentDerived1));

    TestComponentDerived1* pTestDerived1 = nullptr;
    W_TEST_BOOL(world.TryGetComponent(hComponentBase, pTestDerived1));
    W_TEST_BOOL(pTestDerived1 == pComponentDerived1);
    W_TEST_BOOL(pComponentDerived1->GetHandle() == hComponentBase);
    W_TEST_BOOL(pComponentDerived1->GetOwningManager() == pManagerDerived1);

    // Assignment test Derived -> Base
    hComponentBase = pTestDerived1->GetHandle();
    W_TEST_BOOL(world.TryGetComponent(hComponentBase, pTestDerived1));
    W_TEST_BOOL(pComponentDerived1->GetHandle() == hComponentBase);
    W_TEST_BOOL(pComponentDerived1->GetOwningManager() == pManagerDerived1);
  }
}
