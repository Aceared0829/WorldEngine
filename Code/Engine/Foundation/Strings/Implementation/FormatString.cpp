#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Rational.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Variant.h>

WFormatString::WFormatString(const WStringBuilder& s)
{
  m_sString = s.GetView();
}

const char* WFormatString::GetTextCStr(WStringBuilder& out_sString) const
{
  out_sString = m_sString;
  return out_sString.GetData();
}

WStringView WFormatString::BuildFormattedText(WStringBuilder& ref_sStorage, WStringView* pArgs, WUInt32 uiNumArgs) const
{
  WStringView sString = m_sString;

  WUInt32 uiLastParam = WInvalidIndex;

  ref_sStorage.Clear();
  while (!sString.IsEmpty())
  {
    if (sString.StartsWith("%"))
    {
      if (sString.TrimWordStart("%%"))
      {
        ref_sStorage.Append("%"_wsv);
      }
      else
      {
        W_ASSERT_DEBUG(false, "Single percentage signs are not allowed in WFormatString. Did you forgot to migrate a printf-style "
                               "string? Use double percentage signs for the actual character.");
      }
    }
    else if (sString.GetElementCount() >= 3 && *sString.GetStartPointer() == '{' && *(sString.GetStartPointer() + 1) >= '0' && *(sString.GetStartPointer() + 1) <= '9' && *(sString.GetStartPointer() + 2) == '}')
    {
      uiLastParam = *(sString.GetStartPointer() + 1) - '0';
      W_ASSERT_DEV(uiLastParam < uiNumArgs, "Too many placeholders in format string");

      if (uiLastParam < uiNumArgs)
      {
        ref_sStorage.Append(pArgs[uiLastParam]);
      }

      sString.ChopAwayFirstCharacterAscii();
      sString.ChopAwayFirstCharacterAscii();
      sString.ChopAwayFirstCharacterAscii();
    }
    else if (sString.TrimWordStart("{}"))
    {
      ++uiLastParam;
      W_ASSERT_DEV(uiLastParam < uiNumArgs, "Too many placeholders in format string");

      if (uiLastParam < uiNumArgs)
      {
        ref_sStorage.Append(pArgs[uiLastParam]);
      }
    }
    else
    {
      const WUInt32 character = sString.GetCharacter();
      ref_sStorage.Append(character);
      sString.ChopAwayFirstCharacterUtf8();
    }
  }

  return ref_sStorage.GetView();
}

