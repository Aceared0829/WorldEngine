#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

static WHybridArray<WDocumentObjectPropertyEvent, 2> s_Changes;
void TestPropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  s_Changes.PushBack(e);
}

void TestArray(WVariantSubAccessor& ref_accessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, const WDelegate<WVariant()>& getNativeValue)
{
  auto VerifyChange = [&]()
  {
    // Any operation should collapse to the WVariant being set as a whole.
    W_TEST_INT(s_Changes.GetCount(), 1);
    W_TEST_BOOL(s_Changes[0].m_EventType == WDocumentObjectPropertyEvent::Type::PropertySet);
    W_TEST_BOOL(s_Changes[0].m_pObject == pObject);
    W_TEST_BOOL(s_Changes[0].m_sProperty == pProp->GetPropertyName());
    s_Changes.Clear();
  };

  s_Changes.Clear();
  ref_accessor.StartTransaction("Insert Element");
  WInt32 iCount = 0;
  WVariant value = WColor(1, 2, 3);
  W_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  W_TEST_INT(iCount, 0);
  W_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value, 0));
  W_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  W_TEST_INT(iCount, 1);
  W_TEST_BOOL(getNativeValue()[0] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();
  s_Changes.Clear();

  WVariant outValue;
  W_TEST_STATUS(ref_accessor.GetValue(pObject, pProp, outValue, 0));
  W_TEST_BOOL(value == outValue);

  ref_accessor.StartTransaction("Set Element");
  value = WVariantDictionary();
  W_TEST_STATUS(ref_accessor.SetValue(pObject, pProp, value, 0));
  W_TEST_BOOL(getNativeValue()[0] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Insert Element");
  WVariant value2 = "Test";
  W_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value2, 1));
  W_TEST_BOOL(getNativeValue()[0] == value);
  W_TEST_BOOL(getNativeValue()[1] == value2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Move Element");
  W_TEST_STATUS(ref_accessor.MoveValue(pObject, pProp, 1, 0));
  W_TEST_BOOL(getNativeValue()[0] == value2);
  W_TEST_BOOL(getNativeValue()[1] == value);
  W_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  W_TEST_INT(iCount, 2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Remove Element");
  W_TEST_STATUS(ref_accessor.RemoveValue(pObject, pProp, 0));
  W_TEST_BOOL(getNativeValue()[0] == value);
  W_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  W_TEST_INT(iCount, 1);
  ref_accessor.FinishTransaction();
  VerifyChange();
}

void TestDictionary(WVariantSubAccessor& ref_accessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, const WDelegate<WVariant()>& getNativeValue)
{
  auto VerifyChange = [&]()
  {
    // Any operation should collapse to the WVariant being set as a whole.
    W_TEST_INT(s_Changes.GetCount(), 1);
    W_TEST_BOOL(s_Changes[0].m_EventType == WDocumentObjectPropertyEvent::Type::PropertySet);
    W_TEST_BOOL(s_Changes[0].m_pObject == pObject);
    W_TEST_BOOL(s_Changes[0].m_sProperty == pProp->GetPropertyName());
    s_Changes.Clear();
  };

  s_Changes.Clear();
  ref_accessor.StartTransaction("Insert Element");
  WInt32 iCount = 0;
  WVariant value = WColor(1, 2, 3);
  W_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  W_TEST_INT(iCount, 0);
  W_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value, "A"));
  W_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  W_TEST_INT(iCount, 1);
  W_TEST_BOOL(getNativeValue()["A"] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();
  s_Changes.Clear();

  WVariant outValue;
  W_TEST_STATUS(ref_accessor.GetValue(pObject, pProp, outValue, "A"));
  W_TEST_BOOL(value == outValue);

  ref_accessor.StartTransaction("Set Element");
  value = 42u;
  W_TEST_STATUS(ref_accessor.SetValue(pObject, pProp, value, "A"));
  W_TEST_BOOL(getNativeValue()["A"] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Insert Element");
  WVariant value2 = WVariantArray();
  W_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value2, "B"));
  W_TEST_BOOL(getNativeValue()["A"] == value);
  W_TEST_BOOL(getNativeValue()["B"] == value2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Remove Element");
  W_TEST_STATUS(ref_accessor.RemoveValue(pObject, pProp, "A"));
  W_TEST_BOOL(getNativeValue()["B"] == value2);
  W_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  W_TEST_INT(iCount, 1);
  ref_accessor.FinishTransaction();
  VerifyChange();
}

