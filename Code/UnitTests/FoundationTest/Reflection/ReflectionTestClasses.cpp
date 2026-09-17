#include <FoundationTest/FoundationTestPCH.h>

#include <FoundationTest/Reflection/ReflectionTestClasses.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WExampleEnum, 1)
  W_ENUM_CONSTANTS(WExampleEnum::Value1, WExampleEnum::Value2)
  W_ENUM_CONSTANT(WExampleEnum::Value3),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WExampleBitflags, 1)
  W_BITFLAGS_CONSTANTS(WExampleBitflags::Value1, WExampleBitflags::Value2)
  W_BITFLAGS_CONSTANT(WExampleBitflags::Value3),
W_END_STATIC_REFLECTED_BITFLAGS;


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAbstractTestClass, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;


W_BEGIN_STATIC_REFLECTED_TYPE(WAbstractTestStruct, WNoBase, 1, WRTTINoAllocator);
W_END_STATIC_REFLECTED_TYPE;


W_BEGIN_STATIC_REFLECTED_TYPE(WTestStruct, WNoBase, 7, WRTTIDefaultAllocator<WTestStruct>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Float", m_fFloat1)->AddAttributes(new WDefaultValueAttribute(1.1f)),
    W_MEMBER_PROPERTY_READ_ONLY("Vector", m_vProperty3)->AddAttributes(new WDefaultValueAttribute(WVec3(3.0f,4.0f,5.0f))),
    W_ACCESSOR_PROPERTY("Int", GetInt, SetInt)->AddAttributes(new WDefaultValueAttribute(2)),
    W_MEMBER_PROPERTY("UInt8", m_UInt8)->AddAttributes(new WDefaultValueAttribute(6)),
    W_MEMBER_PROPERTY("Variant", m_variant)->AddAttributes(new WDefaultValueAttribute("Test")),
    W_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(0.5))),
    W_MEMBER_PROPERTY("DataBuffer", m_DataBuffer)->AddAttributes(new WDefaultValueAttribute(WTestStruct::GetDefaultDataBuffer())),
    W_MEMBER_PROPERTY("vVec3I", m_vVec3I)->AddAttributes(new WDefaultValueAttribute(WVec3I32(1,2,3))),
    W_MEMBER_PROPERTY("VarianceAngle", m_VarianceAngle)->AddAttributes(new WDefaultValueAttribute(WVarianceTypeAngle(WAngle::MakeFromDegree(90.0f), 0.5f))),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WTestStruct3, WNoBase, 71, WRTTIDefaultAllocator<WTestStruct3>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Float", m_fFloat1)->AddAttributes(new WDefaultValueAttribute(33.3f)),
    W_ACCESSOR_PROPERTY("Int", GetInt, SetInt),
    W_MEMBER_PROPERTY("UInt8", m_UInt8),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(),
    W_CONSTRUCTOR_PROPERTY(double, WInt16),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WTypedObjectStruct, WNoBase, 1, WRTTIDefaultAllocator<WTypedObjectStruct>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Float", m_fFloat1)->AddAttributes(new WDefaultValueAttribute(33.3f)),
    W_MEMBER_PROPERTY("Int", m_iInt32),
    W_MEMBER_PROPERTY("UInt8", m_UInt8),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTestClass1, 11, WRTTIDefaultAllocator<WTestClass1>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("SubStruct", m_Struct),
    // W_MEMBER_PROPERTY("MyVector", m_MyVector), Intentionally not reflected
    W_MEMBER_PROPERTY("Color", m_Color),
    W_ACCESSOR_PROPERTY_READ_ONLY("SubVector", GetVector)->AddAttributes(new WDefaultValueAttribute(WVec3(3, 4, 5)))
  }
    W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

