#pragma once

/// \file

#include <Foundation/Basics.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>

class WRTTI;
class WPropertyAttribute;

/// Determines whether a type is WIsBitflags.
template <typename T>
struct WIsBitflags
{
  static constexpr bool value = false;
};

template <typename T>
struct WIsBitflags<WBitflags<T>>
{
  static constexpr bool value = true;
};

/// Determines whether a type is WIsBitflags.
template <typename T>
struct WIsEnum
{
  static constexpr bool value = std::is_enum<T>::value;
};

template <typename T>
struct WIsEnum<WEnum<T>>
{
  static constexpr bool value = true;
};

/// Flags used to describe a property and its type.
struct WPropertyFlags
{
  using StorageType = WUInt16;

  enum Enum : WUInt16
  {
    StandardType = W_BIT(0), ///< Anything that can be stored inside an WVariant except for pointers and containers.
    IsEnum = W_BIT(1),       ///< enum property, cast to WAbstractEnumerationProperty.
    Bitflags = W_BIT(2),     ///< Bitflags property, cast to WAbstractEnumerationProperty.
    Class = W_BIT(3),        ///< A struct or class. All of the above are mutually exclusive.

    Const = W_BIT(4),        ///< Property value is const.
    Reference = W_BIT(5),    ///< Property value is a reference.
    Pointer = W_BIT(6),      ///< Property value is a pointer.

    PointerOwner = W_BIT(7), ///< This pointer property takes ownership of the passed pointer.
    ReadOnly = W_BIT(8),     ///< Can only be read but not modified.
    Hidden = W_BIT(9),       ///< This property should not appear in the UI.
    Phantom = W_BIT(10),     ///< Phantom types are mirrored types on the editor side. Ie. they do not exist as actual classes in the process. Also used
                              ///< for data driven types, e.g. by the Visual Shader asset.

    VarOut = W_BIT(11),      ///< Tag for non-const-ref function parameters to indicate usage 'out'
    VarInOut = W_BIT(12),    ///< Tag for non-const-ref function parameters to indicate usage 'inout'

    PureFunction = Const,     ///< The visual script function doesn't need an execution pin.

    Default = 0,
    Void = 0
  };

  struct Bits
  {
    StorageType StandardType : 1;
    StorageType IsEnum : 1;
    StorageType Bitflags : 1;
    StorageType Class : 1;

    StorageType Const : 1;
    StorageType Reference : 1;
    StorageType Pointer : 1;

    StorageType PointerOwner : 1;
    StorageType ReadOnly : 1;
    StorageType Hidden : 1;
    StorageType Phantom : 1;

    StorageType VarOut : 1;
    StorageType VarInOut : 1;

    StorageType PureFunction : 1;
  };

  template <class Type>
  static WBitflags<WPropertyFlags> GetParameterFlags()
  {
    using CleanType = typename WTypeTraits<Type>::NonConstReferencePointerType;
    WBitflags<WPropertyFlags> flags;
    constexpr WVariantType::Enum type = static_cast<WVariantType::Enum>(WVariantTypeDeduction<CleanType>::value);
    if constexpr (std::is_same<CleanType, WVariant>::value ||
                  std::is_same<Type, const char*>::value || // We treat const char* as a basic type and not a pointer.
                  (type >= WVariantType::FirstStandardType && type <= WVariantType::LastStandardType))
      flags.Add(WPropertyFlags::StandardType);
    else if constexpr (WIsEnum<CleanType>::value)
      flags.Add(WPropertyFlags::IsEnum);
    else if constexpr (WIsBitflags<CleanType>::value)
      flags.Add(WPropertyFlags::Bitflags);
    else
      flags.Add(WPropertyFlags::Class);

    if constexpr (std::is_const<typename WTypeTraits<Type>::NonReferencePointerType>::value)
      flags.Add(WPropertyFlags::Const);

    if constexpr (std::is_pointer<Type>::value && !std::is_same<Type, const char*>::value)
      flags.Add(WPropertyFlags::Pointer);

    if constexpr (std::is_reference<Type>::value)
      flags.Add(WPropertyFlags::Reference);

    return flags;
  }
};

template <>
inline WBitflags<WPropertyFlags> WPropertyFlags::GetParameterFlags<void>()
{
  return WBitflags<WPropertyFlags>();
}

W_DECLARE_FLAGS_OPERATORS(WPropertyFlags)

/// Describes what category a property belongs to.
struct WPropertyCategory
{
  using StorageType = WUInt8;

