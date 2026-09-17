#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat4.h>

// ****** WColor ******

WColor WColor::MakeNaN()
{
  return WColor(WMath::NaN<float>(), WMath::NaN<float>(), WMath::NaN<float>(), WMath::NaN<float>());
}

WColor WColor::MakeZero()
{
  return WColor(0.0f, 0.0f, 0.0f, 0.0f);
}

WColor WColor::MakeRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha /*= 1.0f*/)
{
  return WColor(fLinearRed, fLinearGreen, fLinearBlue, fLinearAlpha);
}

void WColor::operator=(const WColorLinearUB& cc)
{
  *this = cc.ToLinearFloat();
}

void WColor::operator=(const WColorGammaUB& cc)
{
  *this = cc.ToLinearFloat();
}

bool WColor::IsNormalized() const
{
  W_NAN_ASSERT(this);

  return r <= 1.0f && g <= 1.0f && b <= 1.0f && a <= 1.0f && r >= 0.0f && g >= 0.0f && b >= 0.0f && a >= 0.0f;
}


float WColor::CalcAverageRGB() const
{
  return (1.0f / 3.0f) * (r + g + b);
}

// http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_%28C%29
void WColor::GetHSV(float& out_fHue, float& out_fSat, float& out_fValue) const
{
  // The formula below assumes values in gamma space
  const float r2 = LinearToGamma(r);
  const float g2 = LinearToGamma(g);
  const float b2 = LinearToGamma(b);

  out_fValue = WMath::Max(r2, g2, b2); // Value

  if (out_fValue < WMath::SmallEpsilon<float>())
  {
    out_fHue = 0.0f;
    out_fSat = 0.0f;
    out_fValue = 0.0f;
    return;
  }

  const float invV = 1.0f / out_fValue;
  float norm_r = r2 * invV;
  float norm_g = g2 * invV;
  float norm_b = b2 * invV;
  float rgb_min = WMath::Min(norm_r, norm_g, norm_b);
  float rgb_max = WMath::Max(norm_r, norm_g, norm_b);

  out_fSat = rgb_max - rgb_min; // Saturation

  if (out_fSat == 0)
  {
    out_fHue = 0;
    return;
  }

  // Normalize saturation
  const float rgb_delta_inv = 1.0f / (rgb_max - rgb_min);
  norm_r = (norm_r - rgb_min) * rgb_delta_inv;
  norm_g = (norm_g - rgb_min) * rgb_delta_inv;
  norm_b = (norm_b - rgb_min) * rgb_delta_inv;
  rgb_max = WMath::Max(norm_r, norm_g, norm_b);

  // hue
  if (rgb_max == norm_r)
  {
    out_fHue = 60.0f * (norm_g - norm_b);

    if (out_fHue < 0.0f)
      out_fHue += 360.0f;
  }
  else if (rgb_max == norm_g)
    out_fHue = 120.0f + 60.0f * (norm_b - norm_r);
  else
    out_fHue = 240.0f + 60.0f * (norm_r - norm_g);
}

// http://www.rapidtables.com/convert/color/hsv-to-rgb.htm
WColor WColor::MakeHSV(float fHue, float fSat, float fVal)
{
  W_ASSERT_DEBUG(fHue <= 360 && fHue >= 0, "HSV 'hue' is in invalid range.");
  W_ASSERT_DEBUG(fSat <= 1 && fVal >= 0, "HSV 'saturation' is in invalid range.");
  W_ASSERT_DEBUG(fVal >= 0, "HSV 'value' is in invalid range.");

  float c = fSat * fVal;
  float x = c * (1.0f - WMath::Abs(WMath::Mod(fHue / 60.0f, 2) - 1.0f));
  float m = fVal - c;

  WColor res;
  res.a = 1.0f;

  if (fHue < 60)
  {
    res.r = c + m;
    res.g = x + m;
    res.b = 0 + m;
  }
  else if (fHue < 120)
  {
    res.r = x + m;
    res.g = c + m;
    res.b = 0 + m;
  }
  else if (fHue < 180)
  {
    res.r = 0 + m;
    res.g = c + m;
    res.b = x + m;
  }
  else if (fHue < 240)
  {
    res.r = 0 + m;
    res.g = x + m;
    res.b = c + m;
  }
  else if (fHue < 300)
  {
    res.r = x + m;
    res.g = 0 + m;
    res.b = c + m;
  }
  else
  {
    res.r = c + m;
    res.g = 0 + m;
    res.b = x + m;
  }

  // The formula above produces value in gamma space
  res.r = GammaToLinear(res.r);
  res.g = GammaToLinear(res.g);
  res.b = GammaToLinear(res.b);

  return res;
}

