#pragma once

#ifndef W_INCLUDING_BASICS_H
#  error "Please don't include FormatStringArgs.h directly, but instead include Foundation/Basics.h"
#endif

//Note: duplicate from Foundation/Math/Declarations.h
template <typename Type>
class WAngleTemplate;
using WAngle = WAngleTemplate<float>;
using WAngled = WAngleTemplate<double>;


class WRTTI;
class WStringBuilder;
class WVariant;
class WRational;
struct WTime;

template <typename T>
struct WEnum;
template <typename T>
struct WBitflags;

template <typename T>
const WRTTI* WGetStaticRTTI();

struct WArgI
{
  inline explicit WArgI(WInt64 value, WUInt8 uiWidth = 1, bool bPadWithZeros = false, WUInt8 uiBase = 10)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_uiBase(uiBase)
  {
  }

  WInt64 m_Value;
  WUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  WUInt8 m_uiBase;
};

struct WArgU
{
  inline explicit WArgU(WUInt64 value, WUInt8 uiWidth = 1, bool bPadWithZeros = false, WUInt8 uiBase = 10, bool bUpperCase = false)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_bUpperCase(bUpperCase)
    , m_uiBase(uiBase)
  {
  }

  WUInt64 m_Value;
  WUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  bool m_bUpperCase;
  WUInt8 m_uiBase;
};

struct WArgF
{
  inline explicit WArgF(double value, WInt8 iPrecision = -1, bool bScientific = false, WUInt8 uiWidth = 1, bool bPadWithZeros = false)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_bScientific(bScientific)
    , m_iPrecision(iPrecision)
  {
  }

  double m_Value;
  WUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  bool m_bScientific;
  WInt8 m_iPrecision;
};

struct WArgC
{
  inline explicit WArgC(char value)
    : m_Value(value)
  {
  }

  char m_Value;
};

struct WArgP
{
  inline explicit WArgP(const void* value)
    : m_Value(value)
  {
  }

  const void* m_Value;
};


/// Formats a given number such that it will be in format [0, base){suffix} with suffix
/// representing a power of base. Resulting numbers are output with a precision of 2 fractional digits
/// and fractional digits are subject to rounding, so numbers at the upper boundary of [0, base)
/// may be rounded up to the next power of base.
///
/// E.g.: For the default case base is 1000 and suffixes are the SI unit suffixes (i.e. K for kilo, M for mega etc.)
///       Thus 0 remains 0, 1 remains 1, 1000 becomes 1.00K, and 2534000 becomes 2.53M. But 999.999 will
///       end up being displayed as 1000.00K for base 1000 due to rounding.
struct WArgHumanReadable
{
  inline WArgHumanReadable(const double value, const WUInt64 uiBase, const char* const* const pSuffixes, WUInt32 uiSuffixCount)
    : m_Value(value)
    , m_Base(uiBase)
    , m_Suffixes(pSuffixes)
    , m_SuffixCount(uiSuffixCount)
  {
  }

  inline WArgHumanReadable(const WInt64 value, const WUInt64 uiBase, const char* const* const pSuffixes, WUInt32 uiSuffixCount)
    : WArgHumanReadable(static_cast<double>(value), uiBase, pSuffixes, uiSuffixCount)
  {
  }

  inline explicit WArgHumanReadable(const double value)
    : WArgHumanReadable(value, 1000u, m_DefaultSuffixes, W_ARRAY_SIZE(m_DefaultSuffixes))
  {
  }

  inline explicit WArgHumanReadable(const WInt64 value)
    : WArgHumanReadable(static_cast<double>(value), 1000u, m_DefaultSuffixes, W_ARRAY_SIZE(m_DefaultSuffixes))
  {
  }

  const double m_Value;
  const WUInt64 m_Base;
  const char* const* const m_Suffixes;
  const char* const m_DefaultSuffixes[6] = {"", "K", "M", "G", "T", "P"};
  const WUInt32 m_SuffixCount;
};

