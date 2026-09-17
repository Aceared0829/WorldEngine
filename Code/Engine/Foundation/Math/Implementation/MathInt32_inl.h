#pragma once

namespace WMath
{
  constexpr W_ALWAYS_INLINE WInt32 RoundUp(WInt32 value, WUInt16 uiMultiple)
  {
    //
    return (value >= 0) ? ((value + uiMultiple - 1) / uiMultiple) * uiMultiple : (value / uiMultiple) * uiMultiple;
  }

  constexpr W_ALWAYS_INLINE WInt32 RoundDown(WInt32 value, WUInt16 uiMultiple)
  {
    //
    return (value <= 0) ? ((value - uiMultiple + 1) / uiMultiple) * uiMultiple : (value / uiMultiple) * uiMultiple;
  }

  constexpr W_ALWAYS_INLINE WUInt32 RoundUp(WUInt32 value, WUInt16 uiMultiple)
  {
    //
    return ((value + uiMultiple - 1) / uiMultiple) * uiMultiple;
  }

  constexpr W_ALWAYS_INLINE WUInt32 RoundDown(WUInt32 value, WUInt16 uiMultiple)
  {
    //
    return (value / uiMultiple) * uiMultiple;
  }

  constexpr W_ALWAYS_INLINE bool IsOdd(WInt32 i)
  {
    //
    return ((i & 1) != 0);
  }

  constexpr W_ALWAYS_INLINE bool IsEven(WInt32 i)
  {
    //
    return ((i & 1) == 0);
  }

  W_ALWAYS_INLINE WUInt32 Log2i(WUInt32 uiVal)
  {
    return (uiVal != 0) ? FirstBitHigh(uiVal) : -1;
  }

  constexpr W_ALWAYS_INLINE int Pow2(int i)
  {
    //
    return (1 << i);
  }

  inline int Pow(int iBase, int iExp)
  {
    int res = 1;
    while (iExp > 0)
    {
      res *= iBase;
      --iExp;
    }

    return res;
  }

} // namespace WMath
