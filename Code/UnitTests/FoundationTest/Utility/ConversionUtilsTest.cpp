#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Utilities/ConversionUtils.h>

W_CREATE_SIMPLE_TEST_GROUP(Utility);

W_CREATE_SIMPLE_TEST(Utility, ConversionUtils)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "StringToInt")
  {
    const char* szString = "1a";
    const char* szResultPos = nullptr;

    WInt32 iRes = 42;
    szString = "01234";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 1234);
    W_TEST_BOOL(szResultPos == szString + 5);

    iRes = 42;
    szString = "0";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 0);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0000";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 0);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-999999";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, -999999);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-+999999";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, -999999);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "--999999";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 999999);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "++---+--+--999999";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, -999999);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "++--+--+--999999";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 999999);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "123+456";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 123);
    W_TEST_BOOL(szResultPos == szString + 3);

    iRes = 42;
    szString = "123_456";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 123);
    W_TEST_BOOL(szResultPos == szString + 3);

    iRes = 42;
    szString = "-123-456";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, -123);
    W_TEST_BOOL(szResultPos == szString + 4);


    iRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToInt(nullptr, iRes) == W_FAILURE);
    W_TEST_INT(iRes, 42);

    iRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToInt("", iRes) == W_FAILURE);
    W_TEST_INT(iRes, 42);

    iRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToInt("a", iRes) == W_FAILURE);
    W_TEST_INT(iRes, 42);

    iRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToInt("a15", iRes) == W_FAILURE);
    W_TEST_INT(iRes, 42);

    iRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToInt("+", iRes) == W_FAILURE);
    W_TEST_INT(iRes, 42);

    iRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToInt("-", iRes) == W_FAILURE);
    W_TEST_INT(iRes, 42);

    iRes = 42;
    szString = "1a";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 1);
    W_TEST_BOOL(szResultPos == szString + 1);

    iRes = 42;
    szString = "0 23";
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 0);
    W_TEST_BOOL(szResultPos == szString + 1);

    // overflow check

    iRes = 42;
    szString = "0002147483647"; // valid
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 2147483647);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-2147483648"; // valid
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, (WInt32)0x80000000);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0002147483648"; // invalid
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_FAILURE);
    W_TEST_INT(iRes, 42);

    iRes = 42;
    szString = "-2147483649"; // invalid
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_FAILURE);
    W_TEST_INT(iRes, 42);

    iRes = 42;
    szString = "100'000"; // valid with c++ separator
    W_TEST_BOOL(WConversionUtils::StringToInt(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 100'000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StringToUInt")
  {
    const char* szString = "1a";
    const char* szResultPos = nullptr;

    WUInt32 uiRes = 42;
    szString = "01234";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(uiRes, 1234);
    W_TEST_BOOL(szResultPos == szString + 5);

    uiRes = 42;
    szString = "0";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(uiRes, 0);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "0000";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(uiRes, 0);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "-999999";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_FAILURE);
    W_TEST_INT(uiRes, 42);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "-+999999";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_FAILURE);
    W_TEST_INT(uiRes, 42);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "--999999";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(uiRes, 999999);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "++---+--+--999999";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_FAILURE);
    W_TEST_INT(uiRes, 42);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "++--+--+--999999";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(uiRes, 999999);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "123+456";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(uiRes, 123);
    W_TEST_BOOL(szResultPos == szString + 3);

    uiRes = 42;
    szString = "123_456";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(uiRes, 123);
    W_TEST_BOOL(szResultPos == szString + 3);

    uiRes = 42;
    szString = "-123-456";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_FAILURE);
    W_TEST_INT(uiRes, 42);
    W_TEST_BOOL(szResultPos == szString + 4);


    uiRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToUInt(nullptr, uiRes) == W_FAILURE);
    W_TEST_INT(uiRes, 42);

    uiRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToUInt("", uiRes) == W_FAILURE);
    W_TEST_INT(uiRes, 42);

    uiRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToUInt("a", uiRes) == W_FAILURE);
    W_TEST_INT(uiRes, 42);

    uiRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToUInt("a15", uiRes) == W_FAILURE);
    W_TEST_INT(uiRes, 42);

    uiRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToUInt("+", uiRes) == W_FAILURE);
    W_TEST_INT(uiRes, 42);

    uiRes = 42;
    W_TEST_BOOL(WConversionUtils::StringToUInt("-", uiRes) == W_FAILURE);
    W_TEST_INT(uiRes, 42);

    uiRes = 42;
    szString = "1a";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(uiRes, 1);
    W_TEST_BOOL(szResultPos == szString + 1);

    uiRes = 42;
    szString = "0 23";
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(uiRes, 0);
    W_TEST_BOOL(szResultPos == szString + 1);

    // overflow check

    uiRes = 42;
    szString = "0004294967295"; // valid
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(uiRes, 4294967295u);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "0004294967296"; // invalid
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_FAILURE);
    W_TEST_INT(uiRes, 42);

    uiRes = 42;
    szString = "-1"; // invalid
    W_TEST_BOOL(WConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == W_FAILURE);
    W_TEST_INT(uiRes, 42);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StringToInt64")
  {
    // overflow check
    WInt64 iRes = 42;
    const char* szString = "0002147483639"; // valid
    const char* szResultPos = nullptr;

    W_TEST_BOOL(WConversionUtils::StringToInt64(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 2147483639);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0002147483640"; // also valid with 64bit
    W_TEST_BOOL(WConversionUtils::StringToInt64(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 2147483640);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0009223372036854775807"; // last valid positive number
    W_TEST_BOOL(WConversionUtils::StringToInt64(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, 9223372036854775807);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0009223372036854775808"; // invalid
    W_TEST_BOOL(WConversionUtils::StringToInt64(szString, iRes, &szResultPos) == W_FAILURE);
    W_TEST_INT(iRes, 42);

    iRes = 42;
    szString = "-9223372036854775808"; // last valid negative number
    W_TEST_BOOL(WConversionUtils::StringToInt64(szString, iRes, &szResultPos) == W_SUCCESS);
    W_TEST_INT(iRes, (WInt64)0x8000000000000000);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-9223372036854775809"; // invalid
    W_TEST_BOOL(WConversionUtils::StringToInt64(szString, iRes, &szResultPos) == W_FAILURE);
    W_TEST_INT(iRes, 42);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StringToFloat")
  {
    const char* szString = nullptr;
    const char* szResultPos = nullptr;

    double fRes = 42;
    szString = "23.45";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, 23.45, 0.00001);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "-2345";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, -2345.0, 0.00001);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "-0";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, 0.0, 0.00001);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "0_0000.0_00000_";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, 0.0, 0.00001);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "_0_0000.0_00000_";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_FAILURE);

    fRes = 42;
    szString = ".123456789";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, 0.123456789, 0.00001);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "+123E1";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, 1230.0, 0.00001);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "  \r\t 123e0";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, 123.0, 0.00001);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "\n123e6";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, 123000000.0, 0.00001);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "\n1_2_3e+6";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, 123000000.0, 0.00001);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "  123E-6";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, 0.000123, 0.00001);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = " + - -+-123.45e-10";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, -0.000000012345, 0.0000001);
    W_TEST_BOOL(szResultPos == szString + WStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = nullptr;
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_FAILURE);
    W_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes = 42;
    szString = "";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_FAILURE);
    W_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes = 42;
    szString = "-----";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_FAILURE);
    W_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes = 42;
    szString = " + - +++ - \r \n";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_FAILURE);
    W_TEST_DOUBLE(fRes, 42.0, 0.00001);


    fRes = 42;
    szString = "65.345789xabc";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, 65.345789, 0.000001);
    W_TEST_BOOL(szResultPos == szString + 9);

    fRes = 42;
    szString = " \n \r \t + - 2314565.345789ff xabc";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, -2314565.345789, 0.000001);
    W_TEST_BOOL(szResultPos == szString + 25);

    fRes = 42;
    szString = "100'000.0";
    W_TEST_BOOL(WConversionUtils::StringToFloat(szString, fRes, &szResultPos) == W_SUCCESS);
    W_TEST_DOUBLE(fRes, 100'000.0, 0.000001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StringToBool")
  {
    const char* szString = "";
    const char* szResultPos = nullptr;
    bool bRes = false;

    // true / false
    {
      bRes = false;
      szString = "true,";
      szResultPos = nullptr;
      W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_SUCCESS);
      W_TEST_BOOL(bRes);
      W_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "FALSe,";
      szResultPos = nullptr;
      W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_SUCCESS);
      W_TEST_BOOL(!bRes);
      W_TEST_BOOL(*szResultPos == ',');
    }

    // on / off
    {
      bRes = false;
      szString = "\n on,";
      szResultPos = nullptr;
      W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_SUCCESS);
      W_TEST_BOOL(bRes);
      W_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "\t\t \toFf,";
      szResultPos = nullptr;
      W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_SUCCESS);
      W_TEST_BOOL(!bRes);
      W_TEST_BOOL(*szResultPos == ',');
    }

    // 1 / 0
    {
      bRes = false;
      szString = "\r1,";
      szResultPos = nullptr;
      W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_SUCCESS);
      W_TEST_BOOL(bRes);
      W_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "0,";
      szResultPos = nullptr;
      W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_SUCCESS);
      W_TEST_BOOL(!bRes);
      W_TEST_BOOL(*szResultPos == ',');
    }

    // yes / no
    {
      bRes = false;
      szString = "yes,";
      szResultPos = nullptr;
      W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_SUCCESS);
      W_TEST_BOOL(bRes);
      W_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "NO,";
      szResultPos = nullptr;
      W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_SUCCESS);
      W_TEST_BOOL(!bRes);
      W_TEST_BOOL(*szResultPos == ',');
    }

    // enable / disable
    {
      bRes = false;
      szString = "enable,";
      szResultPos = nullptr;
      W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_SUCCESS);
      W_TEST_BOOL(bRes);
      W_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "disABle,";
      szResultPos = nullptr;
      W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_SUCCESS);
      W_TEST_BOOL(!bRes);
      W_TEST_BOOL(*szResultPos == ',');
    }

    bRes = false;

    szString = "of,";
    szResultPos = nullptr;
    W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_FAILURE);
    W_TEST_BOOL(szResultPos == nullptr);

    szString = "aon";
    szResultPos = nullptr;
    W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_FAILURE);
    W_TEST_BOOL(szResultPos == nullptr);

    szString = "";
    szResultPos = nullptr;
    W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_FAILURE);
    W_TEST_BOOL(szResultPos == nullptr);

    szString = nullptr;
    szResultPos = nullptr;
    W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_FAILURE);
    W_TEST_BOOL(szResultPos == nullptr);

    szString = "tut";
    szResultPos = nullptr;
    W_TEST_BOOL(WConversionUtils::StringToBool(szString, bRes, &szResultPos) == W_FAILURE);
    W_TEST_BOOL(szResultPos == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HexCharacterToIntValue")
  {
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('0'), 0);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('1'), 1);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('2'), 2);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('3'), 3);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('4'), 4);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('5'), 5);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('6'), 6);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('7'), 7);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('8'), 8);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('9'), 9);

    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('a'), 10);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('b'), 11);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('c'), 12);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('d'), 13);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('e'), 14);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('f'), 15);

    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('A'), 10);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('B'), 11);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('C'), 12);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('D'), 13);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('E'), 14);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('F'), 15);

    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('g'), -1);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('h'), -1);
    W_TEST_INT(WConversionUtils::HexCharacterToIntValue('i'), -1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ConvertHexStringToUInt32")
  {
    WUInt32 res;

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("", res).Succeeded());
    W_TEST_BOOL(res == 0);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("0x", res).Succeeded());
    W_TEST_BOOL(res == 0);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("0", res).Succeeded());
    W_TEST_BOOL(res == 0);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("0x0", res).Succeeded());
    W_TEST_BOOL(res == 0);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("a", res).Succeeded());
    W_TEST_BOOL(res == 10);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("0xb", res).Succeeded());
    W_TEST_BOOL(res == 11);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("000c", res).Succeeded());
    W_TEST_BOOL(res == 12);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("AA", res).Succeeded());
    W_TEST_BOOL(res == 170);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("aAjbB", res).Failed());

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("aAbB", res).Succeeded());
    W_TEST_BOOL(res == 43707);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("FFFFffff", res).Succeeded());
    W_TEST_BOOL(res == 0xFFFFFFFF);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("0000FFFFffff", res).Succeeded());
    W_TEST_BOOL(res == 0xFFFF);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt32("100000000", res).Succeeded());
    W_TEST_BOOL(res == 0x10000000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ConvertHexStringToUInt64")
  {
    WUInt64 res;

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("", res).Succeeded());
    W_TEST_BOOL(res == 0);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("0x", res).Succeeded());
    W_TEST_BOOL(res == 0);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("0", res).Succeeded());
    W_TEST_BOOL(res == 0);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("0x0", res).Succeeded());
    W_TEST_BOOL(res == 0);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("a", res).Succeeded());
    W_TEST_BOOL(res == 10);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("0xb", res).Succeeded());
    W_TEST_BOOL(res == 11);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("000c", res).Succeeded());
    W_TEST_BOOL(res == 12);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("AA", res).Succeeded());
    W_TEST_BOOL(res == 170);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("aAjbB", res).Failed());

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("aAbB", res).Succeeded());
    W_TEST_BOOL(res == 43707);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("FFFFffff", res).Succeeded());
    W_TEST_BOOL(res == 4294967295);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("0000FFFFffff", res).Succeeded());
    W_TEST_BOOL(res == 4294967295);

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("0xfffffffffffffffy", res).Failed());

    W_TEST_BOOL(WConversionUtils::ConvertHexStringToUInt64("0xffffffffffffffffy", res).Succeeded());
    W_TEST_BOOL(res == 0xFFFFFFFFFFFFFFFFllu);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ConvertBinaryToHex and ConvertHexStringToBinary")
  {
    WDynamicArray<WUInt8> binary;
    binary.SetCountUninitialized(1024);

    WRandom r;
    r.InitializeFromCurrentTime();

    for (auto& val : binary)
    {
      val = static_cast<WUInt8>(r.UIntInRange(256u));
    }

    WStringBuilder sHex;
    WConversionUtils::ConvertBinaryToHex(binary.GetData(), binary.GetCount(), [&sHex](const char* s)
      { sHex.Append(s); });

    WDynamicArray<WUInt8> binary2;
    binary2.SetCountUninitialized(1024);

    WConversionUtils::ConvertHexToBinary(sHex, binary2.GetData(), binary2.GetCount());

    W_TEST_BOOL(binary == binary2);
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "ExtractFloatsFromString")
  {
    float v[16];

    const char* szText = "This 1 is 2.3 or 3.141 tests in 1.2 strings, maybe 4.5,6.78or9.101!";

    WMemoryUtils::ZeroFill(v, 16);
    W_TEST_INT(WConversionUtils::ExtractFloatsFromString(szText, 0, v), 0);
    W_TEST_FLOAT(v[0], 0.0f, 0.0f);

    WMemoryUtils::ZeroFill(v, 16);
    W_TEST_INT(WConversionUtils::ExtractFloatsFromString(szText, 3, v), 3);
    W_TEST_FLOAT(v[0], 1.0f, 0.0001f);
    W_TEST_FLOAT(v[1], 2.3f, 0.0001f);
    W_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    W_TEST_FLOAT(v[3], 0.0f, 0.0f);

    WMemoryUtils::ZeroFill(v, 16);
    W_TEST_INT(WConversionUtils::ExtractFloatsFromString(szText, 6, v), 6);
    W_TEST_FLOAT(v[0], 1.0f, 0.0001f);
    W_TEST_FLOAT(v[1], 2.3f, 0.0001f);
    W_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    W_TEST_FLOAT(v[3], 1.2f, 0.0001f);
    W_TEST_FLOAT(v[4], 4.5f, 0.0001f);
    W_TEST_FLOAT(v[5], 6.78f, 0.0001f);
    W_TEST_FLOAT(v[6], 0.0f, 0.0f);

    WMemoryUtils::ZeroFill(v, 16);
    W_TEST_INT(WConversionUtils::ExtractFloatsFromString(szText, 10, v), 7);
    W_TEST_FLOAT(v[0], 1.0f, 0.0001f);
    W_TEST_FLOAT(v[1], 2.3f, 0.0001f);
    W_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    W_TEST_FLOAT(v[3], 1.2f, 0.0001f);
    W_TEST_FLOAT(v[4], 4.5f, 0.0001f);
    W_TEST_FLOAT(v[5], 6.78f, 0.0001f);
    W_TEST_FLOAT(v[6], 9.101f, 0.0001f);
    W_TEST_FLOAT(v[7], 0.0f, 0.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ConvertStringToUuid and IsStringUuid")
  {
    WUuid guid;
    WStringBuilder sGuid;

    for (WUInt32 i = 0; i < 100; ++i)
    {
      guid = WUuid::MakeUuid();

      WConversionUtils::ToString(guid, sGuid);

      W_TEST_BOOL(WConversionUtils::IsStringUuid(sGuid));

      WUuid guid2 = WConversionUtils::ConvertStringToUuid(sGuid);

      W_TEST_BOOL(guid == guid2);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TryConvertStringToUuid")
  {
    WUuid guid = WUuid::MakeUuid();
    WStringBuilder sGuid;
    WConversionUtils::ToString(guid, sGuid);

    WUuid result;
    W_TEST_RESULT(WConversionUtils::TryConvertStringToUuid(sGuid, result));
    W_TEST_BOOL(result == guid);

    // The point of this function: anything that is not a uuid has to be reported rather than
    // asserted, because callers use it to find out which of the two they were given.
    const WUuid untouched = WUuid::MakeUuid();

    for (const char* szNotAUuid : {"", "Textures/Wood.png", "D:/Some/Path.WScene", "{ not-a-uuid }",
           "05af8d07-0b38-44a6-8d50-49731ae2625d" /* no braces */})
    {
      result = untouched;
      W_TEST_BOOL(WConversionUtils::TryConvertStringToUuid(szNotAUuid, result).Failed());
      W_TEST_BOOL(result == untouched); // must not be modified on failure
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetColorName")
  {
    W_TEST_STRING(WString(WConversionUtils::GetColorName(WColorGammaUB(1, 2, 3))), "#010203");
    W_TEST_STRING(WString(WConversionUtils::GetColorName(WColorGammaUB(10, 20, 30, 40))), "#0A141E28");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetColorByName")
  {
    W_TEST_BOOL(WConversionUtils::GetColorByName("#010203") == WColorGammaUB(1, 2, 3));
    W_TEST_BOOL(WConversionUtils::GetColorByName("#0A141E28") == WColorGammaUB(10, 20, 30, 40));

    W_TEST_BOOL(WConversionUtils::GetColorByName("#010203") == WColorGammaUB(1, 2, 3));
    W_TEST_BOOL(WConversionUtils::GetColorByName("#0a141e28") == WColorGammaUB(10, 20, 30, 40));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetColorByName and GetColorName")
  {
#define Check(name)                                                                     \
  {                                                                                     \
    bool valid = false;                                                                 \
    const WColor c = WConversionUtils::GetColorByName(W_PP_STRINGIFY(name), &valid); \
    W_TEST_BOOL(valid);                                                                \
    WString sName = WConversionUtils::GetColorName(c);                                \
    W_TEST_STRING(sName, W_PP_STRINGIFY(name));                                       \
  }

#define Check2(name, otherName)                                                         \
  {                                                                                     \
    bool valid = false;                                                                 \
    const WColor c = WConversionUtils::GetColorByName(W_PP_STRINGIFY(name), &valid); \
    W_TEST_BOOL(valid);                                                                \
    WString sName = WConversionUtils::GetColorName(c);                                \
    W_TEST_STRING(sName, W_PP_STRINGIFY(otherName));                                  \
  }

    Check(AliceBlue);
    Check(AntiqueWhite);
    Check(Aqua);
    Check(Aquamarine);
    Check(Azure);
    Check(Beige);
    Check(Bisque);
    Check(Black);
    Check(BlanchedAlmond);
    Check(Blue);
    Check(BlueViolet);
    Check(Brown);
    Check(BurlyWood);
    Check(CadetBlue);
    Check(Chartreuse);
    Check(Chocolate);
    Check(Coral);
    Check(CornflowerBlue); // The Original!
    Check(Cornsilk);
    Check(Crimson);
    Check2(Cyan, Aqua);
    Check(DarkBlue);
    Check(DarkCyan);
    Check(DarkGoldenRod);
    Check(DarkGray);
    Check2(DarkGrey, DarkGray);
    Check(DarkGreen);
    Check(DarkKhaki);
    Check(DarkMagenta);
    Check(DarkOliveGreen);
    Check(DarkOrange);
    Check(DarkOrchid);
    Check(DarkRed);
    Check(DarkSalmon);
    Check(DarkSeaGreen);
    Check(DarkSlateBlue);
    Check(DarkSlateGray);
    Check2(DarkSlateGrey, DarkSlateGray);
    Check(DarkTurquoise);
    Check(DarkViolet);
    Check(DeepPink);
    Check(DeepSkyBlue);
    Check(DimGray);
    Check2(DimGrey, DimGray);
    Check(DodgerBlue);
    Check(FireBrick);
    Check(FloralWhite);
    Check(ForestGreen);
    Check(Fuchsia);
    Check(Gainsboro);
    Check(GhostWhite);
    Check(Gold);
    Check(GoldenRod);
    Check(Gray);
    Check2(Grey, Gray);
    Check(Green);
    Check(GreenYellow);
    Check(HoneyDew);
    Check(HotPink);
    Check(IndianRed);
    Check(Indigo);
    Check(Ivory);
    Check(Khaki);
    Check(Lavender);
    Check(LavenderBlush);
    Check(LawnGreen);
    Check(LemonChiffon);
    Check(LightBlue);
    Check(LightCoral);
    Check(LightCyan);
    Check(LightGoldenRodYellow);
    Check(LightGray);
    Check2(LightGrey, LightGray);
    Check(LightGreen);
    Check(LightPink);
    Check(LightSalmon);
    Check(LightSeaGreen);
    Check(LightSkyBlue);
    Check(LightSlateGray);
    Check2(LightSlateGrey, LightSlateGray);
    Check(LightSteelBlue);
    Check(LightYellow);
    Check(Lime);
    Check(LimeGreen);
    Check(Linen);
    Check2(Magenta, Fuchsia);
    Check(Maroon);
    Check(MediumAquaMarine);
    Check(MediumBlue);
    Check(MediumOrchid);
    Check(MediumPurple);
    Check(MediumSeaGreen);
    Check(MediumSlateBlue);
    Check(MediumSpringGreen);
    Check(MediumTurquoise);
    Check(MediumVioletRed);
    Check(MidnightBlue);
    Check(MintCream);
    Check(MistyRose);
    Check(Moccasin);
    Check(NavajoWhite);
    Check(Navy);
    Check(OldLace);
    Check(Olive);
    Check(OliveDrab);
    Check(Orange);
    Check(OrangeRed);
    Check(Orchid);
    Check(PaleGoldenRod);
    Check(PaleGreen);
    Check(PaleTurquoise);
    Check(PaleVioletRed);
    Check(PapayaWhip);
    Check(PeachPuff);
    Check(Peru);
    Check(Pink);
    Check(Plum);
    Check(PowderBlue);
    Check(Purple);
    Check(RebeccaPurple);
    Check(Red);
    Check(RosyBrown);
    Check(RoyalBlue);
    Check(SaddleBrown);
    Check(Salmon);
    Check(SandyBrown);
    Check(SeaGreen);
    Check(SeaShell);
    Check(Sienna);
    Check(Silver);
    Check(SkyBlue);
    Check(SlateBlue);
    Check(SlateGray);
    Check2(SlateGrey, SlateGray);
    Check(Snow);
    Check(SpringGreen);
    Check(SteelBlue);
    Check(Tan);
    Check(Teal);
    Check(Thistle);
    Check(Tomato);
    Check(Turquoise);
    Check(Violet);
    Check(Wheat);
    Check(White);
    Check(WhiteSmoke);
    Check(Yellow);
    Check(YellowGreen);
  }
}
