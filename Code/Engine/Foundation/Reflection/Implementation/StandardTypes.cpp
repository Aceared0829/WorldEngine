#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WEnumBase, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WBitflagsBase, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WReflectedClass, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

// *********************************************
// ***** Standard POD Types for Properties *****

W_BEGIN_STATIC_REFLECTED_TYPE(bool, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(float, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(double, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WInt8, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WUInt8, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WInt16, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WUInt16, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WInt32, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WUInt32, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WInt64, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WUInt64, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WConstCharPtr, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WTime, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(MakeFromNanoseconds, In, "Nanoseconds")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeFromMicroseconds, In, "Microseconds")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeFromMilliseconds, In, "Milliseconds")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeFromSeconds, In, "Seconds")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeFromMinutes, In, "Minutes")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeFromHours, In, "Hours")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeZero)->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(AsFloatInSeconds),
    W_SCRIPT_FUNCTION_PROPERTY(GetMilliseconds),
    W_SCRIPT_FUNCTION_PROPERTY(GetSeconds),
    W_SCRIPT_FUNCTION_PROPERTY(GetMinutes),
    W_SCRIPT_FUNCTION_PROPERTY(GetHours),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WColor, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("r", r),
    W_MEMBER_PROPERTY("g", g),
    W_MEMBER_PROPERTY("b", b),
    W_MEMBER_PROPERTY("a", a),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(float, float, float),
    W_CONSTRUCTOR_PROPERTY(float, float, float, float),
    W_CONSTRUCTOR_PROPERTY(WColorLinearUB),
    W_CONSTRUCTOR_PROPERTY(WColorGammaUB),
    W_SCRIPT_FUNCTION_PROPERTY(MakeRGBA, In, "R", In, "G", In, "B", In, "A")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeHSV, In, "Hue", In, "Saturation", In, "Value")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(GetSaturation),
    W_SCRIPT_FUNCTION_PROPERTY(GetLuminance),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WColorBaseUB, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("r", r),
    W_MEMBER_PROPERTY("g", g),
    W_MEMBER_PROPERTY("b", b),
    W_MEMBER_PROPERTY("a", a),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WUInt8, WUInt8, WUInt8),
    W_CONSTRUCTOR_PROPERTY(WUInt8, WUInt8, WUInt8, WUInt8),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WColorGammaUB, WColorBaseUB, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WUInt8, WUInt8, WUInt8),
    W_CONSTRUCTOR_PROPERTY(WUInt8, WUInt8, WUInt8, WUInt8),
    W_CONSTRUCTOR_PROPERTY(const WColor&),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WColorLinearUB, WColorBaseUB, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WUInt8, WUInt8, WUInt8),
    W_CONSTRUCTOR_PROPERTY(WUInt8, WUInt8, WUInt8, WUInt8),
    W_CONSTRUCTOR_PROPERTY(const WColor&),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVec2, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("x", x),
    W_MEMBER_PROPERTY("y", y),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(float),
    W_CONSTRUCTOR_PROPERTY(float, float),
    W_SCRIPT_FUNCTION_PROPERTY(Make, In, "X", In, "Y")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(GetLength<float>),
    W_SCRIPT_FUNCTION_PROPERTY(GetLengthSquared),
    W_SCRIPT_FUNCTION_PROPERTY(GetDistanceTo<float>, In, "v"),
    W_SCRIPT_FUNCTION_PROPERTY(GetSquaredDistanceTo<float>, In, "v"),
    W_SCRIPT_FUNCTION_PROPERTY(GetNormalized<float>),
    W_SCRIPT_FUNCTION_PROPERTY(Dot, In, "v"),
    W_SCRIPT_FUNCTION_PROPERTY(GetAsVec3, In , "Z"),
    W_SCRIPT_FUNCTION_PROPERTY(GetAsVec4, In , "Z", In, "W"),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVec3, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("x", x),
    W_MEMBER_PROPERTY("y", y),
    W_MEMBER_PROPERTY("z", z),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(float),
    W_CONSTRUCTOR_PROPERTY(float, float, float),
    W_SCRIPT_FUNCTION_PROPERTY(Make, In, "X", In, "Y", In, "Z")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(GetLength<float>),
    W_SCRIPT_FUNCTION_PROPERTY(GetLengthSquared),
    W_SCRIPT_FUNCTION_PROPERTY(GetDistanceTo<float>, In, "v"),
    W_SCRIPT_FUNCTION_PROPERTY(GetSquaredDistanceTo<float>, In, "v"),
    W_SCRIPT_FUNCTION_PROPERTY(GetNormalized<float>),
    W_SCRIPT_FUNCTION_PROPERTY(Dot, In, "v"),
    W_SCRIPT_FUNCTION_PROPERTY(CrossRH, In, "v"),
    W_SCRIPT_FUNCTION_PROPERTY(GetAsVec2),
    W_SCRIPT_FUNCTION_PROPERTY(GetAsVec4, In, "W"),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVec4, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("x", x),
    W_MEMBER_PROPERTY("y", y),
    W_MEMBER_PROPERTY("z", z),
    W_MEMBER_PROPERTY("w", w),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(float),
    W_CONSTRUCTOR_PROPERTY(float, float, float, float),
    W_SCRIPT_FUNCTION_PROPERTY(Make, In, "X", In, "Y", In, "Z", In, "W")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(GetLength<float>),
    W_SCRIPT_FUNCTION_PROPERTY(GetLengthSquared),
    W_SCRIPT_FUNCTION_PROPERTY(GetNormalized<float>),
    W_SCRIPT_FUNCTION_PROPERTY(Dot, In, "v"),
    W_SCRIPT_FUNCTION_PROPERTY(GetAsVec2),
    W_SCRIPT_FUNCTION_PROPERTY(GetAsVec3),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVec2I32, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("x", x),
    W_MEMBER_PROPERTY("y", y),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WInt32),
    W_CONSTRUCTOR_PROPERTY(WInt32, WInt32),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVec3I32, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("x", x),
    W_MEMBER_PROPERTY("y", y),
    W_MEMBER_PROPERTY("z", z),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WInt32),
    W_CONSTRUCTOR_PROPERTY(WInt32, WInt32, WInt32),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVec4I32, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("x", x),
    W_MEMBER_PROPERTY("y", y),
    W_MEMBER_PROPERTY("z", z),
    W_MEMBER_PROPERTY("w", w),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WInt32),
    W_CONSTRUCTOR_PROPERTY(WInt32, WInt32, WInt32, WInt32),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVec2U32, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("x", x),
    W_MEMBER_PROPERTY("y", y),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WUInt32),
    W_CONSTRUCTOR_PROPERTY(WUInt32, WUInt32),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVec3U32, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("x", x),
    W_MEMBER_PROPERTY("y", y),
    W_MEMBER_PROPERTY("z", z),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WUInt32),
    W_CONSTRUCTOR_PROPERTY(WUInt32, WUInt32, WUInt32),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVec4U32, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("x", x),
    W_MEMBER_PROPERTY("y", y),
    W_MEMBER_PROPERTY("z", z),
    W_MEMBER_PROPERTY("w", w),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WUInt32),
    W_CONSTRUCTOR_PROPERTY(WUInt32, WUInt32, WUInt32, WUInt32),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WQuat, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("x", x),
    W_MEMBER_PROPERTY("y", y),
    W_MEMBER_PROPERTY("z", z),
    W_MEMBER_PROPERTY("w", w),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(float, float, float, float),
    W_SCRIPT_FUNCTION_PROPERTY(MakeFromAxisAndAngle, In, "Axis", In, "Angle")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeShortestRotation, In, "DirFrom", In, "DirTo")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeSlerp, In, "From", In, "To", In, "Lerp")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeFromEulerAngles, In, "Roll", In, "Pitch", In, "Yaw")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(GetInverse),
    W_SCRIPT_FUNCTION_PROPERTY(Rotate, In, "v"),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WMat3, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WMat4, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WTransform, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Position", m_vPosition),
    W_MEMBER_PROPERTY("Rotation", m_qRotation),
    W_MEMBER_PROPERTY("Scale", m_vScale),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WVec3, WQuat),
    W_CONSTRUCTOR_PROPERTY(WVec3, WQuat, WVec3),
    W_SCRIPT_FUNCTION_PROPERTY(Make, In, "Position", In, "Rotation", In, "Scale")->AddFlags(WPropertyFlags::PureFunction)->AddAttributes(
      new WFunctionArgumentAttributes(2, new WDefaultValueAttribute(WVec3(1)))),
    W_SCRIPT_FUNCTION_PROPERTY(MakeLocalTransform, In, "Parent", In, "GlobalChild")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeGlobalTransform, In, "Parent", In, "LocalChild")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(TransformPosition, In, "Position"),
    W_SCRIPT_FUNCTION_PROPERTY(TransformDirection, In, "Direction"),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WBasisAxis, 1)