WInt32 WTestClass2Allocator::m_iAllocs = 0;
WInt32 WTestClass2Allocator::m_iDeallocs = 0;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTestClass2, 22, WTestClass2Allocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("CharPtr", GetCharPtr, SetCharPtr)->AddAttributes(new WDefaultValueAttribute("AAA")),
    W_ACCESSOR_PROPERTY("String", GetString, SetString)->AddAttributes(new WDefaultValueAttribute("BBB")),
    W_ACCESSOR_PROPERTY("StringView", GetStringView, SetStringView)->AddAttributes(new WDefaultValueAttribute("CCC")),
    W_MEMBER_PROPERTY("Time", m_Time),
    W_ENUM_MEMBER_PROPERTY("Enum", WExampleEnum, m_enumClass),
    W_BITFLAGS_MEMBER_PROPERTY("Bitflags", WExampleBitflags, m_bitflagsClass),
    W_ARRAY_MEMBER_PROPERTY("Array", m_array),
    W_MEMBER_PROPERTY("Variant", m_Variant),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTestClass2b, 24, WRTTIDefaultAllocator<WTestClass2b>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Text2b", GetText, SetText),
    W_MEMBER_PROPERTY("SubStruct", m_Struct),
    W_MEMBER_PROPERTY("Color", m_Color),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTestArrays, 1, WRTTIDefaultAllocator<WTestArrays>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Hybrid", m_Hybrid),
    W_ARRAY_MEMBER_PROPERTY("HybridChar", m_HybridChar),
    W_ARRAY_MEMBER_PROPERTY("Dynamic", m_Dynamic),
    W_ARRAY_MEMBER_PROPERTY("Deque", m_Deque),
    W_ARRAY_MEMBER_PROPERTY("Custom", m_CustomVariant),

    W_ARRAY_MEMBER_PROPERTY_READ_ONLY("HybridRO", m_Hybrid),
    W_ARRAY_MEMBER_PROPERTY_READ_ONLY("HybridCharRO", m_HybridChar),
    W_ARRAY_MEMBER_PROPERTY_READ_ONLY("DynamicRO", m_Dynamic),
    W_ARRAY_MEMBER_PROPERTY_READ_ONLY("DequeRO", m_Deque),
    W_ARRAY_MEMBER_PROPERTY_READ_ONLY("CustomRO", m_CustomVariant),

    W_ARRAY_ACCESSOR_PROPERTY("AcHybrid", GetCount, GetValue, SetValue, Insert, Remove),
    W_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcHybridRO", GetCount, GetValue),
    W_ARRAY_ACCESSOR_PROPERTY("AcHybridChar", GetCountChar, GetValueChar, SetValueChar, InsertChar, RemoveChar),
    W_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcHybridCharRO", GetCountChar, GetValueChar),
    W_ARRAY_ACCESSOR_PROPERTY("AcDynamic", GetCountDyn, GetValueDyn, SetValueDyn, InsertDyn, RemoveDyn),
    W_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcDynamicRO", GetCountDyn, GetValueDyn),
    W_ARRAY_ACCESSOR_PROPERTY("AcDeque", GetCountDeq, GetValueDeq, SetValueDeq, InsertDeq, RemoveDeq),
    W_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcDequeRO", GetCountDeq, GetValueDeq),
    W_ARRAY_ACCESSOR_PROPERTY("AcCustom", GetCountCustom, GetValueCustom, SetValueCustom, InsertCustom, RemoveCustom),
    W_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcCustomRO", GetCountCustom, GetValueCustom),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WUInt32 WTestArrays::GetCount() const
{
  return m_Hybrid.GetCount();
}
double WTestArrays::GetValue(WUInt32 uiIndex) const
{
  return m_Hybrid[uiIndex];
}
void WTestArrays::SetValue(WUInt32 uiIndex, double value)
{
  m_Hybrid[uiIndex] = value;
}
void WTestArrays::Insert(WUInt32 uiIndex, double value)
{
  m_Hybrid.InsertAt(uiIndex, value);
}
void WTestArrays::Remove(WUInt32 uiIndex)
{
  m_Hybrid.RemoveAtAndCopy(uiIndex);
}

WUInt32 WTestArrays::GetCountChar() const
{
  return m_HybridChar.GetCount();
}
const char* WTestArrays::GetValueChar(WUInt32 uiIndex) const
{
  return m_HybridChar[uiIndex];
}
void WTestArrays::SetValueChar(WUInt32 uiIndex, const char* value)
{
  m_HybridChar[uiIndex] = value;
}
void WTestArrays::InsertChar(WUInt32 uiIndex, const char* value)
{
  m_HybridChar.InsertAt(uiIndex, value);
}
void WTestArrays::RemoveChar(WUInt32 uiIndex)
{
  m_HybridChar.RemoveAtAndCopy(uiIndex);
}