  enum Enum
  {
    Constant, ///< The property is a constant value that is stored inside the RTTI data.
    Member,   ///< The property is a 'member property', i.e. it represents some accessible value. Cast to WAbstractMemberProperty.
    Function, ///< The property is a function which can be called. Cast to WAbstractFunctionProperty.
    Array,    ///< The property is actually an array of values. The array dimensions might be changeable. Cast to WAbstractArrayProperty.
    Set,      ///< The property is actually a set of values. Cast to WAbstractSetProperty.
    Map,      ///< The property is actually a map from string to values. Cast to WAbstractMapProperty.
    Default = Member
  };
};

/// Base interface for all properties in the reflection system.
///
/// Properties represent accessible data members, functions, or virtual data in reflected types.
/// This base class provides the common interface and metadata for property introspection.
///
/// Property categories:
/// - Constant: Compile-time constant values stored in RTTI data
/// - Member: Object data members with getter/setter access
/// - Function: Callable methods with parameters and return values
/// - Array: Container properties with indexed access
/// - Set: Collection properties with unique elements
/// - Map: Key-value pair collections
///
/// Key features:
/// - Property flags for type information (const, reference, pointer, etc.)
/// - Attribute system for metadata (UI hints, validation, etc.)
/// - Type-safe casting to specific property interfaces
/// - Hierarchical property introspection
///
/// Usage: Properties are typically registered through reflection macros rather than created manually.
class W_FOUNDATION_DLL WAbstractProperty
{
public:
  /// The constructor must get the name of the property. The string must be a compile-time constant.
  WAbstractProperty(const char* szPropertyName) { m_szPropertyName = szPropertyName; }

  virtual ~WAbstractProperty();

  /// Returns the name of the property.
  const char* GetPropertyName() const { return m_szPropertyName; }

  /// Returns the type information of the constant property. Use this to cast this property to a specific version of
  /// WTypedConstantProperty.
  virtual const WRTTI* GetSpecificType() const = 0;

  /// Returns the category of this property. Cast this property to the next higher type for more information.
  virtual WPropertyCategory::Enum GetCategory() const = 0; // [tested]

  /// Returns the flags of the property.
  const WBitflags<WPropertyFlags>& GetFlags() const { return m_Flags; };

  /// Adds flags to the property. Returns itself to allow to be called during initialization.
  WAbstractProperty* AddFlags(WBitflags<WPropertyFlags> flags)
  {
    m_Flags.Add(flags);
    return this;
  };

  /// Adds attributes to the property. Returns itself to allow to be called during initialization. Allocate an attribute using
  /// standard 'new'.
  WAbstractProperty* AddAttributes(WPropertyAttribute* pAttrib1, WPropertyAttribute* pAttrib2 = nullptr, WPropertyAttribute* pAttrib3 = nullptr,
    WPropertyAttribute* pAttrib4 = nullptr, WPropertyAttribute* pAttrib5 = nullptr, WPropertyAttribute* pAttrib6 = nullptr)
  {
    W_ASSERT_DEV(pAttrib1 != nullptr, "invalid attribute");

    m_Attributes.PushBack(pAttrib1);
    if (pAttrib2)
      m_Attributes.PushBack(pAttrib2);
    if (pAttrib3)
      m_Attributes.PushBack(pAttrib3);
    if (pAttrib4)
      m_Attributes.PushBack(pAttrib4);
    if (pAttrib5)
      m_Attributes.PushBack(pAttrib5);
    if (pAttrib6)
      m_Attributes.PushBack(pAttrib6);
    return this;
  };

  /// Returns the array of property attributes.
  WArrayPtr<const WPropertyAttribute* const> GetAttributes() const { return m_Attributes; }

  /// Returns the first attribute that derives from the given type, or nullptr if nothing is found.
  template <typename Type>
  const Type* GetAttributeByType() const;

protected:
  WBitflags<WPropertyFlags> m_Flags;
  const char* m_szPropertyName;
  WHybridArray<const WPropertyAttribute*, 2, WStaticsAllocatorWrapper> m_Attributes; // Do not track RTTI data.
};

/// This is the base class for all constant properties that are stored inside the RTTI data.
class W_FOUNDATION_DLL WAbstractConstantProperty : public WAbstractProperty
{
public:
  /// Passes the property name through to WAbstractProperty.
  WAbstractConstantProperty(const char* szPropertyName)
    : WAbstractProperty(szPropertyName)
  {
  }

