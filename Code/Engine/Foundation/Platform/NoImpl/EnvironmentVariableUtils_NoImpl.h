#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/EnvironmentVariableUtils.h>

WString WEnvironmentVariableUtils::GetValueStringImpl(WStringView sName, WStringView sDefault)
{
  W_IGNORE_UNUSED(sName);
  W_IGNORE_UNUSED(sDefault);
  W_ASSERT_NOT_IMPLEMENTED
  return "";
}

WResult WEnvironmentVariableUtils::SetValueStringImpl(WStringView sName, WStringView sValue)
{
  W_IGNORE_UNUSED(sName);
  W_IGNORE_UNUSED(sValue);
  W_ASSERT_NOT_IMPLEMENTED
  return W_FAILURE;
}

bool WEnvironmentVariableUtils::IsVariableSetImpl(WStringView sName)
{
  W_IGNORE_UNUSED(sName);
  return false;
}

WResult WEnvironmentVariableUtils::UnsetVariableImpl(WStringView sName)
{
  W_IGNORE_UNUSED(sName);
  return W_FAILURE;
}
