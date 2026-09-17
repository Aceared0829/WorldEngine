#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec4.h>

/// WColor represents an RGBA color in linear color space. Values are stored as float, allowing HDR values and full precision color
/// modifications.
///
/// WColor is the central class to handle colors throughout the engine. With floating point precision it can handle any value, including HDR colors.
/// Since it is stored in linear space, doing color transformations (e.g. adding colors or multiplying them) work as expected.
///
/// When you need to pass colors to the GPU you have multiple options.
///   * If you can spare the bandwidth, you should prefer to use floating point formats, e.g. the same as WColor on the CPU.
///   * If you need higher precision and HDR values, you can use WColorLinear16f as a storage format with only half the memory footprint.
///   * If you need to preserve memory and LDR values are sufficient, you should use WColorGammaUB. This format uses 8 Bit per pixel
///     but stores colors in Gamma space, resulting in higher precision in the range that the human eye can distinguish better.
///     However, when you store a color in Gamma space, you need to make sure to convert it back to linear space before doing ANY computations
///     with it. E.g. your shader needs to convert the color.
///   * You can also use 8 Bit per pixel with a linear color space by using WColorLinearUB, however this may give very noticeable precision loss.
///
/// When working with color in your code, be aware to always use the correct class to handle color conversions properly.
/// E.g. when you hardcode a color in source code, you might go to a Paint program, pick a nice color and then type that value into the
/// source code. Note that ALL colors that you see on screen are implicitly in sRGB / Gamma space. That means you should do the following cast:\n
///
/// \code
///   WColor linear = WColorGammaUB(100, 149, 237);
/// \endcode
///
/// This will automatically convert the color from Gamma to linear space. From there on all mathematical operations are possible.
///
/// The inverse has to be done when you want to present the value of a color in a UI:
///
/// \code
///   WColorGammaUB gamma = WColor(0.39f, 0.58f, 0.93f);
/// \endcode
///
/// Now the integer values in \a gamma can be used to e.g. populate a color picker and the color displayed on screen will show up the same, as
/// in a gamma correct 3D rendering.
///
///
///
/// The predefined colors can be seen at http://www.w3schools.com/colors/colors_names.asp
class W_FOUNDATION_DLL WColor
{
public:
  W_DECLARE_POD_TYPE();

