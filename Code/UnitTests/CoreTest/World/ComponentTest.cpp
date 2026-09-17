#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>

namespace
{
  class TestComponent;
  class TestComponentManager : public WComponentManager<TestComponent, WBlockStorageType::FreeList>
  {
  public:
    TestComponentManager(WWorld* pWorld)
      : WComponentManager<TestComponent, WBlockStorageType::FreeList>(pWorld)
    {
    }

    virtual void Initialize() override
    {
      auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update, this);
      auto desc2 = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update2, this);
      auto desc3 = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update3, this);
      desc3.m_fPriority = 1000.0f;

      auto desc4 = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::AUpdate3, this);
      desc4.m_fPriority = 1000.0f;

      desc.m_DependsOn.PushBack(WMakeHashedString("TestComponentManager::Update2")); // update2 will be called before update
      desc.m_DependsOn.PushBack(WMakeHashedString("TestComponentManager::Update3")); // update3 will be called before update

      auto descAsync = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::UpdateAsync, this);
      descAsync.m_Phase = WWorldUpdatePhase::Async;
      descAsync.m_uiAsyncPhaseBatchSize = 20;

      // Update functions are now registered in reverse order, so we can test whether dependencies work.
      this->RegisterUpdateFunction(descAsync);
      this->RegisterUpdateFunction(desc4);
      this->RegisterUpdateFunction(desc3);
      this->RegisterUpdateFunction(desc2);
      this->RegisterUpdateFunction(desc);
    }

    void Update(const WWorldModule::UpdateContext& context);
    void Update2(const WWorldModule::UpdateContext& context);
    void Update3(const WWorldModule::UpdateContext& context);
    void AUpdate3(const WWorldModule::UpdateContext& context);
    void UpdateAsync(const WWorldModule::UpdateContext& context);
  };

  class TestComponent : public WComponent
  {
    W_DECLARE_COMPONENT_TYPE(TestComponent, WComponent, TestComponentManager);

  public:
    TestComponent() = default;
    ~TestComponent() = default;

    virtual void Initialize() override { ++s_iInitCounter; }
    virtual void Deinitialize() override { --s_iInitCounter; }

    virtual void OnActivated() override
    {
      ++s_iActivateCounter;

      SpawnOther();
    }

    virtual void OnDeactivated() override { --s_iActivateCounter; }

    virtual void OnSimulationStarted() override { ++s_iSimulationStartedCounter; }

    void Update() { m_iSomeData *= 5; }

    void Update2() { m_iSomeData += 3; }

    void SpawnOther();

    WInt32 m_iSomeData = 1;

    static WInt32 s_iInitCounter;
    static WInt32 s_iActivateCounter;
    static WInt32 s_iSimulationStartedCounter;

    static bool s_bSpawnOther;
  };

  WInt32 TestComponent::s_iInitCounter = 0;
  WInt32 TestComponent::s_iActivateCounter = 0;
  WInt32 TestComponent::s_iSimulationStartedCounter = 0;
  bool TestComponent::s_bSpawnOther = false;

  W_BEGIN_COMPONENT_TYPE(TestComponent, 1, WComponentMode::Static)
  W_END_COMPONENT_TYPE

  void TestComponentManager::Update(const WWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }

  void TestComponentManager::Update2(const WWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update2();
    }
  }

  void TestComponentManager::Update3(const WWorldModule::UpdateContext& context) {}

  void TestComponentManager::AUpdate3(const WWorldModule::UpdateContext& context) {}

  void TestComponentManager::UpdateAsync(const WWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }

  using TestComponent2Manager = WComponentManager<class TestComponent2, WBlockStorageType::FreeList>;

  class TestComponent2 : public WComponent
  {
    W_DECLARE_COMPONENT_TYPE(TestComponent2, WComponent, TestComponent2Manager);

    virtual void OnActivated() override { TestComponent::s_iActivateCounter++; }
  };

  W_BEGIN_COMPONENT_TYPE(TestComponent2, 1, WComponentMode::Static)
  W_END_COMPONENT_TYPE

  void TestComponent::SpawnOther()
  {
    if (s_bSpawnOther)
    {
      WGameObjectDesc desc;
      desc.m_hParent = GetOwner()->GetHandle();

      WGameObject* pChild = nullptr;
      GetWorld()->CreateObject(desc, pChild);

      TestComponent2* pChildComponent = nullptr;
      TestComponent2::CreateComponent(pChild, pChildComponent);
    }
  }
} // namespace


