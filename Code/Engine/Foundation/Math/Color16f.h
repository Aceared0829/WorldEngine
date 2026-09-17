#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Float16.h>

/// A 16bit per channel float color storage format.
///
/// For any calculations or conversions use WColor.
/// \see WColor
class W_FOUNDATION_DLL WColorLinear16f
{
public:
  // Means that colors can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  // *** Data ***
public:
  WFloat16 r;
  WFloat16 g;
  WFloat16 b;
  WFloat16 a;

  // *** Constructors ***
public:
  /// default-constructed color is uninitialized (for speed)
  WColorLinear16f(); // [tested]

  /// Initializes the color with r, g, b, a
  WColorLinear16f(WFloat16 r, WFloat16 g, WFloat16 b, WFloat16 a); // [tested]

  /// Initializes the color with WColor
  WColorLinear16f(const WColor& color); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  // *** Functions ***
public:
  /// Conversion to WColor.
  WColor ToLinearFloat() const; // [tested]

  /// Conversion to const WFloat16*.
  const WFloat16* GetData() const { return &r; }

  /// Conversion to WFloat16* - use with care!
  WFloat16* GetData() { return &r; }
};

#include <Foundation/Math/Implementation/Color16f_inl.h>
