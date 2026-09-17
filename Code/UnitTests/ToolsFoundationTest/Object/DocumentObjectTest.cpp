#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

W_CREATE_SIMPLE_TEST_GROUP(DocumentObject);

W_CREATE_SIMPLE_TEST(DocumentObject, DocumentObjectManager)
{
  WTestDocumentObjectManager manager;
  WDocumentObject* pObject = nullptr;
  WDocumentObject* pChildObject = nullptr;
  WDocumentObject* pChildren[4] = {nullptr};
  WDocumentObject* pSubElementObject[4] = {nullptr};

  W_TEST_BLOCK(WTestBlock::Enabled, "DocumentObject")
  {
    W_TEST_BOOL(manager.CanAdd(WObjectTest::GetStaticRTTI(), nullptr, "", 0).Succeeded());
    pObject = manager.CreateObject(WObjectTest::GetStaticRTTI());
    manager.AddObject(pObject, nullptr, "", 0);

    const char* szProperty = "SubObjectSet";
    W_TEST_BOOL(manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, szProperty, 0).Failed());
    W_TEST_BOOL(manager.CanAdd(WObjectTest::GetStaticRTTI(), pObject, szProperty, 0).Succeeded());
    pChildObject = manager.CreateObject(WObjectTest::GetStaticRTTI());
    manager.AddObject(pChildObject, pObject, "SubObjectSet", 0);
    W_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 1);

    W_TEST_BOOL(manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, "ClassPtrArray", 0).Succeeded());
    W_TEST_BOOL(manager.CanAdd(ExtendedOuterClass::GetStaticRTTI(), pObject, "ClassPtrArray", 0).Succeeded());
    W_TEST_BOOL(!manager.CanAdd(WReflectedClass::GetStaticRTTI(), pObject, "ClassPtrArray", 0).Succeeded());

    for (WInt32 i = 0; i < W_ARRAY_SIZE(pChildren); i++)
    {
      W_TEST_BOOL(manager.CanAdd(WObjectTest::GetStaticRTTI(), pChildObject, szProperty, i).Succeeded());
      pChildren[i] = manager.CreateObject(WObjectTest::GetStaticRTTI());
      manager.AddObject(pChildren[i], pChildObject, szProperty, i);
      W_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), i + 1);
    }
    W_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), 4);

    W_TEST_BOOL_MSG(manager.CanMove(pObject, pChildObject, szProperty, 0).Failed(), "Can't move to own child");
    W_TEST_BOOL_MSG(manager.CanMove(pChildren[1], pChildObject, szProperty, 1).Failed(), "Can't move before onself");
    W_TEST_BOOL_MSG(manager.CanMove(pChildren[1], pChildObject, szProperty, 2).Failed(), "Can't move after oneself");
    W_TEST_BOOL_MSG(manager.CanMove(pChildren[1], pChildren[1], szProperty, 0).Failed(), "Can't move into yourself");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "DocumentSubElementObject")
  {
    const char* szProperty = "ClassArray";
    for (WInt32 i = 0; i < W_ARRAY_SIZE(pSubElementObject); i++)
    {
      W_TEST_BOOL(manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, szProperty, i).Succeeded());
      pSubElementObject[i] = manager.CreateObject(OuterClass::GetStaticRTTI());
      manager.AddObject(pSubElementObject[i], pObject, szProperty, i);
      W_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), i + 1);
    }

    W_TEST_BOOL(manager.CanRemove(pSubElementObject[0]).Succeeded());
    manager.RemoveObject(pSubElementObject[0]);
    manager.DestroyObject(pSubElementObject[0]);
    pSubElementObject[0] = nullptr;
    W_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 3);

    WVariant value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    W_TEST_BOOL(value.IsA<WUuid>() && value.Get<WUuid>() == pSubElementObject[1]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    W_TEST_BOOL(value.IsA<WUuid>() && value.Get<WUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 2);
    W_TEST_BOOL(value.IsA<WUuid>() && value.Get<WUuid>() == pSubElementObject[3]->GetGuid());

    W_TEST_BOOL(manager.CanMove(pSubElementObject[1], pObject, szProperty, 2).Succeeded());
    manager.MoveObject(pSubElementObject[1], pObject, szProperty, 2);
    W_TEST_BOOL(manager.CanMove(pSubElementObject[3], pObject, szProperty, 0).Succeeded());
    manager.MoveObject(pSubElementObject[3], pObject, szProperty, 0);

    value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    W_TEST_BOOL(value.IsA<WUuid>() && value.Get<WUuid>() == pSubElementObject[3]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    W_TEST_BOOL(value.IsA<WUuid>() && value.Get<WUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 2);
    W_TEST_BOOL(value.IsA<WUuid>() && value.Get<WUuid>() == pSubElementObject[1]->GetGuid());

    W_TEST_BOOL(manager.CanRemove(pSubElementObject[3]).Succeeded());
    manager.RemoveObject(pSubElementObject[3]);
    manager.DestroyObject(pSubElementObject[3]);
    pSubElementObject[3] = nullptr;
    W_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 2);

    value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    W_TEST_BOOL(value.IsA<WUuid>() && value.Get<WUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    W_TEST_BOOL(value.IsA<WUuid>() && value.Get<WUuid>() == pSubElementObject[1]->GetGuid());

    W_TEST_BOOL(manager.CanMove(pSubElementObject[1], pChildObject, szProperty, 0).Succeeded());
    manager.MoveObject(pSubElementObject[1], pChildObject, szProperty, 0);
    W_TEST_BOOL(manager.CanMove(pSubElementObject[2], pChildObject, szProperty, 0).Succeeded());
    manager.MoveObject(pSubElementObject[2], pChildObject, szProperty, 0);

    W_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 0);
    W_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), 2);

    value = pChildObject->GetTypeAccessor().GetValue(szProperty, 0);
    W_TEST_BOOL(value.IsA<WUuid>() && value.Get<WUuid>() == pSubElementObject[2]->GetGuid());
    value = pChildObject->GetTypeAccessor().GetValue(szProperty, 1);
    W_TEST_BOOL(value.IsA<WUuid>() && value.Get<WUuid>() == pSubElementObject[1]->GetGuid());
  }

  manager.DestroyAllObjects();
}