  // *** Predefined Colors ***
public:
  static const WColor AliceBlue;            ///< #F0F8FF
  static const WColor AntiqueWhite;         ///< #FAEBD7
  static const WColor Aqua;                 ///< #00FFFF
  static const WColor Aquamarine;           ///< #7FFFD4
  static const WColor Azure;                ///< #F0FFFF
  static const WColor Beige;                ///< #F5F5DC
  static const WColor Bisque;               ///< #FFE4C4
  static const WColor Black;                ///< #000000
  static const WColor BlanchedAlmond;       ///< #FFEBCD
  static const WColor Blue;                 ///< #0000FF
  static const WColor BlueViolet;           ///< #8A2BE2
  static const WColor Brown;                ///< #A52A2A
  static const WColor BurlyWood;            ///< #DEB887
  static const WColor CadetBlue;            ///< #5F9EA0
  static const WColor Chartreuse;           ///< #7FFF00
  static const WColor Chocolate;            ///< #D2691E
  static const WColor Coral;                ///< #FF7F50
  static const WColor CornflowerBlue;       ///< #6495ED  The original!
  static const WColor Cornsilk;             ///< #FFF8DC
  static const WColor Crimson;              ///< #DC143C
  static const WColor Cyan;                 ///< #00FFFF
  static const WColor DarkBlue;             ///< #00008B
  static const WColor DarkCyan;             ///< #008B8B
  static const WColor DarkGoldenRod;        ///< #B8860B
  static const WColor DarkGray;             ///< #A9A9A9
  static const WColor DarkGrey;             ///< #A9A9A9
  static const WColor DarkGreen;            ///< #006400
  static const WColor DarkKhaki;            ///< #BDB76B
  static const WColor DarkMagenta;          ///< #8B008B
  static const WColor DarkOliveGreen;       ///< #556B2F
  static const WColor DarkOrange;           ///< #FF8C00
  static const WColor DarkOrchid;           ///< #9932CC
  static const WColor DarkRed;              ///< #8B0000
  static const WColor DarkSalmon;           ///< #E9967A
  static const WColor DarkSeaGreen;         ///< #8FBC8F
  static const WColor DarkSlateBlue;        ///< #483D8B
  static const WColor DarkSlateGray;        ///< #2F4F4F
  static const WColor DarkSlateGrey;        ///< #2F4F4F
  static const WColor DarkTurquoise;        ///< #00CED1
  static const WColor DarkViolet;           ///< #9400D3
  static const WColor DeepPink;             ///< #FF1493
  static const WColor DeepSkyBlue;          ///< #00BFFF
  static const WColor DimGray;              ///< #696969
  static const WColor DimGrey;              ///< #696969
  static const WColor DodgerBlue;           ///< #1E90FF
  static const WColor FireBrick;            ///< #B22222
  static const WColor FloralWhite;          ///< #FFFAF0
  static const WColor ForestGreen;          ///< #228B22
  static const WColor Fuchsia;              ///< #FF00FF
  static const WColor Gainsboro;            ///< #DCDCDC
  static const WColor GhostWhite;           ///< #F8F8FF
  static const WColor Gold;                 ///< #FFD700
  static const WColor GoldenRod;            ///< #DAA520
  static const WColor Gray;                 ///< #808080
  static const WColor Grey;                 ///< #808080
  static const WColor Green;                ///< #008000
  static const WColor GreenYellow;          ///< #ADFF2F
  static const WColor HoneyDew;             ///< #F0FFF0
  static const WColor HotPink;              ///< #FF69B4
  static const WColor IndianRed;            ///< #CD5C5C
  static const WColor Indigo;               ///< #4B0082
  static const WColor Ivory;                ///< #FFFFF0
  static const WColor Khaki;                ///< #F0E68C
  static const WColor Lavender;             ///< #E6E6FA
  static const WColor LavenderBlush;        ///< #FFF0F5
  static const WColor LawnGreen;            ///< #7CFC00
  static const WColor LemonChiffon;         ///< #FFFACD
  static const WColor LightBlue;            ///< #ADD8E6
  static const WColor LightCoral;           ///< #F08080
  static const WColor LightCyan;            ///< #E0FFFF
  static const WColor LightGoldenRodYellow; ///< #FAFAD2
  static const WColor LightGray;            ///< #D3D3D3
  static const WColor LightGrey;            ///< #D3D3D3
  static const WColor LightGreen;           ///< #90EE90
  static const WColor LightPink;            ///< #FFB6C1
  static const WColor LightSalmon;          ///< #FFA07A
  static const WColor LightSeaGreen;        ///< #20B2AA
  static const WColor LightSkyBlue;         ///< #87CEFA
  static const WColor LightSlateGray;       ///< #778899
  static const WColor LightSlateGrey;       ///< #778899
  static const WColor LightSteelBlue;       ///< #B0C4DE
  static const WColor LightYellow;          ///< #FFFFE0
  static const WColor Lime;                 ///< #00FF00
  static const WColor LimeGreen;            ///< #32CD32
  static const WColor Linen;                ///< #FAF0E6
  static const WColor Magenta;              ///< #FF00FF
  static const WColor Maroon;               ///< #800000
  static const WColor MediumAquaMarine;     ///< #66CDAA
  static const WColor MediumBlue;           ///< #0000CD
  static const WColor MediumOrchid;         ///< #BA55D3
  static const WColor MediumPurple;         ///< #9370DB
  static const WColor MediumSeaGreen;       ///< #3CB371
  static const WColor MediumSlateBlue;      ///< #7B68EE
  static const WColor MediumSpringGreen;    ///< #00FA9A
  static const WColor MediumTurquoise;      ///< #48D1CC
  static const WColor MediumVioletRed;      ///< #C71585
  static const WColor MidnightBlue;         ///< #191970
  static const WColor MintCream;            ///< #F5FFFA
  static const WColor MistyRose;            ///< #FFE4E1
  static const WColor Moccasin;             ///< #FFE4B5
  static const WColor NavajoWhite;          ///< #FFDEAD
  static const WColor Navy;                 ///< #000080
  static const WColor OldLace;              ///< #FDF5E6
  static const WColor Olive;                ///< #808000
  static const WColor OliveDrab;            ///< #6B8E23
  static const WColor Orange;               ///< #FFA500
  static const WColor OrangeRed;            ///< #FF4500
  static const WColor Orchid;               ///< #DA70D6
  static const WColor PaleGoldenRod;        ///< #EEE8AA
  static const WColor PaleGreen;            ///< #98FB98
  static const WColor PaleTurquoise;        ///< #AFEEEE
  static const WColor PaleVioletRed;        ///< #DB7093
  static const WColor PapayaWhip;           ///< #FFEFD5
  static const WColor PeachPuff;            ///< #FFDAB9
  static const WColor Peru;                 ///< #CD853F
  static const WColor Pink;                 ///< #FFC0CB
  static const WColor Plum;                 ///< #DDA0DD
  static const WColor PowderBlue;           ///< #B0E0E6
  static const WColor Purple;               ///< #800080
  static const WColor RebeccaPurple;        ///< #663399
  static const WColor Red;                  ///< #FF0000
  static const WColor RosyBrown;            ///< #BC8F8F
  static const WColor RoyalBlue;            ///< #4169E1
  static const WColor SaddleBrown;          ///< #8B4513
  static const WColor Salmon;               ///< #FA8072
  static const WColor SandyBrown;           ///< #F4A460
  static const WColor SeaGreen;             ///< #2E8B57
  static const WColor SeaShell;             ///< #FFF5EE
  static const WColor Sienna;               ///< #A0522D
  static const WColor Silver;               ///< #C0C0C0
  static const WColor SkyBlue;              ///< #87CEEB
  static const WColor SlateBlue;            ///< #6A5ACD
  static const WColor SlateGray;            ///< #708090
  static const WColor SlateGrey;            ///< #708090
  static const WColor Snow;                 ///< #FFFAFA
  static const WColor SpringGreen;          ///< #00FF7F
  static const WColor SteelBlue;            ///< #4682B4
  static const WColor Tan;                  ///< #D2B48C
  static const WColor Teal;                 ///< #008080
  static const WColor Thistle;              ///< #D8BFD8
  static const WColor Tomato;               ///< #FF6347
  static const WColor Turquoise;            ///< #40E0D0
  static const WColor Violet;               ///< #EE82EE
  static const WColor Wheat;                ///< #F5DEB3
  static const WColor White;                ///< #FFFFFF
  static const WColor WhiteSmoke;           ///< #F5F5F5
  static const WColor Yellow;               ///< #FFFF00
  static const WColor YellowGreen;          ///< #9ACD32