W_CREATE_SIMPLE_TEST(DocumentObject, VariantPropertyTest)
{
  W_SCOPE_EXIT(s_Changes.Clear(); s_Changes.Compact(););
  WTestDocument doc("Test", true);
  doc.InitializeAfterLoading(false);
  WObjectAccessorBase* pAccessor = doc.GetObjectAccessor();
  const WDocumentObject* pObject = nullptr;
  const WVariantTestStruct* pNative = nullptr;
  doc.GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&TestPropertyEventHandler));
  W_SCOPE_EXIT(doc.GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&TestPropertyEventHandler)));

  W_TEST_BLOCK(WTestBlock::Enabled, "CreateObject")
  {
    WUuid objGuid;
    pAccessor->StartTransaction("Add Object");
    W_TEST_STATUS(pAccessor->AddObject(nullptr, (const WAbstractProperty*)nullptr, -1, WGetStaticRTTI<WVariantTestStruct>(), objGuid));
    pAccessor->FinishTransaction();
    pObject = pAccessor->GetObject(objGuid);
    pNative = static_cast<WVariantTestStruct*>(doc.m_ObjectMirror.GetNativeObjectPointer(pObject));
  }

  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName("Variant");
  const WAbstractProperty* pPropArray = pObject->GetType()->FindPropertyByName("VariantArray");
  const WAbstractProperty* pPropDict = pObject->GetType()->FindPropertyByName("VariantDictionary");

  W_TEST_BLOCK(WTestBlock::Enabled, "TestVariant")
  {
    pAccessor->StartTransaction("Set as Array");
    W_TEST_STATUS(pAccessor->SetValue(pObject, pProp, WVariantArray()));
    pAccessor->FinishTransaction();
    s_Changes.Clear();

    WVariantSubAccessor accessor(pAccessor, pProp);
    WMap<const WDocumentObject*, WVariant> subItemMap;
    subItemMap.Insert(pObject, WVariant());
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pProp, [&]()
      { return pNative->m_Variant; });
    // What remains is an WVariantDictionary at index 0 that we can recurse into.
    {
      WVariantSubAccessor accessor2(&accessor, pProp);
      WMap<const WDocumentObject*, WVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pProp, [&]()
        { return pNative->m_Variant[0]; });
    }
    pAccessor->StartTransaction("Set as Dict");
    W_TEST_STATUS(pAccessor->SetValue(pObject, pProp, WVariantDictionary()));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    TestDictionary(accessor, pObject, pProp, [&]()
      { return pNative->m_Variant; });
    // What remains is an WVariantArray at index "B" that we can recurse into.
    {
      WVariantSubAccessor accessor2(&accessor, pProp);
      WMap<const WDocumentObject*, WVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pProp, [&]()
        { return pNative->m_Variant["B"]; });
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TestVariantArray")
  {
    pAccessor->StartTransaction("Insert Array");
    W_TEST_STATUS(pAccessor->InsertValue(pObject, pPropArray, WVariantArray(), 0));
    pAccessor->FinishTransaction();

    WVariantSubAccessor accessor(pAccessor, pPropArray);
    WMap<const WDocumentObject*, WVariant> subItemMap;
    subItemMap.Insert(pObject, 0);
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pPropArray, [&]()
      { return pNative->m_VariantArray[0]; });
    // What remains is an WVariantDictionary at index 0 that we can recurse into.
    {
      WVariantSubAccessor accessor2(&accessor, pPropArray);
      WMap<const WDocumentObject*, WVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pPropArray, [&]()
        { return pNative->m_VariantArray[0][0]; });
    }
    pAccessor->StartTransaction("Insert Dictionary");
    W_TEST_STATUS(pAccessor->InsertValue(pObject, pPropArray, WVariantDictionary(), 1));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    subItemMap.Insert(pObject, 1);
    accessor.SetSubItems(subItemMap);
    TestDictionary(accessor, pObject, pPropArray, [&]()
      { return pNative->m_VariantArray[1]; });
    // What remains is an WVariantArray at index "B" that we can recurse into.
    {
      WVariantSubAccessor accessor2(&accessor, pPropArray);
      WMap<const WDocumentObject*, WVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pPropArray, [&]()
        { return pNative->m_VariantArray[1]["B"]; });
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TestVariantDictionary")
  {
    pAccessor->StartTransaction("Insert Array");
    W_TEST_STATUS(pAccessor->InsertValue(pObject, pPropDict, WVariantArray(), "AAA"));
    pAccessor->FinishTransaction();

    WVariantSubAccessor accessor(pAccessor, pPropDict);
    WMap<const WDocumentObject*, WVariant> subItemMap;
    subItemMap.Insert(pObject, "AAA");
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pPropDict, [&]()
      { return *pNative->m_VariantDictionary.GetValue("AAA"); });
    // What remains is an WVariantDictionary at index 0 that we can recurse into.
    {
      WVariantSubAccessor accessor2(&accessor, pPropDict);
      WMap<const WDocumentObject*, WVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pPropDict, [&]()
        { return (*pNative->m_VariantDictionary.GetValue("AAA"))[0]; });
    }
    pAccessor->StartTransaction("Insert Dictionary");
    W_TEST_STATUS(pAccessor->InsertValue(pObject, pPropDict, WVariantDictionary(), "BBB"));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    subItemMap.Insert(pObject, "BBB");
    accessor.SetSubItems(subItemMap);
    TestDictionary(accessor, pObject, pPropDict, [&]()
      { return *pNative->m_VariantDictionary.GetValue("BBB"); });
    // What remains is an WVariantArray at index "B" that we can recurse into.
    {
      WVariantSubAccessor accessor2(&accessor, pPropDict);
      WMap<const WDocumentObject*, WVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pPropDict, [&]()
        { return (*pNative->m_VariantDictionary.GetValue("BBB"))["B"]; });
    }
  }
}

