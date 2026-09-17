#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Math.h>

/// A 8bit per channel color storage format with undefined encoding. It is up to the user to reinterpret as a gamma or linear space
/// color.
///
/// \see WColorLinearUB
/// \see WColorGammaUB
class W_FOUNDATION_DLL WColorBaseUB
{
public:
  W_DECLARE_POD_TYPE();

  WUInt8 r;
  WUInt8 g;
  WUInt8 b;
  WUInt8 a;

  /// Default-constructed color is uninitialized (for speed)
  WColorBaseUB() = default;

  /// Initializes the color with r, g, b, a
  WColorBaseUB(WUInt8 r, WUInt8 g, WUInt8 b, WUInt8 a = 255);

  /// Conversion to const WUInt8*.
  const WUInt8* GetData() const { return &r; }

  /// Conversion to WUInt8*
  WUInt8* GetData() { return &r; }

  /// Packs the 4 color values into a single uint32 with A in the least significant bits and R in the most significant ones.
  [[nodiscard]] WUInt32 ToRGBA8() const
  {
    // RGBA (A at lowest address, R at highest)
    return (static_cast<WUInt32>(r) << 24) +
           (static_cast<WUInt32>(g) << 16) +
           (static_cast<WUInt32>(b) << 8) +
           (static_cast<WUInt32>(a) << 0);
  }

  /// Packs the 4 color values into a single uint32 with R in the least significant bits and A in the most significant ones.
  [[nodiscard]] WUInt32 ToABGR8() const
  {
    // RGBA (A at highest address, R at lowest)
    return (static_cast<WUInt32>(a) << 24) +
           (static_cast<WUInt32>(b) << 16) +
           (static_cast<WUInt32>(g) << 8) +
           (static_cast<WUInt32>(r) << 0);
  }
};

static_assert(sizeof(WColorBaseUB) == 4);

/// A 8bit per channel unsigned normalized (values interpreted as 0-1) color storage format that represents colors in linear space.
///
/// For any calculations or conversions use WColor.
/// \see WColor
class W_FOUNDATION_DLL WColorLinearUB : public WColorBaseUB
{
public:
  W_DECLARE_POD_TYPE();

  /// Default-constructed color is uninitialized (for speed)
  WColorLinearUB() = default; // [tested]

  /// Initializes the color with r, g, b, a
  WColorLinearUB(WUInt8 r, WUInt8 g, WUInt8 b, WUInt8 a = 255); // [tested]

  /// Initializes the color with WColor.
  /// Assumes that the given color is normalized.
  /// \see WColor::IsNormalized
  WColorLinearUB(const WColor& color); // [tested]

  /// Initializes the color with WColor.
  void operator=(const WColor& color); // [tested]

  /// Converts this color to WColor.
  WColor ToLinearFloat() const; // [tested]

  /// Extracts the values from a uint32 with R at the least significant bits, then G, then B and A at the most significant bits.
  static WColorLinearUB MakeFromABGR8(WUInt32 value)
  {
    return WColorLinearUB(static_cast<WUInt8>(value >> 0) & 0xFF,
      static_cast<WUInt8>(value >> 8) & 0xFF,
      static_cast<WUInt8>(value >> 16) & 0xFF,
      static_cast<WUInt8>(value >> 24) & 0xFF);
  }
};

static_assert(sizeof(WColorLinearUB) == 4);

/// A 8bit per channel unsigned normalized (values interpreted as 0-1) color storage format that represents colors in gamma space.
///
/// For any calculations or conversions use WColor.
/// \see WColor
class W_FOUNDATION_DLL WColorGammaUB : public WColorBaseUB
{
public:
  W_DECLARE_POD_TYPE();

  /// Default-constructed color is uninitialized (for speed)
  WColorGammaUB() = default;

  /// Copies the color values. RGB are assumed to be in Gamma space.
  WColorGammaUB(WUInt8 uiGammaRed, WUInt8 uiGammaGreen, WUInt8 uiGammaBlue, WUInt8 uiLinearAlpha = 255); // [tested]

  /// Initializes the color with WColor. Converts the linear space color to gamma space.
  /// Assumes that the given color is normalized.
  /// \see WColor::IsNormalized
  WColorGammaUB(const WColor& color); // [tested]

  /// Initializes the color with WColor. Converts the linear space color to gamma space.
  void operator=(const WColor& color); // [tested]

  /// Converts this color to WColor.
  WColor ToLinearFloat() const;
};

static_assert(sizeof(WColorGammaUB) == 4);


#include <Foundation/Math/Implementation/Color8UNorm_inl.h>
