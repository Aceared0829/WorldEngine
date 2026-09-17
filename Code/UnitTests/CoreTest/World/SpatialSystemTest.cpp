#include <CoreTest/CoreTestPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/Utilities/GraphicsUtils.h>

namespace
{
  static WSpatialData::Category s_SpecialTestCategory = WSpatialData::RegisterCategory("SpecialTestCategory", WSpatialData::Flags::None);

  using TestBoundsComponentManager = WComponentManager<class TestBoundsComponent, WBlockStorageType::Compact>;

  class TestBoundsComponent : public WComponent
  {
    W_DECLARE_COMPONENT_TYPE(TestBoundsComponent, WComponent, TestBoundsComponentManager);

  public:
    virtual void Initialize() override { GetOwner()->UpdateLocalBounds(); }

    void OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg)
    {
      auto& rng = GetWorld()->GetRandomNumberGenerator();

      float x = (float)rng.DoubleMinMax(1.0, 100.0);
      float y = (float)rng.DoubleMinMax(1.0, 100.0);
      float z = (float)rng.DoubleMinMax(1.0, 100.0);

      WBoundingBox bounds = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3::MakeZero(), WVec3(x, y, z));

      WSpatialData::Category category = m_SpecialCategory;
      if (category == WInvalidSpatialDataCategory)
      {
        category = GetOwner()->IsDynamic() ? WDefaultSpatialDataCategories::RenderDynamic : WDefaultSpatialDataCategories::RenderStatic;
      }

      ref_msg.AddBounds(WBoundingBoxSphere::MakeFromBox(bounds), category);
    }

    WSpatialData::Category m_SpecialCategory = WInvalidSpatialDataCategory;
  };

  // clang-format off
  W_BEGIN_COMPONENT_TYPE(TestBoundsComponent, 1, WComponentMode::Static)
  {
    W_BEGIN_MESSAGEHANDLERS
    {
      W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds)
    }
    W_END_MESSAGEHANDLERS;
  }
  W_END_COMPONENT_TYPE;
  // clang-format on

  static WGameObject* CreateObjectAndTestComponent(WWorld& inout_world, bool bDynamic)
  {
    auto& rng = inout_world.GetRandomNumberGenerator();
    constexpr const double range = 10000.0;

    float x = (float)rng.DoubleMinMax(-range, range);
    float y = (float)rng.DoubleMinMax(-range, range);
    float z = (float)rng.DoubleMinMax(-range, range);

    WGameObjectDesc desc;
    desc.m_bDynamic = bDynamic;
    desc.m_LocalPosition = WVec3(x, y, z);

    WGameObject* pObject = nullptr;
    inout_world.CreateObject(desc, pObject);

    TestBoundsComponent* pComponent = nullptr;
    TestBoundsComponent::CreateComponent(pObject, pComponent);

    return pObject;
  }
} // namespace

