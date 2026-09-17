#pragma once

/// \file

#include <Foundation/Reflection/Implementation/MemberProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// The base class for enum and bitflags member properties.
///
/// Cast any property whose type derives from WEnumBase or WBitflagsBase class to access its value.
class WAbstractEnumerationProperty : public WAbstractMemberProperty
{
public:
  /// Passes the property name through to WAbstractMemberProperty.
  WAbstractEnumerationProperty(const char* szPropertyName)
    : WAbstractMemberProperty(szPropertyName)
  {
  }

  /// Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual WInt64 GetValue(const void* pInstance) const = 0;

  /// Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, WInt64 value) const = 0;

  virtual void GetValuePtr(const void* pInstance, void* pObject) const override
  {
    *static_cast<WInt64*>(pObject) = GetValue(pInstance);
  }

  virtual void SetValuePtr(void* pInstance, const void* pObject) const override
  {
    SetValue(pInstance, *static_cast<const WInt64*>(pObject));
  }
};


/// [internal] Base class for enum / bitflags properties that already defines the type.
template <typename EnumType>
class WTypedEnumProperty : public WAbstractEnumerationProperty
{
public:
  /// Passes the property name through to WAbstractEnumerationProperty.
  WTypedEnumProperty(const char* szPropertyName)
    : WAbstractEnumerationProperty(szPropertyName)
  {
  }

  /// Returns the actual type of the property. You can then test whether it derives from WEnumBase or
  ///  WBitflagsBase to determine whether we are dealing with an enum or bitflags property.
  virtual const WRTTI* GetSpecificType() const override // [tested]
  {
    return WGetStaticRTTI<typename WTypeTraits<EnumType>::NonConstReferenceType>();
  }
};


/// [internal] An implementation of WTypedEnumProperty that uses custom getter / setter functions to access an enum property.
template <typename Class, typename EnumType, typename Type>
class WEnumAccessorProperty : public WTypedEnumProperty<EnumType>
{
public:
  using RealType = typename WTypeTraits<Type>::NonConstReferenceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  /// Constructor.
  WEnumAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter)
    : WTypedEnumProperty<EnumType>(szPropertyName)
  {
    W_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    WAbstractMemberProperty::m_Flags.Add(WPropertyFlags::IsEnum);

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      WAbstractMemberProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    W_IGNORE_UNUSED(pInstance);

    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  virtual WInt64 GetValue(const void* pInstance) const override // [tested]
  {
    WEnum<EnumType> enumTemp = (static_cast<const Class*>(pInstance)->*m_Getter)();
    return enumTemp.GetValue();
  }

  virtual void SetValue(void* pInstance, WInt64 value) const override // [tested]
  {
    W_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", WAbstractProperty::GetPropertyName());
    if (m_Setter)
      (static_cast<Class*>(pInstance)->*m_Setter)((typename EnumType::Enum)value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};


/// [internal] An implementation of WTypedEnumProperty that accesses the enum property data directly.
template <typename Class, typename EnumType, typename Type>
class WEnumMemberProperty : public WTypedEnumProperty<EnumType>
{
public:
  using GetterFunc = Type (*)(const Class* pInstance);
  using SetterFunc = void (*)(Class* pInstance, Type value);
  using PointerFunc = void* (*)(const Class* pInstance);

  /// Constructor.
  WEnumMemberProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer)
    : WTypedEnumProperty<EnumType>(szPropertyName)
  {
    W_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    WAbstractMemberProperty::m_Flags.Add(WPropertyFlags::IsEnum);

    m_Getter = getter;
    m_Setter = setter;
    m_Pointer = pointer;

    if (m_Setter == nullptr)
      WAbstractMemberProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override { return m_Pointer(static_cast<const Class*>(pInstance)); }

  virtual WInt64 GetValue(const void* pInstance) const override // [tested]
  {
    WEnum<EnumType> enumTemp = m_Getter(static_cast<const Class*>(pInstance));
    return enumTemp.GetValue();
  }

  virtual void SetValue(void* pInstance, WInt64 value) const override // [tested]
  {
    W_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", WAbstractProperty::GetPropertyName());

    if (m_Setter)
      m_Setter(static_cast<Class*>(pInstance), (typename EnumType::Enum)value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
  PointerFunc m_Pointer;
};
