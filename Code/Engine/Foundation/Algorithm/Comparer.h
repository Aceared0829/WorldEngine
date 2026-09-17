
#pragma once

#include <Foundation/Basics.h>

/// A comparer object is used in sorting algorithms to compare to objects of the same type.
template <typename T>
struct WCompareHelper
{
  /// Returns true if a is less than b
  W_ALWAYS_INLINE bool Less(const T& a, const T& b) const
  {
    return a < b;
  }

  /// Returns true if a is less than b
  template <typename U>
  W_ALWAYS_INLINE bool Less(const T& a, const U& b) const
  {
    return a < b;
  }

  /// Returns true if a is less than b
  template <typename U>
  W_ALWAYS_INLINE bool Less(const U& a, const T& b) const
  {
    return a < b;
  }

  /// Returns true if a is equal to b
  W_ALWAYS_INLINE bool Equal(const T& a, const T& b) const
  {
    return a == b;
  }

  /// Returns true if a is equal to b
  template <typename U>
  W_ALWAYS_INLINE bool Equal(const T& a, const U& b) const
  {
    return a == b;
  }

  /// Returns true if a is equal to b
  template <typename U>
  W_ALWAYS_INLINE bool Equal(const U& a, const T& b) const
  {
    return a == b;
  }
};

// See <Foundation/Strings/String.h> for WString specialization and case insensitive version.
