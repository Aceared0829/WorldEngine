#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WVariantTestStruct, WNoBase, 1, WRTTIDefaultAllocator<WVariantTestStruct>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Variant", m_Variant),
    W_ARRAY_MEMBER_PROPERTY("VariantArray", m_VariantArray),
    W_MAP_MEMBER_PROPERTY("VariantDictionary", m_VariantDictionary)
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WIntegerStruct, WNoBase, 1, WRTTIDefaultAllocator<WIntegerStruct>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Int8", GetInt8, SetInt8),
    W_ACCESSOR_PROPERTY("UInt8", GetUInt8, SetUInt8),
    W_MEMBER_PROPERTY("Int16", m_iInt16),
    W_MEMBER_PROPERTY("UInt16", m_iUInt16),
    W_ACCESSOR_PROPERTY("Int32", GetInt32, SetInt32),
    W_ACCESSOR_PROPERTY("UInt32", GetUInt32, SetUInt32),
    W_MEMBER_PROPERTY("Int64", m_iInt64),
    W_MEMBER_PROPERTY("UInt64", m_iUInt64),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;


W_BEGIN_STATIC_REFLECTED_TYPE(WFloatStruct, WNoBase, 1, WRTTIDefaultAllocator<WFloatStruct>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Float", GetFloat, SetFloat),
    W_ACCESSOR_PROPERTY("Double", GetDouble, SetDouble),
    W_ACCESSOR_PROPERTY("Time", GetTime, SetTime),
    W_ACCESSOR_PROPERTY("Angle", GetAngle, SetAngle),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPODClass, 1, WRTTIDefaultAllocator<WPODClass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Integer", m_IntegerStruct),
    W_MEMBER_PROPERTY("Float", m_FloatStruct),
    W_ACCESSOR_PROPERTY("Bool", GetBool, SetBool),
    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor),
    W_MEMBER_PROPERTY("ColorUB", m_Color2),
    W_ACCESSOR_PROPERTY("CharPtr", GetCharPtr, SetCharPtr),
    W_ACCESSOR_PROPERTY("String", GetString, SetString),
    W_ACCESSOR_PROPERTY("StringView", GetStringView, SetStringView),
    W_ACCESSOR_PROPERTY("Buffer", GetBuffer, SetBuffer),
    W_ACCESSOR_PROPERTY("VarianceAngle", GetCustom, SetCustom),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMathClass, 1, WRTTIDefaultAllocator<WMathClass>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Vec2", GetVec2, SetVec2),
    W_ACCESSOR_PROPERTY("Vec3", GetVec3, SetVec3),
    W_ACCESSOR_PROPERTY("Vec4", GetVec4, SetVec4),
    W_MEMBER_PROPERTY("Vec2I", m_Vec2I),
    W_MEMBER_PROPERTY("Vec3I", m_Vec3I),
    W_MEMBER_PROPERTY("Vec4I", m_Vec4I),
    W_ACCESSOR_PROPERTY("Quat", GetQuat, SetQuat),
    W_ACCESSOR_PROPERTY("Mat3", GetMat3, SetMat3),
    W_ACCESSOR_PROPERTY("Mat4", GetMat4, SetMat4),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;


W_BEGIN_STATIC_REFLECTED_ENUM(WExampleEnum, 1)
  W_ENUM_CONSTANTS(WExampleEnum::Value1, WExampleEnum::Value2)
  W_ENUM_CONSTANT(WExampleEnum::Value3),
W_END_STATIC_REFLECTED_ENUM;


W_BEGIN_STATIC_REFLECTED_BITFLAGS(WExampleBitflags, 1)
  W_BITFLAGS_CONSTANTS(WExampleBitflags::Value1, WExampleBitflags::Value2)
  W_BITFLAGS_CONSTANT(WExampleBitflags::Value3),
W_END_STATIC_REFLECTED_BITFLAGS;


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEnumerationsClass, 1, WRTTIDefaultAllocator<WEnumerationsClass>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("Enum", WExampleEnum, GetEnum, SetEnum),
    W_BITFLAGS_ACCESSOR_PROPERTY("Bitflags", WExampleBitflags, GetBitflags, SetBitflags),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;


W_BEGIN_STATIC_REFLECTED_TYPE(InnerStruct, WNoBase, 1, WRTTIDefaultAllocator<InnerStruct>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("IP1", m_fP1),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;


W_BEGIN_DYNAMIC_REFLECTED_TYPE(OuterClass, 1, WRTTIDefaultAllocator<OuterClass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Inner", m_Inner1),
    W_MEMBER_PROPERTY("OP1", m_fP1),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;


W_BEGIN_DYNAMIC_REFLECTED_TYPE(ExtendedOuterClass, 1, WRTTIDefaultAllocator<ExtendedOuterClass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MORE", m_more),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WObjectTest, 1, WRTTIDefaultAllocator<WObjectTest>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MemberClass", m_MemberClass),
    W_ARRAY_MEMBER_PROPERTY("StandardTypeArray", m_StandardTypeArray),
    W_ARRAY_MEMBER_PROPERTY("ClassArray", m_ClassArray),
    W_ARRAY_MEMBER_PROPERTY("ClassPtrArray", m_ClassPtrArray)->AddFlags(WPropertyFlags::PointerOwner),
    W_SET_ACCESSOR_PROPERTY("StandardTypeSet", GetStandardTypeSet, StandardTypeSetInsert, StandardTypeSetRemove),
    W_SET_MEMBER_PROPERTY("SubObjectSet", m_SubObjectSet)->AddFlags(WPropertyFlags::PointerOwner),
    W_MAP_MEMBER_PROPERTY("StandardTypeMap", m_StandardTypeMap),
    W_MAP_MEMBER_PROPERTY("ClassMap", m_ClassMap),
    W_MAP_MEMBER_PROPERTY("ClassPtrMap", m_ClassPtrMap)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMirrorTest, 1, WRTTIDefaultAllocator<WMirrorTest>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Math", m_math),
    W_MEMBER_PROPERTY("Object", m_object),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WArrayPtr<const WString> WObjectTest::GetStandardTypeSet() const
{
  return m_StandardTypeSet;
}

void WObjectTest::StandardTypeSetInsert(const WString& value)
{
  if (!m_StandardTypeSet.Contains(value))
    m_StandardTypeSet.PushBack(value);
}

void WObjectTest::StandardTypeSetRemove(const WString& value)
{
  m_StandardTypeSet.RemoveAndCopy(value);
}