  /// Returns WPropertyCategory::Constant.
  virtual WPropertyCategory::Enum GetCategory() const override { return WPropertyCategory::Constant; } // [tested]

  /// Returns a pointer to the constant data or nullptr. See WAbstractMemberProperty::GetPropertyPointer for more information.
  virtual void* GetPropertyPointer() const = 0;

  /// Returns the constant value as an WVariant
  virtual WVariant GetConstant() const = 0;
};

/// Base class for properties that represent data members of a class or struct.
///
/// Member properties provide access to object data through getter/setter mechanisms or direct memory access.
/// They support both simple types (int, float) and complex types (classes, structs) with proper type safety.
///
/// Access patterns:
/// - Direct pointer access: GetPropertyPointer() for in-place modification
/// - Value access: GetValuePtr()/SetValuePtr() for type-safe copying
/// - Variant access: Through WReflectionUtils for dynamic typing
///
/// Important: If WPropertyFlags::Pointer is set, you must use GetValuePtr/SetValuePtr instead of casting
/// to WTypedMemberProperty, because pointer properties have different type semantics.
///
/// Performance considerations:
/// - Direct pointer access is fastest but requires type knowledge
/// - Value copying is safer but involves memory operations
/// - Custom accessors (functions) add virtual call overhead
class W_FOUNDATION_DLL WAbstractMemberProperty : public WAbstractProperty
{
public:
  /// Passes the property name through to WAbstractProperty.
  WAbstractMemberProperty(const char* szPropertyName)
    : WAbstractProperty(szPropertyName)
  {
  }

  /// Returns WPropertyCategory::Member.
  virtual WPropertyCategory::Enum GetCategory() const override { return WPropertyCategory::Member; }

  /// Returns a pointer to the property data or nullptr. If a valid pointer is returned, that pointer and the information from
  /// GetSpecificType() can be used to step deeper into the type (if required).
  ///
  /// You need to pass the pointer to an object on which you are operating. This function is mostly of interest when the property itself is
  /// a compound type (a struct or class). If it is a simple type (int, float, etc.) it doesn't make much sense to retrieve the pointer.
  ///
  /// For example GetSpecificType() might return that a property is of type WVec3. In that case one might either stop and just use the code
  /// to handle WVec3 types, or one might continue and enumerate all sub-properties (x, y and z) as well.
  ///
  /// \note There is no guarantee that this function returns a non-nullptr pointer, independent of the type. When a property uses custom
  /// 'accessors' (functions to get / set the property value), it is not possible (or useful) to get the property pointer.
  virtual void* GetPropertyPointer(const void* pInstance) const = 0;

  /// Writes the value of this property in pInstance to pObject.
  /// pObject needs to point to an instance of this property's type.
  virtual void GetValuePtr(const void* pInstance, void* out_pObject) const = 0;

  /// Sets the value of pObject to the property in pInstance.
  /// pObject needs to point to an instance of this property's type.
  virtual void SetValuePtr(void* pInstance, const void* pObject) const = 0;
};


/// Base class for properties that represent arrays or sequential containers.
///
/// Array properties provide indexed access to collections of elements with dynamic or fixed sizing.
/// They support all standard container operations: access, insertion, removal, and resizing.
///
/// Supported container types:
/// - WDynamicArray, WHybridArray, WStaticArray
/// - Standard arrays (T[N])
/// - Custom containers implementing the array interface
///
/// Key operations:
/// - Indexed access: GetValue()/SetValue() by index
/// - Dynamic sizing: SetCount(), Insert(), Remove()
/// - Bulk operations: Clear() for efficiency
///
/// Performance considerations:
/// - GetCount() may involve virtual calls for dynamic containers
/// - Insert/Remove operations may require element shifting
/// - SetCount() can be expensive for complex element types
/// - GetValuePointer() provides direct access when available
class W_FOUNDATION_DLL WAbstractArrayProperty : public WAbstractProperty
{
public:
  /// Passes the property name through to WAbstractProperty.
  WAbstractArrayProperty(const char* szPropertyName)
    : WAbstractProperty(szPropertyName)
  {
  }

  /// Returns WPropertyCategory::Array.
  virtual WPropertyCategory::Enum GetCategory() const override { return WPropertyCategory::Array; }

  /// Returns number of elements.
  virtual WUInt32 GetCount(const void* pInstance) const = 0;

