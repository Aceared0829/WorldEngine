#pragma once

W_ALWAYS_INLINE WColorBaseUB::WColorBaseUB(WUInt8 r, WUInt8 g, WUInt8 b, WUInt8 a /* = 255*/)
{
  this->r = r;
  this->g = g;
  this->b = b;
  this->a = a;
}

W_ALWAYS_INLINE WColorLinearUB::WColorLinearUB(WUInt8 r, WUInt8 g, WUInt8 b, WUInt8 a /* = 255*/)
  : WColorBaseUB(r, g, b, a)
{
}

inline WColorLinearUB::WColorLinearUB(const WColor& color)
{
  *this = color;
}

inline void WColorLinearUB::operator=(const WColor& color)
{
  r = WMath::ColorFloatToByte(color.r);
  g = WMath::ColorFloatToByte(color.g);
  b = WMath::ColorFloatToByte(color.b);
  a = WMath::ColorFloatToByte(color.a);
}

inline WColor WColorLinearUB::ToLinearFloat() const
{
  return WColor(WMath::ColorByteToFloat(r), WMath::ColorByteToFloat(g), WMath::ColorByteToFloat(b), WMath::ColorByteToFloat(a));
}

// *****************

W_ALWAYS_INLINE WColorGammaUB::WColorGammaUB(WUInt8 r, WUInt8 g, WUInt8 b, WUInt8 a)
  : WColorBaseUB(r, g, b, a)
{
}

inline WColorGammaUB::WColorGammaUB(const WColor& color)
{
  *this = color;
}

inline void WColorGammaUB::operator=(const WColor& color)
{
  const WVec3 gamma = WColor::LinearToGamma(WVec3(color.r, color.g, color.b));

  r = WMath::ColorFloatToByte(gamma.x);
  g = WMath::ColorFloatToByte(gamma.y);
  b = WMath::ColorFloatToByte(gamma.z);
  a = WMath::ColorFloatToByte(color.a);
}

inline WColor WColorGammaUB::ToLinearFloat() const
{
  WVec3 gamma;
  gamma.x = WMath::ColorByteToFloat(r);
  gamma.y = WMath::ColorByteToFloat(g);
  gamma.z = WMath::ColorByteToFloat(b);

  const WVec3 linear = WColor::GammaToLinear(gamma);

  return WColor(linear.x, linear.y, linear.z, WMath::ColorByteToFloat(a));
}
