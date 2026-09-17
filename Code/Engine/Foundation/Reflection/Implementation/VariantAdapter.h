#pragma once

#include <Foundation/Types/Variant.h>

template <typename T>
struct WCleanType2
{
  using Type = T;
  using RttiType = T;
};

template <typename T>
struct WCleanType2<WEnum<T>>
{
  using Type = WEnum<T>;
  using RttiType = T;
};

template <typename T>
struct WCleanType2<WBitflags<T>>
{
  using Type = WBitflags<T>;
  using RttiType = T;
};

template <typename T>
struct WCleanType
{
  using Type = typename WTypeTraits<T>::NonConstReferencePointerType;
  using RttiType = typename WCleanType2<typename WTypeTraits<T>::NonConstReferencePointerType>::RttiType;
};

template <>
struct WCleanType<const char*>
{
  using Type = const char*;
  using RttiType = const char*;
};

//////////////////////////////////////////////////////////////////////////

template <typename T>
struct WIsOutParam
{
  enum
  {
    value = false,
  };
};

template <typename T>
struct WIsOutParam<T&>
{
  enum
  {
    value = !std::is_const<typename WTypeTraits<T>::NonReferencePointerType>::value,
  };
};

template <typename T>
struct WIsOutParam<T*>
{
  enum
  {
    value = !std::is_const<typename WTypeTraits<T>::NonReferencePointerType>::value,
  };
};

//////////////////////////////////////////////////////////////////////////

/// Used to determine if the given type is a build-in standard variant type.
template <class T, class C = typename WCleanType<T>::Type>
struct WIsStandardType
{
  enum
  {
    value = WVariant::TypeDeduction<C>::value >= WVariantType::FirstStandardType && WVariant::TypeDeduction<C>::value <= WVariantType::LastStandardType,
  };
};

template <class T>
struct WIsStandardType<T, WVariant>
{
  enum
  {
    value = true,
  };
};

//////////////////////////////////////////////////////////////////////////

/// Used to determine if the given type can be stored by value inside an WVariant (either standard type or custom type).
template <class T, class C = typename WCleanType<T>::Type>
struct WIsValueType
{
  enum
  {
    value = (WVariant::TypeDeduction<C>::value >= WVariantType::FirstStandardType && WVariant::TypeDeduction<C>::value <= WVariantType::LastStandardType) || WVariantTypeDeduction<C>::classification == WVariantClass::CustomTypeCast,
  };
};

template <class T>
struct WIsValueType<T, WVariant>
{
  enum
  {
    value = true,
  };
};

//////////////////////////////////////////////////////////////////////////
/// Used to automatically assign any value to an WVariant using the assignment rules
/// outlined in WAbstractFunctionProperty::Execute.
template <class T,                          ///< Only this parameter needs to be provided, the actual type of the value.
  class C = typename WCleanType<T>::Type,  ///< Same as T but without the const&* fluff.
  int VALUE_TYPE = WIsValueType<T>::value> ///< Is 1 if T is a WTypeFlags::StandardType or a custom type
struct WVariantAssignmentAdapter
{
  using RealType = typename WTypeTraits<T>::NonConstReferencePointerType;
  WVariantAssignmentAdapter(WVariant& value)
    : m_value(value)
  {
  }

  void operator=(RealType* rhs) { m_value = rhs; }
  void operator=(RealType&& rhs)
  {
    if (m_value.IsValid())
      *m_value.Get<RealType*>() = rhs;
  }
  WVariant& m_value;
};

template <class T, class S>
struct WVariantAssignmentAdapter<T, WEnum<S>, 0>
{
  using RealType = typename WTypeTraits<T>::NonConstReferencePointerType;
  WVariantAssignmentAdapter(WVariant& value)
    : m_value(value)
  {
  }

  void operator=(WEnum<S>&& rhs) { m_value = static_cast<WInt64>(rhs.GetValue()); }

  WVariant& m_value;
};

template <class T, class S>
struct WVariantAssignmentAdapter<T, WBitflags<S>, 0>
{
  using RealType = typename WTypeTraits<T>::NonConstReferencePointerType;
  WVariantAssignmentAdapter(WVariant& value)
    : m_value(value)
  {
  }