  /// Writes element at index uiIndex to the target of pObject.
  virtual void GetValue(const void* pInstance, WUInt32 uiIndex, void* pObject) const = 0;

  /// Writes the target of pObject to the element at index uiIndex.
  virtual void SetValue(void* pInstance, WUInt32 uiIndex, const void* pObject) const = 0;

  /// Inserts the target of pObject into the array at index uiIndex.
  virtual void Insert(void* pInstance, WUInt32 uiIndex, const void* pObject) const = 0;

  /// Removes the element in the array at index uiIndex.
  virtual void Remove(void* pInstance, WUInt32 uiIndex) const = 0;

  /// Clears the array.
  virtual void Clear(void* pInstance) const = 0;

  /// Resizes the array to uiCount.
  virtual void SetCount(void* pInstance, WUInt32 uiCount) const = 0;

  virtual void* GetValuePointer(void* pInstance, WUInt32 uiIndex) const
  {
    W_IGNORE_UNUSED(pInstance);
    W_IGNORE_UNUSED(uiIndex);
    return nullptr;
  }
};


/// Base class for properties that represent sets or unique value collections.
///
/// Set properties provide collection semantics with unique elements and no ordering guarantees.
/// They support standard set operations: insertion, removal, membership testing, and enumeration.
///
/// Supported set types:
/// - WSet (tree-based, ordered)
/// - WHashSet (hash-based, unordered)
/// - Custom containers implementing the set interface
///
/// Element type restrictions:
/// - Standard types (int, float, string, etc.)
/// - Pointer types (for object references)
/// - Types with proper equality and hash operations
///
/// Performance characteristics:
/// - Contains(): O(log n) for WSet, O(1) average for WHashSet
/// - Insert/Remove: O(log n) for WSet, O(1) average for WHashSet
/// - GetValues(): O(n) - creates a copy of all elements
class W_FOUNDATION_DLL WAbstractSetProperty : public WAbstractProperty
{
public:
  /// Passes the property name through to WAbstractProperty.
  WAbstractSetProperty(const char* szPropertyName)
    : WAbstractProperty(szPropertyName)
  {
  }

  /// Returns WPropertyCategory::Set.
  virtual WPropertyCategory::Enum GetCategory() const override { return WPropertyCategory::Set; }

  /// Returns whether the set is empty.
  virtual bool IsEmpty(const void* pInstance) const = 0;

  /// Clears the set.
  virtual void Clear(void* pInstance) const = 0;

  /// Inserts the target of pObject into the set.
  virtual void Insert(void* pInstance, const void* pObject) const = 0;

  /// Removes the target of pObject from the set.
  virtual void Remove(void* pInstance, const void* pObject) const = 0;

  /// Returns whether the target of pObject is in the set.
  virtual bool Contains(const void* pInstance, const void* pObject) const = 0;

  /// Writes the content of the set to out_keys.
  virtual void GetValues(const void* pInstance, WDynamicArray<WVariant>& out_keys) const = 0;
};


/// Base class for properties that represent maps or key-value collections.
///
/// Map properties provide associative container semantics with string keys and typed values.
/// They support standard map operations: key-based access, insertion, removal, and enumeration.
///
/// Supported map types:
/// - WMap (tree-based, ordered by key)
/// - WHashTable (hash-based, unordered)
/// - Custom containers implementing the map interface
///
/// Key restrictions:
/// - Keys are always strings (const char*)
/// - Keys must be valid UTF-8 strings
/// - Key comparison is case-sensitive
///
/// Value type restrictions:
/// - Standard types (int, float, string, etc.)
/// - Pointer types (for object references)
/// - Complex types supported through reflection
///
/// Performance characteristics:
/// - GetValue/Contains: O(log n) for WMap, O(1) average for WHashTable
/// - Insert/Remove: O(log n) for WMap, O(1) average for WHashTable
/// - GetKeys: O(n) - creates a copy of all key strings
class W_FOUNDATION_DLL WAbstractMapProperty : public WAbstractProperty
{
public:
  /// Passes the property name through to WAbstractProperty.
  WAbstractMapProperty(const char* szPropertyName)
    : WAbstractProperty(szPropertyName)
  {
  }

  /// Returns WPropertyCategory::Map.
  virtual WPropertyCategory::Enum GetCategory() const override { return WPropertyCategory::Map; }

  /// Returns whether the set is empty.
  virtual bool IsEmpty(const void* pInstance) const = 0;

  /// Clears the set.
  virtual void Clear(void* pInstance) const = 0;