float WColor::GetSaturation() const
{
  float hue, sat, val;
  GetHSV(hue, sat, val);

  return sat;
}

bool WColor::IsValid() const
{
  if (!WMath::IsFinite(r))
    return false;
  if (!WMath::IsFinite(g))
    return false;
  if (!WMath::IsFinite(b))
    return false;
  if (!WMath::IsFinite(a))
    return false;

  return true;
}

bool WColor::IsEqualRGB(const WColor& rhs, float fEpsilon) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return (WMath::IsEqual(r, rhs.r, fEpsilon) && WMath::IsEqual(g, rhs.g, fEpsilon) && WMath::IsEqual(b, rhs.b, fEpsilon));
}

bool WColor::IsEqualRGBA(const WColor& rhs, float fEpsilon) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return (WMath::IsEqual(r, rhs.r, fEpsilon) && WMath::IsEqual(g, rhs.g, fEpsilon) && WMath::IsEqual(b, rhs.b, fEpsilon) &&
          WMath::IsEqual(a, rhs.a, fEpsilon));
}

void WColor::operator/=(float f)
{
  float f_inv = 1.0f / f;
  r *= f_inv;
  g *= f_inv;
  b *= f_inv;
  a *= f_inv;

  W_NAN_ASSERT(this);
}

void WColor::operator*=(const WMat4& rhs)
{
  WVec3 v(r, g, b);
  v = rhs.TransformPosition(v);

  r = v.x;
  g = v.y;
  b = v.z;
}


void WColor::ScaleRGB(float fFactor)
{
  r *= fFactor;
  g *= fFactor;
  b *= fFactor;
}

void WColor::ScaleRGBA(float fFactor)
{
  r *= fFactor;
  g *= fFactor;
  b *= fFactor;
  a *= fFactor;
}

float WColor::ComputeHdrMultiplier() const
{
  return WMath::Max(1.0f, r, g, b);
}

float WColor::ComputeHdrExposureValue() const
{
  return WMath::Log2(ComputeHdrMultiplier());
}

void WColor::ApplyHdrExposureValue(float fEv)
{
  const float factor = WMath::Pow2(fEv);
  r *= factor;
  g *= factor;
  b *= factor;
}


void WColor::NormalizeToLdrRange()
{
  ScaleRGB(1.0f / ComputeHdrMultiplier());
}

WColor WColor::GetDarker(float fFactor /*= 2.0f*/) const
{
  float h, s, v;
  GetHSV(h, s, v);

  return WColor::MakeHSV(h, s, v / fFactor);
}

WColor WColor::GetComplementaryColor() const
{
  float hue, sat, val;
  GetHSV(hue, sat, val);

  WColor Shifted = WColor::MakeHSV(WMath::Mod(hue + 180.0f, 360.0f), sat, val);
  Shifted.a = a;

  return Shifted;
}

const WVec4 WColor::GetAsVec4() const
{
  return WVec4(r, g, b, a);
}

float WColor::GammaToLinear(float fGamma)
{
  return fGamma <= 0.04045f ? (fGamma / 12.92f) : (WMath::Pow((fGamma + 0.055f) / 1.055f, 2.4f));
}

float WColor::LinearToGamma(float fLinear)
{
  // assuming we have linear color (not CIE xyY or CIE XYZ)
  return fLinear <= 0.0031308f ? (12.92f * fLinear) : (1.055f * WMath::Pow(fLinear, 1.0f / 2.4f) - 0.055f);
}

