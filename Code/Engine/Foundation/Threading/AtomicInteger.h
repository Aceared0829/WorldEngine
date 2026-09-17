#pragma once

#include <Foundation/Types/TypeTraits.h>

#include <Foundation/Threading/AtomicUtils.h>

template <int T>
struct WAtomicStorageType
{
};

template <>
struct WAtomicStorageType<1>
{
  using Type = WInt32;
};

template <>
struct WAtomicStorageType<2>
{
  using Type = WInt32;
};

template <>
struct WAtomicStorageType<4>
{
  using Type = WInt32;
};

template <>
struct WAtomicStorageType<8>
{
  using Type = WInt64;
};

/// Thread-safe atomic integer with lock-free operations
///
/// Provides atomic (thread-safe) operations on integer types without requiring explicit locking.
/// All operations are lock-free and use hardware atomic instructions where available.
/// Supports common atomic operations like increment, decrement, compare-and-swap, and bitwise operations.
/// The class is templated to work with various integer types while ensuring proper atomic alignment.
/// Use WAtomicInteger32 or WAtomicInteger64 typedefs for common cases.
template <typename T>
class WAtomicInteger
{
  using UnderlyingType = typename WAtomicStorageType<sizeof(T)>::Type;

public:
  W_DECLARE_POD_TYPE();

  /// Initializes the value to zero.
  WAtomicInteger(); // [tested]

  /// Initializes the object with a value
  WAtomicInteger(const T value); // [tested]

  /// Copy-constructor
  WAtomicInteger(const WAtomicInteger<T>& value); // [tested]

  /// Assigns a new integer value to this object
  WAtomicInteger& operator=(T value); // [tested]

  /// Assignment operator
  WAtomicInteger& operator=(const WAtomicInteger& value); // [tested]

  /// Increments the internal value and returns the incremented value
  T Increment(); // [tested]

  /// Decrements the internal value and returns the decremented value
  T Decrement(); // [tested]

  /// Increments the internal value and returns the value immediately before the increment
  T PostIncrement(); // [tested]

  /// Decrements the internal value and returns the value immediately before the decrement
  T PostDecrement();  // [tested]

  void Add(T x);      // [tested]
  void Subtract(T x); // [tested]

  void And(T x);      // [tested]
  void Or(T x);       // [tested]
  void Xor(T x);      // [tested]

  void Min(T x);      // [tested]
  void Max(T x);      // [tested]

  /// Sets the internal value to x and returns the original internal value.
  T Set(T x); // [tested]

  /// Atomic conditional assignment operation
  ///
  /// Sets the value to x only if the current value equals expected. Returns true if the assignment
  /// occurred, false otherwise. This is a fundamental building block for lock-free algorithms.
  bool TestAndSet(T expected, T x); // [tested]

  /// Atomic compare-and-swap operation returning the previous value
  ///
  /// If the current value equals expected, it is atomically replaced with x.
  /// Always returns the value that was present before the operation, regardless of whether
  /// the swap occurred. This enables retry loops in lock-free algorithms.
  T CompareAndSwap(T expected, T x); // [tested]

  operator T() const;                // [tested]

private:
  UnderlyingType m_Value;
};

/// An atomic boolean variable. This is just a wrapper around an atomic int32 for convenience.
class WAtomicBool
{
public:
  /// Initializes the bool to 'false'.
  WAtomicBool(); // [tested]
  ~WAtomicBool();

  /// Initializes the object with a value
  WAtomicBool(bool value); // [tested]

  /// Copy-constructor
  WAtomicBool(const WAtomicBool& rhs);

  /// Sets the bool to the given value and returns its previous value.
  bool Set(bool value); // [tested]

  /// Sets the bool to the given value.
  void operator=(bool value); // [tested]

  /// Sets the bool to the given value.
  void operator=(const WAtomicBool& rhs);

  /// Returns the current value.
  operator bool() const; // [tested]

  /// Atomic conditional assignment for boolean values
  ///
  /// Sets the value to newValue only if the current value equals expected.
  /// Returns true if the assignment occurred, false otherwise.
  bool TestAndSet(bool bExpected, bool bNewValue);

private:
  WAtomicInteger<WInt32> m_iAtomicInt;
};

// Include inline file
#include <Foundation/Threading/Implementation/AtomicInteger_inl.h>

using WAtomicInteger32 = WAtomicInteger<WInt32>; // [tested]
using WAtomicInteger64 = WAtomicInteger<WInt64>; // [tested]
static_assert(sizeof(WAtomicInteger32) == sizeof(WInt32));
static_assert(sizeof(WAtomicInteger64) == sizeof(WInt64));