  /// Inserts the target of pObject into the set.
  virtual void Insert(void* pInstance, const char* szKey, const void* pObject) const = 0;

  /// Removes the target of pObject from the set.
  virtual void Remove(void* pInstance, const char* szKey) const = 0;

  /// Returns whether the target of pObject is in the set.
  virtual bool Contains(const void* pInstance, const char* szKey) const = 0;

  /// Writes element at index uiIndex to the target of pObject.
  virtual bool GetValue(const void* pInstance, const char* szKey, void* pObject) const = 0;

  /// Writes the content of the set to out_keys.
  virtual void GetKeys(const void* pInstance, WHybridArray<WString, 16>& out_keys) const = 0;
};

/// Use getArgument<N, Args...>::Type to get the type of the Nth argument in Args.
template <int _Index, class... Args>
struct getArgument;

template <class Head, class... Tail>
struct getArgument<0, Head, Tail...>
{
  using Type = Head;
};

template <int _Index, class Head, class... Tail>
struct getArgument<_Index, Head, Tail...>
{
  using Type = typename getArgument<_Index - 1, Tail...>::Type;
};

/// Template that allows to probe a function for a parameter and return type.
template <int I, typename FUNC>
struct WFunctionParameterTypeResolver
{
};

template <int I, typename R, typename... P>
struct WFunctionParameterTypeResolver<I, R (*)(P...)>
{
  enum Constants
  {
    Arguments = sizeof...(P),
  };
  static_assert(I < Arguments, "I needs to be smaller than the number of function parameters.");
  using ParameterType = typename getArgument<I, P...>::Type;
  using ReturnType = R;
};

template <int I, class Class, typename R, typename... P>
struct WFunctionParameterTypeResolver<I, R (Class::*)(P...)>
{
  enum Constants
  {
    Arguments = sizeof...(P),
  };
  static_assert(I < Arguments, "I needs to be smaller than the number of function parameters.");
  using ParameterType = typename getArgument<I, P...>::Type;
  using ReturnType = R;
};

template <int I, class Class, typename R, typename... P>
struct WFunctionParameterTypeResolver<I, R (Class::*)(P...) const>
{
  enum Constants
  {
    Arguments = sizeof...(P),
  };
  static_assert(I < Arguments, "I needs to be smaller than the number of function parameters.");
  using ParameterType = typename getArgument<I, P...>::Type;
  using ReturnType = R;
};

/// Template that allows to probe a single parameter function for parameter and return type.
template <typename FUNC>
struct WMemberFunctionParameterTypeResolver
{
};

template <class Class, typename R, typename P>
struct WMemberFunctionParameterTypeResolver<R (Class::*)(P)>
{
  using ParameterType = P;
  using ReturnType = R;
};

/// Template that allows to probe a container for its element type.
template <typename CONTAINER>
struct WContainerSubTypeResolver
{
};

