#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/VariantType.h>
#include <type_traits>

class WRTTI;
class WReflectedClass;
class WVariant;

/// Flags that describe a reflected type.
struct WTypeFlags
{
  using StorageType = WUInt8;

  enum Enum
  {
    StandardType = W_BIT(0), ///< Anything that can be stored inside an WVariant except for pointers and containers.
    IsEnum = W_BIT(1),       ///< enum struct used for WEnum.
    Bitflags = W_BIT(2),     ///< bitflags struct used for WBitflags.
    Class = W_BIT(3),        ///< A class or struct. The above flags are mutually exclusive.

    Abstract = W_BIT(4),     ///< Type is abstract.
    Phantom = W_BIT(5),      ///< De-serialized type information that cannot be created on this process.
    Minimal = W_BIT(6),      ///< Does not contain any property, function or attribute information. Used only for versioning.
    Default = 0
  };

  struct Bits
  {
    StorageType StandardType : 1;
    StorageType IsEnum : 1;
    StorageType Bitflags : 1;
    StorageType Class : 1;
    StorageType Abstract : 1;
    StorageType Phantom : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WTypeFlags)


// ****************************************************
// ***** Templates for accessing static RTTI data *****

namespace WInternal
{
  /// [internal] Helper struct for accessing static RTTI data.
  template <typename T>
  struct WStaticRTTI
  {
  };

  // Special implementation for types that have no base
  template <>
  struct WStaticRTTI<WNoBase>
  {
    static const WRTTI* GetRTTI() { return nullptr; }
  };

  // Special implementation for void to make function reflection compile void return values without further specialization.
  template <>
  struct WStaticRTTI<void>
  {
    static const WRTTI* GetRTTI() { return nullptr; }
  };

  template <typename T>
  W_ALWAYS_INLINE const WRTTI* GetStaticRTTI(WTraitInt<1>) // class derived from WReflectedClass
  {
    return T::GetStaticRTTI();
  }

  template <typename T>
  W_ALWAYS_INLINE const WRTTI* GetStaticRTTI(WTraitInt<0>) // static rtti
  {
    // Since this is pure C++ and no preprocessor macro, calling it with types such as 'int' and 'WInt32' will
    // actually return the same RTTI object, which would not be possible with a purely macro based solution

    return WStaticRTTI<T>::GetRTTI();
  }

  template <typename Type>
  WBitflags<WTypeFlags> DetermineTypeFlags()
  {
    WBitflags<WTypeFlags> flags;
    WVariantType::Enum type =
      static_cast<WVariantType::Enum>(WVariantTypeDeduction<typename WTypeTraits<Type>::NonConstReferenceType>::value);
    if ((type >= WVariantType::FirstStandardType && type <= WVariantType::LastStandardType) || W_IS_SAME_TYPE(WVariant, Type))
      flags.Add(WTypeFlags::StandardType);
    else
      flags.Add(WTypeFlags::Class);

    if (std::is_abstract<Type>::value)
      flags.Add(WTypeFlags::Abstract);

    return flags;
  }

  template <>
  W_ALWAYS_INLINE WBitflags<WTypeFlags> DetermineTypeFlags<WVariant>()
  {
    return WTypeFlags::StandardType;
  }