//////////////////////////////////////////////////////////////////////////

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgI& arg)
{
  WUInt32 writepos = 0;
  WStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase);
  szTmp[writepos] = '\0';
  return WStringView(szTmp, szTmp + writepos);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, WInt64 iArg)
{
  WUInt32 writepos = 0;
  WStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, iArg, 1, false, 10);
  szTmp[writepos] = '\0';
  return WStringView(szTmp, szTmp + writepos);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, WInt32 iArg)
{
  return BuildString(szTmp, uiLength, (WInt64)iArg);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgU& arg)
{
  WUInt32 writepos = 0;
  WStringUtils::OutputFormattedUInt(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase, arg.m_bUpperCase);
  szTmp[writepos] = '\0';
  return WStringView(szTmp, szTmp + writepos);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, WUInt64 uiArg)
{
  WUInt32 writepos = 0;
  WStringUtils::OutputFormattedUInt(szTmp, uiLength, writepos, uiArg, 1, false, 10, false);
  szTmp[writepos] = '\0';
  return WStringView(szTmp, szTmp + writepos);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, WUInt32 uiArg)
{
  return BuildString(szTmp, uiLength, (WUInt64)uiArg);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgF& arg)
{
  WUInt32 writepos = 0;
  WStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_iPrecision, arg.m_bScientific);
  szTmp[writepos] = '\0';
  return WStringView(szTmp, szTmp + writepos);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, double fArg)
{
  WUInt32 writepos = 0;
  WStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, fArg, 1, false, -1, false);
  szTmp[writepos] = '\0';
  return WStringView(szTmp, szTmp + writepos);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, bool bArg)
{
  W_IGNORE_UNUSED(szTmp);
  W_IGNORE_UNUSED(uiLength);
  return bArg ? "true" : "false";
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const char* szArg)
{
  W_IGNORE_UNUSED(szTmp);
  W_IGNORE_UNUSED(uiLength);
  return szArg;
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const wchar_t* pArg)
{
  const char* start = szTmp;
  if (pArg != nullptr)
  {
    // Code points in UTF-8 can be up to 4 byte, so the end pointer is 3 byte "earlier" than for
    // for a single byte character. One byte for trailing zero is already accounted for in uiLength.
    const char* tmpEnd = szTmp + uiLength - 3u;
    while (*pArg != '\0' && szTmp < tmpEnd)
    {
      // decode utf8 to utf32
      const WUInt32 uiUtf32 = WUnicodeUtils::DecodeWCharToUtf32(pArg);

      // encode utf32 to wchar_t
      WUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, szTmp);
    }
  }

  // Append terminator. As the extra byte for trailing zero is accounted for in uiLength, this is safe.
  *szTmp = '\0';

  return start;
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WString& sArg)
{
  W_IGNORE_UNUSED(szTmp);
  W_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WHashedString& sArg)
{
  W_IGNORE_UNUSED(szTmp);
  W_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WStringBuilder& sArg)
{
  W_IGNORE_UNUSED(szTmp);
  W_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WUntrackedString& sArg)
{
  W_IGNORE_UNUSED(szTmp);
  W_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

const WStringView& BuildString(char* szTmp, WUInt32 uiLength, const WStringView& sArg)
{
  W_IGNORE_UNUSED(szTmp);
  W_IGNORE_UNUSED(uiLength);
  return sArg;
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgC& arg)
{
  W_IGNORE_UNUSED(uiLength);

  szTmp[0] = arg.m_Value;
  szTmp[1] = '\0';

  return WStringView(&szTmp[0], &szTmp[1]);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgP& arg)
{
  WStringUtils::snprintf(szTmp, uiLength, "%p", arg.m_Value);
  return WStringView(szTmp);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, WResult arg)
{
  W_IGNORE_UNUSED(szTmp);
  W_IGNORE_UNUSED(uiLength);

  if (arg.Failed())
    return "<failed>";
  else
    return "<succeeded>";
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WVariant& arg)
{
  WString sString = arg.ConvertTo<WString>();
  WStringUtils::snprintf(szTmp, uiLength, "%s", sString.GetData());
  return WStringView(szTmp);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WAngle& arg)
{
  WUInt32 writepos = 0;
  WStringUtils::OutputFormattedFloat(szTmp, uiLength - 2, writepos, arg.GetDegree(), 1, false, 1, false);

  // Utf-8 representation of the degree sign
  szTmp[writepos + 0] = /*(char)0xC2;*/ -62;
  szTmp[writepos + 1] = /*(char)0xB0;*/ -80;
  szTmp[writepos + 2] = '\0';

  return WStringView(szTmp, szTmp + writepos + 2);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WRational& arg)
{
  WUInt32 writepos = 0;

  if (arg.IsIntegral())
  {
    WStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.GetIntegralResult(), 1, false, 10);

    return WStringView(szTmp, szTmp + writepos);
  }
  else
  {
    WStringUtils::snprintf(szTmp, uiLength, "%i/%i", arg.GetNumerator(), arg.GetDenominator());

    return WStringView(szTmp);
  }
}