  // *** Data ***
public:
  float r;
  float g;
  float b;
  float a;

  // *** Static Functions ***
public:
  /// Returns a color with all four RGBA components set to Not-A-Number (NaN).
  [[nodiscard]] static WColor MakeNaN(); // [tested]

  /// Returns a color with all four RGBA components set to zero. This is different to WColor::Black, which has alpha still set to 1.0.
  [[nodiscard]] static WColor MakeZero(); // [tested]

  /// Returns a color with the given r, g, b, a values. The values must be given in a linear color space.
  [[nodiscard]] static WColor MakeRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f); // [tested]

  // *** Constructors ***
public:
  /// default-constructed color is uninitialized (for speed)
  WColor(); // [tested]

  /// Initializes the color with r, g, b, a. The color values must be given in a linear color space.
  ///
  /// To initialize the color from a Gamma color space, e.g. when using a color value that was determined with a color picker,
  /// use the constructor that takes a WColorGammaUB object for initialization.
  constexpr WColor(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f); // [tested]

  /// Initializes this color from a WColorLinearUB object.
  ///
  /// Prefer to either use linear colors with floating point precision, or to use WColorGammaUB for 8 bit per pixel colors in gamma space.
  WColor(const WColorLinearUB& cc); // [tested]

  /// Initializes this color from a WColorGammaUB object.
  ///
  /// This should be the preferred method when hard-coding colors in source code.
  WColor(const WColorGammaUB& cc); // [tested]

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    W_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// Sets the RGB components, ignores alpha.
  void SetRGB(float fLinearRed, float fLinearGreen, float fLinearBlue); // [tested]

  /// Sets all four RGBA components.
  void SetRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f); // [tested]

  // *** Conversion Operators/Functions ***
