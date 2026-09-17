#pragma once

#include <algorithm>

namespace WMath
{
  template <typename T>
  constexpr W_ALWAYS_INLINE T Square(T f)
  {
    return (f * f);
  }

  template <typename T>
  constexpr W_ALWAYS_INLINE T Sign(T f)
  {
    return (f < 0 ? T(-1) : f > 0 ? T(1)
                                  : 0);
  }

  template <typename T>
  constexpr W_ALWAYS_INLINE T Abs(T f)
  {
    return (f < 0 ? -f : f);
  }

  template <typename T>
  constexpr W_ALWAYS_INLINE T Min(T f1, T f2)
  {
    return (f2 < f1 ? f2 : f1);
  }

  template <typename T, typename... ARGS>
  constexpr W_ALWAYS_INLINE T Min(T f1, T f2, ARGS... f)
  {
    return Min(Min(f1, f2), f...);
  }

  template <typename T>
  constexpr W_ALWAYS_INLINE T Max(T f1, T f2)
  {
    return (f1 < f2 ? f2 : f1);
  }

  template <typename T, typename... ARGS>
  constexpr W_ALWAYS_INLINE T Max(T f1, T f2, ARGS... f)
  {
    return Max(Max(f1, f2), f...);
  }

  template <typename T>
  constexpr W_ALWAYS_INLINE T Clamp(T value, T min_val, T max_val)
  {
    return value < min_val ? min_val : (max_val < value ? max_val : value);
  }

  template <typename T>
  constexpr W_ALWAYS_INLINE T Saturate(T value)
  {
    return Clamp(value, T(0), T(1));
  }

  template <typename Type>
  constexpr Type Invert(Type f)
  {
    static_assert(std::is_floating_point_v<Type>);

    return ((Type)1) / f;
  }

