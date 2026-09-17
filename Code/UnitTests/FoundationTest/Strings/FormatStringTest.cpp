#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>

#include <Foundation/Types/ScopeExit.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>
#include <stdarg.h>

void TestFormat(const WFormatString& str, const char* szExpected)
{
  WStringBuilder sb;
  WStringView szText = str.GetText(sb);

  W_TEST_STRING(szText, szExpected);
}

void TestFormatWChar(const WFormatString& str, const wchar_t* pExpected)
{
  WStringBuilder sb;
  WStringView szText = str.GetText(sb);

  W_TEST_WSTRING(WStringWChar(szText), pExpected);
}

void CompareSnprintf(WStringBuilder& ref_sLog, const WFormatString& str, const char* szFormat, ...)
{
  va_list args;
  va_start(args, szFormat);

  char Temp1[256];
  char Temp2[256];

  // reusing args list crashes on GCC / Clang
  WStringUtils::vsnprintf(Temp1, 256, szFormat, args);
  vsnprintf(Temp2, 256, szFormat, args);
  W_TEST_STRING(Temp1, Temp2);

  WTime t1, t2, t3;
  WStopwatch sw;
  {
    sw.StopAndReset();

    for (WUInt32 i = 0; i < 10000; ++i)
    {
      WStringUtils::vsnprintf(Temp1, 256, szFormat, args);
    }

    t1 = sw.Checkpoint();
  }

  {
    sw.StopAndReset();

    for (WUInt32 i = 0; i < 10000; ++i)
    {
      vsnprintf(Temp2, 256, szFormat, args);
    }

    t2 = sw.Checkpoint();
  }

  {
    WStringBuilder sb;

    sw.StopAndReset();
    for (WUInt32 i = 0; i < 10000; ++i)
    {
      WStringView sText = str.GetText(sb);
    }

    t3 = sw.Checkpoint();
  }

  ref_sLog.AppendFormat("W: {0} msec, std: {1} msec, WFmt: {2} msec : {3} -> {4}\n", WArgF(t1.GetMilliseconds(), 2), WArgF(t2.GetMilliseconds(), 2),
    WArgF(t3.GetMilliseconds(), 2), szFormat, Temp1);

  va_end(args);
}

