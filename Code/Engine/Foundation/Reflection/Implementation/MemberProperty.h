#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/Variant.h>

// ***********************************************
// ***** Base class for accessing properties *****


/// Type-safe base class for accessing member properties with known data types.
///
/// Once you determine a property's type through the reflection system, you can safely cast
/// the abstract property pointer to this typed version. This provides type-safe access to
/// property values without the overhead of variant conversions.
///
/// Example usage:
/// ```cpp
/// auto* abstractProp = rtti->FindPropertyByName("someProperty");
/// if (abstractProp->GetSpecificType() == WGetStaticRTTI<int>())
/// {
///   auto* intProp = static_cast<WTypedMemberProperty<int>*>(abstractProp);
///   int value = intProp->GetValue(instance);
/// }
/// ```
template <typename Type>
class WTypedMemberProperty : public WAbstractMemberProperty
{
public:
  /// Passes the property name through to WAbstractMemberProperty.
  WTypedMemberProperty(const char* szPropertyName)
    : WAbstractMemberProperty(szPropertyName)
  {
    m_Flags = WPropertyFlags::GetParameterFlags<Type>();
    static_assert(
      !std::is_pointer<Type>::value ||
        WVariant::TypeDeduction<typename WTypeTraits<Type>::NonConstReferencePointerType>::value == WVariantType::Invalid,
      "Pointer to standard types are not supported.");
  }

  /// Returns the actual type of the property. You can then compare that with known types, eg. compare it to WGetStaticRTTI<int>()
  /// to see whether this is an int property.
  virtual const WRTTI* GetSpecificType() const override // [tested]
  {
    return WGetStaticRTTI<typename WTypeTraits<Type>::NonConstReferencePointerType>();
  }

  /// Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue(const void* pInstance) const = 0; // [tested]

  /// Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, Type value) const = 0; // [tested]

  virtual void GetValuePtr(const void* pInstance, void* pObject) const override { *static_cast<Type*>(pObject) = GetValue(pInstance); };
  virtual void SetValuePtr(void* pInstance, const void* pObject) const override { SetValue(pInstance, *static_cast<const Type*>(pObject)); };
};

/// Specialization of WTypedMemberProperty for const char*.
///
/// This works because WTypedMemberProperty< typename WTypeTraits<Type>::NonConstReferenceType > in WAccessorProperty
/// does not actually remove the constness of the type but of the pointer, so const char* is not affected.
template <>
class WTypedMemberProperty<const char*> : public WAbstractMemberProperty
{
public:
  WTypedMemberProperty(const char* szPropertyName)
    : WAbstractMemberProperty(szPropertyName)
  {
    // We treat const char* as a basic type and not a pointer.
    m_Flags = WPropertyFlags::GetParameterFlags<const char*>();
  }

  virtual const WRTTI* GetSpecificType() const override // [tested]
  {
    return WGetStaticRTTI<const char*>();
  }

  virtual const char* GetValue(const void* pInstance) const = 0;
  virtual void SetValue(void* pInstance, const char* value) const = 0;
  virtual void GetValuePtr(const void* pInstance, void* pObject) const override { *static_cast<const char**>(pObject) = GetValue(pInstance); };
  virtual void SetValuePtr(void* pInstance, const void* pObject) const override { SetValue(pInstance, *static_cast<const char* const*>(pObject)); };
};


// *******************************************************************
// ***** Class for properties that use custom accessor functions *****

/// Implementation of WTypedMemberProperty that uses custom getter/setter functions to access a property.
///
/// This property type is used when you want to expose computed or transformed values as properties,
/// or when you need to perform validation, logging, or side effects during property access.
/// The actual data may be stored differently than how it's exposed through the property interface.
///
/// Use this when:
/// - Property value needs computation or transformation
/// - You need to validate or clamp values on set
/// - Property access should trigger side effects
/// - The internal storage format differs from the exposed type
template <typename Class, typename Type>
class WAccessorProperty : public WTypedMemberProperty<typename WTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType = typename WTypeTraits<Type>::NonConstReferenceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  /// Constructor.
  WAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter)
    : WTypedMemberProperty<RealType>(szPropertyName)
  {
    W_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      WAbstractMemberProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }

  /// Always returns nullptr; once a property is modified through accessors, there is no point in giving more direct access to
  /// others.
  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    W_IGNORE_UNUSED(pInstance);

    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  /// Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual RealType GetValue(const void* pInstance) const override // [tested]
  {
    return (static_cast<const Class*>(pInstance)->*m_Getter)();
  }

  /// Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, RealType value) const override // [tested]
  {
    W_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", WAbstractProperty::GetPropertyName());

    if (m_Setter)
      (static_cast<Class*>(pInstance)->*m_Setter)(value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};


// *************************************************************
// ***** Classes for properties that are accessed directly *****

/// [internal] Helper class to generate accessor functions for (private) members of another class
template <typename Class, typename Type, Type Class::*Member>
struct WPropertyAccessor
{
  static Type GetValue(const Class* pInstance) { return (*pInstance).*Member; }

  static void SetValue(Class* pInstance, Type value) { (*pInstance).*Member = value; }

  static void* GetPropertyPointer(const Class* pInstance) { return (void*)&((*pInstance).*Member); }
};


/// Implementation of WTypedMemberProperty that provides direct access to member variables.
///
/// This property type offers the most efficient access to object members by directly
/// reading from and writing to the memory location of a class member. It's the preferred
/// choice for simple data members that don't require special handling.
template <typename Class, typename Type>
class WMemberProperty : public WTypedMemberProperty<Type>
{
public:
  using GetterFunc = Type (*)(const Class* pInstance);
  using SetterFunc = void (*)(Class* pInstance, Type value);
  using PointerFunc = void* (*)(const Class* pInstance);

  /// Constructor.
  WMemberProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer)
    : WTypedMemberProperty<Type>(szPropertyName)
  {
    W_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;
    m_Pointer = pointer;

    if (m_Setter == nullptr)
      WAbstractMemberProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }

  /// Returns a pointer to the member property.
  virtual void* GetPropertyPointer(const void* pInstance) const override { return m_Pointer(static_cast<const Class*>(pInstance)); }

  /// Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue(const void* pInstance) const override { return m_Getter(static_cast<const Class*>(pInstance)); }

  /// Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, Type value) const override
  {
    W_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", WAbstractProperty::GetPropertyName());

    if (m_Setter)
      m_Setter(static_cast<Class*>(pInstance), value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
  PointerFunc m_Pointer;
};
