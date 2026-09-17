#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Logging/Log.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/System/EnvironmentVariableUtils.h>
#  include <Foundation/Threading/Mutex.h>
#  include <Foundation/Utilities/ConversionUtils.h>

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <intsafe.h>

WString WEnvironmentVariableUtils::GetValueStringImpl(WStringView sName, WStringView szDefault)
{
  WStringWChar szwName(sName);
  wchar_t szStaticValueBuffer[64] = {0};
  size_t uiRequiredSize = 0;

  errno_t res = _wgetenv_s(&uiRequiredSize, szStaticValueBuffer, szwName);

  // Variable doesn't exist
  if (uiRequiredSize == 0)
  {
    return szDefault;
  }

  // Succeeded
  if (res == 0)
  {
    return WString(szStaticValueBuffer);
  }
  // Static buffer was too small, do a heap allocation to query the value
  else if (res == ERANGE)
  {
    W_ASSERT_DEV(uiRequiredSize != SIZE_T_MAX, "");
    const size_t uiDynamicSize = uiRequiredSize + 1;
    wchar_t* szDynamicBuffer = W_DEFAULT_NEW_RAW_BUFFER(wchar_t, uiDynamicSize);
    WMemoryUtils::ZeroFill(szDynamicBuffer, uiDynamicSize);

    res = _wgetenv_s(&uiRequiredSize, szDynamicBuffer, uiDynamicSize, szwName);

    if (res != 0)
    {
      WLog::Error("Error getting environment variable \"{0}\" with dynamic buffer.", sName);
      W_DEFAULT_DELETE_RAW_BUFFER(szDynamicBuffer);
      return szDefault;
    }
    else
    {
      WString retVal(szDynamicBuffer);
      W_DEFAULT_DELETE_RAW_BUFFER(szDynamicBuffer);
      return retVal;
    }
  }
  else
  {
    WLog::Warning("Couldn't get environment variable value for \"{0}\", got {1} as a result.", sName, res);
    return szDefault;
  }
}

WResult WEnvironmentVariableUtils::SetValueStringImpl(WStringView sName, WStringView szValue)
{
  WStringWChar szwName(sName);
  WStringWChar szwValue(szValue);

  if (_wputenv_s(szwName, szwValue) == 0)
    return W_SUCCESS;
  else
    return W_FAILURE;
}

bool WEnvironmentVariableUtils::IsVariableSetImpl(WStringView sName)
{
  WStringWChar szwName(sName);
  wchar_t szStaticValueBuffer[16] = {0};
  size_t uiRequiredSize = 0;

  errno_t res = _wgetenv_s(&uiRequiredSize, szStaticValueBuffer, szwName);

  if (res == 0 || res == ERANGE)
  {
    // Variable doesn't exist if uiRequiredSize is 0
    return uiRequiredSize > 0;
  }
  else
  {
    WLog::Error("WEnvironmentVariableUtils::IsVariableSet(\"{0}\") got {1} from _wgetenv_s.", sName, res);
    return false;
  }
}

WResult WEnvironmentVariableUtils::UnsetVariableImpl(WStringView sName)
{
  WStringWChar szwName(sName);

  if (_wputenv_s(szwName, L"") == 0)
    return W_SUCCESS;
  else
    return W_FAILURE;
}

#endif
