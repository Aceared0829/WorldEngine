#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/CommandLineOptions.h>

namespace
{
  class LogTestLogInterface : public WLogInterface
  {
  public:
    virtual void HandleLogMessage(const WLoggingEventData& le) override
    {
      switch (le.m_EventType)
      {
        case WLogMsgType::Flush:
          m_Result.Append("[Flush]\n");
          return;
        case WLogMsgType::BeginGroup:
          m_Result.Append(">", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::EndGroup:
          m_Result.Append("<", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::ErrorMsg:
          m_Result.Append("E:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::SeriousWarningMsg:
          m_Result.Append("SW:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::WarningMsg:
          m_Result.Append("W:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::SuccessMsg:
          m_Result.Append("S:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::InfoMsg:
          m_Result.Append("I:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::DevMsg:
          m_Result.Append("E:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case WLogMsgType::DebugMsg:
          m_Result.Append("D:", le.m_sTag, " ", le.m_sText, "\n");
          break;

        default:
          W_REPORT_FAILURE("Invalid msg type");
          break;
      }
    }

    WStringBuilder m_Result;
  };

} // namespace

W_CREATE_SIMPLE_TEST(Utility, CommandLineOptions)
{
  WCommandLineOptionDoc optDoc("__test", "-argDoc", "<doc>", "Doc argument", "no value");

  WCommandLineOptionBool optBool1("__test", "-bool1", "bool argument 1", false);
  WCommandLineOptionBool optBool2("__test", "-bool2", "bool argument 2", true);

  WCommandLineOptionInt optInt1("__test", "-int1", "int argument 1", 1);
  WCommandLineOptionInt optInt2("__test", "-int2", "int argument 2", 0, 4, 8);
  WCommandLineOptionInt optInt3("__test", "-int3", "int argument 3", 6, -8, 8);

  WCommandLineOptionFloat optFloat1("__test", "-float1", "float argument 1", 1);
  WCommandLineOptionFloat optFloat2("__test", "-float2", "float argument 2", 0, 4, 8);
  WCommandLineOptionFloat optFloat3("__test", "-float3", "float argument 3", 6, -8, 8);

  WCommandLineOptionString optString1("__test", "-string1", "string argument 1", "default string");

  WCommandLineOptionPath optPath1("__test", "-path1", "path argument 1", "default path");

  WCommandLineOptionEnum optEnum1("__test", "-enum1", "enum argument 1", "A | B = 2 | C | D | E = 7", 3);

  W_TEST_BLOCK(WTestBlock::Enabled, "WCommandLineOptionBool")
  {
    WCommandLineUtils cmd;
    cmd.InjectCustomArgument("-bool1");
    cmd.InjectCustomArgument("on");

    W_TEST_BOOL(optBool1.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd) == true);
    W_TEST_BOOL(optBool2.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd) == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WCommandLineOptionInt")
  {
    WCommandLineUtils cmd;
    cmd.InjectCustomArgument("-int1");
    cmd.InjectCustomArgument("3");

    cmd.InjectCustomArgument("-int2");
    cmd.InjectCustomArgument("10");

    cmd.InjectCustomArgument("-int3");
    cmd.InjectCustomArgument("-2");

    W_TEST_INT(optInt1.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd), 3);
    W_TEST_INT(optInt2.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd), 0);
    W_TEST_INT(optInt3.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd), -2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WCommandLineOptionFloat")
  {
    WCommandLineUtils cmd;
    cmd.InjectCustomArgument("-float1");
    cmd.InjectCustomArgument("3");

    cmd.InjectCustomArgument("-float2");
    cmd.InjectCustomArgument("10");

    cmd.InjectCustomArgument("-float3");
    cmd.InjectCustomArgument("-2");

    W_TEST_FLOAT(optFloat1.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd), 3, 0.001f);
    W_TEST_FLOAT(optFloat2.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd), 0, 0.001f);
    W_TEST_FLOAT(optFloat3.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd), -2, 0.001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WCommandLineOptionString")
  {
    WCommandLineUtils cmd;
    cmd.InjectCustomArgument("-string1");
    cmd.InjectCustomArgument("hello");

    W_TEST_STRING(optString1.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd), "hello");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WCommandLineOptionPath")
  {
    WCommandLineUtils cmd;
    cmd.InjectCustomArgument("-path1");
    cmd.InjectCustomArgument("C:/test");

    const WString path = optPath1.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd);

#if W_ENABLED(W_PLATFORM_WINDOWS)
    W_TEST_STRING(path, "C:/test");
#else
    W_TEST_BOOL(path.EndsWith("C:/test"));
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WCommandLineOptionEnum")
  {
    {
      WCommandLineUtils cmd;
      cmd.InjectCustomArgument("-enum1");
      cmd.InjectCustomArgument("A");

      W_TEST_INT(optEnum1.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd), 0);
    }

    {
      WCommandLineUtils cmd;
      cmd.InjectCustomArgument("-enum1");
      cmd.InjectCustomArgument("B");

      W_TEST_INT(optEnum1.GetOptionValue(WCommandLineOption::LogMode::Never, &cmd), 2);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "LogAvailableOptions")
  {
    WCommandLineUtils cmd;

    WStringBuilder result;

    W_TEST_BOOL(WCommandLineOption::LogAvailableOptionsToBuffer(result, WCommandLineOption::LogAvailableModes::Always, "__test", &cmd));

    W_TEST_STRING(result, "\
\n\
-argDoc <doc> = no value\n\
    Doc argument\n\
\n\
-bool1 <bool> = false\n\
    bool argument 1\n\
\n\
-bool2 <bool> = true\n\
    bool argument 2\n\
\n\
-int1 <int> = 1\n\
    int argument 1\n\
\n\
-int2 <int> [4 .. 8] = 0\n\
    int argument 2\n\
\n\
-int3 <int> [-8 .. 8] = 6\n\
    int argument 3\n\
\n\
-float1 <float> = 1\n\
    float argument 1\n\
\n\
-float2 <float> [4 .. 8] = 0\n\
    float argument 2\n\
\n\
-float3 <float> [-8 .. 8] = 6\n\
    float argument 3\n\
\n\
-string1 <string> = default string\n\
    string argument 1\n\
\n\
-path1 <path> = default path\n\
    path argument 1\n\
\n\
-enum1 <A | B | C | D | E> = C\n\
    enum argument 1\n\
\n\
\n\
");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsHelpRequested")
  {
    WCommandLineUtils cmd;

    W_TEST_BOOL(!WCommandLineOption::IsHelpRequested(&cmd));

    cmd.InjectCustomArgument("-help");

    W_TEST_BOOL(WCommandLineOption::IsHelpRequested(&cmd));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RequireOptions")
  {
    WCommandLineUtils cmd;
    WString missing;

    W_TEST_BOOL(WCommandLineOption::RequireOptions("-opt1 ; -opt2", &missing, &cmd).Failed());
    W_TEST_STRING(missing, "-opt1");

    cmd.InjectCustomArgument("-opt1");

    W_TEST_BOOL(WCommandLineOption::RequireOptions("-opt1 ; -opt2", &missing, &cmd).Failed());
    W_TEST_STRING(missing, "-opt2");

    cmd.InjectCustomArgument("-opt2");

    W_TEST_BOOL(WCommandLineOption::RequireOptions("-opt1 ; -opt2", &missing, &cmd).Succeeded());
    W_TEST_STRING(missing, "");
  }
}
