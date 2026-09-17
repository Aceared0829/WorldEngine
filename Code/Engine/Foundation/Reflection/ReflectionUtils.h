#pragma once

#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>

class WVariant;
class WAbstractProperty;

/// Helper functions for handling reflection related operations.
///
/// This utility class provides high-level functions for working with WorldEngine's reflection system.
/// It offers functionality for type introspection, property manipulation, enum/bitflag conversion,
/// object comparison, and dependency analysis.
///
/// Key functionality:
/// - Property value access and modification through WVariant
/// - Enum and bitflag string conversion
/// - Type dependency analysis and sorting
/// - Object comparison and default value handling
/// - Safe property access with automatic type checking
class W_FOUNDATION_DLL WReflectionUtils
{
public:
  static const WRTTI* GetCommonBaseType(const WRTTI* pRtti1, const WRTTI* pRtti2);

  /// Returns whether a type can be stored directly inside a WVariant.
  static bool IsBasicType(const WRTTI* pRtti);

  /// Returns whether the property is a non-ptr basic type or custom type.
  static bool IsValueType(const WAbstractProperty* pProp);

  /// Returns the RTTI type matching the variant's type.
  static const WRTTI* GetTypeFromVariant(const WVariant& value);
  static const WRTTI* GetTypeFromVariant(WVariantType::Enum type);

  /// Sets the Nth component of the vector to the given value.
  ///
  /// vector's type needs to be in between WVariant::Type::Vector2 and WVariant::Type::Vector4U.
  static WUInt32 GetComponentCount(WVariantType::Enum type);
  static void SetComponent(WVariant& ref_vector, WUInt32 uiComponent, double fValue);                             // [tested]
  static double GetComponent(const WVariant& vector, WUInt32 uiComponent);

  static WVariant GetMemberPropertyValue(const WAbstractMemberProperty* pProp, const void* pObject);              // [tested] via ToolsFoundation
  static void SetMemberPropertyValue(const WAbstractMemberProperty* pProp, void* pObject, const WVariant& value); // [tested] via ToolsFoundation

  static WVariant GetArrayPropertyValue(const WAbstractArrayProperty* pProp, const void* pObject, WUInt32 uiIndex);
  static void SetArrayPropertyValue(const WAbstractArrayProperty* pProp, void* pObject, WUInt32 uiIndex, const WVariant& value);

  static void InsertSetPropertyValue(const WAbstractSetProperty* pProp, void* pObject, const WVariant& value);
  static void RemoveSetPropertyValue(const WAbstractSetProperty* pProp, void* pObject, const WVariant& value);

  static WVariant GetMapPropertyValue(const WAbstractMapProperty* pProp, const void* pObject, const char* szKey);
  static void SetMapPropertyValue(const WAbstractMapProperty* pProp, void* pObject, const char* szKey, const WVariant& value);

  static void InsertArrayPropertyValue(const WAbstractArrayProperty* pProp, void* pObject, const WVariant& value, WUInt32 uiIndex);
  static void RemoveArrayPropertyValue(const WAbstractArrayProperty* pProp, void* pObject, WUInt32 uiIndex);

  static const WAbstractMemberProperty* GetMemberProperty(const WRTTI* pRtti, WUInt32 uiPropertyIndex);
  static const WAbstractMemberProperty* GetMemberProperty(const WRTTI* pRtti, const char* szPropertyName); // [tested] via ToolsFoundation

  /// Gathers all RTTI types that are derived from pRtti.
  ///
  /// This includes all classes that have pRtti as a base class, either direct or indirect.
  ///
  /// \sa GatherDependentTypes
  static void GatherTypesDerivedFromClass(const WRTTI* pRtti, WSet<const WRTTI*>& out_types);

  /// Gathers all RTTI types that pRtti depends on and adds them to inout_types.
  ///
  /// Dependencies are either member properties or base classes. The output contains the transitive closure of the dependencies.
  /// Note that inout_typesAsSet is not cleared when this function is called.
  /// out_pTypesAsStack is all the dependencies sorted by their appearance in the dependency chain.
  /// The last entry is the lowest in the chain and has no dependencies on its own.
  static void GatherDependentTypes(const WRTTI* pRtti, WSet<const WRTTI*>& inout_typesAsSet, WDynamicArray<const WRTTI*>* out_pTypesAsStack = nullptr);

  /// Sorts the input types according to their dependencies.
  ///
  /// Types that have no dependences come first in the output followed by types that have their dependencies met by
  /// the previous entries in the output.
  /// If a dependent type is not in the given types set the function will fail.
  static WResult CreateDependencySortedTypeArray(const WSet<const WRTTI*>& types, WDynamicArray<const WRTTI*>& out_sortedTypes);

  struct EnumConversionMode
  {
    enum Enum
    {
      FullyQualifiedName,
      ValueNameOnly,
      Default = FullyQualifiedName
    };