WVec3 WColor::GammaToLinear(const WVec3& vGamma)
{
  return WVec3(GammaToLinear(vGamma.x), GammaToLinear(vGamma.y), GammaToLinear(vGamma.z));
}

WVec3 WColor::LinearToGamma(const WVec3& vLinear)
{
  // assuming we have linear color (not CIE xyY or CIE XYZ)
  return WVec3(LinearToGamma(vLinear.x), LinearToGamma(vLinear.y), LinearToGamma(vLinear.z));
}

const WColor WColor::AliceBlue(WColorGammaUB(0xF0, 0xF8, 0xFF));
const WColor WColor::AntiqueWhite(WColorGammaUB(0xFA, 0xEB, 0xD7));
const WColor WColor::Aqua(WColorGammaUB(0x00, 0xFF, 0xFF));
const WColor WColor::Aquamarine(WColorGammaUB(0x7F, 0xFF, 0xD4));
const WColor WColor::Azure(WColorGammaUB(0xF0, 0xFF, 0xFF));
const WColor WColor::Beige(WColorGammaUB(0xF5, 0xF5, 0xDC));
const WColor WColor::Bisque(WColorGammaUB(0xFF, 0xE4, 0xC4));
const WColor WColor::Black(WColorGammaUB(0x00, 0x00, 0x00));
const WColor WColor::BlanchedAlmond(WColorGammaUB(0xFF, 0xEB, 0xCD));
const WColor WColor::Blue(WColorGammaUB(0x00, 0x00, 0xFF));
const WColor WColor::BlueViolet(WColorGammaUB(0x8A, 0x2B, 0xE2));
const WColor WColor::Brown(WColorGammaUB(0xA5, 0x2A, 0x2A));
const WColor WColor::BurlyWood(WColorGammaUB(0xDE, 0xB8, 0x87));
const WColor WColor::CadetBlue(WColorGammaUB(0x5F, 0x9E, 0xA0));
const WColor WColor::Chartreuse(WColorGammaUB(0x7F, 0xFF, 0x00));
const WColor WColor::Chocolate(WColorGammaUB(0xD2, 0x69, 0x1E));
const WColor WColor::Coral(WColorGammaUB(0xFF, 0x7F, 0x50));
const WColor WColor::CornflowerBlue(WColorGammaUB(0x64, 0x95, 0xED)); // The Original!
const WColor WColor::Cornsilk(WColorGammaUB(0xFF, 0xF8, 0xDC));
const WColor WColor::Crimson(WColorGammaUB(0xDC, 0x14, 0x3C));
const WColor WColor::Cyan(WColorGammaUB(0x00, 0xFF, 0xFF));
const WColor WColor::DarkBlue(WColorGammaUB(0x00, 0x00, 0x8B));
const WColor WColor::DarkCyan(WColorGammaUB(0x00, 0x8B, 0x8B));
const WColor WColor::DarkGoldenRod(WColorGammaUB(0xB8, 0x86, 0x0B));
const WColor WColor::DarkGray(WColorGammaUB(0xA9, 0xA9, 0xA9));
const WColor WColor::DarkGrey(WColorGammaUB(0xA9, 0xA9, 0xA9));
const WColor WColor::DarkGreen(WColorGammaUB(0x00, 0x64, 0x00));
const WColor WColor::DarkKhaki(WColorGammaUB(0xBD, 0xB7, 0x6B));
const WColor WColor::DarkMagenta(WColorGammaUB(0x8B, 0x00, 0x8B));
const WColor WColor::DarkOliveGreen(WColorGammaUB(0x55, 0x6B, 0x2F));
const WColor WColor::DarkOrange(WColorGammaUB(0xFF, 0x8C, 0x00));
const WColor WColor::DarkOrchid(WColorGammaUB(0x99, 0x32, 0xCC));
const WColor WColor::DarkRed(WColorGammaUB(0x8B, 0x00, 0x00));
const WColor WColor::DarkSalmon(WColorGammaUB(0xE9, 0x96, 0x7A));
const WColor WColor::DarkSeaGreen(WColorGammaUB(0x8F, 0xBC, 0x8F));
const WColor WColor::DarkSlateBlue(WColorGammaUB(0x48, 0x3D, 0x8B));
const WColor WColor::DarkSlateGray(WColorGammaUB(0x2F, 0x4F, 0x4F));
const WColor WColor::DarkSlateGrey(WColorGammaUB(0x2F, 0x4F, 0x4F));
const WColor WColor::DarkTurquoise(WColorGammaUB(0x00, 0xCE, 0xD1));
const WColor WColor::DarkViolet(WColorGammaUB(0x94, 0x00, 0xD3));
const WColor WColor::DeepPink(WColorGammaUB(0xFF, 0x14, 0x93));
const WColor WColor::DeepSkyBlue(WColorGammaUB(0x00, 0xBF, 0xFF));
const WColor WColor::DimGray(WColorGammaUB(0x69, 0x69, 0x69));
const WColor WColor::DimGrey(WColorGammaUB(0x69, 0x69, 0x69));
const WColor WColor::DodgerBlue(WColorGammaUB(0x1E, 0x90, 0xFF));
const WColor WColor::FireBrick(WColorGammaUB(0xB2, 0x22, 0x22));
const WColor WColor::FloralWhite(WColorGammaUB(0xFF, 0xFA, 0xF0));
const WColor WColor::ForestGreen(WColorGammaUB(0x22, 0x8B, 0x22));
const WColor WColor::Fuchsia(WColorGammaUB(0xFF, 0x00, 0xFF));
const WColor WColor::Gainsboro(WColorGammaUB(0xDC, 0xDC, 0xDC));
const WColor WColor::GhostWhite(WColorGammaUB(0xF8, 0xF8, 0xFF));
const WColor WColor::Gold(WColorGammaUB(0xFF, 0xD7, 0x00));
const WColor WColor::GoldenRod(WColorGammaUB(0xDA, 0xA5, 0x20));
const WColor WColor::Gray(WColorGammaUB(0x80, 0x80, 0x80));
const WColor WColor::Grey(WColorGammaUB(0x80, 0x80, 0x80));
const WColor WColor::Green(WColorGammaUB(0x00, 0x80, 0x00));
const WColor WColor::GreenYellow(WColorGammaUB(0xAD, 0xFF, 0x2F));
const WColor WColor::HoneyDew(WColorGammaUB(0xF0, 0xFF, 0xF0));
const WColor WColor::HotPink(WColorGammaUB(0xFF, 0x69, 0xB4));
const WColor WColor::IndianRed(WColorGammaUB(0xCD, 0x5C, 0x5C));
const WColor WColor::Indigo(WColorGammaUB(0x4B, 0x00, 0x82));
const WColor WColor::Ivory(WColorGammaUB(0xFF, 0xFF, 0xF0));
const WColor WColor::Khaki(WColorGammaUB(0xF0, 0xE6, 0x8C));
const WColor WColor::Lavender(WColorGammaUB(0xE6, 0xE6, 0xFA));
const WColor WColor::LavenderBlush(WColorGammaUB(0xFF, 0xF0, 0xF5));
const WColor WColor::LawnGreen(WColorGammaUB(0x7C, 0xFC, 0x00));
const WColor WColor::LemonChiffon(WColorGammaUB(0xFF, 0xFA, 0xCD));
const WColor WColor::LightBlue(WColorGammaUB(0xAD, 0xD8, 0xE6));
const WColor WColor::LightCoral(WColorGammaUB(0xF0, 0x80, 0x80));
const WColor WColor::LightCyan(WColorGammaUB(0xE0, 0xFF, 0xFF));
const WColor WColor::LightGoldenRodYellow(WColorGammaUB(0xFA, 0xFA, 0xD2));
const WColor WColor::LightGray(WColorGammaUB(0xD3, 0xD3, 0xD3));
const WColor WColor::LightGrey(WColorGammaUB(0xD3, 0xD3, 0xD3));
const WColor WColor::LightGreen(WColorGammaUB(0x90, 0xEE, 0x90));
const WColor WColor::LightPink(WColorGammaUB(0xFF, 0xB6, 0xC1));
const WColor WColor::LightSalmon(WColorGammaUB(0xFF, 0xA0, 0x7A));
const WColor WColor::LightSeaGreen(WColorGammaUB(0x20, 0xB2, 0xAA));
const WColor WColor::LightSkyBlue(WColorGammaUB(0x87, 0xCE, 0xFA));
const WColor WColor::LightSlateGray(WColorGammaUB(0x77, 0x88, 0x99));
const WColor WColor::LightSlateGrey(WColorGammaUB(0x77, 0x88, 0x99));
const WColor WColor::LightSteelBlue(WColorGammaUB(0xB0, 0xC4, 0xDE));
const WColor WColor::LightYellow(WColorGammaUB(0xFF, 0xFF, 0xE0));
const WColor WColor::Lime(WColorGammaUB(0x00, 0xFF, 0x00));
const WColor WColor::LimeGreen(WColorGammaUB(0x32, 0xCD, 0x32));
const WColor WColor::Linen(WColorGammaUB(0xFA, 0xF0, 0xE6));
const WColor WColor::Magenta(WColorGammaUB(0xFF, 0x00, 0xFF));
const WColor WColor::Maroon(WColorGammaUB(0x80, 0x00, 0x00));
const WColor WColor::MediumAquaMarine(WColorGammaUB(0x66, 0xCD, 0xAA));
const WColor WColor::MediumBlue(WColorGammaUB(0x00, 0x00, 0xCD));
const WColor WColor::MediumOrchid(WColorGammaUB(0xBA, 0x55, 0xD3));
const WColor WColor::MediumPurple(WColorGammaUB(0x93, 0x70, 0xDB));
const WColor WColor::MediumSeaGreen(WColorGammaUB(0x3C, 0xB3, 0x71));
const WColor WColor::MediumSlateBlue(WColorGammaUB(0x7B, 0x68, 0xEE));
const WColor WColor::MediumSpringGreen(WColorGammaUB(0x00, 0xFA, 0x9A));
const WColor WColor::MediumTurquoise(WColorGammaUB(0x48, 0xD1, 0xCC));
const WColor WColor::MediumVioletRed(WColorGammaUB(0xC7, 0x15, 0x85));
const WColor WColor::MidnightBlue(WColorGammaUB(0x19, 0x19, 0x70));
const WColor WColor::MintCream(WColorGammaUB(0xF5, 0xFF, 0xFA));
const WColor WColor::MistyRose(WColorGammaUB(0xFF, 0xE4, 0xE1));
const WColor WColor::Moccasin(WColorGammaUB(0xFF, 0xE4, 0xB5));
const WColor WColor::NavajoWhite(WColorGammaUB(0xFF, 0xDE, 0xAD));
const WColor WColor::Navy(WColorGammaUB(0x00, 0x00, 0x80));
const WColor WColor::OldLace(WColorGammaUB(0xFD, 0xF5, 0xE6));
const WColor WColor::Olive(WColorGammaUB(0x80, 0x80, 0x00));
const WColor WColor::OliveDrab(WColorGammaUB(0x6B, 0x8E, 0x23));
const WColor WColor::Orange(WColorGammaUB(0xFF, 0xA5, 0x00));
const WColor WColor::OrangeRed(WColorGammaUB(0xFF, 0x45, 0x00));
const WColor WColor::Orchid(WColorGammaUB(0xDA, 0x70, 0xD6));
const WColor WColor::PaleGoldenRod(WColorGammaUB(0xEE, 0xE8, 0xAA));
const WColor WColor::PaleGreen(WColorGammaUB(0x98, 0xFB, 0x98));
const WColor WColor::PaleTurquoise(WColorGammaUB(0xAF, 0xEE, 0xEE));
const WColor WColor::PaleVioletRed(WColorGammaUB(0xDB, 0x70, 0x93));
const WColor WColor::PapayaWhip(WColorGammaUB(0xFF, 0xEF, 0xD5));
const WColor WColor::PeachPuff(WColorGammaUB(0xFF, 0xDA, 0xB9));
const WColor WColor::Peru(WColorGammaUB(0xCD, 0x85, 0x3F));
const WColor WColor::Pink(WColorGammaUB(0xFF, 0xC0, 0xCB));
const WColor WColor::Plum(WColorGammaUB(0xDD, 0xA0, 0xDD));
const WColor WColor::PowderBlue(WColorGammaUB(0xB0, 0xE0, 0xE6));
const WColor WColor::Purple(WColorGammaUB(0x80, 0x00, 0x80));
const WColor WColor::RebeccaPurple(WColorGammaUB(0x66, 0x33, 0x99));
const WColor WColor::Red(WColorGammaUB(0xFF, 0x00, 0x00));
const WColor WColor::RosyBrown(WColorGammaUB(0xBC, 0x8F, 0x8F));
const WColor WColor::RoyalBlue(WColorGammaUB(0x41, 0x69, 0xE1));
const WColor WColor::SaddleBrown(WColorGammaUB(0x8B, 0x45, 0x13));
const WColor WColor::Salmon(WColorGammaUB(0xFA, 0x80, 0x72));
const WColor WColor::SandyBrown(WColorGammaUB(0xF4, 0xA4, 0x60));
const WColor WColor::SeaGreen(WColorGammaUB(0x2E, 0x8B, 0x57));
const WColor WColor::SeaShell(WColorGammaUB(0xFF, 0xF5, 0xEE));
const WColor WColor::Sienna(WColorGammaUB(0xA0, 0x52, 0x2D));
const WColor WColor::Silver(WColorGammaUB(0xC0, 0xC0, 0xC0));
const WColor WColor::SkyBlue(WColorGammaUB(0x87, 0xCE, 0xEB));
const WColor WColor::SlateBlue(WColorGammaUB(0x6A, 0x5A, 0xCD));
const WColor WColor::SlateGray(WColorGammaUB(0x70, 0x80, 0x90));
const WColor WColor::SlateGrey(WColorGammaUB(0x70, 0x80, 0x90));
const WColor WColor::Snow(WColorGammaUB(0xFF, 0xFA, 0xFA));
const WColor WColor::SpringGreen(WColorGammaUB(0x00, 0xFF, 0x7F));
const WColor WColor::SteelBlue(WColorGammaUB(0x46, 0x82, 0xB4));
const WColor WColor::Tan(WColorGammaUB(0xD2, 0xB4, 0x8C));
const WColor WColor::Teal(WColorGammaUB(0x00, 0x80, 0x80));
const WColor WColor::Thistle(WColorGammaUB(0xD8, 0xBF, 0xD8));
const WColor WColor::Tomato(WColorGammaUB(0xFF, 0x63, 0x47));
const WColor WColor::Turquoise(WColorGammaUB(0x40, 0xE0, 0xD0));
const WColor WColor::Violet(WColorGammaUB(0xEE, 0x82, 0xEE));
const WColor WColor::Wheat(WColorGammaUB(0xF5, 0xDE, 0xB3));
const WColor WColor::White(WColorGammaUB(0xFF, 0xFF, 0xFF));
const WColor WColor::WhiteSmoke(WColorGammaUB(0xF5, 0xF5, 0xF5));
const WColor WColor::Yellow(WColorGammaUB(0xFF, 0xFF, 0x00));
const WColor WColor::YellowGreen(WColorGammaUB(0x9A, 0xCD, 0x32));


WUInt32 WColor::ToRGBA8() const
{
  return WColorLinearUB(*this).ToRGBA8();
}

WUInt32 WColor::ToABGR8() const
{
  return WColorLinearUB(*this).ToABGR8();
}
