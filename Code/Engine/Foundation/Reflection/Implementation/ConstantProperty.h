#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// The base class for all typed member properties. Ie. once the type of a property is determined, it can be cast to the proper
/// version of this.
///
/// For example, when you have a pointer to an WAbstractMemberProperty and it returns that the property is of type 'int', you can cast the
/// pointer to an pointer to WTypedMemberProperty<int> which then allows you to access its values.
template <typename Type>
class WTypedConstantProperty : public WAbstractConstantProperty
{
public:
  /// Passes the property name through to WAbstractMemberProperty.
  WTypedConstantProperty(const char* szPropertyName)
    : WAbstractConstantProperty(szPropertyName)
  {
    m_Flags = WPropertyFlags::GetParameterFlags<Type>();
  }

  /// Returns the actual type of the property. You can then compare that with known types, eg. compare it to WGetStaticRTTI<int>()
  /// to see whether this is an int property.
  virtual const WRTTI* GetSpecificType() const override // [tested]
  {
    return WGetStaticRTTI<typename WTypeTraits<Type>::NonConstReferenceType>();
  }

  /// Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue() const = 0;
};

/// [internal] An implementation of WTypedConstantProperty that accesses the property data directly.
template <typename Type>
class WConstantProperty : public WTypedConstantProperty<Type>
{
public:
  /// Constructor.
  WConstantProperty(const char* szPropertyName, Type value)
    : WTypedConstantProperty<Type>(szPropertyName)
    , m_Value(value)
  {
    W_ASSERT_DEBUG(this->m_Flags.IsSet(WPropertyFlags::StandardType), "Only constants that can be put in an WVariant are currently supported!");
  }

  /// Returns a pointer to the member property.
  virtual void* GetPropertyPointer() const override { return (void*)&m_Value; }

  /// Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue() const override // [tested]
  {
    return m_Value;
  }

  virtual WVariant GetConstant() const override { return WVariant(m_Value); }

private:
  Type m_Value;
};
