#include <FoundationTest/FoundationTestPCH.h>

#if W_ENABLED(W_SUPPORTS_PROCESSES)

#  include <Foundation/System/ProcessGroup.h>
#  include <Foundation/Utilities/CommandLineUtils.h>

W_CREATE_SIMPLE_TEST(System, ProcessGroup)
{
  // we can launch FoundationTest with the -cmd parameter to execute a couple of useful things to test launching process
  const WStringBuilder pathToSelf = WCommandLineUtils::GetGlobalInstance()->GetParameter(0);

  W_TEST_BLOCK(WTestBlock::Enabled, "WaitToFinish")
  {
    WProcessGroup pgroup;
    WStringBuilder out;

    WMutex mutex;

    for (WUInt32 i = 0; i < 8; ++i)
    {
      WProcessOptions opt;
      opt.m_sProcess = pathToSelf;
      opt.m_onStdOut = [&out, &mutex](WStringView sView)
      {
        W_LOCK(mutex);
        out.Append(sView);
      };

      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("1000");
      opt.m_Arguments.PushBack("-stdout");
      opt.m_Arguments.PushBack("Na");

      W_TEST_BOOL(pgroup.Launch(opt).Succeeded());
    }

    // in a debugger with child debugging enabled etc. even 10 seconds can lead to timeouts due to long delays in the IDE
    W_TEST_BOOL(pgroup.WaitToFinish(WTime::MakeFromSeconds(60)).Succeeded());
    W_TEST_STRING(out, "NaNaNaNaNaNaNaNa"); // BATMAN!
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TerminateAll")
  {
    WProcessGroup pgroup;

    WTempHybridArray<WProcess, 8> procs;

    for (WUInt32 i = 0; i < 8; ++i)
    {
      WProcessOptions opt;
      opt.m_sProcess = pathToSelf;

      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("60000");

      W_TEST_BOOL(pgroup.Launch(opt).Succeeded());
    }

    const WTime tStart = WTime::Now();
    W_TEST_BOOL(pgroup.TerminateAll().Succeeded());
    const WTime tDiff = WTime::Now() - tStart;

    W_TEST_BOOL(tDiff < WTime::MakeFromSeconds(10));
  }
}
#endif
