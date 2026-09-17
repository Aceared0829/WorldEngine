#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>

W_CREATE_SIMPLE_TEST(Time, Timestamp)
{
  const WInt64 iFirstContactUnixTimeInSeconds = 2942956800LL;

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructors / Valid Check")
  {
    WTimestamp invalidTimestamp;
    W_TEST_BOOL(!invalidTimestamp.IsValid());

    WTimestamp validTimestamp = WTimestamp::MakeFromInt(0, WSIUnitOfTime::Second);
    W_TEST_BOOL(validTimestamp.IsValid());
    validTimestamp = WTimestamp::MakeInvalid();
    W_TEST_BOOL(!validTimestamp.IsValid());

    WTimestamp currentTimestamp = WTimestamp::CurrentTimestamp();
    // Kind of hard to hit a moving target, let's just test if it is in a probable range.
    W_TEST_BOOL(currentTimestamp.IsValid());
    W_TEST_BOOL_MSG(currentTimestamp.GetInt64(WSIUnitOfTime::Second) > 1384597970LL, "The current time is before this test was written!");
    W_TEST_BOOL_MSG(currentTimestamp.GetInt64(WSIUnitOfTime::Second) < 32531209845LL,
      "This current time is after the year 3000! If this is actually the case, please fix this test.");

    // Sleep for 10 milliseconds
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    W_TEST_BOOL_MSG(currentTimestamp.GetInt64(WSIUnitOfTime::Microsecond) < WTimestamp::CurrentTimestamp().GetInt64(WSIUnitOfTime::Microsecond),
      "Sleeping for 10 ms should cause the timestamp to change!");
    W_TEST_BOOL_MSG(!currentTimestamp.Compare(WTimestamp::CurrentTimestamp(), WTimestamp::CompareMode::Identical),
      "Sleeping for 10 ms should cause the timestamp to change!");

    // a valid timestamp should always be 'newer' than an invalid one
    W_TEST_BOOL(currentTimestamp.Compare(WTimestamp::MakeInvalid(), WTimestamp::CompareMode::Newer) == true);
    // an invalid timestamp should not be 'newer' than any valid one
    W_TEST_BOOL(WTimestamp::MakeInvalid().Compare(currentTimestamp, WTimestamp::CompareMode::Newer) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Public Accessors")
  {
    const WTimestamp epoch = WTimestamp::MakeFromInt(0, WSIUnitOfTime::Second);
    const WTimestamp firstContact = WTimestamp::MakeFromInt(iFirstContactUnixTimeInSeconds, WSIUnitOfTime::Second);
    W_TEST_BOOL(epoch.IsValid());
    W_TEST_BOOL(firstContact.IsValid());

    // GetInt64 / SetInt64
    WTimestamp firstContactTest = WTimestamp::MakeFromInt(iFirstContactUnixTimeInSeconds, WSIUnitOfTime::Second);
    W_TEST_INT(firstContactTest.GetInt64(WSIUnitOfTime::Second), iFirstContactUnixTimeInSeconds);
    W_TEST_INT(firstContactTest.GetInt64(WSIUnitOfTime::Millisecond), iFirstContactUnixTimeInSeconds * 1000LL);
    W_TEST_INT(firstContactTest.GetInt64(WSIUnitOfTime::Microsecond), iFirstContactUnixTimeInSeconds * 1000000LL);
    W_TEST_INT(firstContactTest.GetInt64(WSIUnitOfTime::Nanosecond), iFirstContactUnixTimeInSeconds * 1000000000LL);

    firstContactTest = WTimestamp::MakeFromInt(firstContactTest.GetInt64(WSIUnitOfTime::Second), WSIUnitOfTime::Second);
    W_TEST_BOOL(firstContactTest.Compare(firstContact, WTimestamp::CompareMode::Identical));
    firstContactTest = WTimestamp::MakeFromInt(firstContactTest.GetInt64(WSIUnitOfTime::Millisecond), WSIUnitOfTime::Millisecond);
    W_TEST_BOOL(firstContactTest.Compare(firstContact, WTimestamp::CompareMode::Identical));
    firstContactTest = WTimestamp::MakeFromInt(firstContactTest.GetInt64(WSIUnitOfTime::Microsecond), WSIUnitOfTime::Microsecond);
    W_TEST_BOOL(firstContactTest.Compare(firstContact, WTimestamp::CompareMode::Identical));
    firstContactTest = WTimestamp::MakeFromInt(firstContactTest.GetInt64(WSIUnitOfTime::Nanosecond), WSIUnitOfTime::Nanosecond);
    W_TEST_BOOL(firstContactTest.Compare(firstContact, WTimestamp::CompareMode::Identical));

    // IsEqual
    const WTimestamp firstContactPlusAFewMicroseconds = WTimestamp::MakeFromInt(firstContact.GetInt64(WSIUnitOfTime::Microsecond) + 42, WSIUnitOfTime::Microsecond);
    W_TEST_BOOL(firstContact.Compare(firstContactPlusAFewMicroseconds, WTimestamp::CompareMode::FileTimeEqual));
    W_TEST_BOOL(!firstContact.Compare(firstContactPlusAFewMicroseconds, WTimestamp::CompareMode::Identical));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operators")
  {
    const WTimestamp firstContact = WTimestamp::MakeFromInt(iFirstContactUnixTimeInSeconds, WSIUnitOfTime::Second);

    // Time span arithmetics
    const WTime timeSpan1000s = WTime::MakeFromSeconds(1000);
    W_TEST_BOOL(timeSpan1000s.GetMicroseconds() == 1000000000LL);

    // operator +
    const WTimestamp firstContactPlus1000s = firstContact + timeSpan1000s;
    WInt64 iSpanDiff = firstContactPlus1000s.GetInt64(WSIUnitOfTime::Microsecond) - firstContact.GetInt64(WSIUnitOfTime::Microsecond);
    W_TEST_BOOL(iSpanDiff == 1000000000LL);
    // You can only subtract points in time
    W_TEST_BOOL(firstContactPlus1000s - firstContact == timeSpan1000s);

    const WTimestamp T1000sPlusFirstContact = timeSpan1000s + firstContact;
    iSpanDiff = T1000sPlusFirstContact.GetInt64(WSIUnitOfTime::Microsecond) - firstContact.GetInt64(WSIUnitOfTime::Microsecond);
    W_TEST_BOOL(iSpanDiff == 1000000000LL);
    // You can only subtract points in time
    W_TEST_BOOL(T1000sPlusFirstContact - firstContact == timeSpan1000s);

    // operator -
    const WTimestamp firstContactMinus1000s = firstContact - timeSpan1000s;
    iSpanDiff = firstContactMinus1000s.GetInt64(WSIUnitOfTime::Microsecond) - firstContact.GetInt64(WSIUnitOfTime::Microsecond);
    W_TEST_BOOL(iSpanDiff == -1000000000LL);
    // You can only subtract points in time
    W_TEST_BOOL(firstContact - firstContactMinus1000s == timeSpan1000s);


    // operator += / -=
    WTimestamp testTimestamp = firstContact;
    testTimestamp += timeSpan1000s;
    W_TEST_BOOL(testTimestamp.Compare(firstContactPlus1000s, WTimestamp::CompareMode::Identical));
    testTimestamp -= timeSpan1000s;
    W_TEST_BOOL(testTimestamp.Compare(firstContact, WTimestamp::CompareMode::Identical));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WDateTime conversion")
  {
    // Constructor
    WDateTime invalidDateTime;
    W_TEST_BOOL(!invalidDateTime.GetTimestamp().IsValid());

    const WTimestamp firstContact = WTimestamp::MakeFromInt(iFirstContactUnixTimeInSeconds, WSIUnitOfTime::Second);
    WDateTime firstContactDataTime = WDateTime::MakeFromTimestamp(firstContact);

    // Getter
    W_TEST_INT(firstContactDataTime.GetYear(), 2063);
    W_TEST_INT(firstContactDataTime.GetMonth(), 4);
    W_TEST_INT(firstContactDataTime.GetDay(), 5);
    W_TEST_BOOL(firstContactDataTime.GetDayOfWeek() == 4 ||
                 firstContactDataTime.GetDayOfWeek() == 255); // not supported on all platforms, should output 255 then
    W_TEST_INT(firstContactDataTime.GetHour(), 0);
    W_TEST_INT(firstContactDataTime.GetMinute(), 0);
    W_TEST_INT(firstContactDataTime.GetSecond(), 0);
    W_TEST_INT(firstContactDataTime.GetMicroseconds(), 0);

    // SetTimestamp / GetTimestamp
    WTimestamp currentTimestamp = WTimestamp::CurrentTimestamp();
    WDateTime currentDateTime;
    currentDateTime.SetFromTimestamp(currentTimestamp).AssertSuccess();
    WTimestamp currentTimestamp2 = currentDateTime.GetTimestamp();
    // OS date time functions should be accurate within one second.
    WInt64 iDiff = WMath::Abs(currentTimestamp.GetInt64(WSIUnitOfTime::Microsecond) - currentTimestamp2.GetInt64(WSIUnitOfTime::Microsecond));
    W_TEST_BOOL(iDiff <= 1000000);

    // Setter
    WDateTime oneSmallStep;
    oneSmallStep.SetYear(1969);
    oneSmallStep.SetMonth(7);
    oneSmallStep.SetDay(21);
    oneSmallStep.SetDayOfWeek(1);
    oneSmallStep.SetHour(2);
    oneSmallStep.SetMinute(56);
    oneSmallStep.SetSecond(0);
    oneSmallStep.SetMicroseconds(0);

    WTimestamp oneSmallStepTimestamp = oneSmallStep.GetTimestamp();
    W_TEST_BOOL(oneSmallStepTimestamp.IsValid());
    W_TEST_INT(oneSmallStepTimestamp.GetInt64(WSIUnitOfTime::Second), -14159040LL);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WDateTime formatting")
  {
    WDateTime dateTime;

    dateTime.SetYear(2019);
    dateTime.SetMonth(8);
    dateTime.SetDay(16);
    dateTime.SetDayOfWeek(5);
    dateTime.SetHour(13);
    dateTime.SetMinute(40);
    dateTime.SetSecond(30);
    dateTime.SetMicroseconds(345678);

    char szTimestampFormatted[256] = "";

    // no names, no UTC, no milliseconds
    BuildString(szTimestampFormatted, 256, WArgDateTime(dateTime, WArgDateTime::Default));
    W_TEST_STRING("2019-08-16 - 13:40:30", szTimestampFormatted);
    // no names, no UTC, with milliseconds
    BuildString(szTimestampFormatted, 256, WArgDateTime(dateTime, WArgDateTime::Default | WArgDateTime::ShowMilliseconds));
    W_TEST_STRING("2019-08-16 - 13:40:30.345", szTimestampFormatted);
    // no names, with UTC, no milliseconds
    BuildString(szTimestampFormatted, 256, WArgDateTime(dateTime, WArgDateTime::Default | WArgDateTime::ShowTimeZone));
    W_TEST_STRING("2019-08-16 - 13:40:30 (UTC)", szTimestampFormatted);
    // no names, with UTC, with milliseconds
    BuildString(szTimestampFormatted, 256, WArgDateTime(dateTime, WArgDateTime::ShowDate | WArgDateTime::ShowMilliseconds | WArgDateTime::ShowTimeZone));
    W_TEST_STRING("2019-08-16 - 13:40:30.345 (UTC)", szTimestampFormatted);
    // with names, no UTC, no milliseconds
    BuildString(szTimestampFormatted, 256, WArgDateTime(dateTime, WArgDateTime::DefaultTextual | WArgDateTime::ShowWeekday));
    W_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30", szTimestampFormatted);
    // no names, no UTC, with milliseconds
    BuildString(szTimestampFormatted, 256,
      WArgDateTime(dateTime, WArgDateTime::DefaultTextual | WArgDateTime::ShowWeekday | WArgDateTime::ShowMilliseconds));
    W_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30.345", szTimestampFormatted);
    // no names, with UTC, no milliseconds
    BuildString(szTimestampFormatted, 256, WArgDateTime(dateTime, WArgDateTime::DefaultTextual | WArgDateTime::ShowWeekday | WArgDateTime::ShowTimeZone));
    W_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30 (UTC)", szTimestampFormatted);
    // no names, with UTC, with milliseconds
    BuildString(szTimestampFormatted, 256, WArgDateTime(dateTime, WArgDateTime::DefaultTextual | WArgDateTime::ShowWeekday | WArgDateTime::ShowMilliseconds | WArgDateTime::ShowTimeZone));
    W_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30.345 (UTC)", szTimestampFormatted);

    BuildString(szTimestampFormatted, 256, WArgDateTime(dateTime, WArgDateTime::ShowDate));
    W_TEST_STRING("2019-08-16", szTimestampFormatted);
    BuildString(szTimestampFormatted, 256, WArgDateTime(dateTime, WArgDateTime::TextualDate));
    W_TEST_STRING("2019 Aug 16", szTimestampFormatted);
    BuildString(szTimestampFormatted, 256, WArgDateTime(dateTime, WArgDateTime::ShowTime));
    W_TEST_STRING("13:40", szTimestampFormatted);
    BuildString(szTimestampFormatted, 256, WArgDateTime(dateTime, WArgDateTime::ShowWeekday | WArgDateTime::ShowMilliseconds));
    W_TEST_STRING("(Fri) - 13:40:30.345", szTimestampFormatted);
  }
}
