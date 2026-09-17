
#pragma once

#include <Foundation/Basics.h>

/// A class which can be used to represent rational numbers by stating their numerator and denominator.
///
/// WRational uses the following rules
///   0/0 is legal and will be interpreted as 0/1
///   If you are representing a whole number, the denominator should be 1
///
class WRational
{
public:
  W_DECLARE_POD_TYPE();

  /// Default constructor, initializes to 0/1.
  WRational();

  /// Constructor to initialize a rational
  WRational(WUInt32 uiNumerator, WUInt32 uiDenominator);

  /// returns true if the division of the numerator by the denominator would result in a full integer
  bool IsIntegral() const;

  /// Equality operator
  bool operator==(const WRational& other) const;

  /// Inequality operator
  bool operator!=(const WRational& other) const;

  /// Returns the numerator of the rational number
  WUInt32 GetNumerator() const;

  /// Returns the denominator
  WUInt32 GetDenominator() const;

  /// Returns the result of the division as an integer.
  WUInt32 GetIntegralResult() const;

  /// Returns the result of the division as a floating point number (double).
  double GetFloatingPointResult() const;

  /// Returns true if the rational is valid (follows the rules stated in the class description)
  bool IsValid() const;

  /// This helper returns a reduced fraction in case of an integral input.
  ///
  /// Note that this will assert in DEV builds if this class is not integral.
  WRational ReduceIntegralFraction() const;

protected:
  WUInt32 m_uiNumerator = 0;
  WUInt32 m_uiDenominator = 1;
};

#include <Foundation/Math/Implementation/Rational_inl.h>
