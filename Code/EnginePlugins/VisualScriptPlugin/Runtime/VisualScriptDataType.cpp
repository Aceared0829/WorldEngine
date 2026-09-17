#include <Core/CorePCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/World.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WVisualScriptDataType, 1)
  W_ENUM_CONSTANT(WVisualScriptDataType::Invalid),
  W_ENUM_CONSTANT(WVisualScriptDataType::Bool),
  W_ENUM_CONSTANT(WVisualScriptDataType::Byte),
  W_ENUM_CONSTANT(WVisualScriptDataType::Int),
  W_ENUM_CONSTANT(WVisualScriptDataType::Int64),
  W_ENUM_CONSTANT(WVisualScriptDataType::Float),
  W_ENUM_CONSTANT(WVisualScriptDataType::Double),
  W_ENUM_CONSTANT(WVisualScriptDataType::Color),
  W_ENUM_CONSTANT(WVisualScriptDataType::Vector2),
  W_ENUM_CONSTANT(WVisualScriptDataType::Vector3),
  W_ENUM_CONSTANT(WVisualScriptDataType::Vector4),
  W_ENUM_CONSTANT(WVisualScriptDataType::Quaternion),
  W_ENUM_CONSTANT(WVisualScriptDataType::Transform),
  W_ENUM_CONSTANT(WVisualScriptDataType::Time),
  W_ENUM_CONSTANT(WVisualScriptDataType::Angle),
  W_ENUM_CONSTANT(WVisualScriptDataType::String),
  W_ENUM_CONSTANT(WVisualScriptDataType::HashedString),
  W_ENUM_CONSTANT(WVisualScriptDataType::GameObject),
  W_ENUM_CONSTANT(WVisualScriptDataType::Component),
  W_ENUM_CONSTANT(WVisualScriptDataType::TypedPointer),
  W_ENUM_CONSTANT(WVisualScriptDataType::Variant),
  W_ENUM_CONSTANT(WVisualScriptDataType::Array),
  W_ENUM_CONSTANT(WVisualScriptDataType::Map),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

namespace
{
  static constexpr WVariantType::Enum s_ScriptDataTypeVariantTypes[] = {
    WVariantType::Invalid,           // Invalid,

    WVariantType::Bool,              // Bool,
    WVariantType::UInt8,             // Byte,
    WVariantType::Int32,             // Int,
    WVariantType::Int64,             // Int64,
    WVariantType::Float,             // Float,
    WVariantType::Double,            // Double,
    WVariantType::Color,             // Color,
    WVariantType::Vector2,           // Vector2,
    WVariantType::Vector3,           // Vector3,
    WVariantType::Vector4,           // Vector4,
    WVariantType::Quaternion,        // Quaternion,
    WVariantType::Transform,         // Transform,
    WVariantType::Time,              // Time,
    WVariantType::Angle,             // Angle,
    WVariantType::String,            // String,
    WVariantType::HashedString,      // HashedString,
    WVariantType::TypedObject,       // GameObject,
    WVariantType::TypedObject,       // Component,
    WVariantType::TypedPointer,      // TypedPointer,
    WVariantType::Invalid,           // Variant,
    WVariantType::VariantArray,      // Array,
    WVariantType::VariantDictionary, // Map,
    WVariantType::TypedObject,       // Coroutine,
  };
  static_assert(W_ARRAY_SIZE(s_ScriptDataTypeVariantTypes) == (size_t)WVisualScriptDataType::Count);

  static constexpr WUInt32 s_ScriptDataTypeSizes[] = {
    0,                                      // Invalid,

    sizeof(bool),                           // Bool,
    sizeof(WUInt8),                        // Byte,
    sizeof(WInt32),                        // Int,
    sizeof(WInt64),                        // Int64,
    sizeof(float),                          // Float,
    sizeof(double),                         // Double,
    sizeof(WColor),                        // Color,
    sizeof(WVec2),                         // Vector2,
    sizeof(WVec3),                         // Vector3,
    sizeof(WVec4),                         // Vector4,
    sizeof(WQuat),                         // Quaternion,
    sizeof(WTransform),                    // Transform,
    sizeof(WTime),                         // Time,
    sizeof(WAngle),                        // Angle,
    sizeof(WString),                       // String,
    sizeof(WHashedString),                 // HashedString,
    sizeof(WVisualScriptGameObjectHandle), // GameObject,
    sizeof(WVisualScriptComponentHandle),  // Component,
    sizeof(WTypedPointer),                 // TypedPointer,
    sizeof(WVariant),                      // Variant,
    sizeof(WVariantArray),                 // Array,
    sizeof(WVariantDictionary),            // Map,
    sizeof(WScriptCoroutineHandle),        // Coroutine,
  };
  static_assert(W_ARRAY_SIZE(s_ScriptDataTypeSizes) == (size_t)WVisualScriptDataType::Count);

