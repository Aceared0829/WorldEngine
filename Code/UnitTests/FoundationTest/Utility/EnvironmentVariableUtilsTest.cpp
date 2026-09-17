#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/System/EnvironmentVariableUtils.h>

static WUInt32 uiVersionForVariableSetting = 0;

W_CREATE_SIMPLE_TEST(Utility, EnvironmentVariableUtils)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "GetValueString / GetValueInt")
  {
#if W_ENABLED(W_PLATFORM_WINDOWS)

    // Windows will have "NUMBER_OF_PROCESSORS" and "USERNAME" set, let's see if we can get them
    W_TEST_BOOL(WEnvironmentVariableUtils::IsVariableSet("NUMBER_OF_PROCESSORS"));

    WInt32 iNumProcessors = WEnvironmentVariableUtils::GetValueInt("NUMBER_OF_PROCESSORS", -23);
    W_TEST_BOOL(iNumProcessors > 0);

    W_TEST_BOOL(WEnvironmentVariableUtils::IsVariableSet("USERNAME"));
    WString szUserName = WEnvironmentVariableUtils::GetValueString("USERNAME");
    W_TEST_BOOL(szUserName.GetElementCount() > 0);

#elif W_ENABLED(W_PLATFORM_OSX) || W_ENABLED(W_PLATFORM_LINUX)

    // Mac OS & Linux will have "USER" set
    W_TEST_BOOL(WEnvironmentVariableUtils::IsVariableSet("USER"));
    WString szUserName = WEnvironmentVariableUtils::GetValueString("USER");
    W_TEST_BOOL(szUserName.GetElementCount() > 0);

#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsVariableSet/SetValue")
  {
    WStringBuilder szVarName;
    szVarName.SetFormat("W_THIS_SHOULDNT_EXIST_NOW_OR_THIS_TEST_WILL_FAIL_{0}", uiVersionForVariableSetting++);

    W_TEST_BOOL(!WEnvironmentVariableUtils::IsVariableSet(szVarName));

    WEnvironmentVariableUtils::SetValueString(szVarName, "NOW_IT_SHOULD_BE").IgnoreResult();
    W_TEST_BOOL(WEnvironmentVariableUtils::IsVariableSet(szVarName));

    W_TEST_STRING(WEnvironmentVariableUtils::GetValueString(szVarName), "NOW_IT_SHOULD_BE");

    // Test overwriting the same value again
    WEnvironmentVariableUtils::SetValueString(szVarName, "NOW_IT_SHOULD_BE_SOMETHING_ELSE").IgnoreResult();
    W_TEST_STRING(WEnvironmentVariableUtils::GetValueString(szVarName), "NOW_IT_SHOULD_BE_SOMETHING_ELSE");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Variable with very long value")
  {
    // The Windows implementation has a 64 wchar_t buffer for example. Let's try setting a really
    // long variable and getting it back
    const char* szLongVariable =
      "SOME REALLY LONG VALUE, LETS TEST SOME LIMITS WE MIGHT HIT - 012456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz";

    WStringBuilder szVarName;
    szVarName.SetFormat("W_LONG_VARIABLE_TEST_{0}", uiVersionForVariableSetting++);

    W_TEST_BOOL(!WEnvironmentVariableUtils::IsVariableSet(szVarName));

    WEnvironmentVariableUtils::SetValueString(szVarName, szLongVariable).IgnoreResult();
    W_TEST_BOOL(WEnvironmentVariableUtils::IsVariableSet(szVarName));

    W_TEST_STRING(WEnvironmentVariableUtils::GetValueString(szVarName), szLongVariable);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Unsetting variables")
  {
    const char* szVarName = "W_TEST_HELLO_WORLD";
    W_TEST_BOOL(!WEnvironmentVariableUtils::IsVariableSet(szVarName));

    WEnvironmentVariableUtils::SetValueString(szVarName, "TEST").IgnoreResult();

    W_TEST_BOOL(WEnvironmentVariableUtils::IsVariableSet(szVarName));

    WEnvironmentVariableUtils::UnsetVariable(szVarName).IgnoreResult();
    W_TEST_BOOL(!WEnvironmentVariableUtils::IsVariableSet(szVarName));
  }
}
