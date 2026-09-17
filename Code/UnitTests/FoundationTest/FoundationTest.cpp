#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>
#include <iostream>

WInt32 WConstructionCounter::s_iConstructions = 0;
WInt32 WConstructionCounter::s_iDestructions = 0;
WInt32 WConstructionCounter::s_iConstructionsLast = 0;
WInt32 WConstructionCounter::s_iDestructionsLast = 0;

WInt32 WConstructionCounterRelocatable::s_iConstructions = 0;
WInt32 WConstructionCounterRelocatable::s_iDestructions = 0;
WInt32 WConstructionCounterRelocatable::s_iConstructionsLast = 0;
WInt32 WConstructionCounterRelocatable::s_iDestructionsLast = 0;

W_TESTFRAMEWORK_ENTRY_POINT_BEGIN("FoundationTest", "Foundation Tests")
{
  WCommandLineUtils cmd;
  cmd.SetCommandLine(argc, (const char**)argv, WCommandLineUtils::PreferOsArgs);

  // if the -cmd switch is set, FoundationTest.exe will execute a couple of simple operations and then close
  // this is used to test process launching (e.g. WProcess)
  if (cmd.GetBoolOption("-cmd"))
  {
    // print something to stdout
    WStringView sStdOut = cmd.GetStringOption("-stdout");
    if (!sStdOut.IsEmpty())
    {
      WStringBuilder tmp;
      std::cout << sStdOut.GetData(tmp);
    }

    WStringView sStdErr = cmd.GetStringOption("-stderr");
    if (!sStdErr.IsEmpty())
    {
      WStringBuilder tmp;
      std::cerr << sStdErr.GetData(tmp);
    }

    // wait a little
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(cmd.GetIntOption("-sleep")));

    // shutdown with exit code
    WTestSetup::DeInitTestFramework(true);
    return cmd.GetIntOption("-exitcode");
  }
}
W_TESTFRAMEWORK_ENTRY_POINT_END()
