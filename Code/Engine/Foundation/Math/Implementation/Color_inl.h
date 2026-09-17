#pragma once

inline WColor::WColor()
{
#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float TypeNaN = WMath::NaN<float>();
  r = TypeNaN;
  g = TypeNaN;
  b = TypeNaN;
  a = TypeNaN;
#endif
}

W_FORCE_INLINE constexpr WColor::WColor(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha /* = 1.0f */)
  : r(fLinearRed)
  , g(fLinearGreen)
  , b(fLinearBlue)
  , a(fLinearAlpha)
{
}

inline WColor::WColor(const WColorLinearUB& cc)
{
  *this = cc;
}

inline WColor::WColor(const WColorGammaUB& cc)
{
  *this = cc;
}

inline void WColor::SetRGB(float fLinearRed, float fLinearGreen, float fLinearBlue)
{
  r = fLinearRed;
  g = fLinearGreen;
  b = fLinearBlue;
}

inline void WColor::SetRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha /* = 1.0f */)
{
  r = fLinearRed;
  g = fLinearGreen;
  b = fLinearBlue;
  a = fLinearAlpha;
}

inline WColor WColor::MakeFromKelvin(WUInt32 uiKelvin)
{
  WColor finalColor;
  float kelvin = WMath::Clamp(uiKelvin, 1000u, 40000u) / 1000.0f;
  float kelvin2 = kelvin * kelvin;

  finalColor.r = kelvin < 6.570f ? 1.0f : WMath::Saturate((1.35651f + 0.216422f * kelvin + 0.000633715f * kelvin2) / (-3.24223f + 0.918711f * kelvin));
  finalColor.g = kelvin < 6.570f ? WMath::Saturate((-399.809f + 414.271f * kelvin + 111.543f * kelvin2) / (2779.24f + 164.143f * kelvin + 84.7356f * kelvin2)) : WMath::Saturate((1370.38f + 734.616f * kelvin + 0.689955f * kelvin2) / (-4625.69f + 1699.87f * kelvin));
  finalColor.b = kelvin > 6.570f ? 1.0f : WMath::Saturate((348.963f - 523.53f * kelvin + 183.62f * kelvin2) / (2848.82f - 214.52f * kelvin + 78.8614f * kelvin2));
  finalColor.a = 1.0f;

  return finalColor;
}

// http://en.wikipedia.org/wiki/Luminance_%28relative%29
W_FORCE_INLINE float WColor::GetLuminance() const
{
  return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

inline WColor WColor::GetInvertedColor() const
{
  W_NAN_ASSERT(this);
  W_ASSERT_DEBUG(IsNormalized(), "Cannot invert a color that has values outside the [0; 1] range");

  return WColor(1.0f - r, 1.0f - g, 1.0f - b, 1.0f - a);
}

inline bool WColor::IsNaN() const
{
  if (WMath::IsNaN(r))
    return true;
  if (WMath::IsNaN(g))
    return true;
  if (WMath::IsNaN(b))
    return true;
  if (WMath::IsNaN(a))
    return true;

  return false;
}

inline void WColor::operator+=(const WColor& rhs)
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  r += rhs.r;
  g += rhs.g;
  b += rhs.b;
  a += rhs.a;
}

inline void WColor::operator-=(const WColor& rhs)
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  r -= rhs.r;
  g -= rhs.g;
  b -= rhs.b;
  a -= rhs.a;
}

inline void WColor::operator*=(const WColor& rhs)
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  r *= rhs.r;
  g *= rhs.g;
  b *= rhs.b;
  a *= rhs.a;
}
inline void WColor::operator*=(float f)
{
  r *= f;
  g *= f;
  b *= f;
  a *= f;

  W_NAN_ASSERT(this);
}

inline bool WColor::IsIdenticalRGB(const WColor& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return r == rhs.r && g == rhs.g && b == rhs.b;
}

inline bool WColor::IsIdenticalRGBA(const WColor& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

inline WColor WColor::WithAlpha(float fAlpha) const
{
  return WColor(r, g, b, fAlpha);
}

inline const WColor operator+(const WColor& c1, const WColor& c2)
{
  W_NAN_ASSERT(&c1);
  W_NAN_ASSERT(&c2);

  return WColor(c1.r + c2.r, c1.g + c2.g, c1.b + c2.b, c1.a + c2.a);
}

inline const WColor operator-(const WColor& c1, const WColor& c2)
{
  W_NAN_ASSERT(&c1);
  W_NAN_ASSERT(&c2);

  return WColor(c1.r - c2.r, c1.g - c2.g, c1.b - c2.b, c1.a - c2.a);
}

inline const WColor operator*(const WColor& c1, const WColor& c2)
{
  W_NAN_ASSERT(&c1);
  W_NAN_ASSERT(&c2);

  return WColor(c1.r * c2.r, c1.g * c2.g, c1.b * c2.b, c1.a * c2.a);
}

inline const WColor operator*(float f, const WColor& c)
{
  W_NAN_ASSERT(&c);

  return WColor(c.r * f, c.g * f, c.b * f, c.a * f);
}

inline const WColor operator*(const WColor& c, float f)
{
  W_NAN_ASSERT(&c);

  return WColor(c.r * f, c.g * f, c.b * f, c.a * f);
}

inline const WColor operator*(const WMat4& lhs, const WColor& rhs)
{
  WColor r = rhs;
  r *= lhs;
  return r;
}

inline const WColor operator/(const WColor& c, float f)
{
  W_NAN_ASSERT(&c);

  float f_inv = 1.0f / f;
  return WColor(c.r * f_inv, c.g * f_inv, c.b * f_inv, c.a * f_inv);
}

W_ALWAYS_INLINE bool operator==(const WColor& c1, const WColor& c2)
{
  return c1.IsIdenticalRGBA(c2);
}

W_ALWAYS_INLINE bool operator!=(const WColor& c1, const WColor& c2)
{
  return !c1.IsIdenticalRGBA(c2);
}

W_FORCE_INLINE bool operator<(const WColor& c1, const WColor& c2)
{
  if (c1.r < c2.r)
    return true;
  if (c1.r > c2.r)
    return false;
  if (c1.g < c2.g)
    return true;
  if (c1.g > c2.g)
    return false;
  if (c1.b < c2.b)
    return true;
  if (c1.b > c2.b)
    return false;

  return (c1.a < c2.a);
}