W_CREATE_SIMPLE_TEST(World, SpatialSystem)
{
  WWorldDesc worldDesc("Test");
  worldDesc.m_uiRandomNumberGeneratorSeed = 5;

  WWorld world(worldDesc);
  W_LOCK(world.GetWriteMarker());

  for (WUInt32 i = 0; i < 1000; ++i)
  {
    CreateObjectAndTestComponent(world, i >= 500);
  }

  world.Update();

  WSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = WDefaultSpatialDataCategories::RenderStatic.GetBitmask();

  W_TEST_BLOCK(WTestBlock::Enabled, "FindObjectsInSphere")
  {
    WBoundingSphere testSphere = WBoundingSphere::MakeFromCenterAndRadius(WVec3(100.0f, 60.0f, 400.0f), 3000.0f);

    WDynamicArray<WGameObject*> objectsInSphere;
    WHashSet<WGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindObjectsInSphere(testSphere, queryParams, objectsInSphere);

    for (auto pObject : objectsInSphere)
    {
      WBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      W_TEST_BOOL(testSphere.Overlaps(objSphere));
      W_TEST_BOOL(!uniqueObjects.Insert(pObject));
      W_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      WBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        W_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((WGameObject*)it));
      }
    }

    objectsInSphere.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem()->FindObjectsInSphere(testSphere, queryParams, [&](WGameObject* pObject)
      {
      objectsInSphere.PushBack(pObject);
      W_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return WVisitorExecution::Continue; });

    for (auto pObject : objectsInSphere)
    {
      WBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      W_TEST_BOOL(testSphere.Overlaps(objSphere));
      W_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      WBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        W_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((WGameObject*)it));
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindObjectsInBox")
  {
    WBoundingBox testBox = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3(100.0f, 60.0f, 400.0f), WVec3(3000.0f));

    WDynamicArray<WGameObject*> objectsInBox;
    WHashSet<WGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindObjectsInBox(testBox, queryParams, objectsInBox);

    for (auto pObject : objectsInBox)
    {
      WBoundingBox objBox = pObject->GetGlobalBounds().GetBox();

      W_TEST_BOOL(testBox.Overlaps(objBox));
      W_TEST_BOOL(!uniqueObjects.Insert(pObject));
      W_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      WBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        W_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((WGameObject*)it));
      }
    }

    objectsInBox.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem()->FindObjectsInBox(testBox, queryParams, [&](WGameObject* pObject)
      {
      objectsInBox.PushBack(pObject);
      W_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return WVisitorExecution::Continue; });

    for (auto pObject : objectsInBox)
    {
      WBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      W_TEST_BOOL(testBox.Overlaps(objSphere));
      W_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      WBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        W_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((WGameObject*)it));
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindVisibleObjects")
  {
    constexpr uint32_t numUpdates = 13;

    // update a few times to increase internal frame counter
    for (uint32_t i = 0; i < numUpdates; ++i)
    {
      world.Update();
    }

    // newly created objects should be considered visible in the first frame after creation
    {
      WGameObject* pNewObject = CreateObjectAndTestComponent(world, false);

      world.Update();

      auto visState = pNewObject->GetVisibilityState();
      W_TEST_BOOL(visState == WVisibilityState::Direct);
    }

    // update a few more times to increase internal frame counter
    for (uint32_t i = 0; i < numUpdates; ++i)
    {
      world.Update();
    }

    queryParams.m_uiCategoryBitmask = WDefaultSpatialDataCategories::RenderDynamic.GetBitmask();

    WMat4 lookAt = WGraphicsUtils::CreateLookAtViewMatrix(WVec3::MakeZero(), WVec3::MakeAxisX(), WVec3::MakeAxisZ());
    WMat4 projection = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(WAngle::MakeFromDegree(80.0f), 1.0f, 1.0f, 10000.0f);

    WFrustum testFrustum = WFrustum::MakeFromMVP(projection * lookAt);

    WDynamicArray<const WGameObject*> visibleObjects;
    WHashSet<const WGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindVisibleObjects(testFrustum, queryParams, visibleObjects, {}, WVisibilityState::Direct);

    W_TEST_BOOL(!visibleObjects.IsEmpty());

    for (auto pObject : visibleObjects)
    {
      W_TEST_BOOL(testFrustum.Overlaps(pObject->GetGlobalBoundsSimd().GetSphere()));
      W_TEST_BOOL(!uniqueObjects.Insert(pObject));
      W_TEST_BOOL(pObject->IsDynamic());

      WVisibilityState::Enum visType = pObject->GetVisibilityState();
      W_TEST_BOOL(visType == WVisibilityState::Direct);
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      WGameObject* pObject = it;

      if (testFrustum.GetObjectPosition(pObject->GetGlobalBounds().GetSphere()) == WVolumePosition::Outside)
      {
        WVisibilityState::Enum visType = pObject->GetVisibilityState();
        W_TEST_BOOL(visType == WVisibilityState::Invisible);
      }
    }

    // Move some objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      constexpr const double range = 500.0f;

      if (it->IsDynamic())
      {
        WVec3 pos = it->GetLocalPosition();

        auto& rng = world.GetRandomNumberGenerator();
        pos.x += (float)rng.DoubleMinMax(-range, range);
        pos.y += (float)rng.DoubleMinMax(-range, range);
        pos.z += (float)rng.DoubleMinMax(-range, range);

        it->SetLocalPosition(pos);
      }
    }

    world.Update();

    // Check that last frame visible doesn't reset entirely after moving
    for (const WGameObject* pObject : visibleObjects)
    {
      WVisibilityState::Enum visType = pObject->GetVisibilityState();
      W_TEST_BOOL(visType == WVisibilityState::Direct);
    }
  }

  if (false)
  {
    WStringBuilder outputPath = WTestFramework::GetInstance()->GetAbsOutputPath();
    W_TEST_BOOL(WFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", WDataDirUsage::AllowWrites) == W_SUCCESS);

    WProfilingUtils::SaveProfilingCapture(":output/profiling.json").IgnoreResult();
  }

  // Test multiple categories for spatial data
  W_TEST_BLOCK(WTestBlock::Enabled, "MultipleCategories")
  {
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      WGameObject* pObject = it;

      TestBoundsComponent* pComponent = nullptr;
      TestBoundsComponent::CreateComponent(pObject, pComponent);
      pComponent->m_SpecialCategory = s_SpecialTestCategory;
    }

    world.Update();

    WDynamicArray<WGameObjectHandle> allObjects;
    allObjects.Reserve(world.GetObjectCount());

    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      allObjects.PushBack(it->GetHandle());
    }

    for (WUInt32 i = allObjects.GetCount(); i-- > 0;)
    {
      world.DeleteObjectNow(allObjects[i]);
    }

    world.Update();
  }
}
