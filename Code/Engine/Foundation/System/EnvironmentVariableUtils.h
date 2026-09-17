#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

/// This is a helper class to interact with environment variables.
class W_FOUNDATION_DLL WEnvironmentVariableUtils
{
public:
  /// Returns the current value of the request environment variable. If it isn't set szDefault will be returned.
  static WString GetValueString(WStringView sName, WStringView sDefault = nullptr);

  /// Sets the environment variable for the current execution environment (i.e. this process and child processes created after this call).
  static WResult SetValueString(WStringView sName, WStringView sValue);

  /// Returns the current value of the request environment variable. If it isn't set iDefault will be returned.
  static WInt32 GetValueInt(WStringView sName, WInt32 iDefault = -1);

  /// Sets the environment variable for the current execution environment.
  static WResult SetValueInt(WStringView sName, WInt32 iValue);

  /// Returns true if the environment variable with the given name is set, false otherwise.
  static bool IsVariableSet(WStringView sName);

  /// Removes an environment variable from the current execution context (i.e. this process and child processes created after this call).
  static WResult UnsetVariable(WStringView sName);

private:
  /// [internal]
  static WString GetValueStringImpl(WStringView sName, WStringView sDefault);

  /// [internal]
  static WResult SetValueStringImpl(WStringView sName, WStringView sValue);

  /// [internal]
  static bool IsVariableSetImpl(WStringView sName);

  /// [internal]
  static WResult UnsetVariableImpl(WStringView sName);
};