WUInt32 WTestArrays::GetCountDyn() const
{
  return m_Dynamic.GetCount();
}
const WTestStruct3& WTestArrays::GetValueDyn(WUInt32 uiIndex) const
{
  return m_Dynamic[uiIndex];
}
void WTestArrays::SetValueDyn(WUInt32 uiIndex, const WTestStruct3& value)
{
  m_Dynamic[uiIndex] = value;
}
void WTestArrays::InsertDyn(WUInt32 uiIndex, const WTestStruct3& value)
{
  m_Dynamic.InsertAt(uiIndex, value);
}
void WTestArrays::RemoveDyn(WUInt32 uiIndex)
{
  m_Dynamic.RemoveAtAndCopy(uiIndex);
}

WUInt32 WTestArrays::GetCountDeq() const
{
  return m_Deque.GetCount();
}
const WTestArrays& WTestArrays::GetValueDeq(WUInt32 uiIndex) const
{
  return m_Deque[uiIndex];
}
void WTestArrays::SetValueDeq(WUInt32 uiIndex, const WTestArrays& value)
{
  m_Deque[uiIndex] = value;
}
void WTestArrays::InsertDeq(WUInt32 uiIndex, const WTestArrays& value)
{
  m_Deque.InsertAt(uiIndex, value);
}
void WTestArrays::RemoveDeq(WUInt32 uiIndex)
{
  m_Deque.RemoveAtAndCopy(uiIndex);
}