W_CREATE_SIMPLE_TEST(Strings, FormatString)
{
  WStringBuilder perfLog;

  W_TEST_BLOCK(WTestBlock::Enabled, "Basics")
  {
    const char* tmp = "stringviewstuff";

    const char* sz = "sz";
    WString string = "string";
    WStringBuilder sb = "builder";
    WStringView sv(tmp + 6, tmp + 10);

    TestFormat(WFmt("{0}, {1}, {2}, {3}", WInt8(-1), WInt16(-2), WInt32(-3), WInt64(-4)), "-1, -2, -3, -4");
    TestFormat(WFmt("{0}, {1}, {2}, {3}", WUInt8(1), WUInt16(2), WUInt32(3), WUInt64(4)), "1, 2, 3, 4");

    TestFormat(WFmt("{0}, {1}", WArgHumanReadable(0ll), WArgHumanReadable(1ll)), "0, 1");
    TestFormat(WFmt("{0}, {1}", WArgHumanReadable(-0ll), WArgHumanReadable(-1ll)), "0, -1");
    TestFormat(WFmt("{0}, {1}", WArgHumanReadable(999ll), WArgHumanReadable(1000ll)), "999, 1.00K");
    TestFormat(WFmt("{0}, {1}", WArgHumanReadable(-999ll), WArgHumanReadable(-1000ll)), "-999, -1.00K");
    // 999.999 gets rounded up for precision 2, so result is 1000.00K not 999.99K
    TestFormat(WFmt("{0}, {1}", WArgHumanReadable(999'999ll), WArgHumanReadable(1'000'000ll)), "1000.00K, 1.00M");
    TestFormat(WFmt("{0}, {1}", WArgHumanReadable(-999'999ll), WArgHumanReadable(-1'000'000ll)), "-1000.00K, -1.00M");

    TestFormat(WFmt("{0}, {1}", WArgFileSize(0u), WArgFileSize(1u)), "0B, 1B");
    TestFormat(WFmt("{0}, {1}", WArgFileSize(1023u), WArgFileSize(1024u)), "1023B, 1.00KB");
    // 1023.999 gets rounded up for precision 2, so result is 1024.00KB not 1023.99KB
    TestFormat(WFmt("{0}, {1}", WArgFileSize(1024u * 1024u - 1u), WArgFileSize(1024u * 1024u)), "1024.00KB, 1.00MB");

    const char* const suffixes[] = {" Foo", " Bar", " Foobar"};
    const WUInt32 suffixCount = W_ARRAY_SIZE(suffixes);
    TestFormat(WFmt("{0}", WArgHumanReadable(0ll, 25u, suffixes, suffixCount)), "0 Foo");
    TestFormat(WFmt("{0}", WArgHumanReadable(25ll, 25u, suffixes, suffixCount)), "1.00 Bar");
    TestFormat(WFmt("{0}", WArgHumanReadable(25ll * 25ll * 2ll, 25u, suffixes, suffixCount)), "2.00 Foobar");

    TestFormat(WFmt("{0}", WArgHumanReadable(-0ll, 25u, suffixes, suffixCount)), "0 Foo");
    TestFormat(WFmt("{0}", WArgHumanReadable(-25ll, 25u, suffixes, suffixCount)), "-1.00 Bar");
    TestFormat(WFmt("{0}", WArgHumanReadable(-25ll * 25ll * 2ll, 25u, suffixes, suffixCount)), "-2.00 Foobar");

    TestFormat(WFmt("'{0}, {1}'", "inl", sz), "'inl, sz'");
    TestFormat(WFmt("'{0}'", string), "'string'");
    TestFormat(WFmt("'{0}'", sb), "'builder'");
    TestFormat(WFmt("'{0}'", sv), "'view'");

    TestFormat(WFmt("{3}, {1}, {0}, {2}", WArgF(23.12345f, 1), WArgI(42), 17, 12.34f), "12.34, 42, 23.1, 17");

    const wchar_t* wsz = L"wsz";
    TestFormatWChar(WFmt("'{0}, {1}'", "inl", wsz), L"'inl, wsz'");
    TestFormatWChar(WFmt("'{0}, {1}'", L"inl", wsz), L"'inl, wsz'");
    // Temp buffer limit is 63 byte (64 including trailing zero). Each character in UTF-8 can potentially use 4 byte.
    // All input characters are 1 byte, so the 60th character is the last with 4 bytes left in the buffer.
    // Thus we end up with truncation after 60 characters.
    const wchar_t* wszTooLong = L"123456789.123456789.123456789.123456789.123456789.123456789.WAAAAAAAAAAAAAAH";
    const wchar_t* wszTooLongExpected = L"123456789.123456789.123456789.123456789.123456789.123456789.";
    const wchar_t* wszTooLongExpected2 =
      L"'123456789.123456789.123456789.123456789.123456789.123456789., 123456789.123456789.123456789.123456789.123456789.123456789.'";
    TestFormatWChar(WFmt("{0}", wszTooLong), wszTooLongExpected);
    TestFormatWChar(WFmt("'{0}, {1}'", wszTooLong, wszTooLong), wszTooLongExpected2);
  }

  W_TEST_BLOCK(WTestBlock::DisabledNoWarning, "Compare Performance")
  {
    CompareSnprintf(perfLog, WFmt("Hello {0}, i = {1}, f = {2}", "World", 42, WArgF(3.141f, 2)), "Hello %s, i = %i, f = %.2f", "World", 42, 3.141f);
    CompareSnprintf(perfLog, WFmt("No formatting at all"), "No formatting at all");
    CompareSnprintf(perfLog, WFmt("{0}, {1}, {2}, {3}, {4}", "AAAAAA", "BBBBBBB", "CCCCCC", "DDDDDDDDDDDDD", "EE"), "%s, %s, %s, %s, %s", "AAAAAA",
      "BBBBBBB", "CCCCCC", "DDDDDDDDDDDDD", "EE");
    CompareSnprintf(perfLog, WFmt("{0}", 23), "%i", 23);
    CompareSnprintf(perfLog, WFmt("{0}", 23.123456789), "%f", 23.123456789);
    CompareSnprintf(perfLog, WFmt("{0}", WArgF(23.123456789, 2)), "%.2f", 23.123456789);
    CompareSnprintf(perfLog, WFmt("{0}", WArgI(123456789, 20, true)), "%020i", 123456789);
    CompareSnprintf(perfLog, WFmt("{0}", WArgI(123456789, 20, true, 16)), "%020X", 123456789);
    CompareSnprintf(perfLog, WFmt("{0}", WArgU(1234567890987ll, 30, false, 16)), "%30llx", 1234567890987ll);
    CompareSnprintf(perfLog, WFmt("{0}", WArgU(1234567890987ll, 30, false, 16, true)), "%30llX", 1234567890987ll);
    CompareSnprintf(perfLog, WFmt("{0}, {1}, {2}, {3}, {4}", 0, 1, 2, 3, 4), "%i, %i, %i, %i, %i", 0, 1, 2, 3, 4);
    CompareSnprintf(perfLog, WFmt("{0}, {1}, {2}, {3}, {4}", 0.1, 1.1, 2.1, 3.1, 4.1), "%.1f, %.1f, %.1f, %.1f, %.1f", 0.1, 1.1, 2.1, 3.1, 4.1);
    CompareSnprintf(perfLog, WFmt("{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9),
      "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    CompareSnprintf(perfLog, WFmt("{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}", 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1),
      "%.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f", 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1);
    CompareSnprintf(perfLog, WFmt("{0}", WArgC('z')), "%c", 'z');

    CompareSnprintf(perfLog, WFmt("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9), "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i",
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

    // FILE* file = fopen("D:\\snprintf_perf.txt", "wb");
    // if (file)
    //{
    //  fwrite(perfLog.GetData(), 1, perfLog.GetElementCount(), file);
    //  fclose(file);
    //}
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Auto Increment")
  {
    TestFormat(WFmt("{}{}{}{}", WInt8(1), WInt16(2), WInt32(3), WInt64(4)), "1234");
    TestFormat(WFmt("{3}{2}{1}{0}", WInt8(1), WInt16(2), WInt32(3), WInt64(4)), "4321");

    TestFormat(WFmt("{}, {}, {}, {}", WInt8(-1), WInt16(-2), WInt32(-3), WInt64(-4)), "-1, -2, -3, -4");
    TestFormat(WFmt("{}, {}, {}, {}", WUInt8(1), WUInt16(2), WUInt32(3), WUInt64(4)), "1, 2, 3, 4");

    TestFormat(WFmt("{0}, {}, {}, {}", WUInt8(1), WUInt16(2), WUInt32(3), WUInt64(4)), "1, 2, 3, 4");

    TestFormat(WFmt("{1}, {}, {}, {}", WUInt8(1), WUInt16(2), WUInt32(3), WUInt64(4), WUInt64(5)), "2, 3, 4, 5");

    TestFormat(WFmt("{2}, {}, {1}, {}", WUInt8(1), WUInt16(2), WUInt32(3), WUInt64(4), WUInt64(5)), "3, 4, 2, 3");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WTime")
  {
    TestFormat(WFmt("{}", WTime()), "0ns");
    TestFormat(WFmt("{}", WTime::MakeFromNanoseconds(999)), "999ns");
    TestFormat(WFmt("{}", WTime::MakeFromNanoseconds(999.1)), "999.1ns");
    TestFormat(WFmt("{}", WTime::MakeFromMicroseconds(999)), (const char*)u8"999\u00B5s");     // Utf-8 encoding for the microsecond sign
    TestFormat(WFmt("{}", WTime::MakeFromMicroseconds(999.2)), (const char*)u8"999.2\u00B5s"); // Utf-8 encoding for the microsecond sign
    TestFormat(WFmt("{}", WTime::MakeFromMilliseconds(-999)), "-999ms");
    TestFormat(WFmt("{}", WTime::MakeFromMilliseconds(-999.3)), "-999.3ms");
    TestFormat(WFmt("{}", WTime::MakeFromSeconds(59)), "59sec");
    TestFormat(WFmt("{}", WTime::MakeFromSeconds(-59.9)), "-59.9sec");
    TestFormat(WFmt("{}", WTime::MakeFromSeconds(75)), "1min 15sec");
    TestFormat(WFmt("{}", WTime::MakeFromSeconds(-75.4)), "-1min 15sec");
    TestFormat(WFmt("{}", WTime::MakeFromMinutes(59)), "59min 0sec");
    TestFormat(WFmt("{}", WTime::MakeFromMinutes(-1)), "-1min 0sec");
    TestFormat(WFmt("{}", WTime::MakeFromMinutes(90)), "1h 30min 0sec");
    TestFormat(WFmt("{}", WTime::MakeFromMinutes(-90.5)), "-1h 30min 30sec");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WDateTime")
  {
    {
      WDateTime dt;
      dt.SetYear(2019);
      dt.SetMonth(6);
      dt.SetDay(12);
      dt.SetHour(13);
      dt.SetMinute(26);
      dt.SetSecond(51);
      dt.SetMicroseconds(7000);

      TestFormat(WFmt("{}", dt), "2019-06-12_13-26-51-007");
    }

    {
      WDateTime dt;
      dt.SetYear(0);
      dt.SetMonth(1);
      dt.SetDay(1);
      dt.SetHour(0);
      dt.SetMinute(0);
      dt.SetSecond(0);
      dt.SetMicroseconds(0);

      TestFormat(WFmt("{}", dt), "0000-01-01_00-00-00-000");
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Sensitive Info")
  {
    auto prev = WArgSensitive::s_BuildStringCB;
    W_SCOPE_EXIT(WArgSensitive::s_BuildStringCB = prev);

    WArgSensitive::s_BuildStringCB = WArgSensitive::BuildString_SensitiveUserData_Hash;

    WStringBuilder fmt;

    fmt.SetFormat("Password: {}", WArgSensitive("hunter2", "pwd"));
    W_TEST_STRING(fmt, "Password: sud:pwd#96d66ce6($7)");

    fmt.SetFormat("Password: {}", WArgSensitive("hunter2"));
    W_TEST_STRING(fmt, "Password: sud:#96d66ce6($7)");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WArgEnum")
  {
    TestFormat(WFmt("{}", WArgEnum(WEnum<WExampleEnum>(WExampleEnum::Value1))), "Value1");
    TestFormat(WFmt("{}", WArgEnum(WEnum<WExampleEnum>(WExampleEnum::Value2))), "Value2");
    TestFormat(WFmt("{}", WArgEnum(WEnum<WExampleEnum>(WExampleEnum::Value3))), "Value3");

    // Fully qualified names
    TestFormat(WFmt("{}", WArgEnum(WEnum<WExampleEnum>(WExampleEnum::Value1), true)), "WExampleEnum::Value1");
    TestFormat(WFmt("{}", WArgEnum(WEnum<WExampleEnum>(WExampleEnum::Value2), true)), "WExampleEnum::Value2");

    WBitflags<WExampleBitflags> flags;
    flags.Clear();
    flags.Add(WExampleBitflags::Value1);
    TestFormat(WFmt("{}", WArgEnum(flags)), "Value1");
    TestFormat(WFmt("{}", WArgEnum(flags, true)), "WExampleBitflags::Value1");

    flags.Add(WExampleBitflags::Value2);
    TestFormat(WFmt("{}", WArgEnum(flags)), "Value1|Value2");
    TestFormat(WFmt("{}", WArgEnum(flags, true)), "WExampleBitflags::Value1|WExampleBitflags::Value2");
  }
}