    using StorageType = WUInt8;
  };

  /// Converts an enum or bitfield value into its string representation.
  ///
  /// The type of pEnumerationRtti will be automatically detected. The syntax of out_sOutput equals MSVC debugger output.
  /// For bitflags, multiple values are combined with the '|' operator (e.g., "Flag1 | Flag2").
  /// For enums, single values are returned (e.g., "MyEnum::Value1").
  ///
  /// \param conversionMode Controls whether to include the full type name or just the value name
  /// \return false if the type is not an enum/bitflag or if the value is invalid
  static bool EnumerationToString(const WRTTI* pEnumerationRtti, WInt64 iValue, WStringBuilder& out_sOutput,
    WEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default); // [tested]

  /// Helper template to shorten the call for WEnums
  template <typename T>
  static bool EnumerationToString(WEnum<T> value, WStringBuilder& out_sOutput, WEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default)
  {
    return EnumerationToString(WGetStaticRTTI<T>(), value.GetValue(), out_sOutput, conversionMode);
  }

  /// Helper template to shorten the call for WBitflags
  template <typename T>
  static bool BitflagsToString(WBitflags<T> value, WStringBuilder& out_sOutput, WEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default)
  {
    return EnumerationToString(WGetStaticRTTI<T>(), value.GetValue(), out_sOutput, conversionMode);
  }

  struct EnumKeyValuePair
  {
    WString m_sKey;
    WInt32 m_iValue = 0;
  };

  /// If the given type is an enum, \a entries will be filled with all available keys (strings) and values (integers).
  static void GetEnumKeysAndValues(const WRTTI* pEnumerationRtti, WDynamicArray<EnumKeyValuePair>& ref_entries, WEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default);

  /// Converts an enum or bitfield in its string representation to its value.
  ///
  /// The type of pEnumerationRtti will be automatically detected. The syntax of szValue must equal the MSVC debugger output.
  static bool StringToEnumeration(const WRTTI* pEnumerationRtti, const char* szValue, WInt64& out_iValue); // [tested]

  /// Helper template to shorten the call for WEnums
  template <typename T>
  static bool StringToEnumeration(const char* szValue, WEnum<T>& out_value)
  {
    WInt64 value;
    const auto retval = StringToEnumeration(WGetStaticRTTI<T>(), szValue, value);
    out_value = static_cast<typename T::Enum>(value);
    return retval;
  }

  /// Returns the default value (Enum::Default) for the given enumeration type.
  static WInt64 DefaultEnumerationValue(const WRTTI* pEnumerationRtti); // [tested]

  /// Makes sure the given value is valid under the given enumeration type.
  ///
  /// Invalid bitflag bits are removed and an invalid enum value is replaced by the default value.
  static WInt64 MakeEnumerationValid(const WRTTI* pEnumerationRtti, WInt64 iValue); // [tested]

  /// Templated convenience function that calls IsEqual and automatically deduces the type.
  template <typename T>
  static bool IsEqual(const T* pObject, const T* pObject2)
  {
    return IsEqual(pObject, pObject2, WGetStaticRTTI<T>());
  }

  /// Compares pObject with pObject2 of type pType and returns whether they are equal.
  ///
  /// Performs a deep comparison of all reflected properties. For classes derived from WReflectedClass,
  /// the correct derived type will automatically be determined, so it is not necessary to pass the exact
  /// type into pType. However, the function will return false if pObject and pObject2 actually have
  /// different types.
  ///
  /// Performance warning: This function recursively compares all properties, which can be expensive
  /// for complex object hierarchies or large containers.
  static bool IsEqual(const void* pObject, const void* pObject2, const WRTTI* pType); // [tested]

  /// Compares property pProp of pObject and pObject2 and returns whether it is equal in both.
  static bool IsEqual(const void* pObject, const void* pObject2, const WAbstractProperty* pProp);

  /// Deletes pObject using the allocator found in the owning property's type.
  static void DeleteObject(void* pObject, const WAbstractProperty* pOwnerProperty);

  /// Returns a global default initialization value for the given variant type.
  static WVariant GetDefaultVariantFromType(WVariant::Type::Enum type); // [tested]

  /// Returns the default value for the specific type
  static WVariant GetDefaultVariantFromType(const WRTTI* pRtti);

  /// Returns the default value for the specific type of the given property.
  static WVariant GetDefaultValue(const WAbstractProperty* pProperty, WVariant index = WVariant());


  /// Sets all member properties in \a pObject of type \a pRtti to the value returned by WToolsReflectionUtils::GetDefaultValue()
  static void SetAllMemberPropertiesToDefault(const WRTTI* pRtti, void* pObject);

  /// If pAttrib is valid and its min/max values are compatible, value will be clamped to them.
  /// Returns false if a clamp attribute exists but no clamp code was executed.
  static WResult ClampValue(WVariant& value, const WClampValueAttribute* pAttrib);
};
