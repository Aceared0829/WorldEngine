#include <CoreTest/CoreTestPCH.h>

#include <Core/Collection/CollectionComponent.h>
#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/GraphicsUtils.h>

W_CREATE_SIMPLE_TEST_GROUP(World);

namespace
{
  union TestWorldObjects
  {
    struct
    {
      WGameObject* pParent1;
      WGameObject* pParent2;
      WGameObject* pChild11;
      WGameObject* pChild21;
    };
    WGameObject* pObjects[4];
  };

  TestWorldObjects CreateTestWorld(WWorld& ref_world, bool bDynamic)
  {
    TestWorldObjects testWorldObjects;
    WMemoryUtils::ZeroFill(&testWorldObjects, 1);

    WQuat q = WQuat::MakeFromAxisAndAngle(WVec3(0.0f, 0.0f, 1.0f), WAngle::MakeFromDegree(90.0f));

    WGameObjectDesc desc;
    desc.m_bDynamic = bDynamic;
    desc.m_LocalPosition = WVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling = WVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent1");

    ref_world.CreateObject(desc, testWorldObjects.pParent1);

    desc.m_sName.Assign("Parent2");
    ref_world.CreateObject(desc, testWorldObjects.pParent2);

    desc.m_hParent = testWorldObjects.pParent1->GetHandle();
    desc.m_sName.Assign("Child11");
    ref_world.CreateObject(desc, testWorldObjects.pChild11);

    desc.m_hParent = testWorldObjects.pParent2->GetHandle();
    desc.m_sName.Assign("Child21");
    ref_world.CreateObject(desc, testWorldObjects.pChild21);

    return testWorldObjects;
  }

  void TestTransforms(const TestWorldObjects& o, WVec3 vOffset = WVec3(100.0f, 0.0f, 0.0f))
  {
    const float eps = WMath::DefaultEpsilon<float>();
    WQuat q = WQuat::MakeFromAxisAndAngle(WVec3(0.0f, 0.0f, 1.0f), WAngle::MakeFromDegree(90.0f));

    for (WUInt32 i = 0; i < 2; ++i)
    {
      W_TEST_VEC3(o.pObjects[i]->GetGlobalPosition(), vOffset, 0);
      W_TEST_BOOL(o.pObjects[i]->GetGlobalRotation().IsEqualRotation(q, eps * 10.0f));
      W_TEST_VEC3(o.pObjects[i]->GetGlobalScaling(), WVec3(1.5f, 1.5f, 1.5f), 0);
    }

    for (WUInt32 i = 2; i < 4; ++i)
    {
      W_TEST_VEC3(o.pObjects[i]->GetGlobalPosition(), vOffset + WVec3(0.0f, 150.0f, 0.0f), eps * 2.0f);
      W_TEST_BOOL(o.pObjects[i]->GetGlobalRotation().IsEqualRotation(q * q, eps * 10.0f));
      W_TEST_VEC3(o.pObjects[i]->GetGlobalScaling(), WVec3(2.25f, 2.25f, 2.25f), 0);
    }
  }

  void SanityCheckWorld(WWorld& ref_world)
  {
    struct Traverser
    {
      Traverser(WWorld& ref_world)
        : m_World(ref_world)
      {
      }

      WWorld& m_World;
      WSet<WGameObject*> m_Found;

      WVisitorExecution::Enum Visit(WGameObject* pObject)
      {
        WGameObject* pObject2 = nullptr;
        W_TEST_BOOL_MSG(m_World.TryGetObject(pObject->GetHandle(), pObject2), "Visited object that is not part of the world!");
        W_TEST_BOOL_MSG(pObject2 == pObject, "Handle did not resolve to the same object!");
        W_TEST_BOOL_MSG(!m_Found.Contains(pObject), "Object visited twice!");
        m_Found.Insert(pObject);

        const WUInt32 uiChildren = pObject->GetChildCount();
        WUInt32 uiChildren2 = 0;
        for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
        {
          uiChildren2++;
          auto handle = it->GetHandle();
          WGameObject* pChild = nullptr;
          W_TEST_BOOL_MSG(m_World.TryGetObject(handle, pChild), "Could not resolve child!");
          WGameObject* pParent = pChild->GetParent();
          W_TEST_BOOL_MSG(pParent == pObject, "pObject's child's parent does not point to pObject!");
        }
        W_TEST_INT(uiChildren, uiChildren2);
        return WVisitorExecution::Continue;
      }
    };

    Traverser traverser(ref_world);
    ref_world.Traverse(WWorld::VisitorFunc(&Traverser::Visit, &traverser), WWorld::TraversalMethod::BreadthFirst);
  }