  template <typename T>
  struct WStaticRTTIWrapper
  {
    static_assert(sizeof(T) == 0, "Type has not been declared as reflectable (use W_DECLARE_REFLECTABLE_TYPE macro)");
  };
} // namespace WInternal

/// Retrieves the static RTTI information for any reflected type.
///
/// This is the primary entry point for accessing reflection data. It works with both
/// statically reflected types (using macros) and dynamically reflected types (WReflectedClass).
/// The function automatically detects the reflection method and returns the appropriate RTTI.
///
/// Usage examples:
/// \code
///   const WRTTI* rtti = WGetStaticRTTI<MyClass>();
///   if (rtti->IsDerivedFrom<BaseClass>()) { ... }
/// \endcode
///
/// Performance: This is a compile-time dispatched function with minimal runtime overhead.
template <typename T>
W_ALWAYS_INLINE const WRTTI* WGetStaticRTTI()
{
  return WInternal::GetStaticRTTI<T>(WTraitInt<W_IS_DERIVED_FROM_STATIC(WReflectedClass, T)>());
}

// **************************************************
// ***** Macros for declaring types reflectable *****

#define W_NO_LINKAGE

/// Declares a type to be statically reflectable.
///
/// Insert this macro into the header file of a type to enable static reflection on it.
/// This creates the necessary template specializations for RTTI access.
///
/// Parameters:
/// - Linkage: Usually W_FOUNDATION_DLL, W_CORE_DLL, or W_NO_LINKAGE for static types
/// - TYPE: The fully qualified type name to make reflectable
///
/// Usage:
/// \code
///   // In header file
///   struct MyStruct { int value; };
///   W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, MyStruct);
/// \endcode
///
/// Note: This is not needed if the type already uses dynamic reflection (WReflectedClass).
/// Only use for POD types, enums, or simple classes that don't inherit from WReflectedClass.
#define W_DECLARE_REFLECTABLE_TYPE(Linkage, TYPE)                    \
  namespace WInternal                                                \
  {                                                                   \
    template <>                                                       \
    struct Linkage WStaticRTTIWrapper<TYPE>                          \
    {                                                                 \
      static WRTTI s_RTTI;                                           \
    };                                                                \
                                                                      \
    /* This specialization calls the function to get the RTTI data */ \
    /* This code might get duplicated in different DLLs, but all   */ \
    /* will call the same function, so the RTTI object is unique   */ \
    template <>                                                       \
    struct WStaticRTTI<TYPE>                                         \
    {                                                                 \
      W_ALWAYS_INLINE static const WRTTI* GetRTTI()                 \
      {                                                               \
        return &WStaticRTTIWrapper<TYPE>::s_RTTI;                    \
      }                                                               \
    };                                                                \
  }

/// Insert this into a class/struct to enable properties that are private members.
/// All types that have dynamic reflection (\see W_ADD_DYNAMIC_REFLECTION) already have this ability.
#define W_ALLOW_PRIVATE_PROPERTIES(SELF) friend WRTTI GetRTTI(SELF*)

/// \cond
// internal helper macro
#define W_RTTIINFO_DECL(Type, BaseType, Version) \
                                                  \
  WStringView GetTypeName(Type*)                 \
  {                                               \
    return #Type;                                 \
  }                                               \
  WUInt32 GetTypeVersion(Type*)                  \
  {                                               \
    return Version;                               \
  }                                               \
                                                  \
  WRTTI GetRTTI(Type*);

// internal helper macro
#define W_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, BaseType, AllocatorType)              \
  WRTTI GetRTTI(Type*)                                                            \
  {                                                                                \
    using OwnType = Type;                                                          \
    using OwnBaseType = BaseType;                                                  \
    static AllocatorType Allocator;                                                \
    static WBitflags<WTypeFlags> flags = WInternal::DetermineTypeFlags<Type>(); \
    static WArrayPtr<const WAbstractProperty*> Properties;                       \
    static WArrayPtr<const WAbstractFunctionProperty*> Functions;                \
    static WArrayPtr<const WPropertyAttribute*> Attributes;                      \
    static WArrayPtr<WAbstractMessageHandler*> MessageHandlers;                  \
    static WArrayPtr<WMessageSenderInfo> MessageSenders;

/// \endcond

/// Begins the implementation block for static reflection of a type.
///
/// This macro starts the definition of RTTI data for a type in a source file.
/// Must be paired with W_END_STATIC_REFLECTED_TYPE. Between these macros,
/// use W_BEGIN_PROPERTIES/W_END_PROPERTIES and similar blocks to define reflection data.
///
/// Parameters:
/// - Type: The type being reflected (must match W_DECLARE_REFLECTABLE_TYPE)
/// - BaseType: The base class (use WNoBase if no inheritance)
/// - Version: Version number for serialization compatibility (increment when structure changes)
/// - AllocatorType: Controls dynamic allocation:
///   - WRTTIDefaultAllocator<Type>: Standard heap allocation
///   - WRTTINoAllocator: Disable dynamic creation
///   - Custom allocator: For specialized memory management
///
/// Example:
/// \code
///   // In source file
///   W_BEGIN_STATIC_REFLECTED_TYPE(MyStruct, WNoBase, 1, WRTTIDefaultAllocator<MyStruct>)
///   {
///     W_BEGIN_PROPERTIES
///     {
///       W_MEMBER_PROPERTY("value", value)
///     }
///     W_END_PROPERTIES;
///   }
///   W_END_STATIC_REFLECTED_TYPE;
/// \endcode
#define W_BEGIN_STATIC_REFLECTED_TYPE(Type, BaseType, Version, AllocatorType) \
  W_RTTIINFO_DECL(Type, BaseType, Version)                                    \
  WRTTI WInternal::WStaticRTTIWrapper<Type>::s_RTTI = GetRTTI((Type*)0);    \
  W_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, BaseType, AllocatorType)


/// Ends the reflection code block that was opened with W_BEGIN_STATIC_REFLECTED_TYPE.
#define W_END_STATIC_REFLECTED_TYPE                                                                                                         \
  ;                                                                                                                                          \
  return WRTTI(GetTypeName((OwnType*)0), WGetStaticRTTI<OwnBaseType>(), sizeof(OwnType), GetTypeVersion((OwnType*)0),                      \
    WVariantTypeDeduction<OwnType>::value, flags, &Allocator, Properties, Functions, Attributes, MessageHandlers, MessageSenders, nullptr); \
  }


