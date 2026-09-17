


/// \cond

template <>
struct WVariantTypeDeduction<bool>
{
  static constexpr WVariantType::Enum value = WVariantType::Bool;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = bool;
  using ReturnType = bool;
};

template <>
struct WVariantTypeDeduction<WInt8>
{
  static constexpr WVariantType::Enum value = WVariantType::Int8;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WInt8;
};

template <>
struct WVariantTypeDeduction<WUInt8>
{
  static constexpr WVariantType::Enum value = WVariantType::UInt8;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WUInt8;
};

template <>
struct WVariantTypeDeduction<WInt16>
{
  static constexpr WVariantType::Enum value = WVariantType::Int16;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WInt16;
};

template <>
struct WVariantTypeDeduction<WUInt16>
{
  static constexpr WVariantType::Enum value = WVariantType::UInt16;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WUInt16;
};

template <>
struct WVariantTypeDeduction<WInt32>
{
  static constexpr WVariantType::Enum value = WVariantType::Int32;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WInt32;
};

template <>
struct WVariantTypeDeduction<WUInt32>
{
  static constexpr WVariantType::Enum value = WVariantType::UInt32;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WUInt32;
};

template <>
struct WVariantTypeDeduction<WInt64>
{
  static constexpr WVariantType::Enum value = WVariantType::Int64;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WInt64;
};

template <>
struct WVariantTypeDeduction<WUInt64>
{
  static constexpr WVariantType::Enum value = WVariantType::UInt64;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WUInt64;
};

template <>
struct WVariantTypeDeduction<float>
{
  static constexpr WVariantType::Enum value = WVariantType::Float;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = float;
};

template <>
struct WVariantTypeDeduction<double>
{
  static constexpr WVariantType::Enum value = WVariantType::Double;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = double;
};

template <>
struct WVariantTypeDeduction<WColor>
{
  static constexpr WVariantType::Enum value = WVariantType::Color;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WColor;
};

template <>
struct WVariantTypeDeduction<WColorGammaUB>
{
  static constexpr WVariantType::Enum value = WVariantType::ColorGamma;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WColorGammaUB;
};

template <>
struct WVariantTypeDeduction<WVec2>
{
  static constexpr WVariantType::Enum value = WVariantType::Vector2;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVec2;
};

template <>
struct WVariantTypeDeduction<WVec3>
{
  static constexpr WVariantType::Enum value = WVariantType::Vector3;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVec3;
};

template <>
struct WVariantTypeDeduction<WVec4>
{
  static constexpr WVariantType::Enum value = WVariantType::Vector4;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVec4;
};

template <>
struct WVariantTypeDeduction<WVec2I32>
{
  static constexpr WVariantType::Enum value = WVariantType::Vector2I;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVec2I32;
};

template <>
struct WVariantTypeDeduction<WVec3I32>
{
  static constexpr WVariantType::Enum value = WVariantType::Vector3I;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVec3I32;
};

template <>
struct WVariantTypeDeduction<WVec4I32>
{
  static constexpr WVariantType::Enum value = WVariantType::Vector4I;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVec4I32;
};

template <>
struct WVariantTypeDeduction<WVec2U32>
{
  static constexpr WVariantType::Enum value = WVariantType::Vector2U;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVec2U32;
};

template <>
struct WVariantTypeDeduction<WVec3U32>
{
  static constexpr WVariantType::Enum value = WVariantType::Vector3U;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVec3U32;
};

template <>
struct WVariantTypeDeduction<WVec4U32>
{
  static constexpr WVariantType::Enum value = WVariantType::Vector4U;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVec4U32;
};

template <>
struct WVariantTypeDeduction<WQuat>
{
  static constexpr WVariantType::Enum value = WVariantType::Quaternion;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WQuat;
};

template <>
struct WVariantTypeDeduction<WMat3>
{
  static constexpr WVariantType::Enum value = WVariantType::Matrix3;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WMat3;
};