struct WArgFileSize : public WArgHumanReadable
{
  inline explicit WArgFileSize(const WUInt64 value)
    : WArgHumanReadable(static_cast<double>(value), 1024u, m_ByteSuffixes, W_ARRAY_SIZE(m_ByteSuffixes))
  {
  }

  const char* const m_ByteSuffixes[6] = {"B", "KB", "MB", "GB", "TB", "PB"};
};

/// Wraps a string that may contain sensitive information, such as user file paths.
///
/// The application can specify a function to scramble this type of information. By default no such function is set.
/// A general purpose function is provided with 'BuildString_SensitiveUserData_Hash()'
///
/// \param sSensitiveInfo The information that may need to be scrambled.
/// \param szContext A custom string to identify the 'context', ie. what type of sensitive data is being scrambled.
///        This may be passed through unmodified, or can guide the scrambling function to choose how to output the sensitive data.
struct WArgSensitive
{
  inline explicit WArgSensitive(const WStringView& sSensitiveInfo, const char* szContext = nullptr)
    : m_sSensitiveInfo(sSensitiveInfo)
    , m_szContext(szContext)
  {
  }

  const WStringView m_sSensitiveInfo;
  const char* m_szContext;

  using BuildStringCallback = WStringView (*)(char*, WUInt32, const WArgSensitive&);
  W_FOUNDATION_DLL static BuildStringCallback s_BuildStringCB;

  /// Set s_BuildStringCB to this function to enable scrambling of sensitive data.
  W_FOUNDATION_DLL static WStringView BuildString_SensitiveUserData_Hash(char* szTmp, WUInt32 uiLength, const WArgSensitive& arg);
};

/// Formats an WEnum or WBitflags value as its string representation using the reflection system.
///
/// By default the value name is output without the type prefix (e.g. "Value1" instead of "MyEnum::Value1"). Set bFullyQualifiedName to true to include the type prefix.
/// Requires that the enum/bitflags type has been registered with the reflection system via W_BEGIN_STATIC_REFLECTED_ENUM / W_BEGIN_STATIC_REFLECTED_BITFLAGS.
struct WArgEnum
{
  template <typename T>
  inline explicit WArgEnum(WEnum<T> value, bool bFullyQualifiedName = false)
    : m_pType(WGetStaticRTTI<T>())
    , m_iValue(static_cast<WInt64>(value.GetValue()))
    , m_bFullyQualifiedName(bFullyQualifiedName)
  {
  }

  template <typename T>
  inline explicit WArgEnum(WBitflags<T> value, bool bFullyQualifiedName = false)
    : m_pType(WGetStaticRTTI<T>())
    , m_iValue(static_cast<WInt64>(value.GetValue()))
    , m_bFullyQualifiedName(bFullyQualifiedName)
  {
  }

  const WRTTI* m_pType = nullptr;
  WInt64 m_iValue = 0;
  bool m_bFullyQualifiedName = false;
};

W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgI& arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, WInt64 iArg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, WInt32 iArg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgU& arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, WUInt64 uiArg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, WUInt32 uiArg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgF& arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, double fArg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, bool bArg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const char* szArg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const wchar_t* pArg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WStringBuilder& sArg);
W_FOUNDATION_DLL const WStringView& BuildString(char* szTmp, WUInt32 uiLength, const WStringView& sArg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgC& arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgP& arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, WResult arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WVariant& arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WAngle& arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WRational& arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgHumanReadable& arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WTime& arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgSensitive& arg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgEnum& arg);


#if W_ENABLED(W_COMPILER_GCC) || W_ENABLED(W_COMPILER_CLANG)

// on these platforms "long int" is a different type from "long long int"

W_ALWAYS_INLINE WStringView BuildString(char* szTmp, WUInt32 uiLength, long int iArg)
{
  return BuildString(szTmp, uiLength, static_cast<WInt64>(iArg));
}

W_ALWAYS_INLINE WStringView BuildString(char* szTmp, WUInt32 uiLength, unsigned long int uiArg)
{
  return BuildString(szTmp, uiLength, static_cast<WUInt64>(uiArg));
}

#endif

// add platform specific formatters
#include <FormatString_Platform.h>
