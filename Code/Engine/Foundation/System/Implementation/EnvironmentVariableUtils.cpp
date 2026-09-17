#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Utilities/ConversionUtils.h>

// The POSIX functions are not thread safe by definition.
static WMutex s_EnvVarMutex;


WString WEnvironmentVariableUtils::GetValueString(WStringView sName, WStringView sDefault /*= nullptr*/)
{
  W_ASSERT_DEV(!sName.IsEmpty(), "Null or empty name passed to WEnvironmentVariableUtils::GetValueString()");

  W_LOCK(s_EnvVarMutex);

  return GetValueStringImpl(sName, sDefault);
}

WResult WEnvironmentVariableUtils::SetValueString(WStringView sName, WStringView sValue)
{
  W_LOCK(s_EnvVarMutex);

  return SetValueStringImpl(sName, sValue);
}

WInt32 WEnvironmentVariableUtils::GetValueInt(WStringView sName, WInt32 iDefault /*= -1*/)
{
  W_LOCK(s_EnvVarMutex);

  WString value = GetValueString(sName);

  if (value.IsEmpty())
    return iDefault;

  WInt32 iRetVal = 0;
  if (WConversionUtils::StringToInt(value, iRetVal).Succeeded())
    return iRetVal;
  else
    return iDefault;
}

WResult WEnvironmentVariableUtils::SetValueInt(WStringView sName, WInt32 iValue)
{
  WStringBuilder sb;
  sb.SetFormat("{}", iValue);

  return SetValueString(sName, sb);
}

bool WEnvironmentVariableUtils::IsVariableSet(WStringView sName)
{
  W_LOCK(s_EnvVarMutex);

  return IsVariableSetImpl(sName);
}

WResult WEnvironmentVariableUtils::UnsetVariable(WStringView sName)
{
  W_LOCK(s_EnvVarMutex);

  return UnsetVariableImpl(sName);
}
