#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/MathExpression.h>
#include <Foundation/Containers/List.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/VarianceTypes.h>

#include <string>

#ifdef __clang__
#  pragma clang optimize off
#endif

class TestRefCounted : public WRefCounted
{
public:
  WUInt32 m_uiDummyMember = 0x42u;
};

// declare bitflags using macro magic
W_DECLARE_FLAGS(WUInt32, TestFlags, Bit1, Bit2, Bit3, Bit4);
W_DEFINE_AS_POD_TYPE(TestFlags::Enum);

struct TestFlagsManual
{
  using StorageType = WUInt32;

  enum Enum
  {
    Bit1 = W_BIT(0),
    Bit2 = W_BIT(1),
    Bit3 = W_BIT(2),
    Bit4 = W_BIT(3),
    MultiBits = Bit1 | Bit3,
    Default = 0
  };

  struct Bits
  {
    StorageType Bit1 : 1;
    StorageType Bit2 : 1;
    StorageType Bit3 : 1;
    StorageType Bit4 : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(TestFlagsManual);

class ReflectedTest : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(ReflectedTest, WReflectedClass);

public:
  float u;
  float v;
};

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(ReflectedTest, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("u", u),
    W_MEMBER_PROPERTY("v", v),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


W_CREATE_SIMPLE_TEST(CodeUtils, VisualizerZoo)
{
  struct StuffStruct
  {
    int a;
    float b;
    WString c;
  };
  W_TEST_BOOL(true);
  W_TEST_BLOCK(WTestBlock::Enabled, "Strings")
  {
    WString stringEmpty;
    WString string = u8"こんにちは 世界";
    WString* stringPtr = &string;
    WString stringArray[4] = {"AAA", "BBB", "CCC", "DDD"};
    WStringBuilder stringBuilder = "Test";
    WStringView stringViewEmpty;
    WStringView stringView = string.GetSubString(0, 5);
    WStringIterator stringIteratorEmpty;
    WStringIterator stringIterator = stringView.GetIteratorFront();
    stringIterator++;
    WStringReverseIterator stringReverseIteratorEmpty;
    WStringReverseIterator stringReverseIterator = stringView.GetIteratorBack();
    stringReverseIterator++;

    WHashedString hashedStringEmpty;
    WHashedString hashedString = WMakeHashedString("Test");
    W_TEST_BOOL(true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Containers")
  {
    WDynamicArray<WString> dynamicArray;
    dynamicArray.PushBack("Item1");
    dynamicArray.PushBack("Item2");

    WTempHybridArray<StuffStruct, 4> hybridArray;
    hybridArray.PushBack({1, 2.0f, "Item3"});
    hybridArray.PushBack({2, 3.0f, "Item4"});

    WTempHybridArray<StuffStruct, 1> hybridArray2;
    hybridArray2.PushBack({1, 2.0f, "Item3"});
    hybridArray2.PushBack({2, 3.0f, "Item4"});
    hybridArray2.PushBack({3, 4.0f, "Item5"});

    WSmallArray<WString, 66> smallArray;
    smallArray.PushBack("SmallItem1");
    smallArray.PushBack("SmallItem2");

    WSmallArray<WString, 2> smallArray2;
    smallArray2.PushBack("SmallItem1");
    smallArray2.PushBack("SmallItem2");
    smallArray2.PushBack("SmallItem3");
    smallArray2.PushBack("SmallItem4");

    WStaticArray<float, 2> staticArray;
    staticArray.SetCount(2);
    staticArray[0] = 1.0f;
    staticArray[1] = 2.0f;

    WHashTable<int, StuffStruct> hashTable;
    hashTable.Insert(1, StuffStruct{3, 4.0f, "HashItem1"});
    hashTable.Insert(99, StuffStruct{3, 4.0f, "HashItem1"});

    WHashSet<WString> hashSet;
    hashSet.Insert("HashSetItem1");
    hashSet.Insert("HashSetItem2");

    WList<WString> list;
    list.PushBack("ListItem1");
    list.PushBack("ListItem2");

    WDeque<WString> deque;
    deque.PushBack("DequeItem1");
    deque.PushBack("DequeItem2");
    deque.PushFront("DequeItem0");

    WMap<int, WString> map;
    map.Insert(1, "MapItem1");
    map.Insert(2, "MapItem2");

    WSet<WStringView> set;
    set.Insert("SetItem1");
    set.Insert("SetItem2");

    WStaticRingBuffer<StuffStruct, 4> staticRingBuffer;
    staticRingBuffer.PushBack({5, 6.0f, "StaticRingItem1"});
    staticRingBuffer.PushBack({7, 8.0f, "StaticRingItem2"});
    staticRingBuffer.PushBack({9, 10.0f, "StaticRingItem3"});
    staticRingBuffer.PushBack({11, 12.0f, "StaticRingItem4"});
    staticRingBuffer.PopFront();
    staticRingBuffer.PushBack({13, 14.0f, "StaticRingItem5"});

    WArrayPtr<WString> arrayPtr = WMakeArrayPtr(dynamicArray.GetData(), dynamicArray.GetCount());
    WArrayPtr<StuffStruct> hybridArrayPtr = WMakeArrayPtr(hybridArray.GetData(), hybridArray.GetCount());
    W_TEST_BOOL(true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WVariant")
  {
    WVariant variantInvalid; // Default constructor creates Invalid type
    WVariant variantBool = WVariant(true);
    WVariant variantInt8 = WVariant(static_cast<WInt8>(42));
    WVariant variantUInt8 = WVariant(static_cast<WUInt8>(42));
    WVariant variantInt16 = WVariant(static_cast<WInt16>(42));
    WVariant variantUInt16 = WVariant(static_cast<WUInt16>(42));
    WVariant variantInt32 = WVariant(static_cast<WInt32>(42));
    WVariant variantUInt32 = WVariant(static_cast<WUInt32>(42));
    WVariant variantInt64 = WVariant(static_cast<WInt64>(42));
    WVariant variantUInt64 = WVariant(static_cast<WUInt64>(42));
    WVariant variantFloat = WVariant(42.0f);
    WVariant variantDouble = WVariant(42.0);
    WVariant variantColor = WVariant(WColor(1.0f, 0.5f, 0.25f, 1.0f));
    WVariant variantVector2 = WVariant(WVec2(1.0f, 2.0f));
    WVariant variantVector3 = WVariant(WVec3(1.0f, 2.0f, 3.0f));
    WVariant variantVector4 = WVariant(WVec4(1.0f, 2.0f, 3.0f, 4.0f));
    WVariant variantVector2I = WVariant(WVec2I32(1, 2));
    WVariant variantVector3I = WVariant(WVec3I32(1, 2, 3));
    WVariant variantVector4I = WVariant(WVec4I32(1, 2, 3, 4));
    WVariant variantVector2U = WVariant(WVec2U32(1, 2));
    WVariant variantVector3U = WVariant(WVec3U32(1, 2, 3));
    WVariant variantVector4U = WVariant(WVec4U32(1, 2, 3, 4));
    WVariant variantQuaternion = WVariant(WQuat::MakeIdentity());
    WVariant variantMatrix3 = WVariant(WMat3::MakeIdentity());
    WVariant variantMatrix4 = WVariant(WMat4::MakeIdentity());
    WVariant variantTransform = WVariant(WTransform::MakeIdentity());
    WVariant variantString = WVariant("SampleString");
    WVariant variantStringView = WVariant(WStringView("SampleStringView"));

    WDataBuffer dataBuffer;
    dataBuffer.PushBack(1);
    dataBuffer.PushBack(2);
    dataBuffer.PushBack(3);
    WVariant variantDataBuffer = WVariant(dataBuffer);

    WVariant variantTime = WVariant(WTime::Seconds(42.0));
    WVariant variantUuid = WVariant(WUuid::MakeStableUuidFromInt(42));
    WVariant variantAngle = WVariant(WAngle::MakeFromDegree(45.0f));
    WVariant variantColorGamma = WVariant(WColorGammaUB(128, 192, 255, 255));
    WVariant variantHashedString = WVariant(WMakeHashedString("HashedSample"));
    WVariant variantTempHashedString = WVariant(WTempHashedString("TempHashedSample"));

    // Extended types
    WVariantArray varArray;
    varArray.PushBack(WVariant("Item1"));
    varArray.PushBack(WVariant(42));
    WVariant variantVariantArray = WVariant(varArray);

    WVariantDictionary varDict;
    varDict.Insert("Key1", WVariant("Value1"));
    varDict.Insert("Key2", WVariant(42));
    WVariant variantVariantDictionary = WVariant(varDict);

    ReflectedTest test;
    WVariant variantTypedPointer(&test);
    WTypedPointer ptr = {nullptr, WGetStaticRTTI<ReflectedTest>()};
    WVariant variantTypedPointerNull = ptr;

    WVarianceTypeAngle value2(WAngle::MakeFromRadian(1.57079637f), 0.2f);
    WVariant variantTypedObject(value2);
    W_TEST_BOOL(true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Enum")
  {
    WEnum<WVariantType> enumTest = WVariantType::Angle;
    WEnum<WVariantType> enumArray[3] = {WVariantType::Int32, WVariantType::String, WVariantType::Color};
    WTempHybridArray<WEnum<WVariantType>, 3> hybridEnumArray;
    hybridEnumArray.PushBack(WVariantType::Int32);
    hybridEnumArray.PushBack(WVariantType::String);
    W_TEST_BOOL(true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Bitflags")
  {
    TestFlags::Enum rawBitflags = TestFlags::Bit1;
    TestFlags::Enum rawBitflags2 = (TestFlags::Enum)(TestFlags::Bit1 | TestFlags::Bit2).GetValue();

    WBitflags<TestFlagsManual> bitflagsEmpty;
    WBitflags<TestFlagsManual> bitflags;
    bitflags.Add(TestFlagsManual::Bit1);
    bitflags.Add(TestFlagsManual::Bit3);

    WBitflags<TestFlags> bitflagsArray[2];
    bitflagsArray[0].Add(TestFlags::Bit3);
    bitflagsArray[1].Add(TestFlags::Bit4);

    WTempHybridArray<WBitflags<TestFlags>, 2> hybridBitflagsArray;
    hybridBitflagsArray.PushBack(TestFlags::Bit1 | TestFlags::Bit2);
    hybridBitflagsArray.PushBack(TestFlags::Bit3 | TestFlags::Bit4);
    W_TEST_BOOL(true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Math")
  {
    WVec2 vec2(1.0f, 2.0f);
    WVec3 vec3(1.0f, 2.0f, 3.0f);
    WVec4 vec4(1.0f, 2.0f, 3.0f, 4.0f);
    WVec2I32 vec2I(1, 2);
    WVec3I32 vec3I(1, 2, 3);
    WVec4I32 vec4I(1, 2, 3, 4);
    WVec2U32 vec2U(1, 2);
    WVec3U32 vec3U(1, 2, 3);
    WVec4U32 vec4U(1, 2, 3, 4);
    WQuat quat = WQuat::MakeFromAxisAndAngle(WVec3(0, 1, 0), WAngle::MakeFromDegree(90.0f));
    WMat3 mat3 = WMat3::MakeRotationZ(WAngle::MakeFromDegree(45.0f));
    WMat4 mat4 = WMat4::MakeRotationY(WAngle::MakeFromDegree(30.0f));
    WTransform transform(WVec3(1.0f, 2.0f, 3.0f), quat, WVec3(1.0f, 1.0f, 1.0f));
    WAngle angle = WAngle::MakeFromDegree(60.0f);
    WPlane plane = WPlane::MakeFromNormalAndPoint(WVec3(0, 1, 0), WVec3(0, 0, 0));

    WColor color(1.0f, 0.5f, 0.25f, 1.0f);
    WColorGammaUB colorGamma(128, 192, 255, 255);
    WColorLinearUB colorLinear(128, 192, 255, 255);
    WTime time = WTime::Seconds(42.0);

    WUuid uuid = WConversionUtils::ConvertStringToUuid("{ 01234567-89AB-CDEF-0123-456789ABCDEF }");
    WStringBuilder uuidString;
    WConversionUtils::ToString(uuid, uuidString);
    W_TEST_BOOL(true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "UniquePtr")
  {
    WUniquePtr<ReflectedTest> uniquePtr = W_DEFAULT_NEW(ReflectedTest);
    uniquePtr->u = 1.0f;
    uniquePtr->v = 2.0f;

    WSharedPtr<TestRefCounted> sharedPtr = W_DEFAULT_NEW(TestRefCounted);
    sharedPtr->m_uiDummyMember = 0x42u;

    TestRefCounted testRef;
    WScopedRefPointer<TestRefCounted> scopedRefPointer(&testRef);
    W_TEST_BOOL(true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Mutex")
  {
    WMutex mutex;
    W_LOCK(mutex);
    W_TEST_BOOL(true);
  }
}