/// Begins a block for declaring reflected properties.
///
/// Use this within a reflected type block to start declaring properties.
/// Add property macros (W_MEMBER_PROPERTY, W_ACCESSOR_PROPERTY, etc.) between this
/// and W_END_PROPERTIES.
///
/// Example:
/// \code
///   W_BEGIN_PROPERTIES
///   {
///     W_MEMBER_PROPERTY("Name", m_sName),
///     W_ACCESSOR_PROPERTY("Value", GetValue, SetValue)
///   }
///   W_END_PROPERTIES;
/// \endcode
#define W_BEGIN_PROPERTIES static const WAbstractProperty* PropertyList[] =



/// Ends the property declaration block started with W_BEGIN_PROPERTIES.
#define W_END_PROPERTIES \
  ;                       \
  Properties = PropertyList

/// Within a W_BEGIN_REFLECTED_TYPE / W_END_REFLECTED_TYPE block, use this to start the block that declares all the functions.
#define W_BEGIN_FUNCTIONS static const WAbstractFunctionProperty* FunctionList[] =



/// Ends the block to declare functions that was started with W_BEGIN_FUNCTIONS.
#define W_END_FUNCTIONS \
  ;                      \
  Functions = FunctionList

/// Within a W_BEGIN_REFLECTED_TYPE / W_END_REFLECTED_TYPE block, use this to start the block that declares all the attributes.
#define W_BEGIN_ATTRIBUTES static const WPropertyAttribute* AttributeList[] =



/// Ends the block to declare attributes that was started with W_BEGIN_ATTRIBUTES.
#define W_END_ATTRIBUTES \
  ;                       \
  Attributes = AttributeList

/// Within a W_BEGIN_FUNCTIONS / W_END_FUNCTIONS; block, this adds a member or static function property stored inside the RTTI
/// data.
///
/// \param Function
///   The function to be executed, must match the C++ function name.
#define W_FUNCTION_PROPERTY(Function) (new WFunctionProperty<decltype(&OwnType::Function)>(W_PP_STRINGIFY(Function), &OwnType::Function))

/// Within a W_BEGIN_FUNCTIONS / W_END_FUNCTIONS; block, this adds a member or static function property stored inside the RTTI
/// data. Use this version if you need to change the name of the function or need to cast the function to one of its overload versions.
///
/// \param PropertyName
///   The name under which the property should be registered.
///
/// \param Function
///   The function to be executed, must match the C++ function name including the class name e.g. 'CLASS::NAME'.
#define W_FUNCTION_PROPERTY_EX(PropertyName, Function) (new WFunctionProperty<decltype(&Function)>(PropertyName, &Function))