template <typename T>
struct WContainerSubTypeResolver<WArrayPtr<T>>
{
  using Type = typename WTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct WContainerSubTypeResolver<WDynamicArray<T>>
{
  using Type = typename WTypeTraits<T>::NonConstReferenceType;
};

template <typename T, WUInt32 Size>
struct WContainerSubTypeResolver<WHybridArray<T, Size>>
{
  using Type = typename WTypeTraits<T>::NonConstReferenceType;
};

template <typename T, WUInt32 Size>
struct WContainerSubTypeResolver<WStaticArray<T, Size>>
{
  using Type = typename WTypeTraits<T>::NonConstReferenceType;
};

template <typename T, WUInt16 Size>
struct WContainerSubTypeResolver<WSmallArray<T, Size>>
{
  using Type = typename WTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct WContainerSubTypeResolver<WDeque<T>>
{
  using Type = typename WTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct WContainerSubTypeResolver<WSet<T>>
{
  using Type = typename WTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct WContainerSubTypeResolver<WHashSet<T>>
{
  using Type = typename WTypeTraits<T>::NonConstReferenceType;
};

template <typename K, typename T>
struct WContainerSubTypeResolver<WHashTable<K, T>>
{
  using Type = typename WTypeTraits<T>::NonConstReferenceType;
};

template <typename K, typename T>
struct WContainerSubTypeResolver<WMap<K, T>>
{
  using Type = typename WTypeTraits<T>::NonConstReferenceType;
};


/// Describes what kind of function a property is.
struct WFunctionType
{
  using StorageType = WUInt8;

  enum Enum
  {
    Member,       ///< A normal member function, a valid instance pointer must be provided to call.
    StaticMember, ///< A static member function, instance pointer will be ignored.
    Constructor,  ///< A constructor. Return value is a void* pointing to the new instance allocated with the default allocator.
    Default = Member
  };
};

/// Base class for properties that represent callable functions or methods.
///
/// Function properties enable runtime invocation of methods with type-safe parameter passing
/// and return value handling. They support member functions, static functions, and constructors.
///
/// Function types:
/// - Member: Instance methods requiring a valid object pointer
/// - StaticMember: Static methods, instance pointer ignored
/// - Constructor: Special functions that create new objects
///
/// Parameter handling:
/// - Standard types: Passed by value, exact type matching required
/// - Enums/Bitflags: Passed as WInt64 values through WEnum/WBitflags
/// - Classes: Always passed by pointer regardless of declaration
/// - Out parameters: Written back to source variants after execution
/// - Null values: Represented as invalid variants (except for WVariant parameters)
///
/// Performance considerations:
/// - Function calls involve virtual dispatch and parameter marshaling
/// - Parameter conversion can be expensive for complex types
/// - Out parameter writeback requires variant assignments
/// - Constructor calls include allocation overhead
class W_FOUNDATION_DLL WAbstractFunctionProperty : public WAbstractProperty
{
public:
  /// Passes the property name through to WAbstractProperty.
  WAbstractFunctionProperty(const char* szPropertyName)
    : WAbstractProperty(szPropertyName)
  {
  }

  virtual WPropertyCategory::Enum GetCategory() const override { return WPropertyCategory::Function; }
  /// Returns the type of function, see WFunctionPropertyType::Enum.
  virtual WFunctionType::Enum GetFunctionType() const = 0;
  /// Returns the type of the return value.
  virtual const WRTTI* GetReturnType() const = 0;
  /// Returns property flags of the return value.
  virtual WBitflags<WPropertyFlags> GetReturnFlags() const = 0;
  /// Returns the number of arguments.
  virtual WUInt32 GetArgumentCount() const = 0;
  /// Returns the type of the given argument.
  virtual const WRTTI* GetArgumentType(WUInt32 uiParamIndex) const = 0;
  /// Returns the property flags of the given argument.
  virtual WBitflags<WPropertyFlags> GetArgumentFlags(WUInt32 uiParamIndex) const = 0;

  /// Calls the function. Provide the instance on which the function is supposed to be called.
  ///
  /// arguments must be the size of GetArgumentCount, the following rules apply for both arguments and return value:
  /// Any standard type must be provided by value, even if it is a pointer to one. Types must match exactly, no ConvertTo is called.
  /// enum and bitflags are supported if WEnum / WBitflags is used, value must be provided as WInt64.
  /// Out values (&, *) are written back to the variant they were read from.
  /// Any class is provided by pointer, regardless of whether it is a pointer or not.
  /// The returnValue must only be valid if the return value is a ref or by value class. In that case
  /// returnValue must be a ptr to a valid class instance of the returned type.
  /// An invalid variant is equal to a nullptr, except for if the argument is of type WVariant, in which case
  /// it is impossible to pass along a nullptr.
  virtual void Execute(void* pInstance, WArrayPtr<WVariant> arguments, WVariant& out_returnValue) const = 0;

  virtual const WRTTI* GetSpecificType() const override { return GetReturnType(); }

  /// Adds flags to the property. Returns itself to allow to be called during initialization.
  WAbstractFunctionProperty* AddFlags(WBitflags<WPropertyFlags> flags)
  {
    return static_cast<WAbstractFunctionProperty*>(WAbstractProperty::AddFlags(flags));
  }

  /// Adds attributes to the property. Returns itself to allow to be called during initialization. Allocate an attribute using
  /// standard 'new'.
  WAbstractFunctionProperty* AddAttributes(WPropertyAttribute* pAttrib1, WPropertyAttribute* pAttrib2 = nullptr, WPropertyAttribute* pAttrib3 = nullptr,
    WPropertyAttribute* pAttrib4 = nullptr, WPropertyAttribute* pAttrib5 = nullptr, WPropertyAttribute* pAttrib6 = nullptr)
  {
    return static_cast<WAbstractFunctionProperty*>(WAbstractProperty::AddAttributes(pAttrib1, pAttrib2, pAttrib3, pAttrib4, pAttrib5, pAttrib6));
  }
};
