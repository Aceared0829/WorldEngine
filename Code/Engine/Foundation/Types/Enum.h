#pragma once

#include <Foundation/Basics.h>

/// \file

/// A custom enum implementation that allows to define the underlying storage type to control its memory footprint.
///
/// Advantages over a simple C++ enum:
/// 1) Storage type can be defined
/// 2) Enum is default initialized automatically
/// 3) Definition of the enum itself, the storage type and the default init value is in one place
/// 4) It makes function definitions shorter, instead of:
///      void function(WExampleEnumBase::Enum value)
///    you can write:
///      void function(WExampleEnum value)
/// 5) In all other ways it works exactly like a C++ enum
///
/// Example:
///
/// struct WExampleEnumBase
/// {
///   using StorageType = WUInt8;
///
///   enum Enum
///   {
///     Value1 = 1,          // normal value
///     Value2 = 2,          // normal value
///     Value3 = 3,          // normal value
///     Default = Value1 // Default initialization value (required)
///   };
/// };
/// using WExampleEnum = WEnum<WExampleEnumBase>;
///
/// This defines an "WExampleEnum" which is stored in an WUInt8 and is default initialized with Value1
/// For more examples see the enum test.
template <typename Derived>
struct WEnum : public Derived
{
public:
  using SelfType = WEnum<Derived>;
  using StorageType = typename Derived::StorageType;

  /// Default constructor
  W_ALWAYS_INLINE WEnum()
    : m_Value((StorageType)Derived::Default)
  {
  } // [tested]

  /// Copy constructor
  W_ALWAYS_INLINE WEnum(const SelfType& rh)
    : m_Value(rh.m_Value)
  {
  }

  /// Construct from a C++ enum, and implicit conversion from enum type
  W_ALWAYS_INLINE WEnum(typename Derived::Enum init)
    : m_Value((StorageType)init)
  {
  } // [tested]

  /// Assignment operator
  W_ALWAYS_INLINE void operator=(const SelfType& rh) // [tested]
  {
    m_Value = rh.m_Value;
  }

  /// Assignment operator.
  W_ALWAYS_INLINE void operator=(const typename Derived::Enum value) // [tested]
  {
    m_Value = (StorageType)value;
  }

  /// Comparison operators
  W_ALWAYS_INLINE bool operator==(const SelfType& rhs) const { return m_Value == rhs.m_Value; }
  W_ALWAYS_INLINE bool operator!=(const SelfType& rhs) const { return m_Value != rhs.m_Value; }
  W_ALWAYS_INLINE bool operator>(const SelfType& rhs) const { return m_Value > rhs.m_Value; }
  W_ALWAYS_INLINE bool operator<(const SelfType& rhs) const { return m_Value < rhs.m_Value; }
  W_ALWAYS_INLINE bool operator>=(const SelfType& rhs) const { return m_Value >= rhs.m_Value; }
  W_ALWAYS_INLINE bool operator<=(const SelfType& rhs) const { return m_Value <= rhs.m_Value; }

  W_ALWAYS_INLINE bool operator==(typename Derived::Enum value) const { return m_Value == (StorageType)value; }
  W_ALWAYS_INLINE bool operator!=(typename Derived::Enum value) const { return m_Value != (StorageType)value; }
  W_ALWAYS_INLINE bool operator>(typename Derived::Enum value) const { return m_Value > (StorageType)value; }
  W_ALWAYS_INLINE bool operator<(typename Derived::Enum value) const { return m_Value < (StorageType)value; }
  W_ALWAYS_INLINE bool operator>=(typename Derived::Enum value) const { return m_Value >= (StorageType)value; }
  W_ALWAYS_INLINE bool operator<=(typename Derived::Enum value) const { return m_Value <= (StorageType)value; }

  /// brief Bitwise operators
  W_ALWAYS_INLINE SelfType operator|(const SelfType& rhs) const { return static_cast<typename Derived::Enum>(m_Value | rhs.m_Value); } // [tested]
  W_ALWAYS_INLINE SelfType operator&(const SelfType& rhs) const { return static_cast<typename Derived::Enum>(m_Value & rhs.m_Value); } // [tested]

  /// Implicit conversion to enum type.
  W_ALWAYS_INLINE operator typename Derived::Enum() const // [tested]
  {
    return static_cast<typename Derived::Enum>(m_Value);
  }

  /// Returns the enum value as an integer
  W_ALWAYS_INLINE StorageType GetValue() const // [tested]
  {
    return m_Value;
  }

  /// Sets the enum value through an integer
  W_ALWAYS_INLINE void SetValue(StorageType value) // [tested]
  {
    m_Value = value;
  }

private:
  StorageType m_Value;
};


#define W_ENUM_VALUE_TO_STRING(name) \
  case name:                          \
    return W_PP_STRINGIFY(name);

/// Helper macro to generate a 'ToString' function for enum values.
///
/// Usage: W_ENUM_TO_STRING(Value1, Value2, Value3, Value4)
/// Embed it into a struct (which defines the enums).
/// Example:
/// struct WExampleEnum
/// {
///   enum Enum
///   {
///     A,
///     B,
///     C,
///   };
///
///   W_ENUM_TO_STRING(A, B, C);
/// };
#define W_ENUM_TO_STRING(...)                               \
  const char* ToString(WUInt32 value)                       \
  {                                                          \
    switch (value)                                           \
    {                                                        \
      W_EXPAND_ARGS(W_ENUM_VALUE_TO_STRING, ##__VA_ARGS__) \
      default:                                               \
        return nullptr;                                      \
    }                                                        \
  }
