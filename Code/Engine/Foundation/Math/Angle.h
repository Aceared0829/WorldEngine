#pragma once

/// Float/double wrapper struct for a safe usage and conversions of angles.
///
/// Uses radian internally. Will <b>not</b> automatically keep its range between 0 degree - 360 degree (0 - 2PI) but you can call NormalizeRange to do
/// so.
template <typename Type>
class WAngleTemplate
{
public:
  W_DECLARE_POD_TYPE();


  /// Returns the constant to multiply with an angle in degree to convert it to radians.
  constexpr static W_ALWAYS_INLINE Type DegToRadMultiplier(); // [tested]

  /// Returns the constant to multiply with an angle in degree to convert it to radians.
  constexpr static W_ALWAYS_INLINE Type RadToDegMultiplier(); // [tested]

  /// Converts an angle in degree to radians.
  constexpr static Type DegToRad(Type f); // [tested]

  /// Converts an angle in radians to degree.
  constexpr static Type RadToDeg(Type f); // [tested]

  /// Returns a zero initialized angle. Same as a default constructed object.
  [[nodiscard]] constexpr static WAngleTemplate<Type> MakeZero() { return WAngleTemplate<Type>(); }

  /// Creates an instance of WAngleTemplate<Type> that was initialized from degree. (Performs a conversion)
  [[nodiscard]] constexpr static WAngleTemplate<Type> MakeFromDegree(Type fDegree); // [tested]

  /// Creates an instance of WAngleTemplate<Type> that was initialized from radian. (No need for any conversion)
  [[nodiscard]] constexpr static WAngleTemplate<Type> MakeFromRadian(Type fRadian); // [tested]

  /// Standard constructor, initializing with 0.
  constexpr WAngleTemplate<Type>()
    : m_fRadian(0.0f)
  {
  } // [tested]

  /// For internal use only.
  constexpr explicit WAngleTemplate<Type>(Type fRadian)
    : m_fRadian(fRadian)
  {
  }




  /// Returns the degree value. (Performs a conversion)
  constexpr Type GetDegree() const; // [tested]

  /// Returns the radian value. (No need for any conversion)
  constexpr Type GetRadian() const; // [tested]


  /// Sets the radian value. (No need for any conversion)
  W_ALWAYS_INLINE void SetRadian(Type fRad) { m_fRadian = fRad; };

  /// Brings the angle into the range of 0 degree - 360 degree
  /// \see GetNormalizedRange()
  void NormalizeRange(); // [tested]

  /// Returns an equivalent angle with range between 0 degree - 360 degree
  /// \see NormalizeRange()
  WAngleTemplate<Type> GetNormalizedRange() const; // [tested]

  /// Computes the smallest angle between the two given angles. The angle will always be a positive value.
  /// \note The two angles must be in the same range. E.g. they should be either normalized or at least the absolute angle between them should not be
  /// more than 180 degree.
  constexpr static WAngleTemplate<Type> AngleBetween(WAngleTemplate<Type> a, WAngleTemplate<Type> b); // [tested]

  /// Equality check with epsilon. Simple check without normalization. 360 degree will equal 0 degree, but 720 will not.
  bool IsEqualSimple(WAngleTemplate<Type> rhs, WAngleTemplate<Type> epsilon) const; // [tested]

  /// Equality check with epsilon that uses normalized angles. Will recognize 720 degree == 0 degree.
  bool IsEqualNormalized(WAngleTemplate<Type> rhs, WAngleTemplate<Type> epsilon) const; // [tested]

  // unary operators
  constexpr WAngleTemplate<Type> operator-() const; // [tested]

  // arithmetic operators
  constexpr WAngleTemplate<Type> operator+(WAngleTemplate<Type> r) const; // [tested]
  constexpr WAngleTemplate<Type> operator-(WAngleTemplate<Type> r) const; // [tested]

  // compound assignment operators
  void operator+=(WAngleTemplate<Type> r); // [tested]
  void operator-=(WAngleTemplate<Type> r); // [tested]

  // comparison
  constexpr bool operator==(const WAngleTemplate<Type>& r) const; // [tested]
  constexpr bool operator!=(const WAngleTemplate<Type>& r) const; // [tested]

  // At least the < operator is implement to make clamping etc. work
  constexpr bool operator<(const WAngleTemplate<Type>& r) const;
  constexpr bool operator>(const WAngleTemplate<Type>& r) const;
  constexpr bool operator<=(const WAngleTemplate<Type>& r) const;
  constexpr bool operator>=(const WAngleTemplate<Type>& r) const;

  // Note: relational operators on angles are not really possible - is 0 degree smaller or bigger than 359 degree?

private:

  /// The WRadian value
  Type m_fRadian;

  /// Preventing an include circle by defining pi again (annoying, but unlikely to change ;)). Normally you should use WMath::Pi<Type>()
  constexpr static Type Pi();
};

// Mathematical operators with Type

/// Returns f times angle a.
template <typename Type>
constexpr WAngleTemplate<Type> operator*(const WAngleTemplate<Type>& a, Type f); // [tested]
/// Returns f times angle a.
template <typename Type>
constexpr WAngleTemplate<Type> operator*(Type f, const WAngleTemplate<Type>& a); // [tested]

/// Returns the angle a divided by f.
template <typename Type>
constexpr WAngleTemplate<Type> operator/(const WAngleTemplate<Type>& a, Type f); // [tested]
/// Returns the fraction of angle a divided by angle b.
template <typename Type>
constexpr Type operator/(const WAngleTemplate<Type>& a, const WAngleTemplate<Type>& b); // [tested]

#include <Foundation/Math/Math.h>

#include <Foundation/Math/Implementation/Angle_inl.h>
