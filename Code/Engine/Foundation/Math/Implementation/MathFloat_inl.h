#pragma once

#include <algorithm>
//#include <type_traits>

namespace WMath
{
  W_ALWAYS_INLINE bool IsFinite(float value)
  {
    // Check the 8 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    WIntFloatUnion i2f(value);
    return ((i2f.i & 0x7f800000u) != 0x7f800000u);
  }

  W_ALWAYS_INLINE bool IsNaN(float value)
  {
    // Check the 8 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    WIntFloatUnion i2f(value);
    return (((i2f.i & 0x7f800000u) == 0x7f800000u) && ((i2f.i & 0x7FFFFFu) != 0));
  }

  W_ALWAYS_INLINE float Floor(float f)
  {
    return floorf(f);
  }

  W_ALWAYS_INLINE WInt32 FloorToInt(float f)
  {
    return static_cast<WInt32>(floorf(f));
  }

  W_ALWAYS_INLINE float Ceil(float f)
  {
    return ceilf(f);
  }

  W_ALWAYS_INLINE WInt32 CeilToInt(float f)
  {
    return static_cast<WInt32>(ceilf(f));
  }

  W_ALWAYS_INLINE float Round(float f)
  {
    return Floor(f + 0.5f);
  }

  W_ALWAYS_INLINE WInt32 RoundToInt(float f)
  {
    return FloorToInt(f + 0.5f);
  }

  W_ALWAYS_INLINE float RoundToMultiple(float f, float fMultiple)
  {
    return Round(f / fMultiple) * fMultiple;
  }


  inline float RoundDown(float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor = Floor(fDivides);
    return fFactor * fMultiple;
  }

  inline float RoundUp(float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor = Ceil(fDivides);
    return fFactor * fMultiple;
  }
  template <typename Type>
  W_ALWAYS_INLINE Type Sin(WAngleTemplate<Type> a)
  {
    if constexpr (std::is_same_v<Type, float>)
      return sinf(a.GetRadian());
    else
      return sin(a.GetRadian());
  }

  template <typename Type>
  W_ALWAYS_INLINE Type Cos(WAngleTemplate<Type> a)
  {
    if constexpr (std::is_same_v<Type, float>)
      return cosf(a.GetRadian());
    else
      return cos(a.GetRadian());
  }

  template <typename Type>
  W_ALWAYS_INLINE Type Tan(WAngleTemplate<Type> a)
  {
    if constexpr (std::is_same_v<Type, float>)
      return tanf(a.GetRadian());
    else
      return tan(a.GetRadian());
  }
  
  template <typename Type>
  W_ALWAYS_INLINE WAngleTemplate<Type> ASin(Type f)
  {
    if constexpr (std::is_same_v<Type, float>)
      return WAngleTemplate<Type>::MakeFromRadian(asinf(f));
    else
      return WAngleTemplate<Type>::MakeFromRadian(asin(f));
  }

  template <typename Type>
  W_ALWAYS_INLINE WAngleTemplate<Type> ACos(Type f)
  {
    if constexpr (std::is_same_v<Type, float>)
      return WAngleTemplate<Type>::MakeFromRadian(acosf(f));
    else
      return WAngleTemplate<Type>::MakeFromRadian(acos(f));
  }

  template <typename Type>
  W_ALWAYS_INLINE WAngleTemplate<Type> ATan(Type f)
  {
    if constexpr (std::is_same_v<Type, float>)
      return WAngleTemplate<Type>::MakeFromRadian(atanf(f));
    else
      return WAngleTemplate<Type>::MakeFromRadian(atan(f));
  }

  template <typename Type>
  W_ALWAYS_INLINE WAngleTemplate<Type> ATan2(Type y, Type x)
  {
    if constexpr (std::is_same_v<Type, float>)
      return WAngleTemplate<Type>::MakeFromRadian(atan2f(y, x));
    else
      return WAngleTemplate<Type>::MakeFromRadian(atan2(y, x));
  }

  W_ALWAYS_INLINE float Exp(float f)
  {
    return expf(f);
  }

  W_ALWAYS_INLINE float Ln(float f)
  {
    return logf(f);
  }

  W_ALWAYS_INLINE float Log2(float f)
  {
    return log2f(f);
  }

  W_ALWAYS_INLINE float Log10(float f)
  {
    return log10f(f);
  }

  W_ALWAYS_INLINE float Log(float fBase, float f)
  {
    return log10f(f) / log10f(fBase);
  }

  W_ALWAYS_INLINE float Pow2(float f)
  {
    return exp2f(f);
  }

  W_ALWAYS_INLINE float Pow(float fBase, float fExp)
  {
    return powf(fBase, fExp);
  }

  W_ALWAYS_INLINE float Root(float f, float fNthRoot)
  {
    return powf(f, 1.0f / fNthRoot);
  }

  W_ALWAYS_INLINE float Sqrt(float f)
  {
    return sqrtf(f);
  }

  W_ALWAYS_INLINE float Mod(float f, float fDiv)
  {
    return fmodf(f, fDiv);
  }
} // namespace WMath