public:
  /// Returns a color created from the kelvin temperature. https://wikipedia.org/wiki/Color_temperature
  /// Originally inspired from https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html
  /// But with heavy modification to better fit the mapping shown out in https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
  /// Physically accurate clipping points are 6580K for Red and 6560K for G and B. but approximated to 6570k for all to give a better mapping.
  [[nodiscard]] static WColor MakeFromKelvin(WUInt32 uiKelvin); // [tested]

  /// Sets this color from a HSV (hue, saturation, value) format.
  ///
  /// \a hue is in range [0; 360], \a sat and \a val are in range [0; 1]
  [[nodiscard]] static WColor MakeHSV(float fHue, float fSat, float fVal); // [tested]

  /// Converts the color part to HSV format.
  ///
  /// \a hue is in range [0; 360], \a sat and \a val are in range [0; 1]
  void GetHSV(float& out_fHue, float& out_fSat, float& out_fValue) const; // [tested]

  /// Conversion to const float*
  const float* GetData() const { return &r; } // [tested]

  /// Conversion to float*
  float* GetData() { return &r; } // [tested]

  /// Returns the 4 color values packed in an WVec4
  const WVec4 GetAsVec4() const; // [tested]

  /// Helper function to convert a float color value from gamma space to linear color space.
  static float GammaToLinear(float fGamma); // [tested]
  /// Helper function to convert a float color value from linear space to gamma color space.
  static float LinearToGamma(float fGamma); // [tested]

  /// Helper function to convert a float RGB color value from gamma space to linear color space.
  static WVec3 GammaToLinear(const WVec3& vGamma); // [tested]
  /// Helper function to convert a float RGB color value from linear space to gamma color space.
  static WVec3 LinearToGamma(const WVec3& vGamma); // [tested]

  // *** Color specific functions ***
public:
  /// Returns if the color is in the Range [0; 1] on all 4 channels.
  bool IsNormalized() const; // [tested]

  /// Calculates the average of the RGB channels.
  float CalcAverageRGB() const; // [tested]

  /// Computes saturation.
  float GetSaturation() const; // [tested]

  /// Computes the perceived luminance. Assumes linear color space (http://en.wikipedia.org/wiki/Luminance_%28relative%29).
  float GetLuminance() const; /// [tested]

  /// Performs a simple (1.0 - color) inversion on all four channels.
  ///
  /// Using this function on non-normalized colors will lead to negative results.
  /// \see WColor IsNormalized
  WColor GetInvertedColor() const; // [tested]

  /// Calculates the complementary color for this color (hue shifted by 180 degrees). The complementary color will have the same alpha.
  WColor GetComplementaryColor() const; // [tested]

  /// Multiplies the given factor into red, green and blue, but not alpha.
  void ScaleRGB(float fFactor); // [tested]

  /// Multiplies the given factor into red, green, blue and also alpha.
  void ScaleRGBA(float fFactor); // [tested]

  /// Returns 1 for an LDR color (all ´RGB components < 1). Otherwise the value of the largest component. Ignores alpha.
  float ComputeHdrMultiplier() const; // [tested]

  /// Returns the base-2 logarithm of ComputeHdrMultiplier().
  /// 0 for LDR colors, +1, +2, etc. for HDR colors.
  float ComputeHdrExposureValue() const; // [tested]

  /// Raises 2 to the power \a ev and multiplies RGB with that factor.
  void ApplyHdrExposureValue(float fEv); // [tested]

  /// If this is an HDR color, the largest component value is used to normalize RGB to LDR range. Alpha is unaffected.
  void NormalizeToLdrRange(); // [tested]

  /// Returns a darker color by converting the color to HSV, dividing the *value* by fFactor and converting it back.
  WColor GetDarker(float fFactor = 2.0f) const; // [tested]

  // *** Numeric properties ***
