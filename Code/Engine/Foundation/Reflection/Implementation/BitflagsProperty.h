#pragma once

/// \file

#include <Foundation/Reflection/Implementation/EnumProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// [internal] An implementation of WTypedEnumProperty that uses custom getter / setter functions to access a bitflags property.
template <typename Class, typename EnumType, typename Type>
class WBitflagsAccessorProperty : public WTypedEnumProperty<EnumType>
{
public:
  using RealType = typename WTypeTraits<Type>::NonConstReferenceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  /// Constructor.
  WBitflagsAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter)
    : WTypedEnumProperty<EnumType>(szPropertyName)
  {
    W_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    WAbstractMemberProperty::m_Flags.Add(WPropertyFlags::Bitflags);

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      WAbstractMemberProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  virtual WInt64 GetValue(const void* pInstance) const override // [tested]
  {
    typename EnumType::StorageType enumTemp = (static_cast<const Class*>(pInstance)->*m_Getter)().GetValue();
    return (WInt64)enumTemp;
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


/// [internal] An implementation of WTypedEnumProperty that accesses the bitflags property data directly.
template <typename Class, typename EnumType, typename Type>
class WBitflagsMemberProperty : public WTypedEnumProperty<EnumType>
{
public:
  using GetterFunc = Type (*)(const Class* pInstance);
  using SetterFunc = void (*)(Class* pInstance, Type value);
  using PointerFunc = void* (*)(const Class* pInstance);

  /// Constructor.
  WBitflagsMemberProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer)
    : WTypedEnumProperty<EnumType>(szPropertyName)
  {
    W_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    WAbstractMemberProperty::m_Flags.Add(WPropertyFlags::Bitflags);

    m_Getter = getter;
    m_Setter = setter;
    m_Pointer = pointer;

    if (m_Setter == nullptr)
      WAbstractMemberProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override { return m_Pointer(static_cast<const Class*>(pInstance)); }

  virtual WInt64 GetValue(const void* pInstance) const override // [tested]
  {
    typename EnumType::StorageType enumTemp = m_Getter(static_cast<const Class*>(pInstance)).GetValue();
    return (WInt64)enumTemp;
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