  class CustomCoordinateSystemProvider : public WCoordinateSystemProvider
  {
  public:
    CustomCoordinateSystemProvider(const WWorld* pWorld)
      : WCoordinateSystemProvider(pWorld)
    {
    }

    virtual void GetCoordinateSystem(const WVec3& vGlobalPosition, WCoordinateSystem& out_coordinateSystem) const override
    {
      const WMat3 mTmp = WGraphicsUtils::CreateLookAtViewMatrix(-vGlobalPosition, WVec3(0, 0, 1), WHandedness::LeftHanded);

      out_coordinateSystem.m_vRightDir = mTmp.GetRow(0);
      out_coordinateSystem.m_vUpDir = mTmp.GetRow(1);
      out_coordinateSystem.m_vForwardDir = mTmp.GetRow(2);
    }
  };

  class VelocityTestModule : public WWorldModule
  {
    W_ADD_DYNAMIC_REFLECTION(VelocityTestModule, WWorldModule);
    W_DECLARE_WORLD_MODULE();

  public:
    VelocityTestModule(WWorld* pWorld)
      : WWorldModule(pWorld)
    {
    }

    virtual void Initialize() override
    {
      {
        auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(VelocityTestModule::SetLocalPos, this);
        RegisterUpdateFunction(desc);
      }

      {
        auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(VelocityTestModule::ResetGlobalPos, this);
        RegisterUpdateFunction(desc);
      }
    }

    void SetLocalPos(const UpdateContext&)
    {
      if (m_bSetLocalPos == false)
        return;

      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        WUInt32 i = it->GetHandle().GetInternalID().m_InstanceIndex;

        WVec3 newPos = WVec3(i * 10.0f, 0, 0);
        it->SetLocalPosition(newPos);

        WQuat newRot = WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisZ(), WAngle::MakeFromDegree(i * 30.0f));
        it->SetLocalRotation(newRot);

        if (i > 5)
        {
          it->UpdateGlobalTransform();
        }
        if (i > 8)
        {
          it->UpdateGlobalTransformAndBounds();
        }
      }
    }

    void ResetGlobalPos(const UpdateContext&)
    {
      if (m_bResetGlobalPos == false)
        return;

      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        it->SetGlobalPosition(WVec3::MakeZero());
      }
    }

    bool m_bSetLocalPos = false;
    bool m_bResetGlobalPos = false;
  };

  // clang-format off
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(VelocityTestModule, 1, WRTTINoAllocator)
  W_END_DYNAMIC_REFLECTED_TYPE;
  W_IMPLEMENT_WORLD_MODULE(VelocityTestModule);
  // clang-format on

  WGameObject* CreateObj(WWorld* pWorld, WStringView sName, WGameObject* pParent = nullptr, WStringView sGlobalkey = {})
  {
    WGameObjectDesc gd;
    gd.m_sName.Assign(sName);
    gd.m_hParent = pParent ? pParent->GetHandle() : WGameObjectHandle();

    WGameObject* go;
    pWorld->CreateObject(gd, go);

    if (!sGlobalkey.IsEmpty())
    {
      go->SetGlobalKey(sGlobalkey);
    }

    return go;
  }
} // namespace

class WGameObjectTest
{
public:
  static void TestInternals(WGameObject* pObject, WGameObject* pParent, WUInt32 uiHierarchyLevel)
  {
    W_TEST_INT(pObject->m_uiHierarchyLevel, uiHierarchyLevel);
    W_TEST_BOOL(pObject->m_pTransformationData->m_pObject == pObject);

    if (pParent)
    {
      W_TEST_BOOL(pObject->m_pTransformationData->m_pParentData->m_pObject == pParent);
    }

    W_TEST_BOOL(pObject->m_pTransformationData->m_pParentData == (pParent != nullptr ? pParent->m_pTransformationData : nullptr));
    W_TEST_BOOL(pObject->GetParent() == pParent);
  }
};