public:
  /// Returns true, if any of \a r, \a g, \a b or \a a is NaN.
  bool IsNaN() const; // [tested]

  /// Checks that all components are finite numbers.
  bool IsValid() const; // [tested]

  // *** Operators ***
public:
  /// Converts the color from WColorLinearUB to linear float values.
  void operator=(const WColorLinearUB& cc); // [tested]

  /// Converts the color from WColorGammaUB to linear float values. Gamma is correctly converted to linear space.
  void operator=(const WColorGammaUB& cc); // [tested]

  /// Adds \a rhs component-wise to this color.
  void operator+=(const WColor& rhs); // [tested]

  /// Subtracts \a rhs component-wise from this vector.
  void operator-=(const WColor& rhs); // [tested]

  /// Multiplies \a rhs component-wise with this color.
  void operator*=(const WColor& rhs); // [tested]

  /// Multiplies all components of this color with f.
  void operator*=(float f); // [tested]

  /// Divides all components of this color by f.
  void operator/=(float f); // [tested]

  /// Transforms the RGB components by the matrix. Alpha has no influence on the computation and will stay unmodified. The fourth row of the
  /// matrix is ignored.
  ///
  /// This operation can be used to do basic color correction.
  void operator*=(const WMat4& rhs); // [tested]

  /// Equality Check (bitwise). Only compares RGB, ignores Alpha.
  bool IsIdenticalRGB(const WColor& rhs) const; // [tested]

  /// Equality Check (bitwise). Compares all four components.
  bool IsIdenticalRGBA(const WColor& rhs) const; // [tested]

  /// Equality Check with epsilon. Only compares RGB, ignores Alpha.
  bool IsEqualRGB(const WColor& rhs, float fEpsilon) const; // [tested]

  /// Equality Check with epsilon. Compares all four components.
  bool IsEqualRGBA(const WColor& rhs, float fEpsilon) const; // [tested]

  /// Returns the current color but with changes the alpha value to the given value.
  WColor WithAlpha(float fAlpha) const; // [tested]

  /// Packs the 4 color values as uint8 into a single uint32 with A in the least significant bits and R in the most significant ones.
  [[nodiscard]] WUInt32 ToRGBA8() const; // [tested]

  /// Packs the 4 color values as uint8 into a single uint32 with R in the least significant bits and A in the most significant ones.
  [[nodiscard]] WUInt32 ToABGR8() const; // [tested]
};

// *** Operators ***

/// Component-wise addition.
const WColor operator+(const WColor& c1, const WColor& c2); // [tested]

/// Component-wise subtraction.
const WColor operator-(const WColor& c1, const WColor& c2); // [tested]

/// Component-wise multiplication.
const WColor operator*(const WColor& c1, const WColor& c2); // [tested]

/// Returns a scaled color.
const WColor operator*(float f, const WColor& c); // [tested]

/// Returns a scaled color. Will scale all components.
const WColor operator*(const WColor& c, float f); // [tested]

/// Returns a scaled color. Will scale all components.
const WColor operator/(const WColor& c, float f); // [tested]

/// Transforms the RGB components by the matrix. Alpha has no influence on the computation and will stay unmodified. The fourth row of the
/// matrix is ignored.
///
/// This operation can be used to do basic color correction.
const WColor operator*(const WMat4& lhs, const WColor& rhs); // [tested]

/// Returns true, if both colors are identical in all components.
bool operator==(const WColor& c1, const WColor& c2); // [tested]

/// Returns true, if both colors are not identical in all components.
bool operator!=(const WColor& c1, const WColor& c2); // [tested]

/// Strict weak ordering. Useful for sorting colors into a map.
bool operator<(const WColor& c1, const WColor& c2); // [tested]

static_assert(sizeof(WColor) == 16);

#include <Foundation/Math/Implementation/Color_inl.h>