WStringView BuildString(char* pTmp, WUInt32 uiLength, const WTime& arg)
{
  WUInt32 writepos = 0;

  const double fAbsSec = WMath::Abs(arg.GetSeconds());

  if (fAbsSec < 0.000001)
  {
    WStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetNanoseconds(), 1, false, 1, false, true);
    // szTmp[writepos++] = ' ';
    pTmp[writepos++] = 'n';
    pTmp[writepos++] = 's';
  }
  else if (fAbsSec < 0.001)
  {
    WStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetMicroseconds(), 1, false, 1, false, true);

    // szTmp[writepos++] = ' ';
    // Utf-8 representation of the microsecond (us) sign
    pTmp[writepos++] = /*(char)0xC2;*/ -62;
    pTmp[writepos++] = /*(char)0xB5;*/ -75;
    pTmp[writepos++] = 's';
  }
  else if (fAbsSec < 1.0)
  {
    WStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetMilliseconds(), 1, false, 1, false, true);

    // tmp[writepos++] = ' ';
    pTmp[writepos++] = 'm';
    pTmp[writepos++] = 's';
  }
  else if (fAbsSec < 60.0)
  {
    WStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetSeconds(), 1, false, 1, false, true);

    // szTmp[writepos++] = ' ';
    pTmp[writepos++] = 's';
    pTmp[writepos++] = 'e';
    pTmp[writepos++] = 'c';
  }
  else if (fAbsSec < 60.0 * 60.0)
  {
    double tRem = fAbsSec;

    WInt32 iMin = static_cast<WInt32>(WMath::Trunc(tRem / 60.0));
    tRem -= iMin * 60;
    iMin *= WMath::Sign(static_cast<WInt32>(arg.GetSeconds()));

    const WInt32 iSec = static_cast<WInt32>(WMath::Trunc(tRem));

    writepos = WStringUtils::snprintf(pTmp, uiLength, "%imin %isec", iMin, iSec);
  }
  else
  {
    double tRem = fAbsSec;

    WInt32 iHrs = static_cast<WInt32>(WMath::Trunc(tRem / (60.0 * 60.0)));
    tRem -= iHrs * 60 * 60;
    iHrs *= WMath::Sign(static_cast<WInt32>(arg.GetSeconds()));

    const WInt32 iMin = static_cast<WInt32>(WMath::Trunc(tRem / 60.0));
    tRem -= iMin * 60;

    const WInt32 iSec = static_cast<WInt32>(WMath::Trunc(tRem));

    writepos = WStringUtils::snprintf(pTmp, uiLength, "%ih %imin %isec", iHrs, iMin, iSec);
  }

  pTmp[writepos] = '\0';
  return WStringView(pTmp, pTmp + writepos);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgHumanReadable& arg)
{
  WUInt32 suffixIndex = 0;
  WUInt64 divider = 1;
  double absValue = WMath::Abs(arg.m_Value);
  while (absValue / divider >= arg.m_Base && suffixIndex < arg.m_SuffixCount - 1)
  {
    divider *= arg.m_Base;
    ++suffixIndex;
  }

  WUInt32 writepos = 0;
  if (divider == 1 && WMath::Fraction(arg.m_Value) == 0.0)
  {
    WStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, static_cast<WInt64>(arg.m_Value), 1, false, 10);
  }
  else
  {
    WStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, arg.m_Value / divider, 1, false, 2, false);
  }
  WStringUtils::Copy(szTmp + writepos, uiLength - writepos, arg.m_Suffixes[suffixIndex]);

  return WStringView(szTmp);
}

WArgSensitive::BuildStringCallback WArgSensitive::s_BuildStringCB = nullptr;

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgSensitive& arg)
{
  if (WArgSensitive::s_BuildStringCB)
  {
    return WArgSensitive::s_BuildStringCB(szTmp, uiLength, arg);
  }

  return arg.m_sSensitiveInfo;
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgEnum& arg)
{
  WStringBuilder sTemp;
  const auto mode = arg.m_bFullyQualifiedName ? WReflectionUtils::EnumConversionMode::FullyQualifiedName : WReflectionUtils::EnumConversionMode::ValueNameOnly;
  WReflectionUtils::EnumerationToString(arg.m_pType, arg.m_iValue, sTemp, mode);
  WStringUtils::Copy(szTmp, uiLength, sTemp.GetData());
  return WStringView(szTmp);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WSizeU32& arg)
{
  WUInt32 writepos = 0;
  WStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.width, 1, false, 10);
  szTmp[writepos++] = 'x';
  WStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.height, 1, false, 10);
  szTmp[writepos] = '\0';
  return WStringView(szTmp, szTmp + writepos);
}


WStringView WArgSensitive::BuildString_SensitiveUserData_Hash(char* szTmp, WUInt32 uiLength, const WArgSensitive& arg)
{
  const WUInt32 len = arg.m_sSensitiveInfo.GetElementCount();

  if (len == 0)
    return WStringView();

  if (!WStringUtils::IsNullOrEmpty(arg.m_szContext))
  {
    WStringUtils::snprintf(
      szTmp, uiLength, "sud:%s#%08x($%u)", arg.m_szContext, WHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }
  else
  {
    WStringUtils::snprintf(szTmp, uiLength, "sud:#%08x($%u)", WHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }

  return szTmp;
}
