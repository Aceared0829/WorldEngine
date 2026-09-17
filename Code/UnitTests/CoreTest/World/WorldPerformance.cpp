#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>

namespace
{
  class WTestComponentManager;

  class WTestComponent : public WComponent
  {
    W_DECLARE_COMPONENT_TYPE(WTestComponent, WComponent, WTestComponentManager);
  };

  class WTestComponentManager : public WComponentManager<class WTestComponent, WBlockStorageType::FreeList>
  {
  public:
    WTestComponentManager(WWorld* pWorld)
      : WComponentManager<WTestComponent, WBlockStorageType::FreeList>(pWorld)
    {
      m_qRotation.SetIdentity();
    }

    virtual void Initialize() override
    {
      auto desc = WWorldModule::UpdateFunctionDesc(WWorldModule::UpdateFunction(&WTestComponentManager::Update, this), "Update");
      desc.m_bOnlyUpdateWhenSimulating = false;

      RegisterUpdateFunction(desc);
    }

    void Update(const WWorldModule::UpdateContext& context)
    {
      WQuat qRot = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(2.0f));

      m_qRotation = qRot * m_qRotation;

      for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
      {
        ComponentType* pComponent = it;
        if (pComponent->IsActiveAndInitialized())
        {
          auto pOwner = pComponent->GetOwner();
          pOwner->SetLocalRotation(m_qRotation);
        }
      }
    }

    WQuat m_qRotation;
  };

  // clang-format off
  W_BEGIN_COMPONENT_TYPE(WTestComponent, 1, WComponentMode::Dynamic);
  W_END_COMPONENT_TYPE;
  // clang-format on

  void AddObjectsToWorld(WWorld& ref_world, bool bDynamic, WUInt32 uiNumObjects, WUInt32 uiTreeLevelNumNodeDiv, WUInt32 uiTreeDepth,
    WInt32 iAttachCompsDepth, WGameObjectHandle hParent = WGameObjectHandle())
  {
    if (uiTreeDepth == 0)
      return;

    WGameObjectDesc gd;
    gd.m_bDynamic = bDynamic;
    gd.m_hParent = hParent;

    float posX = 0.0f;
    float posY = uiTreeDepth * 5.0f;

    WTestComponentManager* pMan = ref_world.GetOrCreateComponentManager<WTestComponentManager>();

    for (WUInt32 i = 0; i < uiNumObjects; ++i)
    {
      gd.m_LocalPosition.Set(posX, posY, 0);
      posX += 5.0f;

      WGameObject* pObj;
      auto hObj = ref_world.CreateObject(gd, pObj);

      if (iAttachCompsDepth > 0)
      {
        WTestComponent* comp;
        pMan->CreateComponent(pObj, comp);
      }

      AddObjectsToWorld(
        ref_world, bDynamic, WMath::Max(uiNumObjects / uiTreeLevelNumNodeDiv, 1U), uiTreeLevelNumNodeDiv, uiTreeDepth - 1, iAttachCompsDepth - 1, hObj);
    }
  }

  void MeasureCreationTime(
    bool bDynamic, WUInt32 uiNumObjects, WUInt32 uiTreeLevelNumNodeDiv, WUInt32 uiTreeDepth, WInt32 iAttachCompsDepth, WWorld* pWorld = nullptr)
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);

    if (pWorld == nullptr)
    {
      pWorld = &world;
    }

    W_LOCK(pWorld->GetWriteMarker());

    {
      WStopwatch sw;

      AddObjectsToWorld(*pWorld, bDynamic, uiNumObjects, uiTreeLevelNumNodeDiv, uiTreeDepth, iAttachCompsDepth);

      const WTime tDiff = sw.Checkpoint();

      WTestFramework::Output(WTestOutput::Duration, "Creating %u %s objects (depth: %u): %.2fms", pWorld->GetObjectCount(),
        bDynamic ? "dynamic" : "static", uiTreeDepth, tDiff.GetMilliseconds());
    }
  }

} // namespace


#if W_ENABLED(W_COMPILE_FOR_DEBUG)
static const WTestBlock::Enum EnableInRelease = WTestBlock::DisabledNoWarning;
#else
static const WTestBlock::Enum EnableInRelease = WTestBlock::Enabled;
#endif

