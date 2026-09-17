#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/System/Process.h>
#include <Foundation/Utilities/CommandLineUtils.h>

W_CREATE_SIMPLE_TEST_GROUP(System);

#if W_ENABLED(W_SUPPORTS_PROCESSES)

W_CREATE_SIMPLE_TEST(System, Process)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Command Line")
  {
    WProcessOptions proc;
    proc.m_Arguments.PushBack("-bla");
    proc.m_Arguments.PushBack("blub blub");
    proc.m_Arguments.PushBack("\"di dub\"");
    proc.AddArgument(" -test ");
    proc.AddArgument("-hmpf {}", 27);
    proc.AddCommandLine("-a b   -c  d  -e \"f g h\" ");

    WStringBuilder cmdLine;
    proc.BuildCommandLineString(cmdLine);

    W_TEST_STRING(cmdLine, "-bla \"blub blub\" \"di dub\" \" -test \" \"-hmpf 27\" -a b -c d -e \"f g h\"");
  }

  static const char* g_szTestMsg = "Tell me more!\nAnother line\n520CharactersInOneLineAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_END\nThat's all";
  static const char* g_szTestMsgLine0 = "Tell me more!\n";
  static const char* g_szTestMsgLine1 = "Another line\n";
  static const char* g_szTestMsgLine2 = "520CharactersInOneLineAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_END\n";
  static const char* g_szTestMsgLine3 = "That's all";


  // we can launch FoundationTest with the -cmd parameter to execute a couple of useful things to test launching process
  const WStringBuilder pathToSelf = WCommandLineUtils::GetGlobalInstance()->GetParameter(0);

  W_TEST_BLOCK(WTestBlock::Enabled, "Execute")
  {
    WProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("500");

    WInt32 exitCode = -1;

    if (!W_TEST_BOOL_MSG(WProcess::Execute(opt, &exitCode).Succeeded(), "Failed to start process."))
      return;

    W_TEST_INT(exitCode, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Launch / WaitToFinish")
  {
    WProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("500");

    WProcess proc;
    W_TEST_BOOL(proc.GetState() == WProcessState::NotStarted);

    if (!W_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
      return;

    W_TEST_BOOL(proc.GetState() == WProcessState::Running);
    W_TEST_BOOL(proc.WaitToFinish(WTime::MakeFromSeconds(5)).Succeeded());
    W_TEST_BOOL(proc.GetState() == WProcessState::Finished);
    W_TEST_INT(proc.GetExitCode(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Launch / Terminate")
  {
    WProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("10000");
    opt.m_Arguments.PushBack("-exitcode");
    opt.m_Arguments.PushBack("0");

    WProcess proc;
    W_TEST_BOOL(proc.GetState() == WProcessState::NotStarted);

    if (!W_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
      return;

    W_TEST_BOOL(proc.GetState() == WProcessState::Running);
    W_TEST_BOOL(proc.Terminate().Succeeded());
    W_TEST_BOOL(proc.GetState() == WProcessState::Finished);
    W_TEST_INT(proc.GetExitCode(), -1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Launch / Detach")
  {
    WTime tTerminate;

    {
      WProcessOptions opt;
      opt.m_sProcess = pathToSelf;
      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("10000");

      WProcess proc;
      if (!W_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
        return;

      proc.Detach();

      tTerminate = WTime::Now();
    }

    const WTime tDiff = WTime::Now() - tTerminate;
    W_TEST_BOOL_MSG(tDiff < WTime::MakeFromSeconds(1.0), "Destruction of WProcess should be instant after Detach() was used.");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "STDOUT")
  {
    WDynamicArray<WStringBuilder> lines;
    WStringBuilder out;
    WProcessOptions opt;
    opt.m_onStdOut = [&](WStringView sView)
    {
      out.Append(sView);
      lines.PushBack(sView);
    };

    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stdout");
    opt.m_Arguments.PushBack(g_szTestMsg);

    if (!W_TEST_BOOL_MSG(WProcess::Execute(opt).Succeeded(), "Failed to start process."))
      return;

    if (W_TEST_BOOL(lines.GetCount() == 4))
    {
      lines[0].ReplaceAll("\r\n", "\n");
      W_TEST_STRING(lines[0], g_szTestMsgLine0);
      lines[1].ReplaceAll("\r\n", "\n");
      W_TEST_STRING(lines[1], g_szTestMsgLine1);
      lines[2].ReplaceAll("\r\n", "\n");
      W_TEST_STRING(lines[2], g_szTestMsgLine2);
      lines[3].ReplaceAll("\r\n", "\n");
      W_TEST_STRING(lines[3], g_szTestMsgLine3);
    }

    out.ReplaceAll("\r\n", "\n");
    W_TEST_STRING(out, g_szTestMsg);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "STDERROR")
  {
    WStringBuilder err;
    WProcessOptions opt;
    opt.m_onStdError = [&err](WStringView sView)
    { err.Append(sView); };

    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stderr");
    opt.m_Arguments.PushBack("NOT A VALID COMMAND");
    opt.m_Arguments.PushBack("-exitcode");
    opt.m_Arguments.PushBack("1");

    WInt32 exitCode = 0;

    if (!W_TEST_BOOL_MSG(WProcess::Execute(opt, &exitCode).Succeeded(), "Failed to start process."))
      return;

    W_TEST_BOOL_MSG(!err.IsEmpty(), "Error stream should contain something.");
    W_TEST_INT(exitCode, 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "STDOUT_STDERROR")
  {
    WDynamicArray<WStringBuilder> lines;
    WStringBuilder out;
    WStringBuilder err;
    WProcessOptions opt;
    opt.m_onStdOut = [&](WStringView sView)
    {
      out.Append(sView);
      lines.PushBack(sView);
    };
    opt.m_onStdError = [&err](WStringView sView)
    { err.Append(sView); };
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stdout");
    opt.m_Arguments.PushBack(g_szTestMsg);

    if (!W_TEST_BOOL_MSG(WProcess::Execute(opt).Succeeded(), "Failed to start process."))
      return;

    if (W_TEST_BOOL(lines.GetCount() == 4))
    {
      lines[0].ReplaceAll("\r\n", "\n");
      W_TEST_STRING(lines[0], g_szTestMsgLine0);
      lines[1].ReplaceAll("\r\n", "\n");
      W_TEST_STRING(lines[1], g_szTestMsgLine1);
      lines[2].ReplaceAll("\r\n", "\n");
      W_TEST_STRING(lines[2], g_szTestMsgLine2);
      lines[3].ReplaceAll("\r\n", "\n");
      W_TEST_STRING(lines[3], g_szTestMsgLine3);
    }

    out.ReplaceAll("\r\n", "\n");
    W_TEST_STRING(out, g_szTestMsg);
    W_TEST_BOOL_MSG(err.IsEmpty(), "Error stream should be empty.");
  }
}
#endif
