#include <GameEngineTest/GameEngineTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Utilities/DataStructures/DynamicOctree.h>

W_CREATE_SIMPLE_TEST_GROUP(DataStructures);

namespace DynamicOctreeTestDetail
{
  static WInt32 g_iSearchInstance = 0;
  static bool g_bFoundSearched = false;
  static WUInt32 g_iReturned = 0;

  static bool ObjectFound(void* pPassThrough, WDynamicTreeObjectConst object)
  {
    W_TEST_BOOL(pPassThrough == nullptr);

    ++g_iReturned;

    if (object.Value().m_iObjectInstance == g_iSearchInstance)
      g_bFoundSearched = true;

    // let it give us all the objects in range and count how many that are
    return true;
  }
} // namespace DynamicOctreeTestDetail

W_CREATE_SIMPLE_TEST(DataStructures, DynamicOctree)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "CreateTree / GetBoundingBox")
  {
    WDynamicOctree o;
    o.CreateTree(WVec3(100, 200, 300), WVec3(300, 400, 500), 1.0f);

    const WBoundingBox& bb = o.GetBoundingBox();

    W_TEST_VEC3(bb.GetCenter(), WVec3(100, 200, 300), 0.01f);
    W_TEST_VEC3(bb.GetHalfExtents(), WVec3(500), 0.01f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Insert Inside / Outside")
  {
    const WVec3 c(100, 200, 300);
    const float e = 50;

    WDynamicOctree o;
    o.CreateTree(c, WVec3(e), 1.0f);
    WInt32 iInstance = 0;

    for (float z = -e - 99; z < e + 100; z += 10.0f)
    {
      for (float y = -e - 99; y < e + 100; y += 10.0f)
      {
        for (float x = -e - 99; x < e + 100; x += 10.0f)
        {
          const bool bInside = (z > -e) && (z < e) && (y > -e) && (y < e) && (x > -e) && (x < e);

          W_TEST_BOOL(o.InsertObject(c + WVec3(x, y, z), WVec3(1.0f), 0, iInstance, nullptr, true) == (bInside ? W_SUCCESS : W_FAILURE));
          W_TEST_BOOL(o.InsertObject(c + WVec3(x, y, z), WVec3(1.0f), 0, iInstance, nullptr, false) == W_SUCCESS);

          ++iInstance;
        }
      }
    }
  }

  struct TestObject
  {
    WVec3 m_vPos;
    WVec3 m_vExtents;
    WDynamicTreeObject m_hObject;
  };

  WDeque<TestObject> Objects;

  {
    TestObject to;


    to.m_vPos.Set(-90, 50, 0);
    to.m_vExtents.Set(2.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(-90, 50, 0);
    to.m_vExtents.Set(2.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(-90, 50, 80);
    to.m_vExtents.Set(2.0f, 4.0f, 10.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(0, 0, -50);
    to.m_vExtents.Set(20.0f, 4.0f, 10.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(10, 10, 10);
    to.m_vExtents.Set(50.0f, 2.0f, 1.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(50, -20, 10);
    to.m_vExtents.Set(1.0f, 2.0f, 1.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(50, -20, 10);
    to.m_vExtents.Set(1.0f, 2.0f, 1.0f);

    Objects.PushBack(to);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindObjectsInRange(Point)")
  {
    WDynamicOctree o;
    o.CreateTree(WVec3::MakeZero(), WVec3(100), 1.0f);

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      W_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == W_SUCCESS);

      W_TEST_BOOL(o.IsEmpty() == false);
      W_TEST_INT(o.GetCount(), i + 1);
    }

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, DynamicOctreeTestDetail::ObjectFound, nullptr);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents * 0.9f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents * 0.9f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindObjectsInRange(Radius)")
  {
    WDynamicOctree o;
    o.CreateTree(WVec3::MakeZero(), WVec3(100), 1.0f);

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      W_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == W_SUCCESS);

      W_TEST_BOOL(o.IsEmpty() == false);
      W_TEST_INT(o.GetCount(), i + 1);
    }

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      // point inside object

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents * 0.9f, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents * 0.9f, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      // point outside object

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents + WVec3(2, 0, 0), 2.5f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents - WVec3(0, 2, 0), 2.5f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveObject(handle)")
  {
    WDynamicOctree o;
    o.CreateTree(WVec3::MakeZero(), WVec3(100), 1.0f);

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      W_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == W_SUCCESS);

      W_TEST_BOOL(o.IsEmpty() == false);
      W_TEST_INT(o.GetCount(), i + 1);
    }

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      o.RemoveObject(Objects[i].m_hObject);

      // one less in the tree
      W_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == false);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveObject(index)")
  {
    WDynamicOctree o;
    o.CreateTree(WVec3::MakeZero(), WVec3(100), 1.0f);

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      W_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == W_SUCCESS);

      W_TEST_BOOL(o.IsEmpty() == false);
      W_TEST_INT(o.GetCount(), i + 1);
    }

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      o.RemoveObject(i, i + 1);

      // one less in the tree
      W_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == false);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveObjectsOfType")
  {
    WDynamicOctree o;
    o.CreateTree(WVec3::MakeZero(), WVec3(100), 1.0f);

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      W_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == W_SUCCESS);

      W_TEST_BOOL(o.IsEmpty() == false);
      W_TEST_INT(o.GetCount(), i + 1);
    }

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i + 1;

      o.RemoveObjectsOfType(i);

      // one less in the tree
      W_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      W_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == false);
    }

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      W_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == W_SUCCESS);

      W_TEST_BOOL(o.IsEmpty() == false);
      W_TEST_INT(o.GetCount(), i + 1);
    }

    o.RemoveObjectsOfType(0);

    W_TEST_BOOL(o.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveAllObjects")
  {
    WDynamicOctree o;
    o.CreateTree(WVec3::MakeZero(), WVec3(100), 1.0f);

    for (WUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      W_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == W_SUCCESS);

      W_TEST_BOOL(o.IsEmpty() == false);
      W_TEST_INT(o.GetCount(), i + 1);
    }

    o.RemoveAllObjects();
    W_TEST_BOOL(o.IsEmpty());
    W_TEST_INT(o.GetCount(), 0);
  }
}