  void operator=(WBitflags<S>&& rhs) { m_value = static_cast<WInt64>(rhs.GetValue()); }

  WVariant& m_value;
};

template <class T, class C>
struct WVariantAssignmentAdapter<T, C, 1>
{
  using RealType = typename WTypeTraits<T>::NonConstReferencePointerType;
  WVariantAssignmentAdapter(WVariant& value)
    : m_value(value)
  {
  }

  void operator=(T&& rhs) { m_value = rhs; }

  WVariant& m_value;
};

template <class T>
struct WVariantAssignmentAdapter<T, WVariantArray, 0>
{
  WVariantAssignmentAdapter(WVariant& value)
    : m_value(value)
  {
  }

  void operator=(T&& rhs) { m_value = rhs; }

  WVariant& m_value;
};

template <class T>
struct WVariantAssignmentAdapter<T, WVariantDictionary, 0>
{
  WVariantAssignmentAdapter(WVariant& value)
    : m_value(value)
  {
  }

  void operator=(T&& rhs) { m_value = rhs; }

  WVariant& m_value;
};

//////////////////////////////////////////////////////////////////////////

/// Used to implicitly retrieve any value from an WVariant to be used as a function argument
/// using the assignment rules outlined in WAbstractFunctionProperty::Execute.
template <class T,                          ///< Only this parameter needs to be provided, the actual type of the argument. Rest is used to force specializations.
  class C = typename WCleanType<T>::Type,  ///< Same as T but without the const&* fluff.
  int VALUE_TYPE = WIsValueType<T>::value, ///< Is 1 if T is a WTypeFlags::StandardType or a custom type
  int OUT_PARAM = WIsOutParam<T>::value>   ///< Is 1 if T a non-const reference or pointer.
struct WVariantAdapter
{
  using RealType = typename WTypeTraits<T>::NonConstReferencePointerType;

  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
  }

  operator RealType&() { return *m_value.Get<RealType*>(); }

  operator RealType*() { return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr; }

  WVariant& m_value;
};

template <class T, class S>
struct WVariantAdapter<T, WEnum<S>, 0, 0>
{
  using RealType = typename WTypeTraits<T>::NonConstReferencePointerType;
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue = static_cast<typename S::Enum>(m_value.ConvertTo<WInt64>());
  }

  operator const WEnum<S>&() { return m_realValue; }
  operator const WEnum<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  WVariant& m_value;
  WEnum<S> m_realValue;
};

template <class T, class S>
struct WVariantAdapter<T, WEnum<S>, 0, 1>
{
  using RealType = typename WTypeTraits<T>::NonConstReferencePointerType;
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue = static_cast<typename S::Enum>(m_value.ConvertTo<WInt64>());
  }
  ~WVariantAdapter()
  {
    if (m_value.IsValid())
      m_value = static_cast<WInt64>(m_realValue.GetValue());
  }

  operator WEnum<S>&() { return m_realValue; }
  operator WEnum<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  WVariant& m_value;
  WEnum<S> m_realValue;
};

template <class T, class S>
struct WVariantAdapter<T, WBitflags<S>, 0, 0>
{
  using RealType = typename WTypeTraits<T>::NonConstReferencePointerType;
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue.SetValue(static_cast<typename S::StorageType>(m_value.ConvertTo<WInt64>()));
  }

  operator const WBitflags<S>&() { return m_realValue; }
  operator const WBitflags<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  WVariant& m_value;
  WBitflags<S> m_realValue;
};

template <class T, class S>
struct WVariantAdapter<T, WBitflags<S>, 0, 1>
{
  using RealType = typename WTypeTraits<T>::NonConstReferencePointerType;
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue.SetValue(static_cast<typename S::StorageType>(m_value.ConvertTo<WInt64>()));
  }
  ~WVariantAdapter()
  {
    if (m_value.IsValid())
      m_value = static_cast<WInt64>(m_realValue.GetValue());
  }

  operator WBitflags<S>&() { return m_realValue; }
  operator WBitflags<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  WVariant& m_value;
  WBitflags<S> m_realValue;
};