W_ENUM_CONSTANT(WBasisAxis::PositiveX),
W_ENUM_CONSTANT(WBasisAxis::PositiveY),
W_ENUM_CONSTANT(WBasisAxis::PositiveZ),
W_ENUM_CONSTANT(WBasisAxis::NegativeX),
W_ENUM_CONSTANT(WBasisAxis::NegativeY),
W_ENUM_CONSTANT(WBasisAxis::NegativeZ),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WUuid, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVariant, WNoBase, 3, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVariantArray, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVariantDictionary, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WString, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WStringBuilder, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WUntrackedString, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WStringView, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WDataBuffer, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WHashedString, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WTempHashedString, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WAngle, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(MakeFromDegree, In, "Degree")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(MakeFromRadian, In, "Radian")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(GetNormalizedRange),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WFloatInterval, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Start", m_StartValue),
    W_MEMBER_PROPERTY("End", m_EndValue),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WIntInterval, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Start", m_StartValue),
    W_MEMBER_PROPERTY("End", m_EndValue),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

// **********************************************************************
// ***** Various RTTI infos that can't be put next to their classes *****

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WTypeFlags, 1)
W_BITFLAGS_CONSTANTS(WTypeFlags::StandardType, WTypeFlags::IsEnum, WTypeFlags::Bitflags, WTypeFlags::Class, WTypeFlags::Abstract, WTypeFlags::Phantom, WTypeFlags::Minimal)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WPropertyFlags, 1)
W_BITFLAGS_CONSTANTS(WPropertyFlags::StandardType, WPropertyFlags::IsEnum, WPropertyFlags::Bitflags, WPropertyFlags::Class)
W_BITFLAGS_CONSTANTS(WPropertyFlags::Const, WPropertyFlags::Reference, WPropertyFlags::Pointer)
W_BITFLAGS_CONSTANTS(WPropertyFlags::PointerOwner, WPropertyFlags::ReadOnly, WPropertyFlags::Hidden, WPropertyFlags::Phantom)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_STATIC_REFLECTED_ENUM(WFunctionType, 1)
W_ENUM_CONSTANTS(WFunctionType::Member, WFunctionType::StaticMember, WFunctionType::Constructor)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WVariantType, 1)
W_ENUM_CONSTANTS(WVariantType::Invalid, WVariantType::Bool, WVariantType::Int8, WVariantType::UInt8, WVariantType::Int16, WVariantType::UInt16)
W_ENUM_CONSTANTS(WVariantType::Int32, WVariantType::UInt32, WVariantType::Int64, WVariantType::UInt64, WVariantType::Float, WVariantType::Double)
W_ENUM_CONSTANTS(WVariantType::Color, WVariantType::Vector2, WVariantType::Vector3, WVariantType::Vector4)
W_ENUM_CONSTANTS(WVariantType::Vector2I, WVariantType::Vector3I, WVariantType::Vector4I, WVariantType::Vector2U, WVariantType::Vector3U, WVariantType::Vector4U)
W_ENUM_CONSTANTS(WVariantType::Quaternion, WVariantType::Matrix3, WVariantType::Matrix4, WVariantType::Transform)
W_ENUM_CONSTANTS(WVariantType::String, WVariantType::StringView, WVariantType::DataBuffer, WVariantType::Time, WVariantType::Uuid, WVariantType::Angle, WVariantType::ColorGamma)
W_ENUM_CONSTANTS(WVariantType::HashedString, WVariantType::TempHashedString)
W_ENUM_CONSTANTS(WVariantType::VariantArray, WVariantType::VariantDictionary, WVariantType::TypedPointer, WVariantType::TypedObject)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WPropertyCategory, 1)
W_ENUM_CONSTANTS(WPropertyCategory::Constant, WPropertyCategory::Member, WPropertyCategory::Function, WPropertyCategory::Array, WPropertyCategory::Set, WPropertyCategory::Map)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

W_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_StandardTypes);
