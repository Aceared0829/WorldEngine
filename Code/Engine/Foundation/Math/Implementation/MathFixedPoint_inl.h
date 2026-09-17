#pragma once

#include <Foundation/Math/FixedPoint.h>

/*
namespace WMath
{
#define FIXEDPOINT_OVERLOADS(Bits)                                                                                                       \
  template <>                                                                                                                            \
  W_ALWAYS_INLINE WFixedPoint<Bits> BasicType<WFixedPoint<Bits>>::MaxValue() { return (WFixedPoint<Bits>)((1 << (31 - Bits)) - 1); } \
  template <>                                                                                                                            \
  W_ALWAYS_INLINE WFixedPoint<Bits> BasicType<WFixedPoint<Bits>>::SmallEpsilon() { return (WFixedPoint<Bits>)0.0001; }               \
  template <>                                                                                                                            \
  W_ALWAYS_INLINE WFixedPoint<Bits> BasicType<WFixedPoint<Bits>>::DefaultEpsilon() { return (WFixedPoint<Bits>)0.001; }              \
  template <>                                                                                                                            \
  W_ALWAYS_INLINE WFixedPoint<Bits> BasicType<WFixedPoint<Bits>>::LargeEpsilon() { return (WFixedPoint<Bits>)0.01; }                 \
  template <>                                                                                                                            \
  W_ALWAYS_INLINE WFixedPoint<Bits> BasicType<WFixedPoint<Bits>>::HugeEpsilon() { return (WFixedPoint<Bits>)0.1; }

  FIXEDPOINT_OVERLOADS(1);
  FIXEDPOINT_OVERLOADS(2);
  FIXEDPOINT_OVERLOADS(3);
  FIXEDPOINT_OVERLOADS(4);
  FIXEDPOINT_OVERLOADS(5);
  FIXEDPOINT_OVERLOADS(6);
  FIXEDPOINT_OVERLOADS(7);
  FIXEDPOINT_OVERLOADS(8);
  FIXEDPOINT_OVERLOADS(9);
  FIXEDPOINT_OVERLOADS(10);
  FIXEDPOINT_OVERLOADS(11);
  FIXEDPOINT_OVERLOADS(12);
  FIXEDPOINT_OVERLOADS(13);
  FIXEDPOINT_OVERLOADS(14);
  FIXEDPOINT_OVERLOADS(15);
  FIXEDPOINT_OVERLOADS(16);
  FIXEDPOINT_OVERLOADS(17);
  FIXEDPOINT_OVERLOADS(18);
  FIXEDPOINT_OVERLOADS(19);
  FIXEDPOINT_OVERLOADS(20);
  FIXEDPOINT_OVERLOADS(21);
  FIXEDPOINT_OVERLOADS(22);
  FIXEDPOINT_OVERLOADS(23);
  FIXEDPOINT_OVERLOADS(24);
  FIXEDPOINT_OVERLOADS(25);
  FIXEDPOINT_OVERLOADS(26);
  FIXEDPOINT_OVERLOADS(27);
  FIXEDPOINT_OVERLOADS(28);
  FIXEDPOINT_OVERLOADS(29);
  FIXEDPOINT_OVERLOADS(30);
  //FIXEDPOINT_OVERLOADS(31);

  template <WUInt8 DecimalBits>
  W_FORCE_INLINE WFixedPoint<DecimalBits> Floor(WFixedPoint<DecimalBits> f)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    return (WFixedPoint<DecimalBits>)floor(f.ToDouble());
  }

  template <WUInt8 DecimalBits>
  W_FORCE_INLINE WFixedPoint<DecimalBits> Ceil(WFixedPoint<DecimalBits> f)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    return (WFixedPoint<DecimalBits>)ceil(f.ToDouble());
  }

  template <WUInt8 DecimalBits>
  inline WFixedPoint<DecimalBits> Floor(WFixedPoint<DecimalBits> f, WFixedPoint<DecimalBits> fMultiple)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    WFixedPoint<DecimalBits> fDivides = f / fMultiple;
    WFixedPoint<DecimalBits> fFactor = Floor(fDivides);
    return fFactor * fMultiple;
  }

  template <WUInt8 DecimalBits>
  inline WFixedPoint<DecimalBits> Ceil(WFixedPoint<DecimalBits> f, WFixedPoint<DecimalBits> fMultiple)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    WFixedPoint<DecimalBits> fDivides = f / fMultiple;
    WFixedPoint<DecimalBits> fFactor = Ceil(fDivides);
    return fFactor * fMultiple;
  }

  template <WUInt8 DecimalBits>
  W_FORCE_INLINE WFixedPoint<DecimalBits> Exp(WFixedPoint<DecimalBits> f)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    return (WFixedPoint<DecimalBits>)exp(f.ToDouble());
  }

  template <WUInt8 DecimalBits>
  W_FORCE_INLINE WFixedPoint<DecimalBits> Ln(WFixedPoint<DecimalBits> f)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    return (WFixedPoint<DecimalBits>)log(f.ToDouble());
  }

  template <WUInt8 DecimalBits>
  W_FORCE_INLINE WFixedPoint<DecimalBits> Log2(WFixedPoint<DecimalBits> f)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    return (WFixedPoint<DecimalBits>)(log10(f.ToDouble()) / log10(2.0));
  }

  template <WUInt8 DecimalBits>
  W_FORCE_INLINE WFixedPoint<DecimalBits> Log10(WFixedPoint<DecimalBits> f)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    return (WFixedPoint<DecimalBits>)log10(f.ToDouble());
  }

  template <WUInt8 DecimalBits>
  W_FORCE_INLINE WFixedPoint<DecimalBits> Log(WFixedPoint<DecimalBits> fBase, WFixedPoint<DecimalBits> f)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    return (WFixedPoint<DecimalBits>)(log10(f.ToDouble()) / log10(fBase.ToDouble()));
  }

  template <WUInt8 DecimalBits>
  W_FORCE_INLINE WFixedPoint<DecimalBits> Pow2(WFixedPoint<DecimalBits> f)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    return (WFixedPoint<DecimalBits>)pow(2.0, f.ToDouble());
  }

  template <WUInt8 DecimalBits>
  W_FORCE_INLINE WFixedPoint<DecimalBits> Pow(WFixedPoint<DecimalBits> base, WFixedPoint<DecimalBits> exp)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    return (WFixedPoint<DecimalBits>)pow(base.ToDouble(), exp.ToDouble());
  }

  template <WUInt8 DecimalBits>
  W_FORCE_INLINE WFixedPoint<DecimalBits> Root(WFixedPoint<DecimalBits> f, WFixedPoint<DecimalBits> NthRoot)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    return (WFixedPoint<DecimalBits>)pow(f.ToDouble(), 1.0 / NthRoot.ToDouble());
  }

  template <WUInt8 DecimalBits>
  WFixedPoint<DecimalBits> Sqrt(WFixedPoint<DecimalBits> a)
  {
    return (WFixedPoint<DecimalBits>)sqrt(a.ToDouble());
    //if (a <= WFixedPoint<DecimalBits>(0))
    //  return WFixedPoint<DecimalBits>(0);

    //WFixedPoint<DecimalBits> x = a / 2;

    //for (WUInt32 i = 0; i < 8; ++i)
    //{
    //  WFixedPoint<DecimalBits> ax = a / x;
    //  WFixedPoint<DecimalBits> xpax = x + ax;
    //  x = xpax / 2;
    //}

    //return x;
  }

  template <WUInt8 DecimalBits>
  W_FORCE_INLINE WFixedPoint<DecimalBits> Mod(WFixedPoint<DecimalBits> f, WFixedPoint<DecimalBits> div)
  {
    W_REPORT_FAILURE("This function is not really implemented yet.");

    return (WFixedPoint<DecimalBits>)fmod(f.ToDouble(), div);
  }
}
*/
