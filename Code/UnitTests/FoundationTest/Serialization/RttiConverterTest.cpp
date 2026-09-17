#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

W_CREATE_SIMPLE_TEST_GROUP(Serialization);

class TestContext : public WRttiConverterContext
{
public:
  virtual WInternal::NewInstance<void> CreateObject(const WUuid& guid, const WRTTI* pRtti) override
  {
    auto pObj = pRtti->GetAllocator()->Allocate<void>();
    RegisterObject(guid, pRtti, pObj);
    return pObj;
  }

  virtual void DeleteObject(const WUuid& guid) override
  {
    auto object = GetObjectByGUID(guid);
    object.m_pType->GetAllocator()->Deallocate(object.m_pObject);

    UnregisterObject(guid);
  }
};

template <typename T>
void TestSerialize(T* pObject)
{
  WAbstractObjectGraph graph;
  TestContext context;
  WRttiConverterWriter conv(&graph, &context, true, true);

  const WRTTI* pRtti = WGetStaticRTTI<T>();
  const WUuid guid = WUuid::MakeUuid();

  context.RegisterObject(guid, pRtti, pObject);
  WAbstractObjectNode* pNode = conv.AddObjectToGraph(pRtti, pObject, "root");

  W_TEST_BOOL(pNode->GetGuid() == guid);
  W_TEST_STRING(pNode->GetType(), pRtti->GetTypeName());
  W_TEST_INT(pNode->GetProperties().GetCount(), pNode->GetProperties().GetCount());

  {
    WContiguousMemoryStreamStorage storage;
    WMemoryStreamWriter writer(&storage);
    WMemoryStreamReader reader(&storage);

    WAbstractGraphDdlSerializer::Write(writer, &graph);

    WStringBuilder sData, sData2;
    sData.SetSubString_ElementCount((const char*)storage.GetData(), storage.GetStorageSize32());


    WRttiConverterReader convRead(&graph, &context);
    auto* pRootNode = graph.GetNodeByName("root");
    W_TEST_BOOL(pRootNode != nullptr);

    T target;
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    W_TEST_BOOL(target == *pObject);

    // Overwrite again to test for leaks as existing values have to be removed first by WRttiConverterReader.
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    W_TEST_BOOL(target == *pObject);

    {
      T clone;
      WReflectionSerializer::Clone(pObject, &clone, pRtti);
      W_TEST_BOOL(clone == *pObject);
      W_TEST_BOOL(WReflectionUtils::IsEqual(&clone, pObject, pRtti));
    }

    {
      T* pClone = WReflectionSerializer::Clone(pObject);
      W_TEST_BOOL(*pClone == *pObject);
      W_TEST_BOOL(WReflectionUtils::IsEqual(pClone, pObject));
      // Overwrite again to test for leaks as existing values have to be removed first by clone.
      WReflectionSerializer::Clone(pObject, pClone, pRtti);
      W_TEST_BOOL(*pClone == *pObject);
      W_TEST_BOOL(WReflectionUtils::IsEqual(pClone, pObject, pRtti));
      pRtti->GetAllocator()->Deallocate(pClone);
    }

    WAbstractObjectGraph graph2;
    WAbstractGraphDdlSerializer::Read(reader, &graph2).IgnoreResult();

    WContiguousMemoryStreamStorage storage2;
    WMemoryStreamWriter writer2(&storage2);

    WAbstractGraphDdlSerializer::Write(writer2, &graph2);
    sData2.SetSubString_ElementCount((const char*)storage2.GetData(), storage2.GetStorageSize32());

    W_TEST_BOOL(sData == sData2);
  }

  {
    WContiguousMemoryStreamStorage storage;
    WMemoryStreamWriter writer(&storage);
    WMemoryStreamReader reader(&storage);

    WAbstractGraphBinarySerializer::Write(writer, &graph);

    WRttiConverterReader convRead(&graph, &context);
    auto* pRootNode = graph.GetNodeByName("root");
    W_TEST_BOOL(pRootNode != nullptr);

    T target;
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    W_TEST_BOOL(target == *pObject);

    WAbstractObjectGraph graph2;
    WAbstractGraphBinarySerializer::Read(reader, &graph2);

    WContiguousMemoryStreamStorage storage2;
    WMemoryStreamWriter writer2(&storage2);

    WAbstractGraphBinarySerializer::Write(writer2, &graph2);

    W_TEST_INT(storage.GetStorageSize32(), storage2.GetStorageSize32());

    if (storage.GetStorageSize32() == storage2.GetStorageSize32())
    {
      W_TEST_BOOL(WMemoryUtils::RawByteCompare(storage.GetData(), storage2.GetData(), storage.GetStorageSize32()) == 0);
    }
  }
}

