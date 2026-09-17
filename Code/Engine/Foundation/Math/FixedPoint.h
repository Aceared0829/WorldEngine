#pragma once

#include <Foundation/Math/Declarations.h>

/// Implements fixed point arithmetic for fractional values.
///
/// Advantages over float and double are mostly that the computations are entirely integer-based and therefore
/// have a predictable (i.e. deterministic) result, independent from floating point settings, SSE support and
/// differences among CPUs.
/// Additionally fixed point arithmetic should be quite fast, compare to traditional floating point arithmetic
/// (not comparing it to SSE though).
/// With the template argument 'DecimalBits' you can specify how many bits are used for the fractional part.
/// I.e. a simple integer has zero DecimalBits. For a precision of about 1/1000 you need at least 10 DecimalBits
/// (1 << 10) == 1024.
/// Conversion between integer and fixed point is very fast (a shift), in contrast to float/int conversion.
///
/// If you are using WFixedPoint to get guaranteed deterministic behavior, you should minimize the usage of
/// WFixedPoint <-> float conversions. You can set WFixedPoint variables from float constants, but you should
/// never put data into WFixedPoint variables that was computed using floating point arithmetic (even if the
/// computations are simple and look harmless). Instead do all those computations with WFixedPoint variables.
template <WUInt8 DecimalBits>
class WFixedPoint
{
public:
  /// Default constructor does not do any initialization.
  W_ALWAYS_INLINE WFixedPoint() = default; // [tested]

                                             /// Construct from an integer.
  /* implicit */ WFixedPoint(WInt32 iIntVal) { *this = iIntVal; } // [tested]

                                                                    /// Construct from a float.
  /* implicit */ WFixedPoint(float fVal) { *this = fVal; } // [tested]

                                                            /// Construct from a double.
  /* implicit */ WFixedPoint(double fVal) { *this = fVal; } // [tested]

  /// Assignment from an integer.
  const WFixedPoint<DecimalBits>& operator=(WInt32 iVal); // [tested]

  /// Assignment from a float.
  const WFixedPoint<DecimalBits>& operator=(float fVal); // [tested]

  /// Assignment from a double.
  const WFixedPoint<DecimalBits>& operator=(double fVal); // [tested]

  /// Implicit conversion to int (the fractional part is dropped).
  WInt32 ToInt() const; // [tested]

  /// Implicit conversion to float.
  float ToFloat() const; // [tested]

  /// Implicit conversion to double.
  double ToDouble() const; // [tested]

  /// 'Equality' comparison.
  bool operator==(const WFixedPoint<DecimalBits>& rhs) const { return m_iValue == rhs.m_iValue; } // [tested]

  /// 'Inequality' comparison.
  bool operator!=(const WFixedPoint<DecimalBits>& rhs) const { return m_iValue != rhs.m_iValue; } // [tested]

  /// 'Less than' comparison.
  bool operator<(const WFixedPoint<DecimalBits>& rhs) const { return m_iValue < rhs.m_iValue; } // [tested]

  /// 'Greater than' comparison.
  bool operator>(const WFixedPoint<DecimalBits>& rhs) const { return m_iValue > rhs.m_iValue; } // [tested]

  /// 'Less than or equal' comparison.
  bool operator<=(const WFixedPoint<DecimalBits>& rhs) const { return m_iValue <= rhs.m_iValue; } // [tested]

  /// 'Greater than or equal' comparison.
  bool operator>=(const WFixedPoint<DecimalBits>& rhs) const { return m_iValue >= rhs.m_iValue; } // [tested]


  const WFixedPoint<DecimalBits> operator-() const { return WFixedPoint<DecimalBits>(-m_iValue, true); }

  /// += operator
  void operator+=(const WFixedPoint<DecimalBits>& rhs) { m_iValue += rhs.m_iValue; } // [tested]

  /// -= operator
  void operator-=(const WFixedPoint<DecimalBits>& rhs) { m_iValue -= rhs.m_iValue; } // [tested]

  /// *= operator
  void operator*=(const WFixedPoint<DecimalBits>& rhs); // [tested]

  /// /= operator
  void operator/=(const WFixedPoint<DecimalBits>& rhs); // [tested]

  /// *= operator with integers (more efficient)
  void operator*=(WInt32 rhs) { m_iValue *= rhs; } // [tested]

  /// /= operator with integers (more efficient)
  void operator/=(WInt32 rhs) { m_iValue /= rhs; } // [tested]

  /// Returns the underlying integer value. Mostly useful for serialization (or tests).
  WInt32 GetRawValue() const { return m_iValue; }

  /// Sets the underlying integer value. Mostly useful for serialization (or tests).
  void SetRawValue(WInt32 iVal) { m_iValue = iVal; }

private:
  WInt32 m_iValue;
};

template <WUInt8 DecimalBits>
float ToFloat(WFixedPoint<DecimalBits> f)
{
  return f.ToFloat();
}

// Additional operators:
// WFixedPoint operator+ (WFixedPoint, WFixedPoint); // [tested]
// WFixedPoint operator- (WFixedPoint, WFixedPoint); // [tested]
// WFixedPoint operator* (WFixedPoint, WFixedPoint); // [tested]
// WFixedPoint operator/ (WFixedPoint, WFixedPoint); // [tested]
// WFixedPoint operator* (int, WFixedPoint); // [tested]
// WFixedPoint operator* (WFixedPoint, int); // [tested]
// WFixedPoint operator/ (WFixedPoint, int); // [tested]

#include <Foundation/Math/Implementation/FixedPoint_inl.h>