W_CREATE_SIMPLE_TEST(World, World)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Transforms dynamic")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    W_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    WVec3 offset = WVec3(200.0f, 0.0f, 0.0f);
    o.pParent1->SetLocalPosition(offset);
    o.pParent2->SetLocalPosition(offset);

    world.Update();

    TestTransforms(o, offset);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transforms static")
  {
    WWorldDesc worldDesc("Test");
    worldDesc.m_bReportErrorWhenStaticObjectMoves = false;

    WWorld world(worldDesc);
    W_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, false);

    WVec3 offset = WVec3(200.0f, 0.0f, 0.0f);
    o.pParent1->SetLocalPosition(offset);
    o.pParent2->SetLocalPosition(offset);

    // No need to call world update since global transform is updated immediately for static objects.
    // world.Update();

    TestTransforms(o, offset);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GameObject parenting")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    W_LOCK(world.GetWriteMarker());

    const float eps = WMath::DefaultEpsilon<float>();
    WQuat q = WQuat::MakeFromAxisAndAngle(WVec3(0.0f, 0.0f, 1.0f), WAngle::MakeFromDegree(90.0f));

    WGameObjectDesc desc;
    desc.m_LocalPosition = WVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling = WVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent");

    WGameObject* pParentObject;
    WGameObjectHandle parentObject = world.CreateObject(desc, pParentObject);

    W_TEST_VEC3(pParentObject->GetLocalPosition(), desc.m_LocalPosition, 0);
    W_TEST_BOOL(pParentObject->GetLocalRotation() == desc.m_LocalRotation);
    W_TEST_VEC3(pParentObject->GetLocalScaling(), desc.m_LocalScaling, 0);

    W_TEST_VEC3(pParentObject->GetGlobalPosition(), desc.m_LocalPosition, 0);
    W_TEST_BOOL(pParentObject->GetGlobalRotation().IsEqualRotation(desc.m_LocalRotation, eps * 10.0f));
    W_TEST_VEC3(pParentObject->GetGlobalScaling(), desc.m_LocalScaling, 0);

    W_TEST_BOOL(pParentObject->GetName() == desc.m_sName.GetString());

    desc.m_LocalRotation.SetIdentity();
    desc.m_LocalScaling.Set(1.0f);
    desc.m_hParent = parentObject;

    WGameObjectHandle childObjects[10];
    for (WUInt32 i = 0; i < 10; ++i)
    {
      WStringBuilder sb;
      sb.AppendFormat("Child_{0}", i);
      desc.m_sName.Assign(sb.GetData());

      desc.m_LocalPosition = WVec3(i * 10.0f, 0.0f, 0.0f);

      childObjects[i] = world.CreateObject(desc);
    }

    WUInt32 uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      WStringBuilder sb;
      sb.AppendFormat("Child_{0}", uiCounter);

      W_TEST_BOOL(it->GetName() == sb);

      W_TEST_VEC3(it->GetGlobalPosition(), WVec3(100.0f, uiCounter * 15.0f, 0.0f), eps * 2.0f); // 15 because parent is scaled by 1.5
      W_TEST_BOOL(it->GetGlobalRotation().IsEqualRotation(q, eps * 10.0f));
      W_TEST_VEC3(it->GetGlobalScaling(), WVec3(1.5f, 1.5f, 1.5f), 0.0f);

      ++uiCounter;
    }

    W_TEST_INT(uiCounter, 10);
    W_TEST_INT(pParentObject->GetChildCount(), 10);

    world.DeleteObjectNow(childObjects[0]);
    world.DeleteObjectNow(childObjects[3]);
    world.DeleteObjectNow(childObjects[9]);

    W_TEST_BOOL(!world.IsValidObject(childObjects[0]));
    W_TEST_BOOL(!world.IsValidObject(childObjects[3]));
    W_TEST_BOOL(!world.IsValidObject(childObjects[9]));

    WUInt32 indices[7] = {1, 2, 4, 5, 6, 7, 8};

    uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      WStringBuilder sb;
      sb.AppendFormat("Child_{0}", indices[uiCounter]);

      W_TEST_BOOL(it->GetName() == sb);

      ++uiCounter;
    }

    W_TEST_INT(uiCounter, 7);
    W_TEST_INT(pParentObject->GetChildCount(), 7);

    // do one update step so dead objects get deleted
    world.Update();
    SanityCheckWorld(world);

    W_TEST_BOOL(!world.IsValidObject(childObjects[0]));
    W_TEST_BOOL(!world.IsValidObject(childObjects[3]));
    W_TEST_BOOL(!world.IsValidObject(childObjects[9]));

    uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      WStringBuilder sb;
      sb.AppendFormat("Child_{0}", indices[uiCounter]);

      W_TEST_BOOL(it->GetName() == sb);

      ++uiCounter;
    }

    W_TEST_INT(uiCounter, 7);
    W_TEST_INT(pParentObject->GetChildCount(), 7);

    world.DeleteObjectDelayed(parentObject);
    W_TEST_BOOL(world.IsValidObject(parentObject));

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(indices); ++i)
    {
      W_TEST_BOOL(world.IsValidObject(childObjects[indices[i]]));
    }

    // do one update step so dead objects get deleted
    world.Update();
    SanityCheckWorld(world);

    W_TEST_BOOL(!world.IsValidObject(parentObject));

    for (WUInt32 i = 0; i < 10; ++i)
    {
      W_TEST_BOOL(!world.IsValidObject(childObjects[i]));
    }

    W_TEST_INT(world.GetObjectCount(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Re-parenting 1")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    W_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    o.pParent1->AddChild(o.pParent2->GetHandle());
    o.pParent2->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // No need to update the world since re-parenting is now done immediately.
    // world.Update();

    TestTransforms(o);

    WGameObjectTest::TestInternals(o.pParent1, nullptr, 0);
    WGameObjectTest::TestInternals(o.pParent2, o.pParent1, 1);
    WGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1);
    WGameObjectTest::TestInternals(o.pChild21, o.pParent2, 2);

    W_TEST_INT(o.pParent1->GetChildCount(), 2);
    auto it = o.pParent1->GetChildren();
    W_TEST_BOOL(o.pChild11 == it);
    ++it;
    W_TEST_BOOL(o.pParent2 == it);
    ++it;
    W_TEST_BOOL(!it.IsValid());

    it = o.pParent2->GetChildren();
    W_TEST_BOOL(o.pChild21 == it);
    ++it;
    W_TEST_BOOL(!it.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Re-parenting 2")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    W_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    o.pChild21->SetParent(WGameObjectHandle());
    SanityCheckWorld(world);
    // No need to update the world since re-parenting is now done immediately.
    // world.Update();

    TestTransforms(o);

    WGameObjectTest::TestInternals(o.pParent1, nullptr, 0);
    WGameObjectTest::TestInternals(o.pParent2, nullptr, 0);
    WGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1);
    WGameObjectTest::TestInternals(o.pChild21, nullptr, 0);

    auto it = o.pParent1->GetChildren();
    W_TEST_BOOL(o.pChild11 == it);
    ++it;
    W_TEST_BOOL(!it.IsValid());

    W_TEST_INT(o.pParent2->GetChildCount(), 0);
    it = o.pParent2->GetChildren();
    W_TEST_BOOL(!it.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Re-parenting 3")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    W_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);
    SanityCheckWorld(world);
    // Here we test whether the sibling information is correctly cleared.

    o.pChild21->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) sibling.
    o.pParent2->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) and next (pParent2) sibling.
    o.pChild21->SetParent(WGameObjectHandle());
    SanityCheckWorld(world);
    // pChild21 has no siblings.
    o.pChild21->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) sibling again.
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Traversal")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    W_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, false);

    {
      struct BreadthFirstTest
      {
        BreadthFirstTest() { m_uiCounter = 0; }

        WVisitorExecution::Enum Visit(WGameObject* pObject)
        {
          if (m_uiCounter < W_ARRAY_SIZE(m_o.pObjects))
          {
            W_TEST_BOOL(pObject == m_o.pObjects[m_uiCounter]);
          }

          ++m_uiCounter;
          return WVisitorExecution::Continue;
        }

        WUInt32 m_uiCounter;
        TestWorldObjects m_o;
      };

      BreadthFirstTest bft;
      bft.m_o = o;

      world.Traverse(WWorld::VisitorFunc(&BreadthFirstTest::Visit, &bft), WWorld::BreadthFirst);
      W_TEST_INT(bft.m_uiCounter, W_ARRAY_SIZE(o.pObjects));
    }

    {
      world.CreateObject(WGameObjectDesc());

      struct DepthFirstTest
      {
        DepthFirstTest() { m_uiCounter = 0; }

        WVisitorExecution::Enum Visit(WGameObject* pObject)
        {
          if (m_uiCounter == 0)
          {
            W_TEST_BOOL(pObject == m_o.pParent1);
          }
          else if (m_uiCounter == 1)
          {
            W_TEST_BOOL(pObject == m_o.pChild11);
          }
          else if (m_uiCounter == 2)
          {
            W_TEST_BOOL(pObject == m_o.pParent2);
          }
          else if (m_uiCounter == 3)
          {
            W_TEST_BOOL(pObject == m_o.pChild21);
          }

          ++m_uiCounter;
          if (m_uiCounter >= W_ARRAY_SIZE(m_o.pObjects))
            return WVisitorExecution::Stop;

          return WVisitorExecution::Continue;
        }

        WUInt32 m_uiCounter;
        TestWorldObjects m_o;
      };

      DepthFirstTest dft;
      dft.m_o = o;

      world.Traverse(WWorld::VisitorFunc(&DepthFirstTest::Visit, &dft), WWorld::DepthFirst);
      W_TEST_INT(dft.m_uiCounter, W_ARRAY_SIZE(o.pObjects));
    }

    {
      W_TEST_INT(world.GetObjectCount(), 5);
      world.DeleteObjectNow(o.pChild11->GetHandle(), false);
      W_TEST_INT(world.GetObjectCount(), 4);

      for (auto it = world.GetObjects(); it.IsValid(); ++it)
      {
        W_TEST_BOOL(!it->GetHandle().IsInvalidated());
      }

      const WWorld& constWorld = world;
      for (auto it = constWorld.GetObjects(); it.IsValid(); ++it)
      {
        W_TEST_BOOL(!it->GetHandle().IsInvalidated());
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Multiple Worlds")
  {
    WWorldDesc worldDesc1("Test1");
    WWorld world1(worldDesc1);
    W_LOCK(world1.GetWriteMarker());

    WWorldDesc worldDesc2("Test2");
    WWorld world2(worldDesc2);
    W_LOCK(world2.GetWriteMarker());

    WGameObjectDesc desc;
    desc.m_sName.Assign("Obj1");

    WGameObjectHandle hObj1 = world1.CreateObject(desc);
    W_TEST_BOOL(world1.IsValidObject(hObj1));

    desc.m_sName.Assign("Obj2");

    WGameObjectHandle hObj2 = world2.CreateObject(desc);
    W_TEST_BOOL(world2.IsValidObject(hObj2));

    WGameObject* pObj1 = nullptr;
    const WGameObject* pObjConst = nullptr;
    W_TEST_BOOL(world1.TryGetObject(hObj1, pObj1));
    W_TEST_BOOL(pObj1 != nullptr);

    pObj1->SetGlobalKey("Obj1");
    pObj1 = nullptr;
    W_TEST_BOOL(world1.TryGetObjectWithGlobalKey(WTempHashedString("Obj1"), pObj1));
    W_TEST_BOOL(!world1.TryGetObjectWithGlobalKey(WTempHashedString("Obj2"), pObj1));
    W_TEST_BOOL(pObj1 != nullptr);
    W_TEST_BOOL(world1.TryGetObjectWithGlobalKey(WTempHashedString("Obj1"), pObjConst));
    W_TEST_BOOL(pObj1 == pObjConst);

    WGameObject* pObj2 = nullptr;
    W_TEST_BOOL(world2.TryGetObject(hObj2, pObj2));
    W_TEST_BOOL(pObj2 != nullptr);

    pObj2->SetGlobalKey("Obj2");
    pObj2 = nullptr;
    W_TEST_BOOL(world2.TryGetObjectWithGlobalKey(WTempHashedString("Obj2"), pObj2));
    W_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(WTempHashedString("Obj1"), pObj2));
    W_TEST_BOOL(pObj2 != nullptr);

    pObj2->SetGlobalKey("Deschd");
    W_TEST_BOOL(world2.TryGetObjectWithGlobalKey(WTempHashedString("Deschd"), pObj2));
    W_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(WTempHashedString("Obj2"), pObj2));

    world2.DeleteObjectNow(hObj2);

    W_TEST_BOOL(!world2.IsValidObject(hObj2));
    W_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(WTempHashedString("Deschd"), pObj2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Custom coordinate system")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);

    WSharedPtr<CustomCoordinateSystemProvider> pProvider = W_DEFAULT_NEW(CustomCoordinateSystemProvider, &world);
    CustomCoordinateSystemProvider* pProviderBackup = pProvider.Borrow();

    world.SetCoordinateSystemProvider(pProvider);
    W_TEST_BOOL(&world.GetCoordinateSystemProvider() == pProviderBackup);

    WVec3 pos = WVec3(2, 3, 0);

    WCoordinateSystem coordSys;
    world.GetCoordinateSystem(pos, coordSys);

    W_TEST_VEC3(coordSys.m_vForwardDir, (-pos).GetNormalized(), WMath::SmallEpsilon<float>());
    W_TEST_VEC3(coordSys.m_vUpDir, WVec3(0, 0, 1), WMath::SmallEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Active Flag / Active State")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    W_LOCK(world.GetWriteMarker());

    WGameObjectHandle hParent;
    WGameObjectDesc desc;
    WGameObjectHandle hObjects[10];
    WGameObject* pObjects[10];

    for (WUInt32 i = 0; i < 10; ++i)
    {
      desc.m_hParent = hParent;
      hObjects[i] = world.CreateObject(desc, pObjects[i]);
      hParent = hObjects[i];

      W_TEST_BOOL(pObjects[i]->GetActiveFlag());
      W_TEST_BOOL(pObjects[i]->IsActive());
    }

    WUInt32 iTopDisabled = 1;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (WUInt32 i = 0; i < 10; ++i)
    {
      W_TEST_BOOL(pObjects[i]->GetActiveFlag() == (i != iTopDisabled));
      W_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    pObjects[iTopDisabled]->SetActiveFlag(true);

    for (WUInt32 i = 0; i < 10; ++i)
    {
      W_TEST_BOOL(pObjects[i]->GetActiveFlag() == true);
      W_TEST_BOOL(pObjects[i]->IsActive() == true);
    }

    iTopDisabled = 5;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (WUInt32 i = 0; i < 10; ++i)
    {
      W_TEST_BOOL(pObjects[i]->GetActiveFlag() == (i != iTopDisabled));
      W_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    iTopDisabled = 3;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (WUInt32 i = 0; i < 10; ++i)
    {
      W_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    pObjects[iTopDisabled]->SetActiveFlag(true);

    iTopDisabled = 5;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (WUInt32 i = 0; i < 10; ++i)
    {
      W_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }
  }

#if W_ENABLED(W_GAMEOBJECT_VELOCITY)
  W_TEST_BLOCK(WTestBlock::Enabled, "Velocity")
  {
    constexpr WUInt32 numObjects = 10;

    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    W_LOCK(world.GetWriteMarker());

    auto pModule = world.GetOrCreateModule<VelocityTestModule>();

    WGameObjectDesc objectDesc;
    objectDesc.m_bDynamic = true;

    WGameObjectHandle hObjects[numObjects];
    WGameObject* pObjects[numObjects];
    for (WUInt32 i = 0; i < numObjects; ++i)
    {
      objectDesc.m_LocalPosition = WVec3(0, 0, 5);
      objectDesc.m_LocalRotation = WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisZ(), WAngle::MakeFromDegree(90));

      hObjects[i] = world.CreateObject(objectDesc, pObjects[i]);
    }

    pModule->m_bSetLocalPos = true;
    pModule->m_bResetGlobalPos = false;

    world.GetClock().SetFixedTimeStep(WTime::MakeFromMilliseconds(100));
    world.Update();

    for (auto& pObject : pObjects)
    {
      WUInt32 i = pObject->GetHandle().GetInternalID().m_InstanceIndex;
      WVec3 expectedLastPos = WVec3(0, 0, 5);
      WVec3 expectedPos = WVec3(i * 10.0f, 0, 0);
      WVec3 expectedLinearVelocity = WVec3(i * 100.0f, 0, -50);
      W_TEST_VEC3(pObject->GetLastGlobalTransform().m_vPosition, expectedLastPos, WMath::DefaultEpsilon<float>());
      W_TEST_VEC3(pObject->GetGlobalPosition(), expectedPos, WMath::DefaultEpsilon<float>());
      W_TEST_VEC3(pObject->GetLinearVelocity(), expectedLinearVelocity, WMath::DefaultEpsilon<float>());

      WVec3 expectedAngularVelocity = WVec3(0, 0, (WAngle::MakeFromDegree(i * 30.0f) - WAngle::MakeFromDegree(90)).GetRadian() * 10);
      WVec3 angularVelocity = pObject->GetAngularVelocity();
      W_TEST_VEC3(angularVelocity, expectedAngularVelocity, WMath::DefaultEpsilon<float>());
    }

    pModule->m_bSetLocalPos = false;
    pModule->m_bResetGlobalPos = true;

    world.Update();

    for (auto& pObject : pObjects)
    {
      WUInt32 i = pObject->GetHandle().GetInternalID().m_InstanceIndex;
      WVec3 expectedLastPos = WVec3(i * 10.0f, 0, 0);
      WVec3 expectedLinearVelocity = WVec3(i * -100.0f, 0, 0);
      W_TEST_VEC3(pObject->GetLastGlobalTransform().m_vPosition, expectedLastPos, WMath::DefaultEpsilon<float>());
      W_TEST_VEC3(pObject->GetGlobalPosition(), WVec3::MakeZero(), WMath::DefaultEpsilon<float>());
      W_TEST_VEC3(pObject->GetLinearVelocity(), expectedLinearVelocity, WMath::DefaultEpsilon<float>());
    }
  }
#endif

  W_TEST_BLOCK(WTestBlock::Enabled, "FindChildByName / FindChildByPath / SearchForChildByNameSequence")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    W_LOCK(world.GetWriteMarker());

    auto pRoot1 = CreateObj(&world, "Root1");
    auto pRoot2 = CreateObj(&world, "Root2");

    auto pA1 = CreateObj(&world, "A1", pRoot1);
    auto pB1 = CreateObj(&world, "B1", pRoot1);
    auto pC1 = CreateObj(&world, "C1", pRoot1);

    auto pA2 = CreateObj(&world, "A2", pA1);
    auto pB2 = CreateObj(&world, "B2", pA1);
    auto pC2 = CreateObj(&world, "C2", pA1);

    WCollectionComponent* pCol;
    WCollectionComponent::CreateComponent(pC2, pCol);

    // FindChildByName

    W_TEST_BOOL(pRoot1->FindChildByName("A1", false) == pA1);
    W_TEST_BOOL(pRoot1->FindChildByName("B1", false) == pB1);

    W_TEST_BOOL(pRoot2->FindChildByName("A1", false) == nullptr);

    W_TEST_BOOL(pRoot1->FindChildByName("A2", false) == nullptr);
    W_TEST_BOOL(pRoot1->FindChildByName("B2", false) == nullptr);

    W_TEST_BOOL(pRoot1->FindChildByName("A2", true) == pA2);
    W_TEST_BOOL(pRoot1->FindChildByName("B2", true) == pB2);

    // FindChildByPath

    W_TEST_BOOL(pRoot1->FindChildByPath("") == pRoot1);
    W_TEST_BOOL(pA1->FindChildByPath("") == pA1);

    W_TEST_BOOL(pRoot1->FindChildByPath("A1") == pA1);
    W_TEST_BOOL(pRoot1->FindChildByPath("B1") == pB1);

    W_TEST_BOOL(pRoot1->FindChildByPath("A2") == nullptr);
    W_TEST_BOOL(pRoot1->FindChildByPath("B2") == nullptr);

    W_TEST_BOOL(pRoot1->FindChildByPath("A1/A2") == pA2);
    W_TEST_BOOL(pRoot1->FindChildByPath("A1/B2") == pB2);

    // SearchForChildByNameSequence

    W_TEST_BOOL(pRoot1->SearchForChildByNameSequence("") == pRoot1);
    W_TEST_BOOL(pA1->SearchForChildByNameSequence("") == pA1);

    W_TEST_BOOL(pRoot1->SearchForChildByNameSequence("A1") == pA1);
    W_TEST_BOOL(pRoot1->SearchForChildByNameSequence("B1") == pB1);

    W_TEST_BOOL(pRoot1->SearchForChildByNameSequence("A2") == pA2);
    W_TEST_BOOL(pRoot1->SearchForChildByNameSequence("B2") == pB2);

    W_TEST_BOOL(pRoot1->SearchForChildByNameSequence("A1/A2") == pA2);
    W_TEST_BOOL(pRoot1->SearchForChildByNameSequence("A1/B2") == pB2);

    W_TEST_BOOL(pRoot1->SearchForChildByNameSequence("A1/B2", WGetStaticRTTI<WComponent>()) == nullptr);
    W_TEST_BOOL(pRoot1->SearchForChildByNameSequence("", WGetStaticRTTI<WCollectionComponent>()) == nullptr);
    W_TEST_BOOL(pRoot1->SearchForChildByNameSequence("C2", WGetStaticRTTI<WCollectionComponent>()) == pC2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SearchForObject")
  {
    WWorldDesc worldDesc("Test");
    WWorld world(worldDesc);
    W_LOCK(world.GetWriteMarker());

    auto pKey1 = CreateObj(&world, "Key1", nullptr, "Key1");
    auto pKey2 = CreateObj(&world, "Key2", nullptr, "Key2");
    auto pKey3 = CreateObj(&world, "Key3", nullptr, "Key3");

    auto pA1 = CreateObj(&world, "A", pKey1);
    auto pB1 = CreateObj(&world, "B", pA1);
    auto pC1 = CreateObj(&world, "C", pB1);

    auto pA2 = CreateObj(&world, "A", pKey2, "A2");
    auto pB2 = CreateObj(&world, "B", pA2, "B2");
    auto pC2 = CreateObj(&world, "C", pB2, "C2");

    WCollectionComponent* pCol;
    WCollectionComponent::CreateComponent(pC2, pCol);

    W_TEST_BOOL(world.SearchForObject("G:Key1") == pKey1);
    W_TEST_BOOL(world.SearchForObject("G:Key2") == pKey2);
    W_TEST_BOOL(world.SearchForObject("G:Key3/") == pKey3);
    W_TEST_BOOL(world.SearchForObject("G:key3/") == nullptr); // case sensitive
    W_TEST_BOOL(world.SearchForObject("G:B2") == pB2);
    W_TEST_BOOL(world.SearchForObject("G:none") == nullptr);

    W_TEST_BOOL(world.SearchForObject("A", pKey1) == pA1);
    W_TEST_BOOL(world.SearchForObject("B", pKey1) == pB1);
    W_TEST_BOOL(world.SearchForObject("A/B", pKey1) == pB1);
    W_TEST_BOOL(world.SearchForObject("A/C", pKey1) == pC1);
    W_TEST_BOOL(world.SearchForObject("B/C", pA1) == pC1);
    W_TEST_BOOL(world.SearchForObject("B/C", pA1, WGetStaticRTTI<WCollectionComponent>()) == nullptr);
    W_TEST_BOOL(world.SearchForObject("A", pA1) == nullptr); // A has to be a child

    W_TEST_BOOL(world.SearchForObject("G:A2/C") == pC2);

    W_TEST_BOOL(world.SearchForObject("P:A", pC2) == pA2);
    W_TEST_BOOL(world.SearchForObject("P:D", pC2) == nullptr);
    W_TEST_BOOL(world.SearchForObject("P:A/C", pC2) == pC2);
    W_TEST_BOOL(world.SearchForObject("P:A/C", pC2, WGetStaticRTTI<WCollectionComponent>()) == pC2);

    W_TEST_BOOL(world.SearchForObject("", pA1) == pA1);
    W_TEST_BOOL(world.SearchForObject("") == nullptr);

    W_TEST_BOOL(world.SearchForObject("G:C2/P:A/B", pKey3) == pB2);
    W_TEST_BOOL(world.SearchForObject("G:C2/P:a/B", pKey3) == nullptr);  // case sensitive
    W_TEST_BOOL(world.SearchForObject("G:C2/P:A//B", pKey3) == nullptr); // malformed path

    W_TEST_BOOL(world.SearchForObject("..", pC1) == pB1);
    W_TEST_BOOL(world.SearchForObject("../../", pC1) == pA1);
    W_TEST_BOOL(world.SearchForObject("../..", pC1) == pA1);

    W_TEST_BOOL(world.SearchForObject("G:C2/P:B/../../A", pKey3) == pA2);

    W_TEST_BOOL(world.SearchForObject("G:B2/C/..") == nullptr); // malformed path
  }
}