W_CREATE_SIMPLE_TEST(Serialization, RttiConverter)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "PODs")
  {
    WTestStruct t1;
    t1.m_fFloat1 = 5.0f;
    t1.m_UInt8 = 222;
    t1.m_variant = "A";
    t1.m_Angle = WAngle::MakeFromDegree(5);
    t1.m_DataBuffer.PushBack(1);
    t1.m_DataBuffer.PushBack(5);
    t1.m_vVec3I = WVec3I32(0, 1, 333);
    TestSerialize(&t1);

    {
      WTestStruct clone;
      WReflectionSerializer::Clone(&t1, &clone, WGetStaticRTTI<WTestStruct>());
      W_TEST_BOOL(t1 == clone);
      W_TEST_BOOL(WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestStruct>()));
      clone.m_variant = "Test";
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestStruct>()));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "EmbededStruct")
  {
    WTestClass1 t1;
    t1.m_Color = WColor::Yellow;
    t1.m_Struct.m_fFloat1 = 5.0f;
    t1.m_Struct.m_UInt8 = 222;
    t1.m_Struct.m_variant = "A";
    t1.m_Struct.m_Angle = WAngle::MakeFromDegree(5);
    t1.m_Struct.m_DataBuffer.PushBack(1);
    t1.m_Struct.m_DataBuffer.PushBack(5);
    t1.m_Struct.m_vVec3I = WVec3I32(0, 1, 333);
    TestSerialize(&t1);

    {
      WTestClass1 clone;
      WReflectionSerializer::Clone(&t1, &clone, WGetStaticRTTI<WTestClass1>());
      W_TEST_BOOL(t1 == clone);
      W_TEST_BOOL(WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestClass1>()));
      clone.m_Struct.m_DataBuffer[1] = 6;
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestClass1>()));
      clone.m_Struct.m_DataBuffer[1] = 5;
      clone.m_Struct.m_variant = WVec3(1, 2, 3);
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestClass1>()));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Enum")
  {
    WTestEnumStruct t1;
    t1.m_enum = WExampleEnum::Value2;
    t1.m_enumClass = WExampleEnum::Value3;
    t1.SetEnum(WExampleEnum::Value2);
    t1.SetEnumClass(WExampleEnum::Value3);
    TestSerialize(&t1);

    {
      WTestEnumStruct clone;
      WReflectionSerializer::Clone(&t1, &clone, WGetStaticRTTI<WTestEnumStruct>());
      W_TEST_BOOL(t1 == clone);
      W_TEST_BOOL(WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestEnumStruct>()));
      clone.m_enum = WExampleEnum::Value3;
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestEnumStruct>()));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Bitflags")
  {
    WTestBitflagsStruct t1;
    t1.m_bitflagsClass.SetValue(0);
    t1.SetBitflagsClass(WExampleBitflags::Value1 | WExampleBitflags::Value2);
    TestSerialize(&t1);

    {
      WTestBitflagsStruct clone;
      WReflectionSerializer::Clone(&t1, &clone, WGetStaticRTTI<WTestBitflagsStruct>());
      W_TEST_BOOL(t1 == clone);
      W_TEST_BOOL(WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestBitflagsStruct>()));
      clone.m_bitflagsClass = WExampleBitflags::Value1;
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestBitflagsStruct>()));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Derived Class")
  {
    WTestClass2 t1;
    t1.m_Color = WColor::Yellow;
    t1.m_Struct.m_fFloat1 = 5.0f;
    t1.m_Struct.m_UInt8 = 222;
    t1.m_Struct.m_variant = "A";
    t1.m_Struct.m_Angle = WAngle::MakeFromDegree(5);
    t1.m_Struct.m_DataBuffer.PushBack(1);
    t1.m_Struct.m_DataBuffer.PushBack(5);
    t1.m_Struct.m_vVec3I = WVec3I32(0, 1, 333);
    t1.m_Time = WTime::MakeFromSeconds(22.2f);
    t1.m_enumClass = WExampleEnum::Value3;
    t1.m_bitflagsClass = WExampleBitflags::Value1 | WExampleBitflags::Value2;
    t1.m_array.PushBack(40.0f);
    t1.m_array.PushBack(-1.5f);
    t1.m_Variant = WVec4(1, 2, 3, 4);
    t1.SetCharPtr("Hello");
    t1.SetString("World");
    t1.SetStringView("!!!");
    TestSerialize(&t1);

    {
      WTestClass2 clone;
      WReflectionSerializer::Clone(&t1, &clone, WGetStaticRTTI<WTestClass2>());
      W_TEST_BOOL(t1 == clone);
      W_TEST_BOOL(WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestClass2>()));
      clone.m_Struct.m_DataBuffer[1] = 6;
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestClass2>()));
      clone.m_Struct.m_DataBuffer[1] = 5;
      t1.m_array.PushBack(-1.33f);
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestClass2>()));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Arrays")
  {
    WTestArrays t1;
    t1.m_Hybrid.PushBack(4.5f);
    t1.m_Hybrid.PushBack(2.3f);
    t1.m_HybridChar.PushBack("Test");

    WTestStruct3 ts;
    ts.m_fFloat1 = 5.0f;
    ts.m_UInt8 = 22;
    t1.m_Dynamic.PushBack(ts);
    t1.m_Dynamic.PushBack(ts);
    t1.m_Deque.PushBack(WTestArrays());
    TestSerialize(&t1);

    {
      WTestArrays clone;
      WReflectionSerializer::Clone(&t1, &clone, WGetStaticRTTI<WTestArrays>());
      W_TEST_BOOL(t1 == clone);
      W_TEST_BOOL(WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestArrays>()));
      clone.m_Dynamic.PushBack(WTestStruct3());
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestArrays>()));
      clone.m_Dynamic.PopBack();
      clone.m_Hybrid.PushBack(444.0f);
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestArrays>()));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Sets")
  {
    WTestSets t1;
    t1.m_SetMember.Insert(0);
    t1.m_SetMember.Insert(5);
    t1.m_SetMember.Insert(-33);
    t1.m_SetAccessor.Insert(-0.0f);
    t1.m_SetAccessor.Insert(5.4f);
    t1.m_SetAccessor.Insert(-33.0f);
    t1.m_Deque.PushBack(3);
    t1.m_Deque.PushBack(33);
    t1.m_Array.PushBack("Test");
    t1.m_Array.PushBack("Bla");
    TestSerialize(&t1);

    {
      WTestSets clone;
      WReflectionSerializer::Clone(&t1, &clone, WGetStaticRTTI<WTestSets>());
      W_TEST_BOOL(t1 == clone);
      W_TEST_BOOL(WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestSets>()));
      clone.m_SetMember.Insert(12);
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestSets>()));
      clone.m_SetMember.Remove(12);
      clone.m_Array.PushBack("Bla2");
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestSets>()));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Pointer")
  {
    WTestPtr t1;
    t1.m_sString = "Ttttest";
    t1.m_pArrays = W_DEFAULT_NEW(WTestArrays);
    t1.m_pArraysDirect = W_DEFAULT_NEW(WTestArrays);
    t1.m_ArrayPtr.PushBack(W_DEFAULT_NEW(WTestArrays));
    t1.m_SetPtr.Insert(W_DEFAULT_NEW(WTestSets));
    TestSerialize(&t1);

    {
      WTestPtr clone;
      WReflectionSerializer::Clone(&t1, &clone, WGetStaticRTTI<WTestPtr>());
      W_TEST_BOOL(t1 == clone);
      W_TEST_BOOL(WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestPtr>()));
      clone.m_SetPtr.GetIterator().Key()->m_Deque.PushBack(42);
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestPtr>()));
      clone.m_SetPtr.GetIterator().Key()->m_Deque.PopBack();
      clone.m_ArrayPtr[0]->m_Hybrid.PushBack(123.0f);
      W_TEST_BOOL(!WReflectionUtils::IsEqual(&t1, &clone, WGetStaticRTTI<WTestPtr>()));
    }
  }
}