  W_ALWAYS_INLINE WUInt32 FirstBitLow(WUInt32 value)
  {
    W_ASSERT_DEBUG(value != 0, "FirstBitLow is undefined for 0");

#if W_ENABLED(W_COMPILER_MSVC)
    unsigned long uiIndex = 0;
    _BitScanForward(&uiIndex, value);
    return uiIndex;
#elif W_ENABLED(W_COMPILER_GCC) || W_ENABLED(W_COMPILER_CLANG)
    return __builtin_ctz(value);
#else
    W_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  W_ALWAYS_INLINE WUInt32 FirstBitLow(WUInt64 value)
  {
    W_ASSERT_DEBUG(value != 0, "FirstBitLow is undefined for 0");

#if W_ENABLED(W_COMPILER_MSVC)
    unsigned long uiIndex = 0;
#  if W_ENABLED(W_PLATFORM_64BIT)

    _BitScanForward64(&uiIndex, value);
#  else
    uint32_t lower = static_cast<uint32_t>(value);
    unsigned char returnCode = _BitScanForward(&uiIndex, lower);
    if (returnCode == 0)
    {
      uint32_t upper = static_cast<uint32_t>(value >> 32);
      returnCode = _BitScanForward(&uiIndex, upper);
      if (returnCode > 0) // Only can happen in Release build when W_ASSERT_DEBUG(value != 0) would fail.
      {
        uiIndex += 32;    // Add length of lower to index.
      }
    }
#  endif
    return uiIndex;
#elif W_ENABLED(W_COMPILER_GCC) || W_ENABLED(W_COMPILER_CLANG)
    return __builtin_ctzll(value);
#else
    W_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  W_ALWAYS_INLINE WUInt32 FirstBitHigh(WUInt32 value)
  {
    W_ASSERT_DEBUG(value != 0, "FirstBitHigh is undefined for 0");

#if W_ENABLED(W_COMPILER_MSVC)
    unsigned long uiIndex = 0;
    _BitScanReverse(&uiIndex, value);
    return uiIndex;
#elif W_ENABLED(W_COMPILER_GCC) || W_ENABLED(W_COMPILER_CLANG)
    return 31 - __builtin_clz(value);
#else
    W_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  W_ALWAYS_INLINE WUInt32 FirstBitHigh(WUInt64 value)
  {
    W_ASSERT_DEBUG(value != 0, "FirstBitHigh is undefined for 0");

#if W_ENABLED(W_COMPILER_MSVC)
    unsigned long uiIndex = 0;
#  if W_ENABLED(W_PLATFORM_64BIT)
    _BitScanReverse64(&uiIndex, value);
#  else
    uint32_t upper = static_cast<uint32_t>(value >> 32);
    unsigned char returnCode = _BitScanReverse(&uiIndex, upper);
    if (returnCode == 0)
    {
      uint32_t lower = static_cast<uint32_t>(value);
      returnCode = _BitScanReverse(&uiIndex, lower);
    }
    else
    {
      uiIndex += 32; // Add length of upper to index.
    }
#  endif
    return uiIndex;
#elif W_ENABLED(W_COMPILER_GCC) || W_ENABLED(W_COMPILER_CLANG)
    return 63 - __builtin_clzll(value);
#else
    W_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  W_ALWAYS_INLINE WUInt32 CountTrailingZeros(WUInt32 uiBitmask)
  {
    return (uiBitmask == 0) ? 32 : FirstBitLow(uiBitmask);
  }

  W_ALWAYS_INLINE WUInt32 CountTrailingZeros(WUInt64 uiBitmask)
  {
    const WUInt32 numLow = CountTrailingZeros(static_cast<WUInt32>(uiBitmask & 0xFFFFFFFF));
    const WUInt32 numHigh = CountTrailingZeros(static_cast<WUInt32>((uiBitmask >> 32u) & 0xFFFFFFFF));

    return (numLow == 32) ? (32 + numHigh) : numLow;
  }

  W_ALWAYS_INLINE WUInt32 CountLeadingZeros(WUInt32 uiBitmask)
  {
    return (uiBitmask == 0) ? 32 : (31u - FirstBitHigh(uiBitmask));
  }


  W_ALWAYS_INLINE WUInt32 CountBits(WUInt32 value)
  {
#if W_ENABLED(W_COMPILER_MSVC) && (W_ENABLED(W_PLATFORM_ARCH_X86) || (W_ENABLED(W_PLATFORM_ARCH_ARM) && W_ENABLED(W_PLATFORM_32BIT)))
#  if W_ENABLED(W_PLATFORM_ARCH_X86)
    return __popcnt(value);
#  else
    return _CountOneBits(value);
#  endif
#elif W_ENABLED(W_COMPILER_GCC) || W_ENABLED(W_COMPILER_CLANG)
    return __builtin_popcount(value);
#else
    value = value - ((value >> 1) & 0x55555555u);
    value = (value & 0x33333333u) + ((value >> 2) & 0x33333333u);
    return ((value + (value >> 4) & 0xF0F0F0Fu) * 0x1010101u) >> 24;
#endif
  }

  W_ALWAYS_INLINE WUInt32 CountBits(WUInt64 value)
  {
    WUInt32 result = 0;
    result += CountBits(WUInt32(value));
    result += CountBits(WUInt32(value >> 32));
    return result;
  }

  template <typename Type>
  W_ALWAYS_INLINE constexpr Type Bitmask_LowN(WUInt32 uiNumBitsToSet)
  {
    return (uiNumBitsToSet >= sizeof(Type) * 8) ? ~static_cast<Type>(0) : ((static_cast<Type>(1) << uiNumBitsToSet) - static_cast<Type>(1));
  }

  template <typename Type>
  W_ALWAYS_INLINE constexpr Type Bitmask_HighN(WUInt32 uiNumBitsToSet)
  {
    return (uiNumBitsToSet == 0) ? 0 : ~static_cast<Type>(0) << ((sizeof(Type) * 8) - WMath::Min<WUInt32>(uiNumBitsToSet, sizeof(Type) * 8));
  }

  template <typename T>
  W_ALWAYS_INLINE void Swap(T& ref_f1, T& ref_f2)
  {
    std::swap(ref_f1, ref_f2);
  }

  template <typename T>
  W_FORCE_INLINE T Lerp(T f1, T f2, float fFactor)
  {
    return (T)(f1 + (fFactor * (f2 - f1)));
  }

  template <typename T>
  W_FORCE_INLINE T Lerp(T f1, T f2, double fFactor)
  {
    return (T)(f1 + (fFactor * (f2 - f1)));
  }

  template <typename T>
  W_FORCE_INLINE constexpr float Unlerp(T fMin, T fMax, T fValue)
  {
    return static_cast<float>(fValue - fMin) / static_cast<float>(fMax - fMin);
  }

  ///  Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  constexpr W_FORCE_INLINE T Step(T value, T edge)
  {
    return (value >= edge ? T(1) : T(0));
  }

  constexpr W_FORCE_INLINE bool IsPowerOf2(WInt32 value)
  {
    return (value < 1) ? false : ((value & (value - 1)) == 0);
  }

  constexpr W_FORCE_INLINE bool IsPowerOf2(WUInt32 value)
  {
    return (value < 1) ? false : ((value & (value - 1)) == 0);
  }

  constexpr W_FORCE_INLINE bool IsPowerOf2(WUInt64 value)
  {
    return (value < 1) ? false : ((value & (value - 1)) == 0);
  }

  template <typename Type>
  constexpr bool IsEqual(Type lhs, Type rhs, Type fEpsilon)
  {
    return ((rhs >= lhs - fEpsilon) && (rhs <= lhs + fEpsilon));
  }

  template <typename T>
  constexpr inline bool IsInRange(T value, T minVal, T maxVal)
  {
    return minVal < maxVal ? (value >= minVal) && (value <= maxVal) : (value <= minVal) && (value >= maxVal);
  }

  template <typename Type>
  bool IsZero(Type f, Type fEpsilon)
  {
    W_ASSERT_DEBUG(fEpsilon >= 0, "Epsilon may not be negative.");

    return ((f >= -fEpsilon) && (f <= fEpsilon));
  }

  template <typename Type>
  W_ALWAYS_INLINE Type Trunc(Type f)
  {
    if (f > 0)
      return Floor(f);

    return Ceil(f);
  }

  template <typename Type>
  W_ALWAYS_INLINE Type Fraction(Type f)
  {
    return (f - Trunc(f));
  }

  template <typename Type>
  inline Type SmoothStep(Type x, Type edge1, Type edge2)
  {
    const Type divider = edge2 - edge1;

    if (divider == (Type)0)
    {
      return (x >= edge2) ? 1 : 0;
    }

    x = Saturate((x - edge1) / divider);

    return (x * x * ((Type)3 - ((Type)2 * x)));
  }

  template <typename Type>
  inline Type SmootherStep(Type x, Type edge1, Type edge2)
  {
    const Type divider = edge2 - edge1;

    if (divider == (Type)0)
    {
      return (x >= edge2) ? 1 : 0;
    }

    x = Saturate((x - edge1) / divider);

    return (x * x * x * (x * ((Type)6 * x - (Type)15) + (Type)10));
  }

  template <WUInt32 NumBits>
  inline WUInt32 ColorFloatToUnsignedInt(float value)
  {
    constexpr float fMaxValue = static_cast<float>(WMath::Bitmask_LowN<WUInt32>(NumBits));

    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      return static_cast<WUInt32>(Saturate(value) * fMaxValue + 0.5f);
    }
  }

  W_ALWAYS_INLINE WUInt8 ColorFloatToByte(float value)
  {
    return static_cast<WUInt8>(ColorFloatToUnsignedInt<8>(value));
  }

  W_ALWAYS_INLINE WUInt16 ColorFloatToShort(float value)
  {
    return static_cast<WUInt16>(ColorFloatToUnsignedInt<16>(value));
  }

  inline WInt8 ColorFloatToSignedByte(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      value = Clamp(value, -1.0f, 1.0f) * 127.0f;
      if (value >= 0.0f)
      {
        value += 0.5f;
      }
      else
      {
        value -= 0.5f;
      }
      return static_cast<WInt8>(value);
    }
  }

  inline WInt16 ColorFloatToSignedShort(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      value = Clamp(value, -1.0f, 1.0f) * 32767.0f;
      if (value >= 0.0f)
      {
        value += 0.5f;
      }
      else
      {
        value -= 0.5f;
      }
      return static_cast<WInt16>(value);
    }
  }

  template <WUInt32 NumBits>
  constexpr float ColorUnsignedIntToFloat(WUInt32 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    constexpr WUInt32 uiMaxValue = WMath::Bitmask_LowN<WUInt32>(NumBits);
    constexpr float fMaxValue = static_cast<float>(uiMaxValue);
    return (value & uiMaxValue) * (1.0f / fMaxValue);
  }

  W_ALWAYS_INLINE constexpr float ColorByteToFloat(WUInt8 value)
  {
    return ColorUnsignedIntToFloat<8>(value);
  }

  W_ALWAYS_INLINE constexpr float ColorShortToFloat(WUInt16 value)
  {
    return ColorUnsignedIntToFloat<16>(value);
  }

  constexpr inline float ColorSignedByteToFloat(WInt8 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return (value == -128) ? -1.0f : value * (1.0f / 127.0f);
  }

  constexpr inline float ColorSignedShortToFloat(WInt16 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return (value == -32768) ? -1.0f : value * (1.0f / 32767.0f);
  }

  template <typename T, typename T2>
  W_FORCE_INLINE T EvaluateBezierCurve(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint)
  {
    const T2 mt = 1 - t;
    const T2 mt2 = mt * mt;
    const T2 t2 = t * t;

    const T2 f1 = mt2 * mt;
    const T2 f2 = 3 * mt2 * t;
    const T2 f3 = 3 * mt * t2;
    const T2 f4 = t * t2;

    return f1 * startPoint + f2 * controlPoint1 + f3 * controlPoint2 + f4 * endPoint;
  }

  template <typename T, typename T2>
  W_FORCE_INLINE T EvaluateBezierCurveDerivative(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint)
  {
    const T2 mt = 1 - t;

    const T2 f1 = 3 * mt * mt;
    const T2 f2 = 6 * mt * t;
    const T2 f3 = 3 * t * t;

    return f1 * (controlPoint1 - startPoint) + f2 * (controlPoint2 - controlPoint1) + f3 * (endPoint - controlPoint2);
  }
} // namespace WMath


template <typename T>
constexpr W_FORCE_INLINE WInt32 WMath::FloatToInt32(T value)
{
  return static_cast<WInt32>(value);
}


#if W_DISABLED(W_PLATFORM_ARCH_X86) || (_MSC_VER <= 1916)
constexpr W_FORCE_INLINE WInt64 WMath::FloatToInt(double value)
{
  return static_cast<WInt64>(value);
}
#endif

W_ALWAYS_INLINE WResult WMath::TryConvertToSizeT(size_t& out_uiResult, WUInt64 uiValue)
{
#if W_ENABLED(W_PLATFORM_32BIT)
  if (uiValue <= MaxValue<size_t>())
  {
    out_uiResult = static_cast<size_t>(uiValue);
    return W_SUCCESS;
  }

  return W_FAILURE;
#else
  out_uiResult = static_cast<size_t>(uiValue);
  return W_SUCCESS;
#endif
}

#if W_ENABLED(W_PLATFORM_64BIT)
W_ALWAYS_INLINE size_t WMath::SafeConvertToSizeT(WUInt64 uiValue)
{
  return uiValue;
}
#endif

W_ALWAYS_INLINE constexpr WUInt32 WMath::WrapUInt(WUInt32 uiValue, WUInt32 uiExcludedMaxValue)
{
  return uiValue % uiExcludedMaxValue;
}

W_ALWAYS_INLINE constexpr WInt32 WMath::WrapInt(WInt32 iValue, WUInt32 uiExcludedMaxValue)
{
  const WInt32 wrapped = (iValue % static_cast<WInt32>(uiExcludedMaxValue));
  return wrapped >= 0 ? wrapped : (wrapped + uiExcludedMaxValue);
}

W_ALWAYS_INLINE constexpr WInt32 WMath::WrapInt(WInt32 iValue, WInt32 iMinValue, WInt32 iExcludedMaxValue)
{
  W_ASSERT_DEBUG(iMinValue < iExcludedMaxValue, "Invalid range to wrap integer around.");
  return iMinValue + WrapInt(iValue - iMinValue, static_cast<WUInt32>(iExcludedMaxValue - iMinValue));
}

template<typename Type>
W_ALWAYS_INLINE Type WMath::WrapFloat01(Type fValue)
{
  if (fValue < (Type)0.0)
  {
    return fValue + Ceil(-fValue);
  }
  else if (fValue > (Type)1.0)
  {
    return fValue - Ceil(fValue - (Type)1.0);
  }

  return fValue;
}
template<typename Type>
W_ALWAYS_INLINE Type WMath::WrapFloat(Type fValue, Type fMinValue, Type fMaxValue)
{
  const Type range = fMaxValue - fMinValue;
  return fMinValue + WrapFloat01<Type>((fValue - fMinValue) / range) * range;
}

W_ALWAYS_INLINE constexpr WUInt64 WMath::MakeUInt64(WUInt32 uiHigh32, WUInt32 uiLow32)
{
  return (static_cast<WUInt64>(uiHigh32) << 32) | static_cast<WUInt64>(uiLow32);
}