  static constexpr WUInt32 s_ScriptDataTypeAlignments[] = {
    0,                                       // Invalid,

    alignof(bool),                           // Bool,
    alignof(WUInt8),                        // Byte,
    alignof(WInt32),                        // Int,
    alignof(WInt64),                        // Int64,
    alignof(float),                          // Float,
    alignof(double),                         // Double,
    alignof(WColor),                        // Color,
    alignof(WVec2),                         // Vector2,
    alignof(WVec3),                         // Vector3,
    alignof(WVec4),                         // Vector4,
    alignof(WQuat),                         // Quaternion,
    alignof(WTransform),                    // Transform,
    alignof(WTime),                         // Time,
    alignof(WAngle),                        // Angle,
    alignof(WString),                       // String,
    alignof(WHashedString),                 // HashedString,
    alignof(WVisualScriptGameObjectHandle), // GameObject,
    alignof(WVisualScriptComponentHandle),  // Component,
    alignof(WTypedPointer),                 // TypedPointer,
    alignof(WVariant),                      // Variant,
    alignof(WVariantArray),                 // Array,
    alignof(WVariantDictionary),            // Map,
    alignof(WScriptCoroutineHandle),        // Coroutine,
  };
  static_assert(W_ARRAY_SIZE(s_ScriptDataTypeAlignments) == (size_t)WVisualScriptDataType::Count);

  static constexpr const char* s_ScriptDataTypeNames[] = {
    "Invalid",

    "Bool",
    "Byte",
    "Int",
    "Int64",
    "Float",
    "Double",
    "Color",
    "Vector2",
    "Vector3",
    "Vector4",
    "Quaternion",
    "Transform",
    "Time",
    "Angle",
    "String",
    "HashedString",
    "GameObject",
    "Component",
    "TypedPointer",
    "Variant",
    "Array",
    "Map",
    "Coroutine",
    "", // Count,
    "Enum",
    "Bitflag",
    "Resource",
  };
  static_assert(W_ARRAY_SIZE(s_ScriptDataTypeNames) == (size_t)WVisualScriptDataType::ExtendedCount);
} // namespace

// static
WVariantType::Enum WVisualScriptDataType::GetVariantType(Enum dataType)
{
  W_ASSERT_DEBUG(dataType >= 0 && dataType < W_ARRAY_SIZE(s_ScriptDataTypeVariantTypes), "Out of bounds access");
  return s_ScriptDataTypeVariantTypes[dataType];
}

// static
WVisualScriptDataType::Enum WVisualScriptDataType::FromVariantType(WVariantType::Enum variantType)
{
  switch (variantType)
  {
    case WVariantType::Bool:
      return Bool;
    case WVariantType::Int8:
    case WVariantType::UInt8:
      return Byte;
    case WVariantType::Int16:
    case WVariantType::UInt16:
    case WVariantType::Int32:
    case WVariantType::UInt32:
      return Int;
    case WVariantType::Int64:
    case WVariantType::UInt64:
      return Int64;
    case WVariantType::Float:
      return Float;
    case WVariantType::Double:
      return Double;
    case WVariantType::Color:
      return Color;
    case WVariantType::Vector2:
    case WVariantType::Vector2I:
    case WVariantType::Vector2U:
      return Vector2;
    case WVariantType::Vector3:
    case WVariantType::Vector3I:
    case WVariantType::Vector3U:
      return Vector3;
    case WVariantType::Vector4:
    case WVariantType::Vector4I:
    case WVariantType::Vector4U:
      return Vector4;
    case WVariantType::Quaternion:
      return Quaternion;
    case WVariantType::Transform:
      return Transform;
    case WVariantType::Time:
      return Time;
    case WVariantType::Angle:
      return Angle;
    case WVariantType::String:
    case WVariantType::StringView:
      return String;
    case WVariantType::HashedString:
    case WVariantType::TempHashedString:
      return HashedString;
    case WVariantType::VariantArray:
      return Array;
    case WVariantType::VariantDictionary:
      return Map;
    default:
      return Invalid;
  }
}

