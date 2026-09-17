#pragma once

namespace WMath
{
  W_ALWAYS_INLINE bool IsFinite(double value)
  {
    // Check the 11 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    WInt64DoubleUnion i2f(value);
    return ((i2f.i & 0x7FF0000000000000ull) != 0x7FF0000000000000ull);
  }

  W_ALWAYS_INLINE bool IsNaN(double value)
  {
    // Check the 11 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    WInt64DoubleUnion i2f(value);
    return (((i2f.i & 0x7FF0000000000000ull) == 0x7FF0000000000000ull) && ((i2f.i & 0xFFFFFFFFFFFFFull) != 0));
  }

  W_ALWAYS_INLINE double Floor(double f)
  {
    return floor(f);
  }
  W_ALWAYS_INLINE WInt32 FloorToInt(double f)
  {
    return static_cast<WInt32>(floor(f));
  }


  W_ALWAYS_INLINE double Ceil(double f)
  {
    return ceil(f);
  }

  W_ALWAYS_INLINE WInt32 CeilToInt(double f)
  {
    return static_cast<WInt32>(ceil(f));
  }

  W_ALWAYS_INLINE double Round(double f)
  {
    return Floor(f + 0.5f);
  }
  W_ALWAYS_INLINE WInt32 RoundToInt(double f)
  {
    return FloorToInt(f + 0.5);
  }
  
  inline double RoundDown(double f, double fMultiple)
  {
    double fDivides = f / fMultiple;
    double fFactor = Floor(fDivides);
    return fFactor * fMultiple;
  }

  inline double RoundUp(double f, double fMultiple)
  {
    double fDivides = f / fMultiple;
    double fFactor = Ceil(fDivides);
    return fFactor * fMultiple;
  }

  W_ALWAYS_INLINE double RoundToMultiple(double f, double fMultiple)
  {
    return Round(f / fMultiple) * fMultiple;
  }

  W_ALWAYS_INLINE double Exp(double f)
  {
    return exp(f);
  }

  W_ALWAYS_INLINE double Ln(double f)
  {
    return log(f);
  }

  W_ALWAYS_INLINE double Log2(double f)
  {
    return log10(f) / log10(2.0);
  }

  W_ALWAYS_INLINE double Log10(double f)
  {
    return log10(f);
  }

  W_ALWAYS_INLINE double Log(double fBase, double f)
  {
    return log10(f) / log10(fBase);
  }

  W_ALWAYS_INLINE double Pow2(double f)
  {
    return pow(2.0, f);
  }

  W_ALWAYS_INLINE double Pow(double fBase, double fExp)
  {
    return pow(fBase, fExp);
  }

  W_ALWAYS_INLINE double Root(double f, double fNthRoot)
  {
    return pow(f, 1.0 / fNthRoot);
  }

  W_ALWAYS_INLINE double Sqrt(double f)
  {
    return sqrt(f);
  }

  W_ALWAYS_INLINE double Mod(double f, double fDiv)
  {
    return fmod(f, fDiv);
  }
} // namespace WMath