template <class T, class C>
struct WVariantAdapter<T, C, 1, 0>
{
  using RealType = typename WTypeTraits<T>::NonConstReferencePointerType;
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
  }

  operator const C&()
  {
    if constexpr (WVariantTypeDeduction<C>::classification == WVariantClass::CustomTypeCast)
    {
      if (m_value.GetType() == WVariantType::TypedPointer)
        return *m_value.Get<RealType*>();
    }
    return m_value.Get<RealType>();
  }

  operator const C*()
  {
    if constexpr (WVariantTypeDeduction<C>::classification == WVariantClass::CustomTypeCast)
    {
      if (m_value.GetType() == WVariantType::TypedPointer)
        return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr;
    }
    return m_value.IsValid() ? &m_value.Get<RealType>() : nullptr;
  }

  WVariant& m_value;
};

template <class T, class C>
struct WVariantAdapter<T, C, 1, 1>
{
  using RealType = typename WTypeTraits<T>::NonConstReferencePointerType;
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
    // We ignore the return value here instead const_cast the Get<> result to profit from the Get methods runtime type checks.
    m_value.GetWriteAccess();
  }

  operator C&()
  {
    if (m_value.GetType() == WVariantType::TypedPointer)
      return *m_value.Get<RealType*>();
    else
      return const_cast<RealType&>(m_value.Get<RealType>());
  }
  operator C*()
  {
    if (m_value.GetType() == WVariantType::TypedPointer)
      return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr;
    else
      return m_value.IsValid() ? &const_cast<RealType&>(m_value.Get<RealType>()) : nullptr;
  }

  WVariant& m_value;
};

template <class T>
struct WVariantAdapter<T, WVariant, 1, 0>
{
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
  }

  operator const WVariant&() { return m_value; }
  operator const WVariant*() { return &m_value; }

  WVariant& m_value;
};

template <class T>
struct WVariantAdapter<T, WVariant, 1, 1>
{
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
  }

  operator WVariant&() { return m_value; }
  operator WVariant*() { return &m_value; }

  WVariant& m_value;
};

template <class T>
struct WVariantAdapter<T, WVariantArray, 0, 0>
{
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
  }

  operator const WVariantArray&() { return m_value.Get<WVariantArray>(); }
  operator const WVariantArray*() { return m_value.IsValid() ? &m_value.Get<WVariantArray>() : nullptr; }

  WVariant& m_value;
};

template <class T>
struct WVariantAdapter<T, WVariantArray, 0, 1>
{
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
  }

  operator WVariantArray&() { return m_value.GetWritable<WVariantArray>(); }
  operator WVariantArray*() { return m_value.IsValid() ? &m_value.GetWritable<WVariantArray>() : nullptr; }

  WVariant& m_value;
};

template <class T>
struct WVariantAdapter<T, WVariantDictionary, 0, 0>
{
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
  }

  operator const WVariantDictionary&() { return m_value.Get<WVariantDictionary>(); }
  operator const WVariantDictionary*() { return m_value.IsValid() ? &m_value.Get<WVariantDictionary>() : nullptr; }

  WVariant& m_value;
};

template <class T>
struct WVariantAdapter<T, WVariantDictionary, 0, 1>
{
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
  }

  operator WVariantDictionary&() { return m_value.GetWritable<WVariantDictionary>(); }
  operator WVariantDictionary*() { return m_value.IsValid() ? &m_value.GetWritable<WVariantDictionary>() : nullptr; }

  WVariant& m_value;
};

template <>
struct WVariantAdapter<const char*, const char*, 1, 0>
{
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
  }

  operator const char*() { return m_value.IsValid() ? m_value.Get<WString>().GetData() : nullptr; }

  WVariant& m_value;
};

template <class T>
struct WVariantAdapter<T, WStringView, 1, 0>
{
  WVariantAdapter(WVariant& value)
    : m_value(value)
  {
  }

  operator const WStringView() { return m_value.IsA<WStringView>() ? m_value.Get<WStringView>() : m_value.Get<WString>().GetView(); }

  WVariant& m_value;
};
