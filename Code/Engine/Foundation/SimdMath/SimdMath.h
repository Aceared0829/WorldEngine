#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

struct WSimdMath
{
  static WSimdVec4f Exp(const WSimdVec4f& f);
  static WSimdVec4f Ln(const WSimdVec4f& f);
  static WSimdVec4f Log2(const WSimdVec4f& f);
  static WSimdVec4i Log2i(const WSimdVec4i& i);
  static WSimdVec4f Log10(const WSimdVec4f& f);
  static WSimdVec4f Pow2(const WSimdVec4f& f);

  static WSimdVec4f Sin(const WSimdVec4f& f);
  static WSimdVec4f Cos(const WSimdVec4f& f);
  static WSimdVec4f Tan(const WSimdVec4f& f);

  static WSimdVec4f ASin(const WSimdVec4f& f);
  static WSimdVec4f ACos(const WSimdVec4f& f);
  static WSimdVec4f ATan(const WSimdVec4f& f);
};

#include <Foundation/SimdMath/Implementation/SimdMath_inl.h>
