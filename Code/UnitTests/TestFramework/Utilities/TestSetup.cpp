#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestSetup.h>

#include <TestFramework/Utilities/HTMLOutput.h>

#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/SystemInformation.h>

#ifdef W_USE_QT
#  include <TestFramework/Framework/Qt/qtTestFramework.h>
#  include <TestFramework/Framework/Qt/qtTestGUI.h>
#else
#  include <TestFramework_Platform.h>
#endif

int WTestSetup::s_iArgc = 0;
const char** WTestSetup::s_pArgv = nullptr;

void OutputToConsole(WTestOutput::Enum type, const char* szMsg);

WTestFramework* WTestSetup::InitTestFramework(const char* szTestName, const char* szNiceTestName, int iArgc, const char** pArgv)
{
  s_iArgc = iArgc;
  s_pArgv = pArgv;

  // without a proper file system the current working directory is pretty much useless
  std::string sTestFolder = std::string(WOSFile::GetUserDataFolder());
  if (*sTestFolder.rbegin() != '/')
    sTestFolder.append("/");
  sTestFolder.append("WorldEngine Tests/");
  sTestFolder.append(szTestName);

  std::string sTestDataSubFolder = "Data/UnitTests/";
  sTestDataSubFolder.append(szTestName);

#ifdef W_USE_QT
  WTestFramework* pTestFramework = new WQtTestFramework(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), iArgc, pArgv);
#else
  WTestFramework* pTestFramework = new WTestFramework_Platform(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), iArgc, pArgv);
#endif

  // Register some output handlers to forward all the messages to the console and to an HTML file
  pTestFramework->RegisterOutputHandler(OutputToConsole);
  pTestFramework->RegisterOutputHandler(WOutputToHTML::OutputToHTML);

  WCrashHandler_WriteMiniDump::g_Instance.SetDumpFilePath(pTestFramework->GetAbsOutputPath(), szTestName);
  WCrashHandler::SetCrashHandler(&WCrashHandler_WriteMiniDump::g_Instance);

  return pTestFramework;
}

WTestAppRun WTestSetup::RunTests()
{
  WTestFramework* pTestFramework = WTestFramework::GetInstance();

  // If -list or -help was specified, just quit as these are not supposed to be combined with actual test runs.
  TestSettings settings = pTestFramework->GetSettings();
  if (settings.m_bListTests || settings.m_bShowHelp)
  {
    return WTestAppRun::Quit;
  }

  // Todo: Incorporate all the below in a virtual call of testFramework?
#ifdef W_USE_QT
  if (settings.m_bNoGUI)
  {
    return pTestFramework->RunTestExecutionLoop();
  }

  // Setup Qt Application

  int argc = s_iArgc;
  char** argv = const_cast<char**>(s_pArgv);

  if (qApp != nullptr)
  {
    bool ok = false;
    int iCount = qApp->property("Shared").toInt(&ok);
    W_ASSERT_DEV(ok, "Existing QApplication was not constructed by W!");
    qApp->setProperty("Shared", QVariant::fromValue(iCount + 1));
  }
  else
  {
    new QApplication(argc, argv);
    qApp->setProperty("Shared", QVariant::fromValue((int)1));
    qApp->setApplicationName(pTestFramework->GetTestName());
    WQtTestGUI::SetDarkTheme();
    // Locale fixes required by various third party libraries like RmlGui.
    QLocale::setDefault(QLocale::C);
    const char* locales[] = {"C.UTF-8", "C.utf8", "UTF-8"};
    for (const char* szLocale : locales)
    {
      if (setlocale(LC_ALL, szLocale) != nullptr)
        break;
    }
  }

  // Create main window
  {
    WQtTestGUI mainWindow(*static_cast<WQtTestFramework*>(pTestFramework));
    mainWindow.show();

    qApp->exec();
  }
  {
    const int iCount = qApp->property("Shared").toInt();
    if (iCount == 1)
    {
      delete qApp;
    }
    else
    {
      qApp->setProperty("Shared", QVariant::fromValue(iCount - 1));
    }
  }

  return WTestAppRun::Quit;
#else
  // Run all the tests with the given order
  return pTestFramework->RunTests();
#endif
}

void WTestSetup::DeInitTestFramework(bool bSilent /*= false*/)
{
  WTestFramework* pTestFramework = WTestFramework::GetInstance();

  WStartup::ShutdownCoreSystems();

  TestSettings settings = pTestFramework->GetSettings();
  if (!bSilent)
  {
#if W_ENABLED(W_PLATFORM_WINDOWS)
    if (WSystemInformation::IsDebuggerAttached())
    {
      std::cout << "Press the any key to continue...\n";
      fflush(stdin);
      [[maybe_unused]] int c = getchar();
    }
#endif
  }

  // This is needed as at least windows can't be bothered to write anything
  // to the output streams at all if it's not enough or the app is too fast.
  fflush(stdout);
  fflush(stderr);
  delete pTestFramework;
}

WInt32 WTestSetup::GetFailedTestCount()
{
  return WTestFramework::GetInstance()->GetTestsFailedCount();
}