template <>
struct WVariantTypeDeduction<WMat4>
{
  static constexpr WVariantType::Enum value = WVariantType::Matrix4;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WMat4;
};

template <>
struct WVariantTypeDeduction<WTransform>
{
  static constexpr WVariantType::Enum value = WVariantType::Transform;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WTransform;
};

template <>
struct WVariantTypeDeduction<WString>
{
  static constexpr WVariantType::Enum value = WVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WString;
};

template <>
struct WVariantTypeDeduction<WUntrackedString>
{
  static constexpr WVariantType::Enum value = WVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WString;
};

template <>
struct WVariantTypeDeduction<WStringView>
{
  static constexpr WVariantType::Enum value = WVariantType::StringView;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WStringView;
};

template <>
struct WVariantTypeDeduction<WDataBuffer>
{
  static constexpr WVariantType::Enum value = WVariantType::DataBuffer;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WDataBuffer;
};

template <>
struct WVariantTypeDeduction<char*>
{
  static constexpr WVariantType::Enum value = WVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WString;
};

template <>
struct WVariantTypeDeduction<const char*>
{
  static constexpr WVariantType::Enum value = WVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WString;
};

template <size_t N>
struct WVariantTypeDeduction<char[N]>
{
  static constexpr WVariantType::Enum value = WVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WString;
};

template <size_t N>
struct WVariantTypeDeduction<const char[N]>
{
  static constexpr WVariantType::Enum value = WVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WString;
};

template <>
struct WVariantTypeDeduction<WTime>
{
  static constexpr WVariantType::Enum value = WVariantType::Time;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WTime;
};

template <>
struct WVariantTypeDeduction<WUuid>
{
  static constexpr WVariantType::Enum value = WVariantType::Uuid;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WUuid;
};

template <>
struct WVariantTypeDeduction<WAngle>
{
  static constexpr WVariantType::Enum value = WVariantType::Angle;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WAngle;
};

template <>
struct WVariantTypeDeduction<WHashedString>
{
  static constexpr WVariantType::Enum value = WVariantType::HashedString;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WHashedString;
};

template <>
struct WVariantTypeDeduction<WTempHashedString>
{
  static constexpr WVariantType::Enum value = WVariantType::TempHashedString;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WTempHashedString;
};

template <>
struct WVariantTypeDeduction<WVariantArray>
{
  static constexpr WVariantType::Enum value = WVariantType::VariantArray;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVariantArray;
};

template <>
struct WVariantTypeDeduction<WArrayPtr<WVariant>>
{
  static constexpr WVariantType::Enum value = WVariantType::VariantArray;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVariantArray;
};


template <>
struct WVariantTypeDeduction<WVariantDictionary>
{
  static constexpr WVariantType::Enum value = WVariantType::VariantDictionary;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WVariantDictionary;
};

namespace WInternal
{
  template <int v>
  struct PointerDeductionHelper
  {
  };

  template <>
  struct PointerDeductionHelper<0>
  {
    using StorageType = void*;
  };

  template <>
  struct PointerDeductionHelper<1>
  {
    using StorageType = WReflectedClass*;
  };
} // namespace WInternal

template <>
struct WVariantTypeDeduction<WTypedPointer>
{
  static constexpr WVariantType::Enum value = WVariantType::TypedPointer;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::DirectCast;

  using StorageType = WTypedPointer;
};

template <typename T>
struct WVariantTypeDeduction<T*>
{
  static constexpr WVariantType::Enum value = WVariantType::TypedPointer;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::PointerCast;

  using StorageType = WTypedPointer;
};

template <>
struct WVariantTypeDeduction<WTypedObject>
{
  static constexpr WVariantType::Enum value = WVariantType::TypedObject;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr WVariantClass::Enum classification = WVariantClass::TypedObject;

  using StorageType = WTypedObject;
};

/// \endcond
