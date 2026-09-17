#pragma once

#include <Foundation/Basics.h>

/// Low-level platform-independent atomic operations for thread-safe programming
///
/// Provides atomic (indivisible) operations that are faster than mutexes for simple operations
/// but slower than regular operations. Use only when thread safety is required.
///
/// Important considerations:
/// - Individual operations are atomic, but sequences of operations are not
/// - Only use in code that requires thread safety - atomic ops have performance overhead
/// - For higher-level usage, prefer WAtomicInteger which wraps these utilities
/// - All operations use lock-free hardware instructions where available
///
/// These functions form the foundation for lock-free data structures and algorithms.
struct W_FOUNDATION_DLL WAtomicUtils
{
  /// Atomically reads a 32-bit integer value
  ///
  /// Ensures the read operation is atomic and not subject to partial reads on all platforms.
  static WInt32 Read(const WInt32& iSrc); // [tested]

  /// Atomically reads a 64-bit integer value
  ///
  /// Ensures the read operation is atomic and not subject to partial reads on all platforms.
  static WInt64 Read(const WInt64& iSrc); // [tested]

  /// Increments dest as an atomic operation and returns the new value.
  static WInt32 Increment(WInt32& ref_iDest); // [tested]

  /// Increments dest as an atomic operation and returns the new value.
  static WInt64 Increment(WInt64& ref_iDest); // [tested]

  /// Decrements dest as an atomic operation and returns the new value.
  static WInt32 Decrement(WInt32& ref_iDest); // [tested]

  /// Decrements dest as an atomic operation and returns the new value.
  static WInt64 Decrement(WInt64& ref_iDest); // [tested]

  /// Increments dest as an atomic operation and returns the old value.
  static WInt32 PostIncrement(WInt32& ref_iDest); // [tested]

  /// Increments dest as an atomic operation and returns the old value.
  static WInt64 PostIncrement(WInt64& ref_iDest); // [tested]

  /// Decrements dest as an atomic operation and returns the old value.
  static WInt32 PostDecrement(WInt32& ref_iDest); // [tested]

  /// Decrements dest as an atomic operation and returns the old value.
  static WInt64 PostDecrement(WInt64& ref_iDest); // [tested]

  /// Adds value to dest as an atomic operation.
  static void Add(WInt32& ref_iDest, WInt32 value); // [tested]

  /// Adds value to dest as an atomic operation.
  static void Add(WInt64& ref_iDest, WInt64 value); // [tested]

  /// Performs an atomic bitwise AND on dest using value.
  static void And(WInt32& ref_iDest, WInt32 value); // [tested]

  /// Performs an atomic bitwise AND on dest using value.
  static void And(WInt64& ref_iDest, WInt64 value); // [tested]

  /// Performs an atomic bitwise OR on dest using value.
  static void Or(WInt32& ref_iDest, WInt32 value); // [tested]

  /// Performs an atomic bitwise OR on dest using value.
  static void Or(WInt64& ref_iDest, WInt64 value); // [tested]

  /// Performs an atomic bitwise XOR on dest using value.
  static void Xor(WInt32& ref_iDest, WInt32 value); // [tested]

  /// Performs an atomic bitwise XOR on dest using value.
  static void Xor(WInt64& ref_iDest, WInt64 value); // [tested]

  /// Performs an atomic min operation on dest using value.
  static void Min(WInt32& ref_iDest, WInt32 value); // [tested]

  /// Performs an atomic min operation on dest using value.
  static void Min(WInt64& ref_iDest, WInt64 value); // [tested]

  /// Performs an atomic max operation on dest using value.
  static void Max(WInt32& ref_iDest, WInt32 value); // [tested]

  /// Performs an atomic max operation on dest using value.
  static void Max(WInt64& ref_iDest, WInt64 value); // [tested]

  /// Sets dest to value as an atomic operation and returns the original value of dest.
  static WInt32 Set(WInt32& ref_iDest, WInt32 value); // [tested]

  /// Sets dest to value as an atomic operation and returns the original value of dest.
  static WInt64 Set(WInt64& ref_iDest, WInt64 value); // [tested]

  /// If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(WInt32& ref_iDest, WInt32 iExpected, WInt32 value); // [tested]

  /// If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(WInt64& ref_iDest, WInt64 iExpected, WInt64 value); // [tested]

  /// If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(void** pDest, void* pExpected, void* value); // [tested]

  /// If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static WInt32 CompareAndSwap(WInt32& ref_iDest, WInt32 iExpected, WInt32 value); // [tested]

  /// If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static WInt64 CompareAndSwap(WInt64& ref_iDest, WInt64 iExpected, WInt64 value); // [tested]
};

// include platforma specific implementation
#include <AtomicUtils_Platform.h>
