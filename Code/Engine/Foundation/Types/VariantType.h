#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Types.h>

class WReflectedClass;
class WVariant;
struct WTime;
class WUuid;
class WStringView;
struct WTypedObject;
struct WTypedPointer;

using WDataBuffer = WDynamicArray<WUInt8>;
using WVariantArray = WDynamicArray<WVariant>;
using WVariantDictionary = WHashTable<WString, WVariant>;

/// This enum describes the type of data that is currently stored inside the variant.
struct WVariantType
{
  using StorageType = WUInt8;
  /// This enum describes the type of data that is currently stored inside the variant.
  /// Note that changes to this enum require an increase of the reflection version and either
  /// patches to the serializer or a re-export of binary data that contains WVariants.
  enum Enum : WUInt8
  {
    Invalid = 0, ///< The variant stores no (valid) data at the moment.

    /// *** Types that are flagged as 'StandardTypes' (see DetermineTypeFlags) ***
    FirstStandardType = 1,
    Bool,             ///< The variant stores a bool.
    Int8,             ///< The variant stores an WInt8.
    UInt8,            ///< The variant stores an WUInt8.
    Int16,            ///< The variant stores an WInt16.
    UInt16,           ///< The variant stores an WUInt16.
    Int32,            ///< The variant stores an WInt32.
    UInt32,           ///< The variant stores an WUInt32.
    Int64,            ///< The variant stores an WInt64.
    UInt64,           ///< The variant stores an WUInt64.
    Float,            ///< The variant stores a float.
    Double,           ///< The variant stores a double.
    Color,            ///< The variant stores an WColor.
    Vector2,          ///< The variant stores an WVec2.
    Vector3,          ///< The variant stores an WVec3.
    Vector4,          ///< The variant stores an WVec4.
    Vector2I,         ///< The variant stores an WVec2I32.
    Vector3I,         ///< The variant stores an WVec3I32.
    Vector4I,         ///< The variant stores an WVec4I32.
    Vector2U,         ///< The variant stores an WVec2U32.
    Vector3U,         ///< The variant stores an WVec3U32.
    Vector4U,         ///< The variant stores an WVec4U32.
    Quaternion,       ///< The variant stores an WQuat.
    Matrix3,          ///< The variant stores an WMat3. A heap allocation is required to store this data type.
    Matrix4,          ///< The variant stores an WMat4. A heap allocation is required to store this data type.
    Transform,        ///< The variant stores an WTransform. A heap allocation is required to store this data type.
    String,           ///< The variant stores a string. A heap allocation is required to store this data type.
    StringView,       ///< The variant stores an WStringView.
    DataBuffer,       ///< The variant stores an WDataBuffer, a typedef to DynamicArray<WUInt8>. A heap allocation is required to store this data type.
    Time,             ///< The variant stores an WTime value.
    Uuid,             ///< The variant stores an WUuid value.
    Angle,            ///< The variant stores an WAngle value.
    ColorGamma,       ///< The variant stores an WColorGammaUB value.
    HashedString,     ///< The variant stores an WHashedString value.
    TempHashedString, ///< The variant stores an WTempHashedString value.
    LastStandardType,
    /// *** Types that are flagged as 'StandardTypes' (see DetermineTypeFlags) ***

    FirstExtendedType = 64,
    VariantArray,      ///< The variant stores an array of WVariant's. A heap allocation is required to store this data type.
    VariantDictionary, ///< The variant stores a dictionary (hashmap) of WVariant's. A heap allocation is required to store this type.
    TypedPointer,      ///< The variant stores an WTypedPointer value. Reflected type and data queries will match the pointed to object.
    TypedObject,       ///< The variant stores an WTypedObject value. Reflected type and data queries will match the object. A heap allocation is required to store this type if it is larger than 16 bytes or not POD.
    LastExtendedType,  ///< Number of values for WVariant::Type.

    MAX_ENUM_VALUE = LastExtendedType,
    Default = Invalid  ///< Default value used by WEnum.
  };
};

W_DEFINE_AS_POD_TYPE(WVariantType::Enum);

struct WVariantClass
{
  enum Enum
  {
    Invalid,
    DirectCast,     ///< A standard type
    PointerCast,    ///< Any cast to T*
    TypedObject,    ///< WTypedObject cast. Needed because at no point does and WVariant ever store a WTypedObject so it can't be returned as a const reference.
    CustomTypeCast, ///< Custom object types
  };
};

/// A helper struct to convert the C++ type, which is passed as the template argument, into one of the WVariant::Type enum values.
template <typename T>
struct WVariantTypeDeduction
{
  static constexpr WVariantType::Enum value = WVariantType::Invalid;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr WVariantClass::Enum classification = WVariantClass::Invalid;

  using StorageType = T;
};

/// Declares a custom variant type, allowing it to be stored by value inside an WVariant.
///
/// Needs to be called from the same header that defines the type.
/// \sa W_DEFINE_CUSTOM_VARIANT_TYPE
#define W_DECLARE_CUSTOM_VARIANT_TYPE(TYPE)                                               \
  template <>                                                                              \
  struct WVariantTypeDeduction<TYPE>                                                      \
  {                                                                                        \
    static constexpr WVariantType::Enum value = WVariantType::TypedObject;               \
    static constexpr bool forceSharing = false;                                            \
    static constexpr bool hasReflectedMembers = true;                                      \
    static constexpr WVariantClass::Enum classification = WVariantClass::CustomTypeCast; \
                                                                                           \
    using StorageType = TYPE;                                                              \
  };

#include <Foundation/Types/Implementation/VariantTypeDeduction_inl.h>