WUInt32 WTestArrays::GetCountCustom() const
{
  return m_CustomVariant.GetCount();
}
WVarianceTypeAngle WTestArrays::GetValueCustom(WUInt32 uiIndex) const
{
  return m_CustomVariant[uiIndex];
}
void WTestArrays::SetValueCustom(WUInt32 uiIndex, WVarianceTypeAngle value)
{
  m_CustomVariant[uiIndex] = value;
}
void WTestArrays::InsertCustom(WUInt32 uiIndex, WVarianceTypeAngle value)
{
  m_CustomVariant.InsertAt(uiIndex, value);
}
void WTestArrays::RemoveCustom(WUInt32 uiIndex)
{
  m_CustomVariant.RemoveAtAndCopy(uiIndex);
}

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTestSets, 1, WRTTIDefaultAllocator<WTestSets>)
{
  W_BEGIN_PROPERTIES
  {
    W_SET_MEMBER_PROPERTY("Set", m_SetMember),
    W_SET_MEMBER_PROPERTY_READ_ONLY("SetRO", m_SetMember),
    W_SET_ACCESSOR_PROPERTY("AcSet", GetSet, Insert, Remove),
    W_SET_ACCESSOR_PROPERTY_READ_ONLY("AcSetRO", GetSet),
    W_SET_MEMBER_PROPERTY("HashSet", m_HashSetMember),
    W_SET_MEMBER_PROPERTY_READ_ONLY("HashSetRO", m_HashSetMember),
    W_SET_ACCESSOR_PROPERTY("HashAcSet", GetHashSet, HashInsert, HashRemove),
    W_SET_ACCESSOR_PROPERTY_READ_ONLY("HashAcSetRO", GetHashSet),
    W_SET_ACCESSOR_PROPERTY("AcPseudoSet", GetPseudoSet, PseudoInsert, PseudoRemove),
    W_SET_ACCESSOR_PROPERTY_READ_ONLY("AcPseudoSetRO", GetPseudoSet),
    W_SET_ACCESSOR_PROPERTY("AcPseudoSet2", GetPseudoSet2, PseudoInsert2, PseudoRemove2),
    W_SET_ACCESSOR_PROPERTY_READ_ONLY("AcPseudoSet2RO", GetPseudoSet2),
    W_SET_ACCESSOR_PROPERTY("AcPseudoSet2b", GetPseudoSet2, PseudoInsert2b, PseudoRemove2b),
    W_SET_MEMBER_PROPERTY("CustomHashSet", m_CustomVariant),
    W_SET_MEMBER_PROPERTY_READ_ONLY("CustomHashSetRO", m_CustomVariant),
    W_SET_ACCESSOR_PROPERTY("CustomHashAcSet", GetCustomHashSet, CustomHashInsert, CustomHashRemove),
    W_SET_ACCESSOR_PROPERTY_READ_ONLY("CustomHashAcSetRO", GetCustomHashSet),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WSet<double>& WTestSets::GetSet() const
{
  return m_SetAccessor;
}

void WTestSets::Insert(double value)
{
  m_SetAccessor.Insert(value);
}

void WTestSets::Remove(double value)
{
  m_SetAccessor.Remove(value);
}


const WHashSet<WInt64>& WTestSets::GetHashSet() const
{
  return m_HashSetAccessor;
}

void WTestSets::HashInsert(WInt64 value)
{
  m_HashSetAccessor.Insert(value);
}

void WTestSets::HashRemove(WInt64 value)
{
  m_HashSetAccessor.Remove(value);
}

const WDeque<int>& WTestSets::GetPseudoSet() const
{
  return m_Deque;
}

void WTestSets::PseudoInsert(int value)
{
  if (!m_Deque.Contains(value))
    m_Deque.PushBack(value);
}

void WTestSets::PseudoRemove(int value)
{
  m_Deque.RemoveAndCopy(value);
}


WArrayPtr<const WString> WTestSets::GetPseudoSet2() const
{
  return m_Array;
}

void WTestSets::PseudoInsert2(const WString& value)
{
  if (!m_Array.Contains(value))
    m_Array.PushBack(value);
}

void WTestSets::PseudoRemove2(const WString& value)
{
  m_Array.RemoveAndCopy(value);
}

void WTestSets::PseudoInsert2b(const char* value)
{
  if (!m_Array.Contains(value))
    m_Array.PushBack(value);
}

void WTestSets::PseudoRemove2b(const char* value)
{
  m_Array.RemoveAndCopy(value);
}

const WHashSet<WVarianceTypeAngle>& WTestSets::GetCustomHashSet() const
{
  return m_CustomVariant;
}

void WTestSets::CustomHashInsert(WVarianceTypeAngle value)
{
  m_CustomVariant.Insert(value);
}

void WTestSets::CustomHashRemove(WVarianceTypeAngle value)
{
  m_CustomVariant.Remove(value);
}

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTestMaps, 1, WRTTIDefaultAllocator<WTestMaps>)
{
  W_BEGIN_PROPERTIES
  {
    W_MAP_MEMBER_PROPERTY("Map", m_MapMember),
    W_MAP_MEMBER_PROPERTY_READ_ONLY("MapRO", m_MapMember),
    W_MAP_WRITE_ACCESSOR_PROPERTY("AcMap", GetContainer, Insert, Remove),
    W_MAP_MEMBER_PROPERTY("HashTable", m_HashTableMember),
    W_MAP_MEMBER_PROPERTY_READ_ONLY("HashTableRO", m_HashTableMember),
    W_MAP_WRITE_ACCESSOR_PROPERTY("AcHashTable", GetContainer2, Insert2, Remove2),
    W_MAP_ACCESSOR_PROPERTY("Accessor", GetKeys3, GetValue3, Insert3, Remove3),
    W_MAP_ACCESSOR_PROPERTY_READ_ONLY("AccessorRO", GetKeys3, GetValue3),
    W_MAP_MEMBER_PROPERTY("CustomVariant", m_CustomVariant),
    W_MAP_MEMBER_PROPERTY_READ_ONLY("CustomVariantRO", m_CustomVariant),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool WTestMaps::operator==(const WTestMaps& rhs) const
{
  for (WUInt32 i = 0; i < m_Accessor3.GetCount(); i++)
  {
    bool bRes = false;
    for (WUInt32 j = 0; j < rhs.m_Accessor3.GetCount(); j++)
    {
      if (m_Accessor3[i].m_Key == rhs.m_Accessor3[j].m_Key)
      {
        if (m_Accessor3[i].m_Value == rhs.m_Accessor3[j].m_Value)
          bRes = true;
      }
    }
    if (!bRes)
      return false;
  }
  return m_MapMember == rhs.m_MapMember && m_MapAccessor == rhs.m_MapAccessor && m_HashTableMember == rhs.m_HashTableMember && m_HashTableAccessor == rhs.m_HashTableAccessor && m_CustomVariant == rhs.m_CustomVariant;
}

const WMap<WString, WInt64>& WTestMaps::GetContainer() const
{
  return m_MapAccessor;
}

void WTestMaps::Insert(const char* szKey, WInt64 value)
{
  m_MapAccessor.Insert(szKey, value);
}

void WTestMaps::Remove(const char* szKey)
{
  m_MapAccessor.Remove(szKey);
}

const WHashTable<WString, WString>& WTestMaps::GetContainer2() const
{
  return m_HashTableAccessor;
}

void WTestMaps::Insert2(const char* szKey, const WString& value)
{
  m_HashTableAccessor.Insert(szKey, value);
}


void WTestMaps::Remove2(const char* szKey)
{
  m_HashTableAccessor.Remove(szKey);
}

const WRangeView<const char*, WUInt32> WTestMaps::GetKeys3() const
{
  return WRangeView<const char*, WUInt32>([this]() -> WUInt32
    { return 0; },
    [this]() -> WUInt32
    { return m_Accessor3.GetCount(); },
    [this](WUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const WUInt32& uiIt) -> const char*
    { return m_Accessor3[uiIt].m_Key; });
}

void WTestMaps::Insert3(const char* szKey, const WVariant& value)
{
  for (auto&& t : m_Accessor3)
  {
    if (t.m_Key == szKey)
    {
      t.m_Value = value;
      return;
    }
  }
  auto&& t = m_Accessor3.ExpandAndGetRef();
  t.m_Key = szKey;
  t.m_Value = value;
}

void WTestMaps::Remove3(const char* szKey)
{
  for (WUInt32 i = 0; i < m_Accessor3.GetCount(); i++)
  {
    const Tuple& t = m_Accessor3[i];
    if (t.m_Key == szKey)
    {
      m_Accessor3.RemoveAtAndSwap(i);
      break;
    }
  }
}

bool WTestMaps::GetValue3(const char* szKey, WVariant& out_value) const
{
  for (const auto& t : m_Accessor3)
  {
    if (t.m_Key == szKey)
    {
      out_value = t.m_Value;
      return true;
    }
  }
  return false;
}

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTestPtr, 1, WRTTIDefaultAllocator<WTestPtr>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("ConstCharPtr", GetString, SetString),
    W_ACCESSOR_PROPERTY("ArraysPtr", GetArrays, SetArrays)->AddFlags(WPropertyFlags::PointerOwner),
    W_MEMBER_PROPERTY("ArraysPtrDirect", m_pArraysDirect)->AddFlags(WPropertyFlags::PointerOwner),
    W_ARRAY_MEMBER_PROPERTY("PtrArray", m_ArrayPtr)->AddFlags(WPropertyFlags::PointerOwner),
    W_SET_MEMBER_PROPERTY("PtrSet", m_SetPtr)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;


W_BEGIN_STATIC_REFLECTED_TYPE(WTestEnumStruct, WNoBase, 1, WRTTIDefaultAllocator<WTestEnumStruct>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("m_enum", WExampleEnum, m_enum),
    W_ENUM_MEMBER_PROPERTY("m_enumClass", WExampleEnum, m_enumClass),
    W_ENUM_ACCESSOR_PROPERTY("m_enum2", WExampleEnum, GetEnum, SetEnum),
    W_ENUM_ACCESSOR_PROPERTY("m_enumClass2", WExampleEnum,  GetEnumClass, SetEnumClass),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WTestBitflagsStruct, WNoBase, 1, WRTTIDefaultAllocator<WTestBitflagsStruct>)
{
  W_BEGIN_PROPERTIES
  {
    W_BITFLAGS_MEMBER_PROPERTY("m_bitflagsClass", WExampleBitflags, m_bitflagsClass),
    W_BITFLAGS_ACCESSOR_PROPERTY("m_bitflagsClass2", WExampleBitflags, GetBitflagsClass, SetBitflagsClass),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on