W_CREATE_SIMPLE_TEST(World, Components)
{
  WWorldDesc worldDesc("Test");
  WWorld world(worldDesc);
  W_LOCK(world.GetWriteMarker());

  TestComponentManager* pManager = world.GetOrCreateComponentManager<TestComponentManager>();

  WGameObject* pTestObject1;
  WGameObject* pTestObject2;

  {
    WGameObjectDesc desc;
    WGameObjectHandle hObject = world.CreateObject(desc, pTestObject1);
    W_TEST_BOOL(!hObject.IsInvalidated());
    world.CreateObject(desc, pTestObject2);
  }

  TestComponent* pTestComponent = nullptr;

  TestComponent::s_iInitCounter = 0;
  TestComponent::s_iActivateCounter = 0;
  TestComponent::s_iSimulationStartedCounter = 0;
  TestComponent::s_bSpawnOther = false;

  W_TEST_BLOCK(WTestBlock::Enabled, "Component Init")
  {
    // test recursive write lock
    W_LOCK(world.GetWriteMarker());

    WTypedComponentHandle<TestComponent> handle;
    W_TEST_BOOL(!world.TryGetComponent(handle, pTestComponent));

    // Update with no components created
    world.Update();

    handle = TestComponent::CreateComponent(pTestObject1, pTestComponent);

    TestComponent* pTest = nullptr;
    W_TEST_BOOL(world.TryGetComponent(handle, pTest));
    W_TEST_BOOL(pTest == pTestComponent);
    W_TEST_BOOL(pTestComponent->GetHandle() == handle);

    TestComponent2* pTest2 = nullptr;
    W_TEST_BOOL(!world.TryGetComponent(WComponentHandle(handle), pTest2));

    W_TEST_INT(pTestComponent->m_iSomeData, 1);
    W_TEST_INT(TestComponent::s_iInitCounter, 0);

    for (WUInt32 i = 1; i < 100; ++i)
    {
      pManager->CreateComponent(pTestObject2, pTestComponent);
      pTestComponent->m_iSomeData = i + 1;
    }

    W_TEST_INT(pManager->GetComponentCount(), 100);
    W_TEST_INT(TestComponent::s_iInitCounter, 0);

    // Update with components created
    world.Update();

    W_TEST_INT(pManager->GetComponentCount(), 100);
    W_TEST_INT(TestComponent::s_iInitCounter, 100);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Component Update")
  {
    // test recursive read lock
    W_LOCK(world.GetReadMarker());

    world.Update();

    WUInt32 uiCounter = 0;
    for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
    {
      W_TEST_INT(it->m_iSomeData, (((uiCounter + 4) * 25) + 3) * 25);
      ++uiCounter;
    }

    W_TEST_INT(uiCounter, 100);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Delete Component")
  {
    pManager->DeleteComponent(pTestComponent->GetHandle());
    W_TEST_INT(pManager->GetComponentCount(), 99);
    W_TEST_INT(TestComponent::s_iInitCounter, 99);

    // component should also be removed from the game object
    W_TEST_INT(pTestObject2->GetComponents().GetCount(), 98);

    world.DeleteObjectNow(pTestObject2->GetHandle());
    world.Update();

    W_TEST_INT(TestComponent::s_iInitCounter, 1);

    world.DeleteComponentManager<TestComponentManager>();
    pManager = nullptr;
    W_TEST_INT(TestComponent::s_iInitCounter, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Delete Objects with Component")
  {
    WGameObjectDesc desc;

    WGameObject* pObjectA = nullptr;
    WGameObject* pObjectB = nullptr;
    WGameObject* pObjectC = nullptr;

    desc.m_sName.Assign("A");
    WGameObjectHandle hObjectA = world.CreateObject(desc, pObjectA);
    desc.m_sName.Assign("B");
    WGameObjectHandle hObjectB = world.CreateObject(desc, pObjectB);
    desc.m_sName.Assign("C");
    WGameObjectHandle hObjectC = world.CreateObject(desc, pObjectC);

    W_TEST_BOOL(!hObjectA.IsInvalidated());
    W_TEST_BOOL(!hObjectB.IsInvalidated());
    W_TEST_BOOL(!hObjectC.IsInvalidated());

    TestComponent* pComponentA = nullptr;
    TestComponent* pComponentB = nullptr;
    TestComponent* pComponentC = nullptr;

    WTypedComponentHandle<TestComponent> hComponentA = TestComponent::CreateComponent(pObjectA, pComponentA);
    WTypedComponentHandle<TestComponent> hComponentB = TestComponent::CreateComponent(pObjectB, pComponentB);
    WTypedComponentHandle<TestComponent> hComponentC = TestComponent::CreateComponent(pObjectC, pComponentC);

    W_TEST_BOOL(!hComponentA.IsInvalidated());
    W_TEST_BOOL(!hComponentB.IsInvalidated());
    W_TEST_BOOL(!hComponentC.IsInvalidated());

    world.DeleteObjectNow(pObjectB->GetHandle());

    W_TEST_BOOL(pObjectA->IsActive());
    W_TEST_BOOL(pComponentA->IsActive());
    W_TEST_BOOL(pComponentA->GetOwner() == pObjectA);

    W_TEST_BOOL(!pObjectB->IsActive());
    W_TEST_BOOL(!pComponentB->IsActive());
    W_TEST_BOOL(pComponentB->GetOwner() == nullptr);

    W_TEST_BOOL(pObjectC->IsActive());
    W_TEST_BOOL(pComponentC->IsActive());
    W_TEST_BOOL(pComponentC->GetOwner() == pObjectC);

    world.Update();

    W_TEST_BOOL(world.TryGetObject(hObjectA, pObjectA));
    W_TEST_BOOL(world.TryGetObject(hObjectC, pObjectC));

    // Since we're not recompacting storage for components, pointer should still be valid.
    // W_TEST_BOOL(world.TryGetComponent(hComponentA, pComponentA));
    // W_TEST_BOOL(world.TryGetComponent(hComponentC, pComponentC));

    W_TEST_BOOL(pObjectA->IsActive());
    W_TEST_BOOL(pObjectA->GetName() == "A");
    W_TEST_BOOL(pComponentA->IsActive());
    W_TEST_BOOL(pComponentA->GetOwner() == pObjectA);

    W_TEST_BOOL(pObjectC->IsActive());
    W_TEST_BOOL(pObjectC->GetName() == "C");
    W_TEST_BOOL(pComponentC->IsActive());
    W_TEST_BOOL(pComponentC->GetOwner() == pObjectC);

    // creating a new component should reuse memory from component B
    TestComponent* pComponentB2 = nullptr;
    WTypedComponentHandle<TestComponent> hComponentB2 = TestComponent::CreateComponent(pObjectB, pComponentB2);
    W_TEST_BOOL(!hComponentB2.IsInvalidated());
    W_TEST_BOOL(pComponentB2 == pComponentB);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Get Components")
  {
    const WWorld& constWorld = world;

    const TestComponentManager* pConstManager = constWorld.GetComponentManager<TestComponentManager>();

    for (auto it = pConstManager->GetComponents(); it.IsValid(); it.Next())
    {
      WTypedComponentHandle<TestComponent> hComponent = it->GetHandle();

      const TestComponent* pConstComponent = nullptr;
      W_TEST_BOOL(constWorld.TryGetComponent(hComponent, pConstComponent));
      W_TEST_BOOL(pConstComponent == (const TestComponent*)it);

      W_TEST_BOOL(pConstManager->TryGetComponent(hComponent, pConstComponent));
      W_TEST_BOOL(pConstComponent == (const TestComponent*)it);
    }

    world.DeleteComponentManager<TestComponentManager>();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Component Callbacks")
  {
    WGameObjectDesc desc;
    WGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    // Simulation stopped, component active
    {
      world.SetWorldSimulationEnabled(false);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);

      W_TEST_INT(TestComponent::s_iInitCounter, 0);
      W_TEST_INT(TestComponent::s_iActivateCounter, 0);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 1);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.SetWorldSimulationEnabled(true);
      world.Update();

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 1);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->SetActiveFlag(false);

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 0);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->SetActiveFlag(true);
      world.Update();

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 1);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 2);

      pComponent->DeleteComponent();
    }

    // Simulation stopped, component inactive
    {
      world.SetWorldSimulationEnabled(false);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);
      pComponent->SetActiveFlag(false);

      W_TEST_INT(TestComponent::s_iInitCounter, 0);
      W_TEST_INT(TestComponent::s_iActivateCounter, 0);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 0);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 1);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(false);

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 0);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.SetWorldSimulationEnabled(true);
      world.Update();

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 0);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 1);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->DeleteComponent();
    }

    // Simulation started, component active
    {
      world.SetWorldSimulationEnabled(true);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);

      W_TEST_INT(TestComponent::s_iInitCounter, 0);
      W_TEST_INT(TestComponent::s_iActivateCounter, 0);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 1);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->DeleteComponent();
    }

    // Simulation started, component inactive
    {
      world.SetWorldSimulationEnabled(true);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);
      pComponent->SetActiveFlag(false);

      W_TEST_INT(TestComponent::s_iInitCounter, 0);
      W_TEST_INT(TestComponent::s_iActivateCounter, 0);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 0);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      W_TEST_INT(TestComponent::s_iInitCounter, 1);
      W_TEST_INT(TestComponent::s_iActivateCounter, 1);
      W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->DeleteComponent();
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Component dependent initialization")
  {
    WGameObjectDesc desc;
    WGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    world.SetWorldSimulationEnabled(true);

    TestComponent::s_iInitCounter = 0;
    TestComponent::s_iActivateCounter = 0;
    TestComponent::s_iSimulationStartedCounter = 0;
    TestComponent::s_bSpawnOther = true;

    TestComponent* pComponent = nullptr;
    TestComponent::CreateComponent(pObject, pComponent);

    world.Update();

    W_TEST_INT(TestComponent::s_iInitCounter, 1);
    W_TEST_INT(TestComponent::s_iActivateCounter, 2);
    W_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);
  }
}