W_CREATE_SIMPLE_TEST(DocumentObject, VariantStorageAccessorBounds)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Array index bounds")
  {
    WVariantArray values;
    values.PushBack(WVariant(0));
    values.PushBack(WVariant(1));

    WVariant container = values;
    WVariantStorageAccessor accessor("TestArray", container);

    W_TEST_INT(accessor.GetCount(), 2);

    // Reading inside the array works, past the end fails rather than reading out of bounds.
    WStatus res(W_SUCCESS);
    W_TEST_BOOL(accessor.GetValue(1, &res).IsValid());
    W_TEST_BOOL(res.Succeeded());

    accessor.GetValue(2, &res);
    W_TEST_BOOL(res.Failed());

    accessor.GetValue(99, &res);
    W_TEST_BOOL(res.Failed());

    // Index == GetCount() is one past the last element: valid to insert at, not valid to remove or
    // overwrite. RemoveValue() used to accept it and then read out of bounds.
    W_TEST_BOOL(accessor.RemoveValue(2).Failed());
    W_TEST_BOOL(accessor.RemoveValue(99).Failed());
    W_TEST_BOOL(accessor.SetValue(WVariant(7), 2).Failed());

    W_TEST_INT(accessor.GetCount(), 2);

    // Inserting at the end is allowed, beyond it is not.
    W_TEST_BOOL(accessor.InsertValue(2, WVariant(2)).Succeeded());
    W_TEST_INT(accessor.GetCount(), 3);
    W_TEST_BOOL(accessor.InsertValue(99, WVariant(3)).Failed());

    // Removing the now-last element works, and the one after it still does not.
    W_TEST_BOOL(accessor.RemoveValue(2).Succeeded());
    W_TEST_INT(accessor.GetCount(), 2);
    W_TEST_BOOL(accessor.RemoveValue(2).Failed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Dictionary key bounds")
  {
    WVariantDictionary values;
    values.Insert("A", WVariant(0));

    WVariant container = values;
    WVariantStorageAccessor accessor("TestDict", container);

    WStatus res(W_SUCCESS);
    accessor.GetValue("Missing", &res);
    W_TEST_BOOL(res.Failed());

    W_TEST_BOOL(accessor.RemoveValue("Missing").Failed());
    W_TEST_BOOL(accessor.SetValue(WVariant(1), "Missing").Failed());
    W_TEST_BOOL(accessor.InsertValue("A", WVariant(1)).Failed()); // already exists

    W_TEST_BOOL(accessor.RemoveValue("A").Succeeded());
    W_TEST_INT(accessor.GetCount(), 0);
  }
}
