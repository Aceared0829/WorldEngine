#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

W_CREATE_SIMPLE_TEST(DocumentObject, CommandHistory)
{
  WTestDocument doc("Test", true);
  doc.InitializeAfterLoading(false);
  WObjectAccessorBase* pAccessor = doc.GetObjectAccessor();

  auto CreateObject = [&doc, &pAccessor](const WRTTI* pType) -> const WDocumentObject*
  {
    WUuid objGuid;
    pAccessor->StartTransaction("Add Object");
    W_TEST_STATUS(pAccessor->AddObject(nullptr, (const WAbstractProperty*)nullptr, -1, pType, objGuid));
    pAccessor->FinishTransaction();
    return pAccessor->GetObject(objGuid);
  };

  auto StoreOriginalState = [&doc](WAbstractObjectGraph& ref_graph, const WDocumentObject* pRoot)
  {
    WDocumentObjectConverterWriter writer(&ref_graph, doc.GetObjectManager(), [](const WDocumentObject*, const WAbstractProperty* p)
      { return p->GetAttributeByType<WHiddenAttribute>() == nullptr; });
    WAbstractObjectNode* pAbstractObj = writer.AddObjectToGraph(pRoot);
  };

  auto CompareAgainstOriginalState = [&doc](WAbstractObjectGraph& ref_original, const WDocumentObject* pRoot)
  {
    WAbstractObjectGraph graph;
    WDocumentObjectConverterWriter writer2(&graph, doc.GetObjectManager(), [](const WDocumentObject*, const WAbstractProperty* p)
      { return p->GetAttributeByType<WHiddenAttribute>() == nullptr; });
    WAbstractObjectNode* pAbstractObj2 = writer2.AddObjectToGraph(pRoot);

    WDeque<WAbstractGraphDiffOperation> diff;
    graph.CreateDiffWithBaseGraph(ref_original, diff);
    W_TEST_BOOL(diff.GetCount() == 0);
  };

  const WDocumentObject* pRoot = CreateObject(WGetStaticRTTI<WMirrorTest>());

  WUuid mathGuid = pAccessor->GetByName<WUuid>(pRoot, "Math");
  WUuid objectGuid = pAccessor->GetByName<WUuid>(pRoot, "Object");

  const WDocumentObject* pMath = pAccessor->GetObject(mathGuid);
  const WDocumentObject* pObjectTest = pAccessor->GetObject(objectGuid);

  W_TEST_BLOCK(WTestBlock::Enabled, "SetValue")
  {

    W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), 1);

    auto TestSetValue = [&](const WDocumentObject* pObject, const char* szProperty, WVariant value)
    {
      WAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      WUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();

      pAccessor->StartTransaction("SetValue");
      W_TEST_STATUS(pAccessor->SetValueByName(pObject, szProperty, value));
      pAccessor->FinishTransaction();
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      WVariant newValue;
      W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue));
      W_TEST_BOOL(newValue == value);

      W_TEST_STATUS(doc.GetCommandHistory()->Undo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      CompareAgainstOriginalState(graph, pObject);

      W_TEST_STATUS(doc.GetCommandHistory()->Redo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue));
      W_TEST_BOOL(newValue == value);
    };

    // Math
    TestSetValue(pMath, "Vec2", WVec2(1, 2));
    TestSetValue(pMath, "Vec3", WVec3(1, 2, 3));
    TestSetValue(pMath, "Vec4", WVec4(1, 2, 3, 4));
    TestSetValue(pMath, "Vec2I", WVec2I32(1, 2));
    TestSetValue(pMath, "Vec3I", WVec3I32(1, 2, 3));
    TestSetValue(pMath, "Vec4I", WVec4I32(1, 2, 3, 4));
    WQuat qValue;
    qValue = WQuat::MakeFromEulerAngles(WAngle::MakeFromDegree(30), WAngle::MakeFromDegree(30), WAngle::MakeFromDegree(30));
    TestSetValue(pMath, "Quat", qValue);
    WMat3 mValue;
    mValue = WMat3::MakeRotationX(WAngle::MakeFromDegree(30));
    TestSetValue(pMath, "Mat3", mValue);
    WMat4 mValue2;
    mValue2.SetIdentity();
    mValue2 = WMat4::MakeRotationX(WAngle::MakeFromDegree(30));
    TestSetValue(pMath, "Mat4", mValue2);

    // Integer
    const WDocumentObject* pInteger = CreateObject(WGetStaticRTTI<WIntegerStruct>());
    TestSetValue(pInteger, "Int8", WInt8(-5));
    TestSetValue(pInteger, "UInt8", WUInt8(5));
    TestSetValue(pInteger, "Int16", WInt16(-5));
    TestSetValue(pInteger, "UInt16", WUInt16(5));
    TestSetValue(pInteger, "Int32", WInt32(-5));
    TestSetValue(pInteger, "UInt32", WUInt32(5));
    TestSetValue(pInteger, "Int64", WInt64(-5));
    TestSetValue(pInteger, "UInt64", WUInt64(5));

    // Test automatic type conversions
    TestSetValue(pInteger, "Int8", WInt16(-5));
    TestSetValue(pInteger, "Int8", WInt32(-5));
    TestSetValue(pInteger, "Int8", WInt64(-5));
    TestSetValue(pInteger, "Int8", float(-5));
    TestSetValue(pInteger, "Int8", WUInt8(5));

    TestSetValue(pInteger, "Int64", WInt32(-5));
    TestSetValue(pInteger, "Int64", WInt16(-5));
    TestSetValue(pInteger, "Int64", WInt8(-5));
    TestSetValue(pInteger, "Int64", float(-5));
    TestSetValue(pInteger, "Int64", WUInt8(5));

    TestSetValue(pInteger, "UInt64", WUInt32(5));
    TestSetValue(pInteger, "UInt64", WUInt16(5));
    TestSetValue(pInteger, "UInt64", WUInt8(5));
    TestSetValue(pInteger, "UInt64", float(5));
    TestSetValue(pInteger, "UInt64", WInt8(5));

    // Float
    const WDocumentObject* pFloat = CreateObject(WGetStaticRTTI<WFloatStruct>());
    TestSetValue(pFloat, "Float", -5.0f);
    TestSetValue(pFloat, "Double", -5.0);
    TestSetValue(pFloat, "Time", WTime::MakeFromMinutes(3.0f));
    TestSetValue(pFloat, "Angle", WAngle::MakeFromDegree(45.0f));

    TestSetValue(pFloat, "Float", 5.0);
    TestSetValue(pFloat, "Float", WInt8(-5));
    TestSetValue(pFloat, "Float", WUInt8(5));

    // Misc PODs
    const WDocumentObject* pPOD = CreateObject(WGetStaticRTTI<WPODClass>());
    TestSetValue(pPOD, "Bool", true);
    TestSetValue(pPOD, "Bool", false);
    TestSetValue(pPOD, "Color", WColor(1.0f, 2.0f, 3.0f, 4.0f));
    TestSetValue(pPOD, "ColorUB", WColorGammaUB(200, 100, 255));
    TestSetValue(pPOD, "String", "Test");
    WVarianceTypeAngle customFloat;
    customFloat.m_Value = WAngle::MakeFromDegree(45.0f);
    customFloat.m_fVariance = 1.0f;
    TestSetValue(pPOD, "VarianceAngle", customFloat);

    // Enumerations
    const WDocumentObject* pEnum = CreateObject(WGetStaticRTTI<WEnumerationsClass>());
    TestSetValue(pEnum, "Enum", (WInt8)WExampleEnum::Value2);
    TestSetValue(pEnum, "Enum", (WInt64)WExampleEnum::Value2);
    TestSetValue(pEnum, "Bitflags", (WUInt8)WExampleBitflags::Value2);
    TestSetValue(pEnum, "Bitflags", (WInt64)WExampleBitflags::Value2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "InsertValue")
  {
    auto TestInsertValue = [&](const WDocumentObject* pObject, const char* szProperty, WVariant value, WVariant index)
    {
      WAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const WUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const WInt32 iArraySize = pAccessor->GetCountByName(pObject, szProperty);

      pAccessor->StartTransaction("InsertValue");
      W_TEST_STATUS(pAccessor->InsertValueByName(pObject, szProperty, value, index));
      pAccessor->FinishTransaction();
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize + 1);
      WVariant newValue;
      W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, index));
      W_TEST_BOOL(newValue == value);

      W_TEST_STATUS(doc.GetCommandHistory()->Undo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      W_TEST_STATUS(doc.GetCommandHistory()->Redo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize + 1);
      W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, index));
      W_TEST_BOOL(newValue == value);
    };

    TestInsertValue(pObjectTest, "StandardTypeArray", double(0), 0);
    TestInsertValue(pObjectTest, "StandardTypeArray", double(2), 1);
    TestInsertValue(pObjectTest, "StandardTypeArray", double(1), 1);

    TestInsertValue(pObjectTest, "StandardTypeSet", "A", 0);
    TestInsertValue(pObjectTest, "StandardTypeSet", "C", 1);
    TestInsertValue(pObjectTest, "StandardTypeSet", "B", 1);

    TestInsertValue(pObjectTest, "StandardTypeMap", double(0), "A");
    TestInsertValue(pObjectTest, "StandardTypeMap", double(2), "C");
    TestInsertValue(pObjectTest, "StandardTypeMap", double(1), "B");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MoveValue")
  {
    auto TestMoveValue = [&](const WDocumentObject* pObject, const char* szProperty, WVariant oldIndex, WVariant newIndex, WArrayPtr<WVariant> expectedOutcome)
    {
      WAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const WUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const WInt32 iArraySize = pAccessor->GetCountByName(pObject, szProperty);
      W_TEST_INT(iArraySize, expectedOutcome.GetCount());

      WDynamicArray<WVariant> values;
      W_TEST_STATUS(pAccessor->GetValuesByName(pObject, szProperty, values));
      W_TEST_INT(iArraySize, values.GetCount());

      pAccessor->StartTransaction("MoveValue");
      W_TEST_STATUS(pAccessor->MoveValueByName(pObject, szProperty, oldIndex, newIndex));
      pAccessor->FinishTransaction();
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);

      for (WInt32 i = 0; i < iArraySize; i++)
      {
        WVariant newValue;
        W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, i));
        W_TEST_BOOL(newValue == expectedOutcome[i]);
      }

      W_TEST_STATUS(doc.GetCommandHistory()->Undo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      W_TEST_STATUS(doc.GetCommandHistory()->Redo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);

      for (WInt32 i = 0; i < iArraySize; i++)
      {
        WVariant newValue;
        W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, i));
        W_TEST_BOOL(newValue == expectedOutcome[i]);
      }
    };

    {
      WVariant expectedValues[3] = {0, 1, 2};
      // Move first element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 0, WArrayPtr<WVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 1, WArrayPtr<WVariant>(expectedValues));
      // Move last element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 2, WArrayPtr<WVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 3, WArrayPtr<WVariant>(expectedValues));
    }
    {
      // Move first element to the end.
      WVariant expectedValues[3] = {1, 2, 0};
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 3, WArrayPtr<WVariant>(expectedValues));
    }
    {
      // Move last element to the front.
      WVariant expectedValues[3] = {0, 1, 2};
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 0, WArrayPtr<WVariant>(expectedValues));
    }
    {
      // Move first element to the middle
      WVariant expectedValues[3] = {1, 0, 2};
      TestMoveValue(pObjectTest, "StandardTypeArray", 0, 2, WArrayPtr<WVariant>(expectedValues));
    }
    {
      // Move last element to the middle
      WVariant expectedValues[3] = {1, 2, 0};
      TestMoveValue(pObjectTest, "StandardTypeArray", 2, 1, WArrayPtr<WVariant>(expectedValues));
    }

    {
      WVariant expectedValues[3] = {"A", "B", "C"};
      // Move first element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 0, WArrayPtr<WVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 1, WArrayPtr<WVariant>(expectedValues));
      // Move last element before or after itself (no-op)
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 2, WArrayPtr<WVariant>(expectedValues));
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 3, WArrayPtr<WVariant>(expectedValues));
    }
    {
      // Move first element to the end.
      WVariant expectedValues[3] = {"B", "C", "A"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 3, WArrayPtr<WVariant>(expectedValues));
    }
    {
      // Move last element to the front.
      WVariant expectedValues[3] = {"A", "B", "C"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 0, WArrayPtr<WVariant>(expectedValues));
    }
    {
      // Move first element to the middle
      WVariant expectedValues[3] = {"B", "A", "C"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 0, 2, WArrayPtr<WVariant>(expectedValues));
    }
    {
      // Move last element to the middle
      WVariant expectedValues[3] = {"B", "C", "A"};
      TestMoveValue(pObjectTest, "StandardTypeSet", 2, 1, WArrayPtr<WVariant>(expectedValues));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveValue")
  {
    auto TestRemoveValue = [&](const WDocumentObject* pObject, const char* szProperty, WVariant index, WArrayPtr<WVariant> expectedOutcome)
    {
      WAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const WUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const WInt32 iArraySize = pAccessor->GetCountByName(pObject, szProperty);
      W_TEST_INT(iArraySize - 1, expectedOutcome.GetCount());

      WDynamicArray<WVariant> values;
      WDynamicArray<WVariant> keys;
      {
        W_TEST_STATUS(pAccessor->GetValuesByName(pObject, szProperty, values));
        W_TEST_INT(iArraySize, values.GetCount());

        W_TEST_STATUS(pAccessor->GetKeysByName(pObject, szProperty, keys));
        W_TEST_INT(iArraySize, keys.GetCount());
        WUInt32 uiIndex = keys.IndexOf(index);
        keys.RemoveAtAndSwap(uiIndex);
        values.RemoveAtAndSwap(uiIndex);
        W_TEST_INT(iArraySize - 1, keys.GetCount());
      }

      pAccessor->StartTransaction("RemoveValue");
      W_TEST_STATUS(pAccessor->RemoveValueByName(pObject, szProperty, index));
      pAccessor->FinishTransaction();
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize - 1);

      if (pObject->GetType()->FindPropertyByName(szProperty)->GetCategory() == WPropertyCategory::Map)
      {
        for (WInt32 i = 0; i < iArraySize - 1; i++)
        {
          const WVariant& key = keys[i];
          const WVariant& value = values[i];
          WVariant newValue;
          W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, key));
          W_TEST_BOOL(newValue == value);
          W_TEST_BOOL(expectedOutcome.Contains(newValue));
        }
      }
      else
      {
        for (WInt32 i = 0; i < iArraySize - 1; i++)
        {
          WVariant newValue;
          W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, i));
          W_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }

      W_TEST_STATUS(doc.GetCommandHistory()->Undo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      W_TEST_STATUS(doc.GetCommandHistory()->Redo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize - 1);

      if (pObject->GetType()->FindPropertyByName(szProperty)->GetCategory() == WPropertyCategory::Map)
      {
        for (WInt32 i = 0; i < iArraySize - 1; i++)
        {
          const WVariant& key = keys[i];
          const WVariant& value = values[i];
          WVariant newValue;
          W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, key));
          W_TEST_BOOL(newValue == value);
          W_TEST_BOOL(expectedOutcome.Contains(newValue));
        }
      }
      else
      {
        for (WInt32 i = 0; i < iArraySize - 1; i++)
        {
          WVariant newValue;
          W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, i));
          W_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }
    };

    // StandardTypeArray
    {
      WVariant expectedValues[2] = {2, 0};
      TestRemoveValue(pObjectTest, "StandardTypeArray", 0, WArrayPtr<WVariant>(expectedValues));
    }
    {
      WVariant expectedValues[1] = {2};
      TestRemoveValue(pObjectTest, "StandardTypeArray", 1, WArrayPtr<WVariant>(expectedValues));
    }
    {
      TestRemoveValue(pObjectTest, "StandardTypeArray", 0, WArrayPtr<WVariant>());
    }
    // StandardTypeSet
    {
      WVariant expectedValues[2] = {"B", "C"};
      TestRemoveValue(pObjectTest, "StandardTypeSet", 2, WArrayPtr<WVariant>(expectedValues));
    }
    {
      WVariant expectedValues[1] = {"C"};
      TestRemoveValue(pObjectTest, "StandardTypeSet", 0, WArrayPtr<WVariant>(expectedValues));
    }
    {
      TestRemoveValue(pObjectTest, "StandardTypeSet", 0, WArrayPtr<WVariant>());
    }
    // StandardTypeMap
    {
      WVariant expectedValues[2] = {1, 2};
      TestRemoveValue(pObjectTest, "StandardTypeMap", "A", WArrayPtr<WVariant>(expectedValues));
    }
    {
      WVariant expectedValues[1] = {1};
      TestRemoveValue(pObjectTest, "StandardTypeMap", "C", WArrayPtr<WVariant>(expectedValues));
    }
    {
      TestRemoveValue(pObjectTest, "StandardTypeMap", "B", WArrayPtr<WVariant>());
    }
  }

  auto CreateGuid = [](const char* szType, WInt32 iIndex) -> WUuid
  {
    WUuid A = WUuid::MakeStableUuidFromString(szType);
    WUuid B = WUuid::MakeStableUuidFromInt(iIndex);
    A.CombineWithSeed(B);
    return A;
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "AddObject")
  {
    auto TestAddObject = [&](const WDocumentObject* pObject, const char* szProperty, WVariant index, const WRTTI* pType, WUuid& inout_object)
    {
      WAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject);

      const WUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const WInt32 iArraySize = pAccessor->GetCountByName(pObject, szProperty);

      pAccessor->StartTransaction("TestAddObject");
      W_TEST_STATUS(pAccessor->AddObjectByName(pObject, szProperty, index, pType, inout_object));
      pAccessor->FinishTransaction();
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize + 1);
      WVariant newValue;
      W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, index));
      W_TEST_BOOL(newValue == inout_object);

      W_TEST_STATUS(doc.GetCommandHistory()->Undo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject);

      W_TEST_STATUS(doc.GetCommandHistory()->Redo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pObject, szProperty), iArraySize + 1);
      W_TEST_STATUS(pAccessor->GetValueByName(pObject, szProperty, newValue, index));
      W_TEST_BOOL(newValue == inout_object);
    };

    WUuid A = CreateGuid("ClassArray", 0);
    WUuid B = CreateGuid("ClassArray", 1);
    WUuid C = CreateGuid("ClassArray", 2);

    TestAddObject(pObjectTest, "ClassArray", 0, WGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassArray", 1, WGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassArray", 1, WGetStaticRTTI<OuterClass>(), B);

    A = CreateGuid("ClassPtrArray", 0);
    B = CreateGuid("ClassPtrArray", 1);
    C = CreateGuid("ClassPtrArray", 2);

    TestAddObject(pObjectTest, "ClassPtrArray", 0, WGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassPtrArray", 1, WGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassPtrArray", 1, WGetStaticRTTI<OuterClass>(), B);

    A = CreateGuid("SubObjectSet", 0);
    B = CreateGuid("SubObjectSet", 1);
    C = CreateGuid("SubObjectSet", 2);

    TestAddObject(pObjectTest, "SubObjectSet", 0, WGetStaticRTTI<WObjectTest>(), A);
    TestAddObject(pObjectTest, "SubObjectSet", 1, WGetStaticRTTI<WObjectTest>(), C);
    TestAddObject(pObjectTest, "SubObjectSet", 1, WGetStaticRTTI<WObjectTest>(), B);

    A = CreateGuid("ClassMap", 0);
    B = CreateGuid("ClassMap", 1);
    C = CreateGuid("ClassMap", 2);

    TestAddObject(pObjectTest, "ClassMap", "A", WGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassMap", "C", WGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassMap", "B", WGetStaticRTTI<OuterClass>(), B);

    A = CreateGuid("ClassPtrMap", 0);
    B = CreateGuid("ClassPtrMap", 1);
    C = CreateGuid("ClassPtrMap", 2);

    TestAddObject(pObjectTest, "ClassPtrMap", "A", WGetStaticRTTI<OuterClass>(), A);
    TestAddObject(pObjectTest, "ClassPtrMap", "C", WGetStaticRTTI<OuterClass>(), C);
    TestAddObject(pObjectTest, "ClassPtrMap", "B", WGetStaticRTTI<OuterClass>(), B);
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "MoveObject")
  {
    auto TestMoveObjectFailure = [&](const WDocumentObject* pObject, const char* szProperty, WVariant newIndex)
    {
      pAccessor->StartTransaction("MoveObject");
      W_TEST_BOOL(pAccessor->MoveObjectByName(pObject, pObject->GetParent(), szProperty, newIndex).Failed());
      pAccessor->CancelTransaction();
    };

    auto TestMoveObject = [&](const WDocumentObject* pObject, const char* szProperty, WVariant newIndex, WArrayPtr<WUuid> expectedOutcome)
    {
      WAbstractObjectGraph graph;
      StoreOriginalState(graph, pObject->GetParent());

      const WUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const WInt32 iArraySize = pAccessor->GetCountByName(pObject->GetParent(), szProperty);
      W_TEST_INT(iArraySize, expectedOutcome.GetCount());

      WDynamicArray<WVariant> values;
      W_TEST_STATUS(pAccessor->GetValuesByName(pObject->GetParent(), szProperty, values));
      W_TEST_INT(iArraySize, values.GetCount());

      pAccessor->StartTransaction("MoveObject");
      W_TEST_STATUS(pAccessor->MoveObjectByName(pObject, pObject->GetParent(), szProperty, newIndex));
      pAccessor->FinishTransaction();
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pObject->GetParent(), szProperty), iArraySize);

      for (WInt32 i = 0; i < iArraySize; i++)
      {
        WVariant newValue;
        W_TEST_STATUS(pAccessor->GetValueByName(pObject->GetParent(), szProperty, newValue, i));
        W_TEST_BOOL(newValue == expectedOutcome[i]);
      }

      W_TEST_STATUS(doc.GetCommandHistory()->Undo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      W_TEST_INT(pAccessor->GetCountByName(pObject->GetParent(), szProperty), iArraySize);
      CompareAgainstOriginalState(graph, pObject->GetParent());

      W_TEST_STATUS(doc.GetCommandHistory()->Redo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pObject->GetParent(), szProperty), iArraySize);

      for (WInt32 i = 0; i < iArraySize; i++)
      {
        WVariant newValue;
        W_TEST_STATUS(pAccessor->GetValueByName(pObject->GetParent(), szProperty, newValue, i));
        W_TEST_BOOL(newValue == expectedOutcome[i]);
      }
    };

    WUuid A = CreateGuid("ClassArray", 0);
    WUuid B = CreateGuid("ClassArray", 1);
    WUuid C = CreateGuid("ClassArray", 2);
    const WDocumentObject* pA = pAccessor->GetObject(A);
    const WDocumentObject* pB = pAccessor->GetObject(B);
    const WDocumentObject* pC = pAccessor->GetObject(C);

    {
      // Move first element before or after itself (no-op)
      TestMoveObjectFailure(pA, "ClassArray", 0);
      TestMoveObjectFailure(pA, "ClassArray", 1);
      // Move last element before or after itself (no-op)
      TestMoveObjectFailure(pC, "ClassArray", 2);
      TestMoveObjectFailure(pC, "ClassArray", 3);
    }
    {
      // Move first element to the end.
      WUuid expectedValues[3] = {B, C, A};
      TestMoveObject(pA, "ClassArray", 3, WArrayPtr<WUuid>(expectedValues));
    }
    {
      // Move last element to the front.
      WUuid expectedValues[3] = {A, B, C};
      TestMoveObject(pA, "ClassArray", 0, WArrayPtr<WUuid>(expectedValues));
    }
    {
      // Move first element to the middle
      WUuid expectedValues[3] = {B, A, C};
      TestMoveObject(pA, "ClassArray", 2, WArrayPtr<WUuid>(expectedValues));
    }
    {
      // Move last element to the middle
      WUuid expectedValues[3] = {B, C, A};
      TestMoveObject(pC, "ClassArray", 1, WArrayPtr<WUuid>(expectedValues));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveObject")
  {
    auto TestRemoveObject = [&](const WDocumentObject* pObject, WArrayPtr<WUuid> expectedOutcome)
    {
      auto pParent = pObject->GetParent();
      WString sProperty = pObject->GetParentProperty();

      WAbstractObjectGraph graph;
      StoreOriginalState(graph, pParent);
      const WUInt32 uiUndoHistorySize = doc.GetCommandHistory()->GetUndoStackSize();
      const WInt32 iArraySize = pAccessor->GetCountByName(pParent, sProperty);
      W_TEST_INT(iArraySize - 1, expectedOutcome.GetCount());


      WDynamicArray<WVariant> values;
      WDynamicArray<WVariant> keys;
      {
        W_TEST_STATUS(pAccessor->GetValuesByName(pParent, sProperty, values));
        W_TEST_INT(iArraySize, values.GetCount());

        W_TEST_STATUS(pAccessor->GetKeysByName(pParent, sProperty, keys));
        W_TEST_INT(iArraySize, keys.GetCount());
        WUInt32 uiIndex = keys.IndexOf(pObject->GetPropertyIndex());
        keys.RemoveAtAndSwap(uiIndex);
        values.RemoveAtAndSwap(uiIndex);
        W_TEST_INT(iArraySize - 1, keys.GetCount());
      }

      pAccessor->StartTransaction("RemoveValue");
      W_TEST_STATUS(pAccessor->RemoveObject(pObject));
      pAccessor->FinishTransaction();
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pParent, sProperty), iArraySize - 1);

      if (pParent->GetType()->FindPropertyByName(sProperty)->GetCategory() == WPropertyCategory::Map)
      {
        for (WInt32 i = 0; i < iArraySize - 1; i++)
        {
          const WVariant& key = keys[i];
          const WVariant& value = values[i];
          WVariant newValue;
          W_TEST_STATUS(pAccessor->GetValueByName(pParent, sProperty, newValue, key));
          W_TEST_BOOL(newValue == value);
          W_TEST_BOOL(expectedOutcome.Contains(newValue.Get<WUuid>()));
        }
      }
      else
      {
        for (WInt32 i = 0; i < iArraySize - 1; i++)
        {
          WVariant newValue;
          W_TEST_STATUS(pAccessor->GetValueByName(pParent, sProperty, newValue, i));
          W_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }

      W_TEST_STATUS(doc.GetCommandHistory()->Undo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 1);
      W_TEST_INT(pAccessor->GetCountByName(pParent, sProperty), iArraySize);
      CompareAgainstOriginalState(graph, pParent);

      W_TEST_STATUS(doc.GetCommandHistory()->Redo());
      W_TEST_INT(doc.GetCommandHistory()->GetUndoStackSize(), uiUndoHistorySize + 1);
      W_TEST_INT(doc.GetCommandHistory()->GetRedoStackSize(), 0);
      W_TEST_INT(pAccessor->GetCountByName(pParent, sProperty), iArraySize - 1);

      if (pParent->GetType()->FindPropertyByName(sProperty)->GetCategory() == WPropertyCategory::Map)
      {
        for (WInt32 i = 0; i < iArraySize - 1; i++)
        {
          const WVariant& key = keys[i];
          const WVariant& value = values[i];
          WVariant newValue;
          W_TEST_STATUS(pAccessor->GetValueByName(pParent, sProperty, newValue, key));
          W_TEST_BOOL(newValue == value);
          W_TEST_BOOL(expectedOutcome.Contains(newValue.Get<WUuid>()));
        }
      }
      else
      {
        for (WInt32 i = 0; i < iArraySize - 1; i++)
        {
          WVariant newValue;
          W_TEST_STATUS(pAccessor->GetValueByName(pParent, sProperty, newValue, i));
          W_TEST_BOOL(newValue == expectedOutcome[i]);
        }
      }
    };

    auto ClearContainer = [&](const char* szContainer)
    {
      WUuid A = CreateGuid(szContainer, 0);
      WUuid B = CreateGuid(szContainer, 1);
      WUuid C = CreateGuid(szContainer, 2);
      const WDocumentObject* pA = pAccessor->GetObject(A);
      const WDocumentObject* pB = pAccessor->GetObject(B);
      const WDocumentObject* pC = pAccessor->GetObject(C);
      {
        WUuid expectedValues[2] = {B, C};
        TestRemoveObject(pA, WArrayPtr<WUuid>(expectedValues));
      }
      {
        WUuid expectedValues[1] = {C};
        TestRemoveObject(pB, WArrayPtr<WUuid>(expectedValues));
      }
      {
        TestRemoveObject(pC, WArrayPtr<WUuid>());
      }
    };

    ClearContainer("ClassArray");
    ClearContainer("ClassPtrArray");
    ClearContainer("SubObjectSet");
    ClearContainer("ClassMap");
    ClearContainer("ClassPtrMap");
  }
}