W_CREATE_SIMPLE_TEST(World, Profile_Creation)
{
  W_TEST_BLOCK(EnableInRelease, "Create many objects")
  {
    // it makes no difference whether we create static or dynamic objects
    static bool bDynamic = true;
    bDynamic = !bDynamic;

    MeasureCreationTime(bDynamic, 10, 1, 4, 0);
    MeasureCreationTime(bDynamic, 10, 1, 5, 0);
    MeasureCreationTime(bDynamic, 100, 1, 2, 0);
    MeasureCreationTime(bDynamic, 10000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 100000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 1000000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 100, 1, 3, 0);
    MeasureCreationTime(bDynamic, 3, 1, 12, 0);
    MeasureCreationTime(bDynamic, 1, 1, 80, 0);
  }
}

W_CREATE_SIMPLE_TEST(World, Profile_Deletion)
{
  W_TEST_BLOCK(EnableInRelease, "Delete many objects")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 2, &world);

    WStopwatch sw;

    W_LOCK(world.GetWriteMarker());
    WUInt32 uiNumObjects = world.GetObjectCount();

    world.Clear();
    world.Update();

    const WTime tDiff = sw.Checkpoint();
    WTestFramework::Output(WTestOutput::Duration, "Deleting %u objects: %.2fms", uiNumObjects, tDiff.GetMilliseconds());
  }
}

W_CREATE_SIMPLE_TEST(World, Profile_Update)
{
  W_TEST_BLOCK(EnableInRelease, "Update 1,000,000 static objects")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    MeasureCreationTime(false, 100, 1, 3, 0, &world);

    WStopwatch sw;

    // first round always has some overhead
    for (WUInt32 i = 0; i < 3; ++i)
    {
      W_LOCK(world.GetWriteMarker());
      world.Update();

      const WTime tDiff = sw.Checkpoint();

      WTestFramework::Output(WTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  W_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic objects")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 0, &world);

    WStopwatch sw;

    // first round always has some overhead
    for (WUInt32 i = 0; i < 3; ++i)
    {
      W_LOCK(world.GetWriteMarker());
      world.Update();

      const WTime tDiff = sw.Checkpoint();

      WTestFramework::Output(WTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  W_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic objects with components")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 2, &world);

    WStopwatch sw;

    // first round always has some overhead
    for (WUInt32 i = 0; i < 3; ++i)
    {
      W_LOCK(world.GetWriteMarker());
      world.Update();

      const WTime tDiff = sw.Checkpoint();

      WTestFramework::Output(WTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  W_TEST_BLOCK(EnableInRelease, "Update 250,000 dynamic objects")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    MeasureCreationTime(true, 200, 5, 6, 0, &world);

    WStopwatch sw;

    // first round always has some overhead
    for (WUInt32 i = 0; i < 3; ++i)
    {
      W_LOCK(world.GetWriteMarker());
      world.Update();

      const WTime tDiff = sw.Checkpoint();

      WTestFramework::Output(WTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  W_TEST_BLOCK(EnableInRelease, "MT Update 250,000 dynamic objects")
  {
    WWorldDesc worldDesc("Test");
    worldDesc.m_bAutoCreateSpatialSystem = false; // allows multi-threaded update
    WWorld world(worldDesc);
    MeasureCreationTime(true, 200, 5, 6, 0, &world);

    WStopwatch sw;

    // first round always has some overhead
    for (WUInt32 i = 0; i < 3; ++i)
    {
      W_LOCK(world.GetWriteMarker());
      world.Update();

      const WTime tDiff = sw.Checkpoint();

      WTestFramework::Output(WTestOutput::Duration, "Updating %u objects (MT): %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  W_TEST_BLOCK(EnableInRelease, "MT Update 1,000,000 dynamic objects")
  {
    WWorldDesc worldDesc("Test");
    worldDesc.m_bAutoCreateSpatialSystem = false; // allows multi-threaded update
    WWorld world(worldDesc);
    MeasureCreationTime(true, 100, 1, 3, 1, &world);

    WStopwatch sw;

    // first round always has some overhead
    for (WUInt32 i = 0; i < 3; ++i)
    {
      W_LOCK(world.GetWriteMarker());
      world.Update();

      const WTime tDiff = sw.Checkpoint();

      WTestFramework::Output(WTestOutput::Duration, "Updating %u objects (MT): %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }
}