/// \internal Used by W_SCRIPT_FUNCTION_PROPERTY
#define _W_SCRIPT_FUNCTION_PARAM(type, name) WScriptableFunctionAttribute::ArgType::type, name

/// Convenience macro to declare a function that can be called from scripts.
///
/// \param Function
///   The function to be executed, must match the C++ function name including the class name e.g. 'CLASS::NAME'.
///
/// Internally this calls W_FUNCTION_PROPERTY and adds a WScriptableFunctionAttribute.
/// Use the variadic arguments in pairs to configure how each function parameter gets exposed.
///   Use 'In', 'Out' or 'Inout' to specify whether a function parameter is only read, or also written back to.
///   Follow it with a string to specify the name under which the parameter should show up.
///
/// Example:
///   W_SCRIPT_FUNCTION_PROPERTY(MyFunc1NoParams)
///   W_SCRIPT_FUNCTION_PROPERTY(MyFunc2FloatInDoubleOut, In, "FloatValue", Out, "DoubleResult")
#define W_SCRIPT_FUNCTION_PROPERTY(Function, ...) \
  W_FUNCTION_PROPERTY(Function)->AddAttributes(new WScriptableFunctionAttribute(W_EXPAND_ARGS_PAIR_COMMA(_W_SCRIPT_FUNCTION_PARAM, ##__VA_ARGS__)))

/// Within a W_BEGIN_FUNCTIONS / W_END_FUNCTIONS; block, this adds a constructor function property stored inside the RTTI data.
///
/// \param Function
///   The function to be executed in the form of CLASS::FUNCTION_NAME.
#define W_CONSTRUCTOR_PROPERTY(...) (new WConstructorFunctionProperty<OwnType, ##__VA_ARGS__>())


// [internal] Helper macro to get the return type of a getter function.
#define W_GETTER_TYPE(Class, GetterFunc) decltype(std::declval<Class>().GetterFunc())

/// Within a W_BEGIN_PROPERTIES / W_END_PROPERTIES; block, this adds a property that uses custom getter / setter functions.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param Getter
///   The getter function for this property.
/// \param Setter
///   The setter function for this property.
///
/// \note There does not actually need to be a variable for this type of properties, as all accesses go through functions.
/// Thus you can for example expose a 'vector' property that is actually stored as a column of a matrix.
#define W_ACCESSOR_PROPERTY(PropertyName, Getter, Setter) \
  (new WAccessorProperty<OwnType, W_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))

/// Same as W_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define W_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, Getter) \
  (new WAccessorProperty<OwnType, W_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, nullptr))

// [internal] Helper macro to get the return type of a array getter function.
#define W_ARRAY_GETTER_TYPE(Class, GetterFunc) decltype(std::declval<Class>().GetterFunc(0))

/// Within a W_BEGIN_PROPERTIES / W_END_PROPERTIES; block, this adds a property that uses custom functions to access an array.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetCount
///   Function signature: WUInt32 GetCount() const;
/// \param Getter
///   Function signature: Type GetValue(WUInt32 uiIndex) const;
/// \param Setter
///   Function signature: void SetValue(WUInt32 uiIndex, Type value);
/// \param Insert
///   Function signature: void Insert(WUInt32 uiIndex, Type value);
/// \param Remove
///   Function signature: void Remove(WUInt32 uiIndex);
#define W_ARRAY_ACCESSOR_PROPERTY(PropertyName, GetCount, Getter, Setter, Insert, Remove) \
  (new WAccessorArrayProperty<OwnType, W_ARRAY_GETTER_TYPE(OwnType, OwnType::Getter)>(   \
    PropertyName, &OwnType::GetCount, &OwnType::Getter, &OwnType::Setter, &OwnType::Insert, &OwnType::Remove))

/// Same as W_ARRAY_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define W_ARRAY_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetCount, Getter)             \
  (new WAccessorArrayProperty<OwnType, W_ARRAY_GETTER_TYPE(OwnType, OwnType::Getter)>( \
    PropertyName, &OwnType::GetCount, &OwnType::Getter, nullptr, nullptr, nullptr))

#define W_SET_CONTAINER_TYPE(Class, GetterFunc) decltype(std::declval<Class>().GetterFunc())

#define W_SET_CONTAINER_SUB_TYPE(Class, GetterFunc) \
  WContainerSubTypeResolver<WTypeTraits<decltype(std::declval<Class>().GetterFunc())>::NonConstReferenceType>::Type

/// Within a W_BEGIN_PROPERTIES / W_END_PROPERTIES; block, this adds a property that uses custom functions to access a set.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetValues
///   Function signature: Container<Type> GetValues() const;
/// \param Insert
///   Function signature: void Insert(Type value);
/// \param Remove
///   Function signature: void Remove(Type value);
///
/// \note Container<Type> can be any container that can be iterated via range based for loops.
#define W_SET_ACCESSOR_PROPERTY(PropertyName, GetValues, Insert, Remove)                                            \
  (new WAccessorSetProperty<OwnType, WFunctionParameterTypeResolver<0, decltype(&OwnType::Insert)>::ParameterType, \
    W_SET_CONTAINER_TYPE(OwnType, GetValues)>(PropertyName, &OwnType::GetValues, &OwnType::Insert, &OwnType::Remove))

/// Same as W_SET_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define W_SET_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetValues)                                                              \
  (new WAccessorSetProperty<OwnType, W_SET_CONTAINER_SUB_TYPE(OwnType, GetValues), W_SET_CONTAINER_TYPE(OwnType, GetValues)>( \
    PropertyName, &OwnType::GetValues, nullptr, nullptr))

/// Within a W_BEGIN_PROPERTIES / W_END_PROPERTIES; block, this adds a property that uses custom functions to for write access to a
/// map.
///   Use this if you have a WHashTable or WMap to expose directly and just want to be informed of write operations.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetContainer
///   Function signature: const Container<Key, Type>& GetValues() const;
/// \param Insert
///   Function signature: void Insert(const char* szKey, Type value);
/// \param Remove
///   Function signature: void Remove(const char* szKey);
///
/// \note Container can be WMap or WHashTable
#define W_MAP_WRITE_ACCESSOR_PROPERTY(PropertyName, GetContainer, Insert, Remove)                                        \
  (new WWriteAccessorMapProperty<OwnType, WFunctionParameterTypeResolver<1, decltype(&OwnType::Insert)>::ParameterType, \
    W_SET_CONTAINER_TYPE(OwnType, GetContainer)>(PropertyName, &OwnType::GetContainer, &OwnType::Insert, &OwnType::Remove))

/// Within a W_BEGIN_PROPERTIES / W_END_PROPERTIES; block, this adds a property that uses custom functions to access a map.
///   Use this if you you want to hide the implementation details of the map from the user.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetKeyRange
///   Function signature: const Range GetValues() const;
///   Range has to be an object that a ranged based for-loop can iterate over containing the keys
///   implicitly convertible to Type / WString.
/// \param GetValue
///   Function signature: bool GetValue(const char* szKey, Type& value) const;
///   Returns whether the the key existed. value must be a non const ref as it is written to.
/// \param Insert
///   Function signature: void Insert(const char* szKey, Type value);
///   value can also be const and/or a reference.
/// \param Remove
///   Function signature: void Remove(const char* szKey);
///
/// \note Container can be WMap or WHashTable
#define W_MAP_ACCESSOR_PROPERTY(PropertyName, GetKeyRange, GetValue, Insert, Remove)                                \
  (new WAccessorMapProperty<OwnType, WFunctionParameterTypeResolver<1, decltype(&OwnType::Insert)>::ParameterType, \
    W_SET_CONTAINER_TYPE(OwnType, GetKeyRange)>(PropertyName, &OwnType::GetKeyRange, &OwnType::GetValue, &OwnType::Insert, &OwnType::Remove))

/// Same as W_MAP_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define W_MAP_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetKeyRange, GetValue)                                           \
  (new WAccessorMapProperty<OwnType,                                                                                     \
    WTypeTraits<WFunctionParameterTypeResolver<1, decltype(&OwnType::GetValue)>::ParameterType>::NonConstReferenceType, \
    W_SET_CONTAINER_TYPE(OwnType, GetKeyRange)>(PropertyName, &OwnType::GetKeyRange, &OwnType::GetValue, nullptr, nullptr))



/// Within a W_BEGIN_PROPERTIES / W_END_PROPERTIES; block, this adds a property that uses custom getter / setter functions.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param EnumType
///   The name of the enum struct used by WEnum.
/// \param Getter
///   The getter function for this property.
/// \param Setter
///   The setter function for this property.
#define W_ENUM_ACCESSOR_PROPERTY(PropertyName, EnumType, Getter, Setter) \
  (new WEnumAccessorProperty<OwnType, EnumType, W_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))

