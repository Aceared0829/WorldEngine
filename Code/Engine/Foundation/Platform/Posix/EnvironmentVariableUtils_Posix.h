#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/EnvironmentVariableUtils.h>
#include <stdlib.h>

WString WEnvironmentVariableUtils::GetValueStringImpl(WStringView sName, WStringView sDefault)
{
  WStringBuilder tmp;
  const char* value = getenv(sName.GetData(tmp));
  return value != nullptr ? value : sDefault;
}

WResult WEnvironmentVariableUtils::SetValueStringImpl(WStringView sName, WStringView sValue)
{
  WStringBuilder tmp, tmp2;
  if (setenv(sName.GetData(tmp), sValue.GetData(tmp2), 1) == 0)
    return W_SUCCESS;
  else
    return W_FAILURE;
}

bool WEnvironmentVariableUtils::IsVariableSetImpl(WStringView sName)
{
  WStringBuilder tmp;
  return getenv(sName.GetData(tmp)) != nullptr;
}

WResult WEnvironmentVariableUtils::UnsetVariableImpl(WStringView sName)
{
  WStringBuilder tmp;
  if (unsetenv(sName.GetData(tmp)) == 0)
    return W_SUCCESS;
  else
    return W_FAILURE;
}
