#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>

struct WSIUnitOfTime
{
  enum Enum
  {
    Nanosecond,  ///< SI-unit of time (10^-9 second)
    Microsecond, ///< SI-unit of time (10^-6 second)
    Millisecond, ///< SI-unit of time (10^-3 second)
    Second,      ///< SI-unit of time (base unit)
  };
};

/// The timestamp class encapsulates a date in time as microseconds since Unix epoch.
///
/// The value is represented by an WInt64 and allows storing time stamps from roughly
/// -291030 BC to 293970 AC.
/// Use this class to efficiently store a timestamp that is valid across platforms.
class W_FOUNDATION_DLL WTimestamp
{
public:
  struct CompareMode
  {
    enum Enum
    {
      FileTimeEqual, ///< Uses a resolution that guarantees that a file's timestamp is considered equal on all platforms.
      Identical,     ///< Uses maximal stored resolution.
      Newer,         ///< Just compares values and returns true if the left-hand side is larger than the right hand side
    };
  };
  ///  Returns the current timestamp. Returned value will always be valid.
  ///
  /// Depending on the platform the precision varies between seconds and nanoseconds.
  static const WTimestamp CurrentTimestamp(); // [tested]

  W_DECLARE_POD_TYPE();

  // *** Constructors ***
public:
  /// Creates an invalidated timestamp.
  WTimestamp(); // [tested]

  /// Returns an invalid timestamp
  [[nodiscard]] static WTimestamp MakeInvalid() { return WTimestamp(); }

  /// Returns a timestamp initialized from 'iTimeValue' in 'unitOfTime' since Unix epoch.
  [[nodiscard]] static WTimestamp MakeFromInt(WInt64 iTimeValue, WSIUnitOfTime::Enum unitOfTime);

  // *** Public Functions ***
public:
  /// Returns whether the timestamp is valid.
  bool IsValid() const; // [tested]

  /// Returns the number of 'unitOfTime' since Unix epoch.
  WInt64 GetInt64(WSIUnitOfTime::Enum unitOfTime) const; // [tested]

  /// Returns whether this timestamp is considered equal to 'rhs' in the given mode.
  ///
  /// Use CompareMode::FileTime when working with file time stamps across platforms.
  /// It will use the lowest resolution supported by all platforms to make sure the
  /// timestamp of a file is considered equal regardless on which platform it was retrieved.
  bool Compare(const WTimestamp& rhs, CompareMode::Enum mode) const; // [tested]

  // *** Operators ***
public:
  /// Adds the time value of "timeSpan" to this data value.
  void operator+=(const WTime& timeSpan); // [tested]

  /// Subtracts the time value of "timeSpan" from this date value.
  void operator-=(const WTime& timeSpan); // [tested]

  /// Returns the time span between this timestamp and "other".
  const WTime operator-(const WTimestamp& other) const; // [tested]

  /// Returns a timestamp that is "timeSpan" further into the future from this timestamp.
  const WTimestamp operator+(const WTime& timeSpan) const; // [tested]

  /// Returns a timestamp that is "timeSpan" further into the past from this timestamp.
  const WTimestamp operator-(const WTime& timeSpan) const; // [tested]


private:
  static constexpr const WInt64 W_INVALID_TIME_STAMP = WMath::MinValue<WInt64>();

  W_ALLOW_PRIVATE_PROPERTIES(WTimestamp);
  /// The date is stored as microseconds since Unix epoch.
  WInt64 m_iTimestamp = W_INVALID_TIME_STAMP;
};

/// Returns a timestamp that is "timeSpan" further into the future from "timestamp".
const WTimestamp operator+(WTime& ref_timeSpan, const WTimestamp& timestamp);

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WTimestamp);

/// The WDateTime class can be used to convert WTimestamp into a human readable form.
///
/// Note: As WTimestamp is microseconds since Unix epoch, the values in this class will always be
/// in UTC.
class W_FOUNDATION_DLL WDateTime
{
public:
  /// Creates an empty date time instance with an invalid date.
  ///
  /// Day, Month and Year will be invalid and must be set.
  WDateTime(); // [tested]
  ~WDateTime();

  /// Checks whether all values are within valid ranges.
  bool IsValid() const;

  /// Returns a date time that is all zero.
  [[nodiscard]] static WDateTime MakeZero() { return WDateTime(); }

  /// Sets this instance to the given timestamp.
  ///
  /// This calls SetFromTimestamp() internally and asserts that the conversion succeeded.
  /// Use SetFromTimestamp() directly, if you need to be able to react to invalid data.
  [[nodiscard]] static WDateTime MakeFromTimestamp(WTimestamp timestamp);