/// Same as W_ENUM_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define W_ENUM_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, EnumType, Getter) \
  (new WEnumAccessorProperty<OwnType, EnumType, W_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, nullptr))

/// Same as W_ENUM_ACCESSOR_PROPERTY, but for bitfields.
#define W_BITFLAGS_ACCESSOR_PROPERTY(PropertyName, BitflagsType, Getter, Setter) \
  (new WBitflagsAccessorProperty<OwnType, BitflagsType, W_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))

/// Same as W_BITFLAGS_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define W_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, BitflagsType, Getter) \
  (new WBitflagsAccessorProperty<OwnType, BitflagsType, W_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, nullptr))


// [internal] Helper macro to get the type of a class member.
#define W_MEMBER_TYPE(Class, Member) decltype(std::declval<Class>().Member)

#define W_MEMBER_CONTAINER_SUB_TYPE(Class, Member) \
  WContainerSubTypeResolver<WTypeTraits<decltype(std::declval<Class>().Member)>::NonConstReferenceType>::Type

/// Within a W_BEGIN_PROPERTIES / W_END_PROPERTIES; block, this adds a property that actually exists as a member.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param MemberName
///   The name of the member variable that should get exposed as a property.
///
/// \note Since the member is exposed directly, there is no way to know when the variable was modified. That also means
/// no custom limits to the values can be applied. If that becomes necessary, just add getter / setter functions and
/// expose the property as a W_ENUM_ACCESSOR_PROPERTY instead.
#define W_MEMBER_PROPERTY(PropertyName, MemberName)                                                   \
  (new WMemberProperty<OwnType, W_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,                    \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue, \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// Same as W_MEMBER_PROPERTY, but the property is read-only.
#define W_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                  \
  (new WMemberProperty<OwnType, W_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,                             \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, nullptr, \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// Same as W_MEMBER_PROPERTY, but the property is an array (WHybridArray, WDynamicArray or WDeque).
#define W_ARRAY_MEMBER_PROPERTY(PropertyName, MemberName)                                                                                  \
  (new WMemberArrayProperty<OwnType, W_MEMBER_TYPE(OwnType, MemberName), W_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>(PropertyName, \
    &WArrayPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer,                        \
    &WArrayPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))

/// Same as W_MEMBER_PROPERTY, but the property is a read-only array (WArrayPtr, WHybridArray, WDynamicArray or WDeque).
#define W_ARRAY_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                                   \
  (new WMemberArrayReadOnlyProperty<OwnType, W_MEMBER_TYPE(OwnType, MemberName), W_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>( \
    PropertyName, &WArrayPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer))

/// Same as W_MEMBER_PROPERTY, but the property is a set (WSet, WHashSet).
#define W_SET_MEMBER_PROPERTY(PropertyName, MemberName)                                                                                  \
  (new WMemberSetProperty<OwnType, W_MEMBER_TYPE(OwnType, MemberName), W_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>(PropertyName, \
    &WSetPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer,                        \
    &WSetPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))

/// Same as W_MEMBER_PROPERTY, but the property is a read-only set (WSet, WHashSet).
#define W_SET_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                           \
  (new WMemberSetProperty<OwnType, W_MEMBER_TYPE(OwnType, MemberName), W_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>( \
    PropertyName, &WSetPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, nullptr))

/// Same as W_MEMBER_PROPERTY, but the property is a map (WMap, WHashTable).
#define W_MAP_MEMBER_PROPERTY(PropertyName, MemberName)                                                                                  \
  (new WMemberMapProperty<OwnType, W_MEMBER_TYPE(OwnType, MemberName), W_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>(PropertyName, \
    &WMapPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer,                        \
    &WMapPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))

/// Same as W_MEMBER_PROPERTY, but the property is a read-only map (WMap, WHashTable).
#define W_MAP_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                           \
  (new WMemberMapProperty<OwnType, W_MEMBER_TYPE(OwnType, MemberName), W_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>( \
    PropertyName, &WMapPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, nullptr))

/// Within a W_BEGIN_PROPERTIES / W_END_PROPERTIES; block, this adds a property that actually exists as a member.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param EnumType
///   Name of the struct used by WEnum.
/// \param MemberName
///   The name of the member variable that should get exposed as a property.
///
/// \note Since the member is exposed directly, there is no way to know when the variable was modified. That also means
/// no custom limits to the values can be applied. If that becomes necessary, just add getter / setter functions and
/// expose the property as a W_ACCESSOR_PROPERTY instead.
#define W_ENUM_MEMBER_PROPERTY(PropertyName, EnumType, MemberName)                                    \
  (new WEnumMemberProperty<OwnType, EnumType, W_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,      \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue, \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// Same as W_ENUM_MEMBER_PROPERTY, but the property is read-only.
#define W_ENUM_MEMBER_PROPERTY_READ_ONLY(PropertyName, EnumType, MemberName)                                   \
  (new WEnumMemberProperty<OwnType, EnumType, W_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,               \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, nullptr, \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// Same as W_ENUM_MEMBER_PROPERTY, but for bitfields.
#define W_BITFLAGS_MEMBER_PROPERTY(PropertyName, BitflagsType, MemberName)                               \
  (new WBitflagsMemberProperty<OwnType, BitflagsType, W_MEMBER_TYPE(OwnType, MemberName)>(PropertyName, \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue,    \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue,    \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// Same as W_ENUM_MEMBER_PROPERTY_READ_ONLY, but for bitfields.
#define W_BITFLAGS_MEMBER_PROPERTY_READ_ONLY(PropertyName, BitflagsType, MemberName)                           \
  (new WBitflagsMemberProperty<OwnType, BitflagsType, W_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,       \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, nullptr, \
    &WPropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))



/// Within a W_BEGIN_PROPERTIES / W_END_PROPERTIES; block, this adds a constant property stored inside the RTTI data.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param Value
///   The constant value to be stored.
#define W_CONSTANT_PROPERTY(PropertyName, Value) (new WConstantProperty<decltype(Value)>(PropertyName, Value))



// [internal] Helper macro
#define W_ENUM_VALUE_TO_CONSTANT_PROPERTY(name) W_CONSTANT_PROPERTY(W_PP_STRINGIFY(name), (Storage)name),

/// Within a W_BEGIN_STATIC_REFLECTED_ENUM / W_END_STATIC_REFLECTED_ENUM block, this converts a
/// list of enum values into constant RTTI properties.
#define W_ENUM_CONSTANTS(...) W_EXPAND_ARGS(W_ENUM_VALUE_TO_CONSTANT_PROPERTY, ##__VA_ARGS__)

/// Within a W_BEGIN_STATIC_REFLECTED_ENUM / W_END_STATIC_REFLECTED_ENUM block, this converts a
/// an enum value into a constant RTTI property.
#define W_ENUM_CONSTANT(Value) W_CONSTANT_PROPERTY(W_PP_STRINGIFY(Value), (Storage)Value)

/// Within a W_BEGIN_STATIC_REFLECTED_BITFLAGS / W_END_STATIC_REFLECTED_BITFLAGS block, this converts a
/// list of bitflags into constant RTTI properties.
#define W_BITFLAGS_CONSTANTS(...) W_EXPAND_ARGS(W_ENUM_VALUE_TO_CONSTANT_PROPERTY, ##__VA_ARGS__)

/// Within a W_BEGIN_STATIC_REFLECTED_BITFLAGS / W_END_STATIC_REFLECTED_BITFLAGS block, this converts a
/// an bitflags into a constant RTTI property.
#define W_BITFLAGS_CONSTANT(Value) W_CONSTANT_PROPERTY(W_PP_STRINGIFY(Value), (Storage)Value)



/// Implements the necessary functionality for an enum to be statically reflectable.
///
/// \param Type
///   The enum struct used by WEnum for which reflection should be defined.
/// \param Version
///   The version of \a Type. Must be increased when the class changes.
#define W_BEGIN_STATIC_REFLECTED_ENUM(Type, Version)                          \
  W_BEGIN_STATIC_REFLECTED_TYPE(Type, WEnumBase, Version, WRTTINoAllocator) \
    ;                                                                          \
    using Storage = Type::StorageType;                                         \
    W_BEGIN_PROPERTIES                                                        \
      {                                                                        \
        W_CONSTANT_PROPERTY(W_PP_STRINGIFY(Type::Default), (Storage)Type::Default),

#define W_END_STATIC_REFLECTED_ENUM \
  }                                  \
  W_END_PROPERTIES                  \
  ;                                  \
  flags |= WTypeFlags::IsEnum;      \
  flags.Remove(WTypeFlags::Class);  \
  W_END_STATIC_REFLECTED_TYPE


/// Implements the necessary functionality for bitflags to be statically reflectable.
///
/// \param Type
///   The bitflags struct used by WBitflags for which reflection should be defined.
/// \param Version
///   The version of \a Type. Must be increased when the class changes.
#define W_BEGIN_STATIC_REFLECTED_BITFLAGS(Type, Version)                          \
  W_BEGIN_STATIC_REFLECTED_TYPE(Type, WBitflagsBase, Version, WRTTINoAllocator) \
    ;                                                                              \
    using Storage = Type::StorageType;                                             \
    W_BEGIN_PROPERTIES                                                            \
      {                                                                            \
        W_CONSTANT_PROPERTY(W_PP_STRINGIFY(Type::Default), (Storage)Type::Default),

#define W_END_STATIC_REFLECTED_BITFLAGS \
  }                                      \
  W_END_PROPERTIES                      \
  ;                                      \
  flags |= WTypeFlags::Bitflags;        \
  flags.Remove(WTypeFlags::Class);      \
  W_END_STATIC_REFLECTED_TYPE



/// Within an W_BEGIN_REFLECTED_TYPE / W_END_REFLECTED_TYPE block, use this to start the block that declares all the message
/// handlers.
#define W_BEGIN_MESSAGEHANDLERS static WAbstractMessageHandler* HandlerList[] =


/// Ends the block to declare message handlers that was started with W_BEGIN_MESSAGEHANDLERS.
#define W_END_MESSAGEHANDLERS \
  ;                            \
  MessageHandlers = HandlerList


/// Within an W_BEGIN_MESSAGEHANDLERS / W_END_MESSAGEHANDLERS; block, this adds another message handler.
///
/// \param MessageType
///   The type of message that this handler function accepts. You may add 'const' in front of it.
/// \param FunctionName
///   The actual C++ name of the message handler function.
///
/// \note A message handler is a function that takes one parameter of type WMessage (or a derived type) and returns void.
#define W_MESSAGE_HANDLER(MessageType, FunctionName)                                                                                   \
  new WInternal::MessageHandler<W_IS_CONST_MESSAGE_HANDLER(OwnType, MessageType, &OwnType::FunctionName)>::Impl<OwnType, MessageType, \
    &OwnType::FunctionName>()


/// Within an W_BEGIN_REFLECTED_TYPE / W_END_REFLECTED_TYPE block, use this to start the block that declares all the message
/// senders.
#define W_BEGIN_MESSAGESENDERS static WMessageSenderInfo SenderList[] =


/// Ends the block to declare message senders that was started with W_BEGIN_MESSAGESENDERS.
#define W_END_MESSAGESENDERS \
  ;                           \
  MessageSenders = SenderList;

/// Within an W_BEGIN_MESSAGESENDERS / W_END_MESSAGESENDERS block, this adds another message sender.
///
/// \param MemberName
///   The name of the member variable that should get exposed as a message sender.
///
/// \note A message sender must be derived from WMessageSenderBase.
#define W_MESSAGE_SENDER(MemberName)                                                \
  {                                                                                  \
    #MemberName, WGetStaticRTTI<W_MEMBER_TYPE(OwnType, MemberName)::MessageType>() \
  }
