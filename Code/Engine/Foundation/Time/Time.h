#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/StaticSubSystem.h>

/// The time class encapsulates a double value storing the time in seconds.
///
/// It offers convenient functions to get the time in other units.
/// WTime is a high-precision time using the OS specific high-precision timing functions
/// and may thus be used for profiling as well as simulation code.
struct W_FOUNDATION_DLL WTime
{
public:
  /// Gets the current time
  static WTime Now(); // [tested]

  /// Creates an instance of WTime that was initialized from nanoseconds.
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime MakeFromNanoseconds(double fNanoseconds) { return WTime(fNanoseconds * 0.000000001); }
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime Nanoseconds(double fNanoseconds) { return WTime(fNanoseconds * 0.000000001); }

  /// Creates an instance of WTime that was initialized from microseconds.
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime MakeFromMicroseconds(double fMicroseconds) { return WTime(fMicroseconds * 0.000001); }
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime Microseconds(double fMicroseconds) { return WTime(fMicroseconds * 0.000001); }

  /// Creates an instance of WTime that was initialized from milliseconds.
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime MakeFromMilliseconds(double fMilliseconds) { return WTime(fMilliseconds * 0.001); }
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime Milliseconds(double fMilliseconds) { return WTime(fMilliseconds * 0.001); }

  /// Creates an instance of WTime that was initialized from seconds.
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime MakeFromSeconds(double fSeconds) { return WTime(fSeconds); }
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime Seconds(double fSeconds) { return WTime(fSeconds); }

  /// Creates an instance of WTime that was initialized from minutes.
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime MakeFromMinutes(double fMinutes) { return WTime(fMinutes * 60); }
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime Minutes(double fMinutes) { return WTime(fMinutes * 60); }

  /// Creates an instance of WTime that was initialized from hours.
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime MakeFromHours(double fHours) { return WTime(fHours * 60 * 60); }
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime Hours(double fHours) { return WTime(fHours * 60 * 60); }

  /// Creates an instance of WTime that was initialized with zero.
  [[nodiscard]] W_ALWAYS_INLINE constexpr static WTime MakeZero() { return WTime(0.0); }

  W_DECLARE_POD_TYPE();

  /// The default constructor sets the time to zero.
  W_ALWAYS_INLINE constexpr WTime() = default;

  /// Returns true if the stored time is exactly zero. That typically means the value was not changed from the default.
  W_ALWAYS_INLINE constexpr bool IsZero() const { return m_fTime == 0.0; }

  /// Checks for a negative time value.
  W_ALWAYS_INLINE constexpr bool IsNegative() const { return m_fTime < 0.0; }

  /// Checks for a positive time value. This does not include zero.
  W_ALWAYS_INLINE constexpr bool IsPositive() const { return m_fTime > 0.0; }

  /// Returns true if the stored time is zero or negative.
  W_ALWAYS_INLINE constexpr bool IsZeroOrNegative() const { return m_fTime <= 0.0; }

  /// Returns true if the stored time is zero or positive.
  W_ALWAYS_INLINE constexpr bool IsZeroOrPositive() const { return m_fTime >= 0.0; }

  /// Returns the time as a float value (in seconds).
  ///
  /// Useful for simulation time steps etc.
  /// Please note that it is not recommended to use the float value for long running
  /// time calculations since the precision can deteriorate quickly. (Only use for delta times is recommended)
  constexpr float AsFloatInSeconds() const;

  /// Returns the nanoseconds value
  constexpr double GetNanoseconds() const;

  /// Returns the microseconds value
  constexpr double GetMicroseconds() const;

  /// Returns the milliseconds value
  constexpr double GetMilliseconds() const;

  /// Returns the seconds value.
  constexpr double GetSeconds() const;

  /// Returns the minutes value.
  constexpr double GetMinutes() const;

  /// Returns the hours value.
  constexpr double GetHours() const;

  /// Subtracts the time value of "other" from this instances value.
  constexpr void operator-=(const WTime& other);

  /// Adds the time value of "other" to this instances value.
  constexpr void operator+=(const WTime& other);

  /// Multiplies the time by the given factor
  constexpr void operator*=(double fFactor);

  /// Divides the time by the given factor
  constexpr void operator/=(double fFactor);

  /// Returns the difference: "this instance - other"
  constexpr WTime operator-(const WTime& other) const;

  /// Returns the sum: "this instance + other"
  constexpr WTime operator+(const WTime& other) const;

  constexpr WTime operator-() const;

  constexpr bool operator<(const WTime& rhs) const { return m_fTime < rhs.m_fTime; }
  constexpr bool operator<=(const WTime& rhs) const { return m_fTime <= rhs.m_fTime; }
  constexpr bool operator>(const WTime& rhs) const { return m_fTime > rhs.m_fTime; }
  constexpr bool operator>=(const WTime& rhs) const { return m_fTime >= rhs.m_fTime; }
  constexpr bool operator==(const WTime& rhs) const { return m_fTime == rhs.m_fTime; }
  constexpr bool operator!=(const WTime& rhs) const { return m_fTime != rhs.m_fTime; }

private:
  /// For internal use only.
  constexpr explicit WTime(double fTime);

  /// The time is stored in seconds
  double m_fTime = 0.0;

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Time);

  static void Initialize();
};

constexpr WTime operator*(const WTime& t, double f);
constexpr WTime operator*(double f, const WTime& t);
constexpr WTime operator*(const WTime& f, const WTime& t); // not physically correct, but useful (should result in seconds squared)

constexpr WTime operator/(const WTime& t, double f);
constexpr WTime operator/(double f, const WTime& t);
constexpr WTime operator/(const WTime& f, const WTime& t); // not physically correct, but useful (should result in a value without a unit)


#include <Foundation/Time/Implementation/Time_inl.h>