WProcessingStream::DataType WVisualScriptDataType::GetStreamDataType(Enum dataType)
{
  // We treat WColor and WVec4 as the same in the visual script <=> expression binding
  // so ensure that they have the same size and layout
  static_assert(sizeof(WColor) == sizeof(WVec4));
  static_assert(offsetof(WColor, r) == offsetof(WVec4, x));
  static_assert(offsetof(WColor, g) == offsetof(WVec4, y));
  static_assert(offsetof(WColor, b) == offsetof(WVec4, z));
  static_assert(offsetof(WColor, a) == offsetof(WVec4, w));

  switch (dataType)
  {
    case Int:
      return WProcessingStream::DataType::Int;
    case Float:
      return WProcessingStream::DataType::Float;
    case Vector2:
      return WProcessingStream::DataType::Float2;
    case Vector3:
      return WProcessingStream::DataType::Float3;
    case Vector4:
    case Color:
      return WProcessingStream::DataType::Float4;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  return WProcessingStream::DataType::Float;
}

// static
const WRTTI* WVisualScriptDataType::GetRtti(Enum dataType)
{
  // Define table here to prevent issues with static initialization order
  static const WRTTI* s_Rttis[] = {
    nullptr,                                    // Invalid,

    WGetStaticRTTI<bool>(),                    // Bool,
    WGetStaticRTTI<WUInt8>(),                 // Byte,
    WGetStaticRTTI<WInt32>(),                 // Int,
    WGetStaticRTTI<WInt64>(),                 // Int64,
    WGetStaticRTTI<float>(),                   // Float,
    WGetStaticRTTI<double>(),                  // Double,
    WGetStaticRTTI<WColor>(),                 // Color,
    WGetStaticRTTI<WVec2>(),                  // Vector2,
    WGetStaticRTTI<WVec3>(),                  // Vector3,
    WGetStaticRTTI<WVec4>(),                  // Vector4,
    WGetStaticRTTI<WQuat>(),                  // Quaternion,
    WGetStaticRTTI<WTransform>(),             // Transform,
    WGetStaticRTTI<WTime>(),                  // Time,
    WGetStaticRTTI<WAngle>(),                 // Angle,
    WGetStaticRTTI<WString>(),                // String,
    WGetStaticRTTI<WHashedString>(),          // HashedString,
    WGetStaticRTTI<WGameObjectHandle>(),      // GameObject,
    WGetStaticRTTI<WComponentHandle>(),       // Component,
    nullptr,                                    // TypedPointer,
    WGetStaticRTTI<WVariant>(),               // Variant,
    WGetStaticRTTI<WVariantArray>(),          // Array,
    WGetStaticRTTI<WVariantDictionary>(),     // Map,
    WGetStaticRTTI<WScriptCoroutineHandle>(), // Coroutine,
    nullptr,                                    // Count,
    nullptr,                                    // EnumValue,
    nullptr,                                    // BitflagValue,
    nullptr,                                    // Resource,
  };
  static_assert(W_ARRAY_SIZE(s_Rttis) == (size_t)WVisualScriptDataType::ExtendedCount);

  W_ASSERT_DEBUG(dataType >= 0 && dataType < W_ARRAY_SIZE(s_Rttis), "Out of bounds access");
  return s_Rttis[dataType];
}

// static
WVisualScriptDataType::Enum WVisualScriptDataType::FromRtti(const WRTTI* pRtti)
{
  Enum res = FromVariantType(pRtti->GetVariantType());
  if (res != Invalid)
    return res;

  if (pRtti->IsDerivedFrom<WGameObject>() || pRtti == WGetStaticRTTI<WGameObjectHandle>())
    return GameObject;

  if (pRtti->IsDerivedFrom<WComponent>() || pRtti == WGetStaticRTTI<WComponentHandle>())
    return Component;

  if (pRtti == WGetStaticRTTI<WScriptCoroutineHandle>())
    return Coroutine;

  if (pRtti->GetTypeFlags().IsSet(WTypeFlags::Class))
    return TypedPointer;

  if (pRtti->GetTypeFlags().IsSet(WTypeFlags::IsEnum))
    return EnumValue;

  if (pRtti->GetTypeFlags().IsSet(WTypeFlags::Bitflags))
    return BitflagValue;

  if (pRtti == WGetStaticRTTI<WVariant>())
    return Variant;

  return Invalid;
}

// static
WUInt32 WVisualScriptDataType::GetStorageSize(Enum dataType)
{
  W_ASSERT_DEBUG(dataType >= 0 && dataType < W_ARRAY_SIZE(s_ScriptDataTypeSizes), "Out of bounds access");
  return s_ScriptDataTypeSizes[dataType];
}

// static
WUInt32 WVisualScriptDataType::GetStorageAlignment(Enum dataType)
{
  W_ASSERT_DEBUG(dataType >= 0 && dataType < W_ARRAY_SIZE(s_ScriptDataTypeAlignments), "Out of bounds access");
  return s_ScriptDataTypeAlignments[dataType];
}

// static
const char* WVisualScriptDataType::GetName(Enum dataType)
{
  if (dataType == AnyPointer)
  {
    return "Pointer";
  }
  else if (dataType == Any)
  {
    return "Any";
  }

  W_ASSERT_DEBUG(dataType >= 0 && dataType < W_ARRAY_SIZE(s_ScriptDataTypeNames), "Out of bounds access");
  return s_ScriptDataTypeNames[dataType];
}

// static
bool WVisualScriptDataType::CanConvertTo(Enum sourceDataType, Enum targetDataType)
{
  if (sourceDataType == targetDataType ||
      sourceDataType == Any ||
      targetDataType == Any ||
      targetDataType == String ||
      targetDataType == HashedString ||
      targetDataType == Variant)
    return true;

  if ((IsNumberOrBool(sourceDataType) || (sourceDataType == EnumValue || sourceDataType == BitflagValue)) &&
      (IsNumberOrBool(targetDataType) || (targetDataType == EnumValue || targetDataType == BitflagValue)))
    return true;

  if ((IsNumberOrBool(sourceDataType) && (IsVector(targetDataType))) ||
      (sourceDataType == Vector3 && targetDataType == Transform))
    return true;

  if (IsPointer(sourceDataType) &&
      (targetDataType == WVisualScriptDataType::AnyPointer || targetDataType == WVisualScriptDataType::Bool))
    return true;

  return false;
}

//////////////////////////////////////////////////////////////////////////

WGameObject* WVisualScriptGameObjectHandle::GetPtr(WUInt32 uiExecutionCounter) const
{
  if (m_uiExecutionCounter == uiExecutionCounter || m_Handle.IsInvalidated() || m_Handle.GetInternalID().m_Data == 0)
  {
    return m_Ptr;
  }

  m_Ptr = nullptr;
  m_uiExecutionCounter = uiExecutionCounter;

  if (WWorld* pWorld = WWorld::GetWorld(m_Handle))
  {
    bool objectExists = pWorld->TryGetObject(m_Handle, m_Ptr);
    W_IGNORE_UNUSED(objectExists);
  }

  return m_Ptr;
}

WComponent* WVisualScriptComponentHandle::GetPtr(WUInt32 uiExecutionCounter) const
{
  if (m_uiExecutionCounter == uiExecutionCounter || m_Handle.IsInvalidated() || m_Handle.GetInternalID().m_Data == 0)
  {
    return m_Ptr;
  }

  m_Ptr = nullptr;
  m_uiExecutionCounter = uiExecutionCounter;

  if (WWorld* pWorld = WWorld::GetWorld(m_Handle))
  {
    bool componentExists = pWorld->TryGetComponent(m_Handle, m_Ptr);
    W_IGNORE_UNUSED(componentExists);
  }

  return m_Ptr;
}


W_STATICLINK_FILE(VisualScriptPlugin, VisualScriptPlugin_Runtime_VisualScriptDataType);
