#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_LINUX)

#  include <Foundation/Strings/FormatString.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Strings/StringBuilder.h>

#  include <string.h>

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgErrno& arg)
{
  const char* szErrorMsg = std::strerror(arg.m_iErrno);
  WStringUtils::snprintf(szTmp, uiLength, "%i (\"%s\")", arg.m_iErrno, szErrorMsg);
  return WStringView(szTmp);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgErrorCode& arg)
{
  WStringUtils::snprintf(szTmp, uiLength, "%u", arg.m_ErrorCode);
  return WStringView(szTmp);
}
#endif