  /// Converts this instance' values into a WTimestamp.
  ///
  /// The conversion is done via the OS and can fail for values that are outside the supported range.
  /// In this case, the returned value will be invalid. Anything after 1970 and before the
  /// not so distant future should be safe.
  [[nodiscard]] const WTimestamp GetTimestamp() const; // [tested]

  /// Sets this instance to the given timestamp.
  ///
  /// The conversion is done via the OS and will fail for invalid dates and values outside the supported range,
  /// in which case W_FAILURE will be returned.
  /// Anything after 1970 and before the not so distant future should be safe.
  WResult SetFromTimestamp(WTimestamp timestamp);

  // *** Accessors ***
public:
  /// Returns the currently set year.
  WUInt32 GetYear() const; // [tested]

  /// Sets the year to the given value.
  void SetYear(WInt16 iYear); // [tested]

  /// Returns the currently set month.
  WUInt8 GetMonth() const; // [tested]

  /// Sets the month to the given value. Asserts that the value is in the valid range [1, 12].
  void SetMonth(WUInt8 uiMonth); // [tested]

  /// Returns the currently set day.
  WUInt8 GetDay() const; // [tested]

  /// Sets the day to the given value. Asserts that the value is in the valid range [1, 31].
  void SetDay(WUInt8 uiDay); // [tested]

  /// Returns the currently set day of week.
  WUInt8 GetDayOfWeek() const;

  /// Sets the day of week to the given value. Asserts that the value is in the valid range [0, 6].
  void SetDayOfWeek(WUInt8 uiDayOfWeek);

  /// Returns the currently set hour.
  WUInt8 GetHour() const; // [tested]

  /// Sets the hour to the given value. Asserts that the value is in the valid range [0, 23].
  void SetHour(WUInt8 uiHour); // [tested]

  /// Returns the currently set minute.
  WUInt8 GetMinute() const; // [tested]

  /// Sets the minute to the given value. Asserts that the value is in the valid range [0, 59].
  void SetMinute(WUInt8 uiMinute); // [tested]

  /// Returns the currently set second.
  WUInt8 GetSecond() const; // [tested]

  /// Sets the second to the given value. Asserts that the value is in the valid range [0, 59].
  void SetSecond(WUInt8 uiSecond); // [tested]

  /// Returns the currently set microseconds.
  WUInt32 GetMicroseconds() const; // [tested]

  /// Sets the microseconds to the given value. Asserts that the value is in the valid range [0, 999999].
  void SetMicroseconds(WUInt32 uiMicroSeconds); // [tested]

private:
  /// The fraction of a second in microseconds of this date [0, 999999].
  WUInt32 m_uiMicroseconds = 0;
  /// The year of this date [-32k, +32k].
  WInt16 m_iYear = 0;
  /// The month of this date [1, 12].
  WUInt8 m_uiMonth = 0;
  /// The day of this date [1, 31].
  WUInt8 m_uiDay = 0;
  /// The day of week of this date [0, 6].
  WUInt8 m_uiDayOfWeek = 0;
  /// The hour of this date [0, 23].
  WUInt8 m_uiHour = 0;
  /// The number of minutes of this date [0, 59].
  WUInt8 m_uiMinute = 0;
  /// The number of seconds of this date [0, 59].
  WUInt8 m_uiSecond = 0;
};

W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WDateTime& arg);

struct WArgDateTime
{
  enum FormattingFlags
  {
    ShowDate = W_BIT(0),
    TextualDate = ShowDate | W_BIT(1),
    ShowWeekday = W_BIT(2),
    ShowTime = W_BIT(3),
    ShowSeconds = ShowTime | W_BIT(4),
    ShowMilliseconds = ShowSeconds | W_BIT(5),
    ShowTimeZone = W_BIT(6),

    Default = ShowDate | ShowSeconds,
    DefaultTextual = TextualDate | ShowSeconds,
  };

  /// Initialized a formatting object for an WDateTime instance.
  /// \param dateTime The WDateTime instance to format.
  /// \param bUseNames Indicates whether to use names for days of week and months (true)
  ///        or a purely numerical representation (false).
  /// \param bShowTimeZoneIndicator Whether to indicate the timezone of the WDateTime object.
  inline explicit WArgDateTime(const WDateTime& dateTime, WUInt32 uiFormattingFlags = Default)
    : m_Value(dateTime)
    , m_uiFormattingFlags(uiFormattingFlags)
  {
  }

  WDateTime m_Value;
  WUInt32 m_uiFormattingFlags;
};

W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgDateTime& arg);

#include <Foundation/Time/Implementation/Timestamp_inl.h>
