#include <TestFramework/TestFrameworkPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/TraceWriter.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/System/Process.h>
#include <Foundation/System/StackTracer.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <TestFramework/Utilities/TestOrder.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <cstdlib>
#include <stdexcept>

#ifdef W_TESTFRAMEWORK_USE_FILESERVE
#  include <FileservePlugin/Client/FileserveClient.h>
#  include <FileservePlugin/Client/FileserveDataDir.h>
#  include <FileservePlugin/FileservePluginDLL.h>
#endif

WTestFramework* WTestFramework::s_pInstance = nullptr;

const char* WTestFramework::s_szTestBlockName = "";
int WTestFramework::s_iAssertCounter = 0;
bool WTestFramework::s_bCallstackOnAssert = false;
WLog::TimestampMode WTestFramework::s_LogTimestampMode = WLog::TimestampMode::None;

WCommandLineOptionPath opt_OrderFile("_TestFramework", "-order", "Path to a file that defines which tests to run.", "");
WCommandLineOptionPath opt_SettingsFile("_TestFramework", "-settings", "Path to a file containing the test settings.", "");
WCommandLineOptionBool opt_NoGui("_TestFramework", "-noGui", "Runs the tests in console mode, without showing a window. This is what automated runs want: the tests start on their own and the process exits when they are done, whether they passed or not.", false);
WCommandLineOptionBool opt_Timestamps("_TestFramework", "-timestamps", "Show timestamps in logs.", false);
WCommandLineOptionInt opt_Revision("_TestFramework", "-rev", "Revision number to pass through to JSON output.", -1);
WCommandLineOptionInt opt_Assert("_TestFramework", "-assert", "Whether to assert when a test fails.", (int)AssertOnTestFail::AssertIfDebuggerAttached);
WCommandLineOptionString opt_Filter("_TestFramework", "-filter",
  "Only run tests whose name contains this (case insensitive), matched against both test and sub-test names, so '-filter JSON' runs "
  "everything with JSON in the name. Shell style wildcards are accepted too: '*' and '?', so '-filter \"IO*Stream\"' or "
  "'-filter \"IOStream?\"' work as expected. Without this every test runs. Use -list to see the available names.",
  "");
WCommandLineOptionPath opt_Json("_TestFramework", "-json", "JSON file to write.", "");
WCommandLineOptionPath opt_OutputDir("_TestFramework", "-outputDir", "Output directory", "");
WCommandLineOptionBool opt_List("_TestFramework", "-list", "List all test names and exit.", false);
WCommandLineOptionBool opt_DebugDevice("_TestFramework", "-debugdevice", "Whether to create a debug GAL device.", false);

constexpr int s_iMaxErrorMessageLength = 512;

// Standard iterative wildcard match ('*' = any run of characters, '?' = exactly one), case insensitive.
// Matches the whole string, so a caller wanting a "contains" match has to write the surrounding '*' themselves.
static bool MatchesWildcard_NoCase(const char* szText, const char* szPattern)
{
  const char* szTextStar = nullptr;
  const char* szPatternStar = nullptr;

  auto ToLower = [](char c) -> char
  { return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c; };

  while (*szText != '\0')
  {
    if (*szPattern == '*')
    {
      szPatternStar = szPattern++;
      szTextStar = szText;
    }
    else if (*szPattern == '?' || ToLower(*szPattern) == ToLower(*szText))
    {
      ++szPattern;
      ++szText;
    }
    else if (szPatternStar != nullptr)
    {
      szPattern = szPatternStar + 1;
      szText = ++szTextStar;
    }
    else
    {
      return false;
    }
  }

  while (*szPattern == '*')
    ++szPattern;

  return *szPattern == '\0';
}

static bool TestAssertHandler(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  if (WTestFramework::s_bCallstackOnAssert)
  {
    void* pBuffer[64];
    WArrayPtr<void*> tempTrace(pBuffer);
    const WUInt32 uiNumTraces = WStackTracer::GetStackTrace(tempTrace, nullptr);
    WStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &WLog::Print);
  }

  WTestFramework::Error(szExpression, szSourceFile, (WInt32)uiLine, szFunction, szAssertMsg);

  // if a debugger is attached, one typically always wants to know about asserts
  if (WSystemInformation::IsDebuggerAttached())
    return true;

  WTestFramework::GetInstance()->AbortTests();

  return WTestFramework::GetAssertOnTestFail();
}

////////////////////////////////////////////////////////////////////////
// WTestFramework public functions
////////////////////////////////////////////////////////////////////////

WTestFramework::WTestFramework(const char* szTestName, const char* szAbsTestOutputDir, const char* szRelTestDataDir, int iArgc, const char** pArgv)
  : m_sTestName(szTestName)
  , m_sAbsTestOutputDir(szAbsTestOutputDir)
  , m_sRelTestDataDir(szRelTestDataDir)
{
  s_pInstance = this;

  WCommandLineUtils::GetGlobalInstance()->SetCommandLine(iArgc, pArgv, WCommandLineUtils::PreferOsArgs);

  GetTestSettingsFromCommandLine(*WCommandLineUtils::GetGlobalInstance());
}

WTestFramework::~WTestFramework()
{
  if (m_bIsInitialized)
    DeInitialize();
  s_pInstance = nullptr;
}

void WTestFramework::Initialize()
{
  m_Settings.m_bShowHelp = WCommandLineOption::IsHelpRequested();

  if (m_Settings.m_bNoGUI)
  {
    // if the UI is run with GUI disabled, set the environment variable W_SILENT_ASSERTS
    // to make sure that no child process that the tests launch shows an assert dialog in case of a crash
    WEnvironmentVariableUtils::SetValueInt("W_SILENT_ASSERTS", 1).IgnoreResult();
  }

  if (m_Settings.m_bShowTimestampsInLog)
  {
    WTestFramework::s_LogTimestampMode = WLog::TimestampMode::TimeOnly;
    WLogWriter::Console::SetTimestampMode(WLog::TimestampMode::TimeOnly);
  }

  // Don't do this, it will spam the log with sub-system messages
  // WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);
  // WGlobalLog::AddLogWriter(WLogWriter::VisualStudio::LogMessageHandler);

  WStartup::AddApplicationTag("testframework");
  WStartup::StartupCoreSystems();
  W_SCOPE_EXIT(WStartup::ShutdownCoreSystems());

  // We have exit here after the core systems logic, or we hit issues with allocators of RTTI types.
  if (m_Settings.m_bShowHelp)
  {
    // Printed here rather than at the start of this function: before StartupCoreSystems() there is no
    // log writer yet, so WLog::Print() went nowhere and '-help' produced no output at all.
    WStringBuilder cmdHelp;
    if (WCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, WCommandLineOption::LogAvailableModes::IfHelpRequested, "_TestFramework;cvar"))
    {
      WLog::Print(cmdHelp);
    }

    return;
  }

  // if tests need to write data back through Fileserve (e.g. image comparison results), they can do that through a data dir mounted with
  // this path
  WFileSystem::SetSpecialDirectory("Wtest", WTestFramework::GetInstance()->GetAbsOutputPath());

  // Setting W assert handler
  m_PreviousAssertHandler = WGetAssertHandler();
  WSetAssertHandler(TestAssertHandler);

  CreateOutputFolder();
  WFileSystem::DetectSdkRootDirectory().IgnoreResult();

  WCommandLineUtils& cmd = *WCommandLineUtils::GetGlobalInstance();
  // figure out which tests exist
  GatherAllTests();

  // Handle -list option: print all test names and exit
  if (opt_List.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified, &cmd))
  {
    for (const auto& testEntry : m_TestEntries)
    {
      WStringBuilder line;
      line.SetFormat("{}\n", testEntry.m_szTestName);
      WLog::Print(line);
    }
    m_Settings.m_bListTests = true;
    m_bIsInitialized = true;
    return;
  }

  if (!m_Settings.m_bNoGUI || opt_OrderFile.IsOptionSpecified(nullptr, &cmd))
  {
    // load the test order from file, if that file does not exist, the array is not modified.
    LoadTestOrder();
  }
  ApplyTestOrderFromCommandLine(cmd);

  if (!m_Settings.m_bNoGUI || opt_SettingsFile.IsOptionSpecified(nullptr, &cmd))
  {
    // Load the test settings from file, if that file does not exist, the settings are not modified.
    LoadTestSettings();
    // Overwrite loaded test settings with command line
    GetTestSettingsFromCommandLine(cmd);
  }

  // save the current order back to the same file
  AutoSaveTestOrder();

  m_bIsInitialized = true;
}

void WTestFramework::DeInitialize()
{
  m_bIsInitialized = false;

  // Initialize() returns early for -help and -list, before the assert handler is installed, so there
  // may be nothing to restore. Setting a null handler crashes on the next assert.
  if (m_PreviousAssertHandler != nullptr)
  {
    WSetAssertHandler(m_PreviousAssertHandler);
    m_PreviousAssertHandler = nullptr;
  }
}

const char* WTestFramework::GetTestName() const
{
  return m_sTestName.c_str();
}

const char* WTestFramework::GetAbsOutputPath() const
{
  return m_sAbsTestOutputDir.c_str();
}


const char* WTestFramework::GetRelTestDataPath() const
{
  return m_sRelTestDataDir.c_str();
}

const char* WTestFramework::GetAbsTestOrderFilePath() const
{
  return m_sAbsTestOrderFilePath.c_str();
}

const char* WTestFramework::GetAbsTestSettingsFilePath() const
{
  return m_sAbsTestSettingsFilePath.c_str();
}

void WTestFramework::RegisterOutputHandler(OutputHandler handler)
{
  // do not register a handler twice
  for (WUInt32 i = 0; i < m_OutputHandlers.size(); ++i)
  {
    if (m_OutputHandlers[i] == handler)
      return;
  }

  m_OutputHandlers.push_back(handler);
}


void WTestFramework::SetImageDiffExtraInfoCallback(ImageDiffExtraInfoCallback provider)
{
  m_ImageDiffExtraInfoCallback = provider;
}

bool WTestFramework::GetAssertOnTestFail()
{
  switch (s_pInstance->m_Settings.m_AssertOnTestFail)
  {
    case AssertOnTestFail::DoNotAssert:
      return false;
    case AssertOnTestFail::AssertIfDebuggerAttached:
      return WSystemInformation::IsDebuggerAttached();
    case AssertOnTestFail::AlwaysAssert:
      return true;
  }
  return false;
}

void WTestFramework::GatherAllTests()
{
  m_TestEntries.clear();

  m_iErrorCount = 0;
  m_iTestsFailed = 0;
  m_iTestsPassed = 0;
  m_uiExecutingTest = WInvalidIndex;
  m_uiExecutingSubTest = WInvalidIndex;
  m_bSubTestInitialized = false;

  // first let all simple tests register themselves
  {
    WRegisterTestHelper* pHelper = WRegisterTestHelper::GetFirstInstance();

    while (pHelper)
    {
      pHelper->RegisterTest();

      pHelper = pHelper->GetNextInstance();
    }
  }

  WTestConfiguration config;
  WTestBaseClass* pTestClass = WTestBaseClass::GetFirstInstance();

  while (pTestClass)
  {
    pTestClass->ClearSubTests();
    pTestClass->SetupSubTests();
    pTestClass->UpdateConfiguration(config);

    WTestEntry e;
    e.m_pTest = pTestClass;
    e.m_szTestName = pTestClass->GetTestName();
    e.m_sNotAvailableReason = pTestClass->IsTestAvailable();

    for (WUInt32 i = 0; i < pTestClass->m_Entries.size(); ++i)
    {
      WSubTestEntry st;
      st.m_szSubTestName = pTestClass->m_Entries[i].m_szName;
      st.m_iSubTestIdentifier = pTestClass->m_Entries[i].m_iIdentifier;

      e.m_SubTests.push_back(st);
    }

    m_TestEntries.push_back(e);

    pTestClass = pTestClass->GetNextInstance();
  }
  ::SortTestsAlphabetically(m_TestEntries);

  m_Result.SetupTests(m_TestEntries, config);
}

void WTestFramework::GetTestSettingsFromCommandLine(const WCommandLineUtils& cmd)
{
  // use a local instance of WCommandLineUtils as global instance is not guaranteed to have been set up
  // for all call sites of this method.

  m_Settings.m_bNoGUI = opt_NoGui.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  if (opt_Assert.IsOptionSpecified(nullptr, &cmd))
  {
    const int assertOnTestFailure = opt_Assert.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
    switch (assertOnTestFailure)
    {
      case 0:
        m_Settings.m_AssertOnTestFail = AssertOnTestFail::DoNotAssert;
        break;
      case 1:
        m_Settings.m_AssertOnTestFail = AssertOnTestFail::AssertIfDebuggerAttached;
        break;
      case 2:
        m_Settings.m_AssertOnTestFail = AssertOnTestFail::AlwaysAssert;
        break;
    }
  }

  WStringBuilder tmp;

  opt_Timestamps.SetDefaultValue(m_Settings.m_bShowTimestampsInLog);
  m_Settings.m_bShowTimestampsInLog = opt_Timestamps.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  m_Settings.m_iRevision = opt_Revision.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  m_Settings.m_sTestFilter = opt_Filter.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified, &cmd).GetData(tmp);

  if (opt_Json.IsOptionSpecified(nullptr, &cmd))
  {
    m_Settings.m_sJsonOutput = opt_Json.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  }

  if (opt_OutputDir.IsOptionSpecified(nullptr, &cmd))
  {
    m_sAbsTestOutputDir = opt_OutputDir.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  }

  if (m_Settings.m_bNoGUI)
  {
    // In console mode nobody changed the test order or the settings interactively, so there is nothing
    // worth persisting - and writing it would overwrite what the GUI last stored.
    m_Settings.m_bSaveState = false;
  }

  if (opt_OrderFile.IsOptionSpecified(nullptr, &cmd))
  {
    m_sAbsTestOrderFilePath = opt_OrderFile.GetOptionValue(WCommandLineOption::LogMode::Always);
    // If a custom order file was provided, don't overwrite it
    m_Settings.m_bSaveState = false;
  }
  else
  {
    m_sAbsTestOrderFilePath = m_sAbsTestOutputDir + std::string("/TestOrder.txt");
  }

  if (opt_SettingsFile.IsOptionSpecified(nullptr, &cmd))
  {
    m_sAbsTestSettingsFilePath = opt_SettingsFile.GetOptionValue(WCommandLineOption::LogMode::Always);
    // If a custom settings file was provided, don't overwrite it
    m_Settings.m_bSaveState = false;
  }
  else
  {
    m_sAbsTestSettingsFilePath = m_sAbsTestOutputDir + std::string("/TestSettings.txt");
  }
}

void WTestFramework::LoadTestOrder()
{
  ::LoadTestOrder(m_sAbsTestOrderFilePath.c_str(), m_TestEntries);
}

void WTestFramework::ApplyTestOrderFromCommandLine(const WCommandLineUtils& cmd)
{
  // A filter decides on its own which tests run, overriding whatever a test order file enabled or
  // disabled - otherwise a stale order file would silently subtract from what was asked for. Without
  // a filter nothing is changed here, and every test is enabled unless an order file said otherwise.
  if (!m_Settings.m_sTestFilter.empty())
  {
    const char* szFilter = m_Settings.m_sTestFilter.c_str();
    const bool bHasWildcard = strchr(szFilter, '*') != nullptr || strchr(szFilter, '?') != nullptr;

    // Without a wildcard the filter is a plain 'contains' check, which is what most people type
    // ('-filter JSON'). With a wildcard it becomes a full match, so the caller controls where the
    // free parts are ('-filter "IO*Stream"').
    auto MatchesFilter = [&](const char* szName)
    {
      if (bHasWildcard)
        return MatchesWildcard_NoCase(szName, szFilter);

      return WStringUtils::FindSubString_NoCase(szName, szFilter) != nullptr;
    };

    const WUInt32 uiTestCount = GetTestCount();
    for (WUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
    {
      const bool bEnableTest = MatchesFilter(m_TestEntries[uiTestIdx].m_szTestName);
      bool bAnySubTestEnabled = bEnableTest;
      const WUInt32 uiSubTestCount = (WUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
      for (WUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
      {
        const bool bEnableSubTest = bEnableTest || MatchesFilter(m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_szSubTestName);
        m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_bEnableTest = bEnableSubTest;
        bAnySubTestEnabled |= bEnableSubTest;
      }
      m_TestEntries[uiTestIdx].m_bEnableTest = bAnySubTestEnabled;
    }
  }
}

void WTestFramework::LoadTestSettings()
{
  ::LoadTestSettings(m_sAbsTestSettingsFilePath.c_str(), m_Settings);
}

void WTestFramework::CreateOutputFolder()
{
  WOSFile::CreateDirectoryStructure(m_sAbsTestOutputDir.c_str()).IgnoreResult();

  W_ASSERT_RELEASE(WOSFile::ExistsDirectory(m_sAbsTestOutputDir.c_str()), "Failed to create output directory '{0}'", m_sAbsTestOutputDir.c_str());
}

void WTestFramework::UpdateReferenceImages()
{
  WStringBuilder sDir;
  if (WFileSystem::ResolveSpecialDirectory(">sdk", sDir).Failed())
    return;

  sDir.AppendPath(GetRelTestDataPath());

  const WStringBuilder sNewFiles(m_sAbsTestOutputDir.c_str(), "/Images_Result");
  const WStringBuilder sRefFiles(sDir, "/", m_sImageReferenceFolderName.c_str());

#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS) && W_ENABLED(W_SUPPORTS_FILE_STATS)

  // Check if optipng is available
  bool bOptiPngAvailable = false;
  WStringBuilder sOptiPng;

#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  sOptiPng = WFileSystem::GetSdkRootDirectory();
  sOptiPng.AppendPath("Data/Tools/Precompiled/optipng/optipng.exe");
  bOptiPngAvailable = WOSFile::ExistsFile(sOptiPng);
#  elif W_ENABLED(W_PLATFORM_LINUX)
  sOptiPng = "optipng";
  WProcessOptions po;
  po.AddArgument("-h");
  po.m_sProcess = sOptiPng;
  bOptiPngAvailable = WProcess::Execute(po).Succeeded();
#  endif

  if (bOptiPngAvailable)
  {
    WStringBuilder sPath;

    WFileSystemIterator it;
    it.StartSearch(sNewFiles, WFileSystemIteratorFlags::ReportFiles);
    for (; it.IsValid(); it.Next())
    {
      it.GetStats().GetFullPath(sPath);

      WProcessOptions opt;
      opt.m_sProcess = sOptiPng;
      opt.m_Arguments.PushBack(sPath);
      WProcess::Execute(opt).IgnoreResult();
    }
  }

  // If a suffixed variant of a result file already exists in the reference folder, update it in-place.
  // This handles the case where someone previously captured a suffixed reference image (e.g. "foo-amd.png")
  // and now wants to update it: the new result "foo.png" gets moved to "foo-amd.png" if that file exists.
  {
    WStringBuilder sFullPath, sResultFileName, sTargetPath;

    WFileSystemIterator it;
    it.StartSearch(sNewFiles, WFileSystemIteratorFlags::ReportFiles);
    for (; it.IsValid(); it.Next())
    {
      it.GetStats().GetFullPath(sFullPath);
      sResultFileName = it.GetStats().m_sName;
      sResultFileName.RemoveFileExtension(); // e.g. "Basics_Line_Rendering_000"

      // Search the reference folder for any file whose name starts with this base name followed by '-'
      WFileSystemIterator refIt;
      refIt.StartSearch(sRefFiles, WFileSystemIteratorFlags::ReportFiles);
      for (; refIt.IsValid(); refIt.Next())
      {
        WStringBuilder sRefFileName = refIt.GetStats().m_sName;
        sRefFileName.RemoveFileExtension();

        if (sRefFileName.StartsWith(sResultFileName) && sRefFileName.GetCharacterCount() > sResultFileName.GetCharacterCount() && sRefFileName.GetData()[sResultFileName.GetElementCount()] == '-')
        {
          refIt.GetStats().GetFullPath(sTargetPath);
          WOSFile::DeleteFile(sTargetPath).IgnoreResult();
          WOSFile::MoveFileOrDirectory(sFullPath, sTargetPath).IgnoreResult();
          break;
        }
      }
    }
  }

  // copy the remaining files to the default directory
  WOSFile::CopyFolder(sNewFiles, sRefFiles).IgnoreResult();
  WOSFile::DeleteFolder(sNewFiles).IgnoreResult();
#endif
}

void WTestFramework::AutoSaveTestOrder()
{
  if (!m_Settings.m_bSaveState)
    return;

  SaveTestOrder(m_sAbsTestOrderFilePath.c_str());
  SaveTestSettings(m_sAbsTestSettingsFilePath.c_str());
}

void WTestFramework::SaveTestOrder(const char* const szFilePath)
{
  ::SaveTestOrder(szFilePath, m_TestEntries);
}

void WTestFramework::SaveTestSettings(const char* const szFilePath)
{
  ::SaveTestSettings(szFilePath, m_Settings);
}

void WTestFramework::SetAllTestsEnabledStatus(bool bEnable)
{
  const WUInt32 uiTestCount = GetTestCount();
  for (WUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_TestEntries[uiTestIdx].m_bEnableTest = bEnable;
    const WUInt32 uiSubTestCount = (WUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
    for (WUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
    {
      m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_bEnableTest = bEnable;
    }
  }
}

void WTestFramework::SetAllFailedTestsEnabledStatus()
{
  const auto& LastResult = GetTestResult();

  const WUInt32 uiTestCount = GetTestCount();
  for (WUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    const auto& TestRes = LastResult.GetTestResultData(uiTestIdx, -1);
    m_TestEntries[uiTestIdx].m_bEnableTest = TestRes.m_bExecuted && !TestRes.m_bSuccess;

    const WUInt32 uiSubTestCount = (WUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
    for (WUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
    {
      const auto& SubTestRes = LastResult.GetTestResultData(uiTestIdx, uiSubTest);
      m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_bEnableTest = SubTestRes.m_bExecuted && !SubTestRes.m_bSuccess;
    }
  }
}

void WTestFramework::SetTestTimeout(WUInt32 uiTestTimeoutMS)
{
  {
    std::scoped_lock<std::mutex> lock(m_TimeoutLock);
    m_uiTimeoutMS = uiTestTimeoutMS;
  }
  UpdateTestTimeout();
}

WUInt32 WTestFramework::GetTestTimeout() const
{
  return m_uiTimeoutMS;
}

void WTestFramework::TimeoutThread()
{
  std::unique_lock<std::mutex> lock(m_TimeoutLock);
  while (m_bUseTimeout)
  {
    if (m_uiTimeoutMS == 0)
    {
      // If no timeout is set, we simply put the thread to sleep.
      m_TimeoutCV.wait(lock, [this]
        { return !m_bUseTimeout; });
    }
    // We want to be notified when we reach the timeout and not when we are spuriously woken up.
    // Thus we continue waiting via the predicate if we are still using a timeout until we are either
    // woken up via the CV or reach the timeout.
    else if (!m_TimeoutCV.wait_for(lock, std::chrono::milliseconds(m_uiTimeoutMS), [this]
               { return !m_bUseTimeout || m_bArm; }))
    {
      if (WSystemInformation::IsDebuggerAttached())
      {
        // Should we attach a debugger mid run and reach the timeout we obviously do not want to terminate.
        continue;
      }

      // CV was not signaled until the timeout was reached.
      WTestFramework::Output(WTestOutput::Error, "Timeout reached, terminating app.");
      // The top level exception handler takes care of all the shutdown logic already (app specific logic, crash dump, callstack etc)
      // which we do not want to duplicate here so we simply throw an unhandled exception.
      throw std::runtime_error("Timeout reached, terminating app.");
    }
    m_bArm = false;
  }
}


void WTestFramework::UpdateTestTimeout()
{
  {
    std::scoped_lock<std::mutex> lock(m_TimeoutLock);
    if (!m_bUseTimeout)
    {
      return;
    }
    m_bArm = true;
  }
  m_TimeoutCV.notify_one();
}

void WTestFramework::ResetTests()
{
  m_iErrorCount = 0;
  m_iTestsFailed = 0;
  m_iTestsPassed = 0;
  m_uiExecutingTest = WInvalidIndex;
  m_uiExecutingSubTest = WInvalidIndex;
  m_bSubTestInitialized = false;
  m_bAbortTests = false;

  m_Result.Reset();
}

WTestAppRun WTestFramework::RunTestExecutionLoop()
{
  if (!m_bIsInitialized)
  {
    Initialize();

#ifdef W_TESTFRAMEWORK_USE_FILESERVE
    if (WFileserveClient::GetSingleton() == nullptr)
    {
      W_DEFAULT_NEW(WFileserveClient);

      if (WFileserveClient::GetSingleton()->SearchForServerAddress().Failed())
      {
        WFileserveClient::GetSingleton()->WaitForServerInfo().IgnoreResult();
      }
    }

    if (WFileserveClient::GetSingleton()->EnsureConnected(WTime::MakeFromSeconds(-30)).Failed())
    {
      Error("Failed to establish a Fileserve connection", "", 0, "WTestFramework::RunTestExecutionLoop", "");
      return WTestAppRun::Quit;
    }
#endif
  }

#ifdef W_TESTFRAMEWORK_USE_FILESERVE
  WFileserveClient::GetSingleton()->UpdateClient();
#endif


  if (m_uiExecutingTest == WInvalidIndex)
  {
    StartTests();
    m_uiExecutingTest = 0;
    W_ASSERT_DEV(m_uiExecutingSubTest == WInvalidIndex, "Invalid test framework state");
    W_ASSERT_DEV(!m_bSubTestInitialized, "Invalid test framework state");
  }

  ExecuteNextTest();

  if (m_uiExecutingTest >= (WUInt32)m_TestEntries.size())
  {
    EndTests();

#ifdef W_TESTFRAMEWORK_USE_FILESERVE
    if (WFileserveClient* pClient = WFileserveClient::GetSingleton())
    {
      // shutdown the fileserve client
      W_DEFAULT_DELETE(pClient);
    }
#endif

    return WTestAppRun::Quit;
  }

  return WTestAppRun::Continue;
}

void WTestFramework::StartTests()
{
  ResetTests();
  m_bTestsRunning = true;
  WTestFramework::Output(WTestOutput::StartOutput, "");

  // Start timeout thread.
  std::scoped_lock lock(m_TimeoutLock);
  m_bUseTimeout = true;
  m_bArm = false;
  m_TimeoutThread = std::thread(&WTestFramework::TimeoutThread, this);
}

// Redirects engine warnings / errors to test-framework output
static void LogWriter(const WLoggingEventData& e)
{
  const WStringBuilder sText = e.m_sText;

  switch (e.m_EventType)
  {
    case WLogMsgType::ErrorMsg:
      WTestFramework::Output(WTestOutput::Error, "WLog Error: %s", sText.GetData());
      break;
    case WLogMsgType::SeriousWarningMsg:
      WTestFramework::Output(WTestOutput::Error, "WLog Serious Warning: %s", sText.GetData());
      break;
    case WLogMsgType::WarningMsg:
      WTestFramework::Output(WTestOutput::Warning, "WLog Warning: %s", sText.GetData());
      break;
    case WLogMsgType::InfoMsg:
    case WLogMsgType::DevMsg:
    case WLogMsgType::DebugMsg:
    {
      if (e.m_sTag.IsEqual_NoCase("test"))
        WTestFramework::Output(WTestOutput::Details, sText.GetData());
    }
    break;

    default:
      return;
  }
}

void WTestFramework::ExecuteNextTest()
{
  W_ASSERT_DEV(m_uiExecutingTest >= 0, "Invalid current test.");

  if (m_uiExecutingTest == (WUInt32)GetTestCount())
    return;

  if (!m_TestEntries[m_uiExecutingTest].m_bEnableTest)
  {
    // next time run the next test and start with the first subtest
    m_uiExecutingTest++;
    m_uiExecutingSubTest = WInvalidIndex;
    return;
  }

  WTestEntry& TestEntry = m_TestEntries[m_uiExecutingTest];
  WTestBaseClass* pTestClass = m_TestEntries[m_uiExecutingTest].m_pTest;

  // Execute test
  {
    if (m_uiExecutingSubTest == WInvalidIndex) // no subtest has run yet, so initialize the test first
    {
      if (m_bAbortTests)
      {
        m_uiExecutingTest = (WUInt32)m_TestEntries.size(); // skip to the end of all tests
        m_uiExecutingSubTest = WInvalidIndex;
        return;
      }

      m_uiExecutingSubTest = 0;
      m_fTotalTestDuration = 0.0;

      // Reset assert counter. This variable is used to reduce the overhead of counting millions of asserts.
      s_iAssertCounter = 0;
      m_uiCurrentTestIndex = m_uiExecutingTest;
      // Log writer translates engine warnings / errors into test framework error messages.
      WGlobalLog::AddLogWriter(LogWriter);
      WGlobalLog::AddLogWriter(WLogWriter::Tracing::LogMessageHandler);

      m_iErrorCountBeforeTest = GetTotalErrorCount();

      WTestFramework::Output(WTestOutput::BeginBlock, "Executing Test: '%s'", TestEntry.m_szTestName);

      // *** Test Initialization ***
      if (TestEntry.m_sNotAvailableReason.empty())
      {
        UpdateTestTimeout();
        if (pTestClass->DoTestInitialization().Failed())
        {
          m_uiExecutingSubTest = (WUInt32)TestEntry.m_SubTests.size(); // make sure all sub-tests are skipped
        }
      }
      else
      {
        WTestFramework::Output(WTestOutput::ImportantInfo, "Test not available: %s", TestEntry.m_sNotAvailableReason.c_str());
        m_uiExecutingSubTest = (WUInt32)TestEntry.m_SubTests.size(); // make sure all sub-tests are skipped
      }
    }

    if (m_uiExecutingSubTest < (WUInt32)TestEntry.m_SubTests.size())
    {
      WSubTestEntry& subTest = TestEntry.m_SubTests[m_uiExecutingSubTest];
      WInt32 iSubTestIdentifier = subTest.m_iSubTestIdentifier;

      if (!subTest.m_bEnableTest)
      {
        ++m_uiExecutingSubTest;
        return;
      }

      if (!m_bSubTestInitialized)
      {
        if (m_bAbortTests)
        {
          // tests shall be aborted, so do not start a new one

          m_uiExecutingTest = (WInt32)m_TestEntries.size(); // skip to the end of all tests
          m_uiExecutingSubTest = WInvalidIndex;
          return;
        }

        m_fTotalSubTestDuration = 0.0;
        m_uiSubTestInvocationCount = 0;

        // First flush of assert counter, these are all asserts during test init.
        FlushAsserts();
        m_uiCurrentSubTestIndex = m_uiExecutingSubTest;
        WTestFramework::Output(WTestOutput::BeginBlock, "Executing Sub-Test: '%s'", subTest.m_szSubTestName);

        // *** Sub-Test Initialization ***
        UpdateTestTimeout();
        m_bSubTestInitialized = pTestClass->DoSubTestInitialization(iSubTestIdentifier).Succeeded();
      }

      WTestAppRun subTestResult = WTestAppRun::Quit;

      if (m_bSubTestInitialized)
      {
        // *** Run Sub-Test ***
        double fDuration = 0.0;

        // start with 1
        ++m_uiSubTestInvocationCount;

        UpdateTestTimeout();
        subTestResult = pTestClass->DoSubTestRun(iSubTestIdentifier, fDuration, m_uiSubTestInvocationCount);
        s_szTestBlockName = "";

        if (m_bImageComparisonScheduled)
        {
          W_TEST_IMAGE(m_uiComparisonImageNumber, m_uiMaxImageComparisonError);
          m_bImageComparisonScheduled = false;
        }


        if (m_bDepthImageComparisonScheduled)
        {
          W_TEST_DEPTH_IMAGE(m_uiComparisonDepthImageNumber, m_uiMaxDepthImageComparisonError);
          m_bDepthImageComparisonScheduled = false;
        }

        // I guess we can require that tests are written in a way that they can be interrupted
        if (m_bAbortTests)
          subTestResult = WTestAppRun::Quit;

        m_fTotalSubTestDuration += fDuration;
      }

      // this is executed when sub-test initialization failed or the sub-test reached its end
      if (subTestResult == WTestAppRun::Quit)
      {
        // *** Sub-Test De-Initialization ***
        UpdateTestTimeout();
        pTestClass->DoSubTestDeInitialization(iSubTestIdentifier);

        bool bSubTestSuccess = m_bSubTestInitialized && (m_Result.GetErrorMessageCount(m_uiExecutingTest, m_uiExecutingSubTest) == 0);
        WTestFramework::TestResult(m_uiExecutingSubTest, bSubTestSuccess, m_fTotalSubTestDuration);

        m_fTotalTestDuration += m_fTotalSubTestDuration;

        // advance to the next (sub) test
        m_bSubTestInitialized = false;
        ++m_uiExecutingSubTest;

        // Second flush of assert counter, these are all asserts for the current subtest.
        FlushAsserts();
        WTestFramework::Output(WTestOutput::EndBlock, "");
        m_uiCurrentSubTestIndex = WInvalidIndex;
      }
    }

    if (m_bAbortTests || m_uiExecutingSubTest >= (WInt32)TestEntry.m_SubTests.size())
    {
      // *** Test De-Initialization ***
      if (TestEntry.m_sNotAvailableReason.empty())
      {
        // We only call DoTestInitialization under this condition so DoTestDeInitialization must be guarded by the same.
        UpdateTestTimeout();
        pTestClass->DoTestDeInitialization();
      }
      // Third and last flush of assert counter, these are all asserts for the test de-init.
      FlushAsserts();

      WGlobalLog::RemoveLogWriter(WLogWriter::Tracing::LogMessageHandler);
      WGlobalLog::RemoveLogWriter(LogWriter);

      bool bTestSuccess = m_iErrorCountBeforeTest == GetTotalErrorCount();
      WTestFramework::TestResult(-1, bTestSuccess, m_fTotalTestDuration);
      WTestFramework::Output(WTestOutput::EndBlock, "");
      m_uiCurrentTestIndex = WInvalidIndex;

      // advance to the next test
      m_uiExecutingTest++;
      m_uiExecutingSubTest = WInvalidIndex;
    }
  }
}

void WTestFramework::EndTests()
{
  m_bTestsRunning = false;

  if (GetTestsPassedCount() + GetTestsFailedCount() == 0)
  {
    // Counted as a failure on purpose: a run that executed nothing must not report success, or a
    // typo in -filter looks like a green build.
    if (m_Settings.m_sTestFilter.empty())
    {
      WTestFramework::Output(WTestOutput::Error, "No tests were run, because no tests are enabled.");
    }
    else
    {
      WTestFramework::Output(WTestOutput::Error, "No tests were run: the -filter '%s' did not match any test or sub-test name. Use -list to see the available names.",
        m_Settings.m_sTestFilter.c_str());
    }

    m_iTestsFailed++;
  }

  if (GetTestsFailedCount() == 0)
    WTestFramework::Output(WTestOutput::FinalResult, "All tests passed.");
  else
    WTestFramework::Output(WTestOutput::FinalResult, "Tests failed: %i. Tests passed: %i", GetTestsFailedCount(), GetTestsPassedCount());

  if (!m_Settings.m_sJsonOutput.empty())
    m_Result.WriteJsonToFile(m_Settings.m_sJsonOutput.c_str());

  m_uiExecutingTest = WInvalidIndex;
  m_uiExecutingSubTest = WInvalidIndex;
  m_bAbortTests = false;

  // Stop timeout thread.
  {
    std::scoped_lock lock(m_TimeoutLock);
    m_bUseTimeout = false;
    m_TimeoutCV.notify_one();
  }
  m_TimeoutThread.join();
}

void WTestFramework::AbortTests()
{
  m_bAbortTests = true;
}

WUInt32 WTestFramework::GetTestCount() const
{
  return (WUInt32)m_TestEntries.size();
}

WUInt32 WTestFramework::GetTestEnabledCount() const
{
  WUInt32 uiEnabledCount = 0;
  const WUInt32 uiTests = GetTestCount();
  for (WUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    uiEnabledCount += m_TestEntries[uiTest].m_bEnableTest ? 1 : 0;
  }
  return uiEnabledCount;
}

WUInt32 WTestFramework::GetSubTestEnabledCount(WUInt32 uiTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return 0;

  WUInt32 uiEnabledCount = 0;
  const WUInt32 uiSubTests = (WUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  for (WUInt32 uiSubTest = 0; uiSubTest < uiSubTests; ++uiSubTest)
  {
    uiEnabledCount += m_TestEntries[uiTestIndex].m_SubTests[uiSubTest].m_bEnableTest ? 1 : 0;
  }
  return uiEnabledCount;
}

const std::string& WTestFramework::IsTestAvailable(WUInt32 uiTestIndex) const
{
  W_ASSERT_DEV(uiTestIndex < GetTestCount(), "Test index {0} is larger than number of tests {1}.", uiTestIndex, GetTestCount());
  return m_TestEntries[uiTestIndex].m_sNotAvailableReason;
}

bool WTestFramework::IsTestEnabled(WUInt32 uiTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return false;

  return m_TestEntries[uiTestIndex].m_bEnableTest;
}

bool WTestFramework::IsSubTestEnabled(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return false;

  const WUInt32 uiSubTests = (WUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  if (uiSubTestIndex >= uiSubTests)
    return false;

  return m_TestEntries[uiTestIndex].m_SubTests[uiSubTestIndex].m_bEnableTest;
}

void WTestFramework::SetTestEnabled(WUInt32 uiTestIndex, bool bEnabled)
{
  if (uiTestIndex >= GetTestCount())
    return;

  m_TestEntries[uiTestIndex].m_bEnableTest = bEnabled;
}

void WTestFramework::SetSubTestEnabled(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, bool bEnabled)
{
  if (uiTestIndex >= GetTestCount())
    return;

  const WUInt32 uiSubTests = (WUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  if (uiSubTestIndex >= uiSubTests)
    return;

  m_TestEntries[uiTestIndex].m_SubTests[uiSubTestIndex].m_bEnableTest = bEnabled;
}

WInt32 WTestFramework::GetCurrentSubTestIdentifier() const
{
  return GetCurrentSubTest()->m_iSubTestIdentifier;
}

WUInt32 WTestFramework::FindSubTestIndexForSubTestIdentifier(WInt32 iSubTestIdentifier) const
{
  const WTestEntry* pTest = GetCurrentTest();

  const WUInt32 uiSubTests = (WUInt32)pTest->m_SubTests.size();
  for (WUInt32 i = 0; i < uiSubTests; ++i)
  {
    if (pTest->m_SubTests[i].m_iSubTestIdentifier == iSubTestIdentifier)
      return i;
  }

  return WInvalidIndex;
}

WTestEntry* WTestFramework::GetTest(WUInt32 uiTestIndex)
{
  if (uiTestIndex >= GetTestCount())
    return nullptr;

  return &m_TestEntries[uiTestIndex];
}

const WTestEntry* WTestFramework::GetTest(WUInt32 uiTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return nullptr;

  return &m_TestEntries[uiTestIndex];
}

const WTestEntry* WTestFramework::GetCurrentTest() const
{
  return GetTest(GetCurrentTestIndex());
}

const WSubTestEntry* WTestFramework::GetCurrentSubTest() const
{
  if (auto pTest = GetCurrentTest())
  {
    if (m_uiCurrentSubTestIndex >= (WInt32)pTest->m_SubTests.size())
      return nullptr;

    return &pTest->m_SubTests[m_uiCurrentSubTestIndex];
  }

  return nullptr;
}

TestSettings WTestFramework::GetSettings() const
{
  return m_Settings;
}

void WTestFramework::SetSettings(const TestSettings& settings)
{
  m_Settings = settings;
}

WTestFrameworkResult& WTestFramework::GetTestResult()
{
  return m_Result;
}

WInt32 WTestFramework::GetTotalErrorCount() const
{
  return m_iErrorCount;
}

WInt32 WTestFramework::GetTestsPassedCount() const
{
  return m_iTestsPassed;
}

WInt32 WTestFramework::GetTestsFailedCount() const
{
  return m_iTestsFailed;
}

double WTestFramework::GetTotalTestDuration() const
{
  return m_Result.GetTotalTestDuration();
}

////////////////////////////////////////////////////////////////////////
// WTestFramework protected functions
////////////////////////////////////////////////////////////////////////

static bool g_bBlockOutput = false;

void WTestFramework::OutputImpl(WTestOutput::Enum Type, const char* szMsg)
{
  std::scoped_lock _(m_OutputMutex);

  if (Type == WTestOutput::Error)
  {
    m_iErrorCount++;
  }
  // pass the output to all the registered output handlers, which will then write it to the console, file, etc.
  for (WUInt32 i = 0; i < m_OutputHandlers.size(); ++i)
  {
    m_OutputHandlers[i](Type, szMsg);
  }

  if (g_bBlockOutput)
    return;

  m_Result.TestOutput(m_uiCurrentTestIndex, m_uiCurrentSubTestIndex, Type, szMsg);
}

void WTestFramework::ErrorImpl(const char* szError, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg)
{
  std::scoped_lock _(m_OutputMutex);

  m_Result.TestError(m_uiCurrentTestIndex, m_uiCurrentSubTestIndex, szError, WTestFramework::s_szTestBlockName, szFile, iLine, szFunction, szMsg);

  g_bBlockOutput = true;
  WTestFramework::Output(WTestOutput::Error, "%s", szError); // This will also increase the global error count.
  WTestFramework::Output(WTestOutput::BeginBlock, "");
  {
    if ((WTestFramework::s_szTestBlockName != nullptr) && (WTestFramework::s_szTestBlockName[0] != '\0'))
      WTestFramework::Output(WTestOutput::Message, "Block: '%s'", WTestFramework::s_szTestBlockName);

    WTestFramework::Output(WTestOutput::ImportantInfo, "File: %s", szFile);
    WTestFramework::Output(WTestOutput::ImportantInfo, "Line: %i", iLine);
    WTestFramework::Output(WTestOutput::ImportantInfo, "Function: %s", szFunction);

    if ((szMsg != nullptr) && (szMsg[0] != '\0'))
      WTestFramework::Output(WTestOutput::Message, "Error: %s", szMsg);
  }
  WTestFramework::Output(WTestOutput::EndBlock, "");
  g_bBlockOutput = false;
}

void WTestFramework::TestResultImpl(WUInt32 uiSubTestIndex, bool bSuccess, double fDuration)
{
  std::scoped_lock _(m_OutputMutex);

  m_Result.TestResult(m_uiCurrentTestIndex, uiSubTestIndex, bSuccess, fDuration);

  const WUInt32 uiMin = (WUInt32)(fDuration / 1000.0 / 60.0);
  const WUInt32 uiSec = (WUInt32)(fDuration / 1000.0 - uiMin * 60.0);
  const WUInt32 uiMS = (WUInt32)(fDuration - uiSec * 1000.0);

  WTestFramework::Output(WTestOutput::Duration, "%i:%02i:%03i", uiMin, uiSec, uiMS);

  if (uiSubTestIndex == WInvalidIndex)
  {
    const char* szTestName = m_TestEntries[m_uiCurrentTestIndex].m_szTestName;
    if (bSuccess)
    {
      m_iTestsPassed++;
      WTestFramework::Output(WTestOutput::Success, "Test '%s' succeeded (%.2f sec).", szTestName, m_fTotalTestDuration / 1000.0f);

      if (GetSettings().m_bAutoDisableSuccessfulTests)
      {
        m_TestEntries[m_uiCurrentTestIndex].m_bEnableTest = false;
        WTestFramework::AutoSaveTestOrder();
      }
    }
    else
    {
      m_iTestsFailed++;
      WTestFramework::Output(WTestOutput::Error, "Test '%s' failed: %i Errors (%.2f sec).", szTestName, (WUInt32)m_Result.GetErrorMessageCount(m_uiCurrentTestIndex, uiSubTestIndex), m_fTotalTestDuration / 1000.0f);
    }
  }
  else
  {
    const char* szSubTestName = m_TestEntries[m_uiCurrentTestIndex].m_SubTests[uiSubTestIndex].m_szSubTestName;
    if (bSuccess)
    {
      WTestFramework::Output(WTestOutput::Success, "Sub-Test '%s' succeeded (%.2f sec).", szSubTestName, m_fTotalSubTestDuration / 1000.0f);

      if (GetSettings().m_bAutoDisableSuccessfulTests)
      {
        m_TestEntries[m_uiCurrentTestIndex].m_SubTests[uiSubTestIndex].m_bEnableTest = false;
        WTestFramework::AutoSaveTestOrder();
      }
    }
    else
    {
      WTestFramework::Output(WTestOutput::Error, "Sub-Test '%s' failed: %i Errors (%.2f sec).", szSubTestName, (WUInt32)m_Result.GetErrorMessageCount(m_uiCurrentTestIndex, uiSubTestIndex), m_fTotalSubTestDuration / 1000.0f);
    }
  }
}

void WTestFramework::SetSubTestStatusImpl(WUInt32 uiSubTestIndex, const char* szStatus)
{
  std::scoped_lock _(m_OutputMutex);

  if (m_uiCurrentTestIndex != WInvalidIndex && uiSubTestIndex != WInvalidIndex)
  {
    const WSubTestEntry& subtest = m_TestEntries[m_uiCurrentTestIndex].m_SubTests[uiSubTestIndex];

    m_Result.SetCustomStatus(m_uiCurrentTestIndex, uiSubTestIndex, szStatus);

    if (!WStringUtils::IsNullOrEmpty(szStatus))
    {
      WTestFramework::Output(WTestOutput::Details, "Status of sub-test '%s': %s.", subtest.m_szSubTestName, szStatus);
    }
  }
}

void WTestFramework::FlushAsserts()
{
  std::scoped_lock _(m_OutputMutex);
  m_Result.AddAsserts(m_uiCurrentTestIndex, m_uiCurrentSubTestIndex, s_iAssertCounter);
  s_iAssertCounter = 0;
}

void WTestFramework::ScheduleImageComparison(WUInt32 uiImageNumber, WUInt32 uiMaxError)
{
  m_bImageComparisonScheduled = true;
  m_uiMaxImageComparisonError = uiMaxError;
  m_uiComparisonImageNumber = uiImageNumber;
}

void WTestFramework::ScheduleDepthImageComparison(WUInt32 uiImageNumber, WUInt32 uiMaxError)
{
  m_bDepthImageComparisonScheduled = true;
  m_uiMaxDepthImageComparisonError = uiMaxError;
  m_uiComparisonDepthImageNumber = uiImageNumber;
}

void WTestFramework::GenerateComparisonImageName(WUInt32 uiImageNumber, WStringBuilder& ref_sImgName)
{
  WTestEntry* pMainTest = GetTest(GetCurrentTestIndex());

  const char* szTestName = pMainTest->m_szTestName;
  const WSubTestEntry& subTest = pMainTest->m_SubTests[GetCurrentSubTestIndex()];
  pMainTest->m_pTest->MapImageNumberToString(szTestName, subTest, uiImageNumber, ref_sImgName);
}

void WTestFramework::GetCurrentComparisonImageName(WStringBuilder& ref_sImgName)
{
  GenerateComparisonImageName(m_uiComparisonImageNumber, ref_sImgName);
}

void WTestFramework::SetImageReferenceFolderName(const char* szFolderName)
{
  m_sImageReferenceFolderName = szFolderName;
}

void WTestFramework::AddImageReferenceTag(const char* szTag)
{
  if (WStringUtils::IsNullOrEmpty(szTag))
    return;

  m_ImageReferenceTags.PushBack(szTag);
  Output(WTestOutput::Details, "Added ImageReference tag '%s'", szTag);
}

void WTestFramework::ClearImageReferenceTags()
{
  m_ImageReferenceTags.Clear();
}

void WTestFramework::SetImageReferenceTagsFromEnvironment(WStringView sPlatform, WStringView sRenderer, WStringView sAdapterName)
{
  ClearImageReferenceTags();

  // Platform tag
  {
    WStringBuilder sPlatformTag = sPlatform;
    sPlatformTag.ToLower();
    if (!sPlatformTag.IsEmpty() && sPlatformTag != "windows") // windows is the default, no tag needed
    {
      AddImageReferenceTag(sPlatformTag);
    }
  }

  if (sRenderer.IsEmpty())
    return;

  const bool bIsDX11 = sRenderer.IsEqual_NoCase("DX11");
  const bool bIsVulkan = sRenderer.IsEqual_NoCase("Vulkan");

  const bool bIsRefDriver = (sAdapterName == "Microsoft Basic Render Driver" || sAdapterName.StartsWith_NoCase("Intel(R) UHD Graphics"));
  const bool bIsAMD = (sAdapterName.FindSubString_NoCase("AMD") != nullptr || sAdapterName.FindSubString_NoCase("Radeon") != nullptr);
  const bool bIsNvidia = (sAdapterName.FindSubString_NoCase("Nvidia") != nullptr || sAdapterName.FindSubString_NoCase("GeForce") != nullptr);
  const bool bIsIntel = (sAdapterName.FindSubString_NoCase("Intel") != nullptr);
  const bool bIsLLVMPipe = (sAdapterName.FindSubString_NoCase("llvmpipe") != nullptr);
  const bool bIsSwiftShader = (sAdapterName.FindSubString_NoCase("SwiftShader") != nullptr);

  if (bIsDX11)
  {
    if (bIsRefDriver)
    {
      AddImageReferenceTag("d3dref");
    }
    else if (bIsAMD)
    {
      AddImageReferenceTag("amd");
    }
    else if (bIsNvidia)
    {
      AddImageReferenceTag("nvidia");
    }
    else if (bIsIntel)
    {
      AddImageReferenceTag("intel");
    }
  }
  else if (bIsVulkan)
  {
    AddImageReferenceTag("vulkan");

    if (bIsLLVMPipe)
    {
      AddImageReferenceTag("llvmpipe");
    }
    else if (bIsSwiftShader)
    {
      AddImageReferenceTag("swiftshader");
    }
    else if (bIsAMD)
    {
      AddImageReferenceTag("amd");
    }
    else if (bIsNvidia)
    {
      AddImageReferenceTag("nvidia");
    }
    else if (bIsIntel)
    {
      AddImageReferenceTag("intel");
    }
  }
}

void WTestFramework::WriteImageDiffHtml(const char* szFileName, const WImage& referenceImgRgb, const WImage& referenceImgAlpha, const WImage& capturedImgRgb, const WImage& capturedImgAlpha, const WImage& diffImgRgb, const WImage& diffImgAlpha, WUInt32 uiError, WUInt32 uiThreshold, WUInt8 uiMinDiffRgb, WUInt8 uiMaxDiffRgb,
  WUInt8 uiMinDiffAlpha, WUInt8 uiMaxDiffAlpha)
{
  WFileWriter outputFile;
  if (outputFile.Open(szFileName).Failed())
  {
    WTestFramework::Output(WTestOutput::Warning, "Could not open HTML diff file \"%s\" for writing.", szFileName);
    return;
  }

  const char* szTestName = GetTest(GetCurrentTestIndex())->m_szTestName;
  const char* szSubTestName = GetTest(GetCurrentTestIndex())->m_SubTests[GetCurrentSubTestIndex()].m_szSubTestName;

  WStringBuilder tmp(szTestName, " - ", szSubTestName);

  WStringBuilder output;
  WImageUtils::CreateImageDiffHtml(output, tmp, referenceImgRgb, referenceImgAlpha, capturedImgRgb, capturedImgAlpha, diffImgRgb, diffImgAlpha, uiError, uiThreshold, uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha);

  if (m_ImageDiffExtraInfoCallback)
  {
    tmp.Clear();

    WDynamicArray<std::pair<WString, WString>> extraInfo = m_ImageDiffExtraInfoCallback();

    for (const auto& labelValuePair : extraInfo)
    {
      tmp.AppendFormat("<tr>\n"
                       "<td>{}:</td>\n"
                       "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                       "</tr>\n",
        labelValuePair.first, labelValuePair.second);
    }

    output.ReplaceFirst("<!-- STATS-TABLE-START -->", tmp);
  }

  outputFile.WriteBytes(output.GetData(), output.GetElementCount()).AssertSuccess();
  outputFile.Close();
}

bool WTestFramework::PerformImageComparison(WStringBuilder sImgName, const WImage& img, WUInt32 uiMaxError, bool bIsLineImage, char* szErrorMsg)
{
  WImage imgRgba;
  if (WImageConversion::Convert(img, imgRgba, WImageFormat::R8G8B8A8_UNORM).Failed())
  {
    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Captured Image '%s' could not be converted to RGBA8", sImgName.GetData());
    return false;
  }

  WStringBuilder sImgPathResult;
  sImgPathResult = ":imgout/Images_Result";
  sImgPathResult.AppendPath(sImgName);
  sImgPathResult.ChangeFileExtension(".png");

  auto SaveResultImage = [&]()
  {
    imgRgba.SaveTo(sImgPathResult).IgnoreResult();
  };

  // if a previous output image exists, get rid of it
  WFileSystem::DeleteFile(sImgPathResult);

  // Helper: performs the actual pixel comparison against a reference image at the given path.
  // Returns true if the comparison passes. On failure, populates the out parameters.
  auto TryCompareWithImage = [&](const WStringBuilder& sRefPath, WUInt32& out_uiError, WImage& out_imgExpRgba, WImage& out_imgDiffRgba) -> bool
  {
    WImage imgExp;
    if (imgExp.LoadFrom(sRefPath).Failed())
      return false;

    if (WImageConversion::Convert(imgExp, out_imgExpRgba, WImageFormat::R8G8B8A8_UNORM).Failed())
    {
      safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' could not be converted to RGBA8", sRefPath.GetData());
      return false;
    }

    if (imgRgba.GetWidth() != out_imgExpRgba.GetWidth() || imgRgba.GetHeight() != out_imgExpRgba.GetHeight())
    {
      safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' size (%ix%i) does not match captured image size (%ix%i)", sRefPath.GetData(), out_imgExpRgba.GetWidth(), out_imgExpRgba.GetHeight(), imgRgba.GetWidth(), imgRgba.GetHeight());
      out_uiError = 0xFFFFFFFFu;
      return false;
    }

    if (bIsLineImage)
      WImageUtils::ComputeImageDifferenceABSRelaxed(out_imgExpRgba, imgRgba, out_imgDiffRgba);
    else
      WImageUtils::ComputeImageDifferenceABS(out_imgExpRgba, imgRgba, out_imgDiffRgba);

    out_uiError = WImageUtils::ComputeMeanSquareError(out_imgDiffRgba, 32);
    return (out_uiError <= uiMaxError);
  };

  // Build the list of candidate reference paths to try:
  // 1. Base image (no suffix)
  // 2. Single-tag suffixes in registration order
  // 3. Multi-tag combinations (pairs, triples, ...) in registration order
  WTempHybridArray<WStringBuilder, 8> candidatePaths;

  {
    // Base path
    WStringBuilder sBase = m_sImageReferenceFolderName.c_str();
    sBase.AppendPath(sImgName);
    sBase.ChangeFileExtension(".png");
    candidatePaths.PushBack(sBase);
  }

  const WUInt32 uiTagCount = m_ImageReferenceTags.GetCount();
  if (uiTagCount > 0)
  {
    // Generate all non-empty subsets of tags, ordered by subset size (1-tag first, then 2-tag, etc.)
    // For each subset size k, iterate over all combinations of k tags in registration order.
    for (WUInt32 uiSubsetSize = 1; uiSubsetSize <= uiTagCount; ++uiSubsetSize)
    {
      // Use a bitmask to enumerate all subsets of the given size
      // For up to ~20 tags this is efficient enough
      const WUInt32 uiFullMask = (1u << uiTagCount) - 1u;
      for (WUInt32 uiMask = 0; uiMask <= uiFullMask; ++uiMask)
      {
        if (WMath::CountBits(uiMask) != uiSubsetSize)
          continue;

        WStringBuilder sSuffixedName = sImgName;
        for (WUInt32 t = 0; t < uiTagCount; ++t)
        {
          if ((uiMask & (1u << t)) != 0)
          {
            sSuffixedName.Append("-");
            sSuffixedName.Append(m_ImageReferenceTags[t].GetData());
          }
        }

        WStringBuilder sPath = m_sImageReferenceFolderName.c_str();
        sPath.AppendPath(sSuffixedName);
        sPath.ChangeFileExtension(".png");
        candidatePaths.PushBack(sPath);
      }
    }
  }

  // Track the best (lowest error) failed comparison attempt
  struct FailureInfo
  {
    WStringBuilder sPath;
    WUInt32 uiError = 0xFFFFFFFFu;
    WImage imgExpRgba;
    WImage imgDiffRgba;
    bool bWasTried = false; // true if the file existed and comparison ran
  };
  FailureInfo bestFailure;

  for (const auto& sRefPath : candidatePaths)
  {
    if (!WFileSystem::ExistsFile(sRefPath))
      continue;

    WUInt32 uiError = 0xFFFFFFFFu;
    WImage imgExpRgba, imgDiffRgba;
    const bool bPassed = TryCompareWithImage(sRefPath, uiError, imgExpRgba, imgDiffRgba);

    if (bPassed)
      return true;

    // Track the best (lowest error) failure for reporting
    if (!bestFailure.bWasTried || uiError < bestFailure.uiError)
    {
      bestFailure.sPath = sRefPath;
      bestFailure.uiError = uiError;
      bestFailure.imgExpRgba = std::move(imgExpRgba);
      bestFailure.imgDiffRgba = std::move(imgDiffRgba);
      bestFailure.bWasTried = true;
    }
  }

  // All candidates either didn't exist or failed comparison
  SaveResultImage();

  if (!bestFailure.bWasTried)
  {
    // No reference image found at all — report the base path
    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' could not be read", candidatePaths[0].GetData());
    return false;
  }

  // Report the best (lowest-error) failure, and write diff outputs for it
  {
    WImage& imgExpRgba = bestFailure.imgExpRgba;
    WImage& imgDiffRgba = bestFailure.imgDiffRgba;
    const WStringBuilder& sBestPath = bestFailure.sPath;
    const WUInt32 uiBestError = bestFailure.uiError;

    if (uiBestError != 0xFFFFFFFFu) // size mismatch sets error to max; skip diff for those
    {
      WUInt8 uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha;
      WImageUtils::Normalize(imgDiffRgba, uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha);

      WImage imgDiffRgb;
      WImageConversion::Convert(imgDiffRgba, imgDiffRgb, WImageFormat::R8G8B8_UNORM).IgnoreResult();

      WStringBuilder sImgDiffName;
      sImgDiffName.SetFormat(":imgout/Images_Diff/{0}.png", sImgName);
      imgDiffRgb.SaveTo(sImgDiffName).IgnoreResult();

      WImage imgDiffAlpha;
      WImageUtils::ExtractAlphaChannel(imgDiffRgba, imgDiffAlpha);

      WStringBuilder sImgDiffAlphaName;
      sImgDiffAlphaName.SetFormat(":imgout/Images_Diff/{0}_alpha.png", sImgName);
      imgDiffAlpha.SaveTo(sImgDiffAlphaName).IgnoreResult();

      WImage imgExpRgb;
      WImageConversion::Convert(imgExpRgba, imgExpRgb, WImageFormat::R8G8B8_UNORM).IgnoreResult();
      WImage imgExpAlpha;
      WImageUtils::ExtractAlphaChannel(imgExpRgba, imgExpAlpha);

      WImage imgRgb;
      WImageConversion::Convert(imgRgba, imgRgb, WImageFormat::R8G8B8_UNORM).IgnoreResult();
      WImage imgAlpha;
      WImageUtils::ExtractAlphaChannel(imgRgba, imgAlpha);

      WStringBuilder sDiffHtmlPath;
      sDiffHtmlPath.SetFormat(":imgout/Html_Diff/{0}.html", sImgName);
      WriteImageDiffHtml(sDiffHtmlPath, imgExpRgb, imgExpAlpha, imgRgb, imgAlpha, imgDiffRgb, imgDiffAlpha, uiBestError, uiMaxError, uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha);

      safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Error: Image Comparison Failed: MSE of %u exceeds threshold of %u for image '%s'.", uiBestError, uiMaxError, sBestPath.GetData());

      WStringBuilder sDataDirRelativePath;
      WFileSystem::ResolvePath(sDiffHtmlPath, nullptr, &sDataDirRelativePath).IgnoreResult();
      WTestFramework::Output(WTestOutput::ImageDiffFile, sDataDirRelativePath);
    }
    else
    {
      safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "%s", szErrorMsg); // size mismatch message already set by TryCompareWithImage
    }
  }

  return false;
}

bool WTestFramework::CompareImages(WUInt32 uiImageNumber, WUInt32 uiMaxError, char* szErrorMsg, bool bIsDepthImage, bool bIsLineImage)
{
  WStringBuilder sImgName;
  GenerateComparisonImageName(uiImageNumber, sImgName);

  WImage img;
  if (bIsDepthImage)
  {
    sImgName.Append("-depth");
    if (GetTest(GetCurrentTestIndex())->m_pTest->GetDepthImage(img, *GetCurrentSubTest(), uiImageNumber).Failed())
    {
      safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Depth image '%s' could not be captured", sImgName.GetData());
      return false;
    }
  }
  else
  {
    if (GetTest(GetCurrentTestIndex())->m_pTest->GetImage(img, *GetCurrentSubTest(), uiImageNumber).Failed())
    {
      safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Image '%s' could not be captured", sImgName.GetData());
      return false;
    }
  }

  bool bImagesMatch = true;
  if (img.GetNumArrayIndices() <= 1)
  {
    bImagesMatch = PerformImageComparison(sImgName, img, uiMaxError, bIsLineImage, szErrorMsg);
  }
  else
  {
    WStringBuilder lastError;
    for (WUInt32 i = 0; i < img.GetNumArrayIndices(); ++i)
    {
      WStringBuilder subImageName;
      subImageName.AppendFormat("{0}_{1}", sImgName, i);
      if (!PerformImageComparison(subImageName, img.GetSubImageView(0, 0, i), uiMaxError, bIsLineImage, szErrorMsg))
      {
        bImagesMatch = false;
        if (!lastError.IsEmpty())
        {
          WTestFramework::Output(WTestOutput::Error, "%s", lastError.GetData());
        }
        lastError = szErrorMsg;
      }
    }
  }

  if (m_ImageComparisonCallback)
  {
    m_ImageComparisonCallback(bImagesMatch);
  }

  return bImagesMatch;
}

void WTestFramework::SetImageComparisonCallback(const ImageComparisonCallback& callback)
{
  m_ImageComparisonCallback = callback;
}

WResult WTestFramework::CaptureRegressionStat(WStringView sTestName, WStringView sName, WStringView sUnit, float value, WInt32 iTestId)
{
  WStringBuilder strippedTestName = sTestName;
  strippedTestName.ReplaceAll(" ", "");

  WStringBuilder perTestName;
  if (iTestId < 0)
  {
    perTestName.SetFormat("{}_{}", strippedTestName, sName);
  }
  else
  {
    perTestName.SetFormat("{}_{}_{}", strippedTestName, sName, iTestId);
  }

  {
    WStringBuilder regression;
    // The 6 floating point digits are forced as per a requirement of the CI
    // feature that parses these values.
    regression.SetFormat("[test][REGRESSION:{}:{}:{}]", perTestName, sUnit, WArgF(value, 6));
    WLog::Info(regression);
  }

  return W_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// WTestFramework static functions
////////////////////////////////////////////////////////////////////////

void WTestFramework::Output(WTestOutput::Enum type, const char* szMsg, ...)
{
  va_list args;
  va_start(args, szMsg);

  OutputArgs(type, szMsg, args);

  va_end(args);
}

void WTestFramework::OutputArgs(WTestOutput::Enum type, const char* szMsg, va_list szArgs)
{
  // format the output text
  char szBuffer[1024 * 10];
  WInt32 pos = 0;

  if (WTestFramework::s_LogTimestampMode != WLog::TimestampMode::None)
  {
    if (type == WTestOutput::BeginBlock || type == WTestOutput::EndBlock || type == WTestOutput::ImportantInfo || type == WTestOutput::Details || type == WTestOutput::Success || type == WTestOutput::Message || type == WTestOutput::Warning || type == WTestOutput::Error ||
        type == WTestOutput::FinalResult)
    {
      WStringBuilder timestamp;

      WLog::GenerateFormattedTimestamp(WTestFramework::s_LogTimestampMode, timestamp);
      pos = WStringUtils::snprintf(szBuffer, W_ARRAY_SIZE(szBuffer), "%s", timestamp.GetData());
    }
  }
  WStringUtils::vsnprintf(szBuffer + pos, W_ARRAY_SIZE(szBuffer) - pos, szMsg, szArgs);

  GetInstance()->OutputImpl(type, szBuffer);
}

void WTestFramework::Error(const char* szError, const char* szFile, WInt32 iLine, const char* szFunction, WStringView sMsg, ...)
{
  va_list args;
  va_start(args, sMsg);

  Error(szError, szFile, iLine, szFunction, sMsg, args);

  va_end(args);
}

void WTestFramework::Error(const char* szError, const char* szFile, WInt32 iLine, const char* szFunction, WStringView sMsg, va_list szArgs)
{
  // format the output text
  char szBuffer[1024 * 10];
  WStringUtils::vsnprintf(szBuffer, W_ARRAY_SIZE(szBuffer), WString(sMsg).GetData(), szArgs);

  GetInstance()->ErrorImpl(szError, szFile, iLine, szFunction, szBuffer);
}

void WTestFramework::TestResult(WUInt32 uiSubTestIndex, bool bSuccess, double fDuration)
{
  GetInstance()->TestResultImpl(uiSubTestIndex, bSuccess, fDuration);
}

void WTestFramework::SetSubTestStatus(WUInt32 uiSubTestIndex, const char* szStatus)
{
  GetInstance()->SetSubTestStatusImpl(uiSubTestIndex, szStatus);
}

////////////////////////////////////////////////////////////////////////
// W_TEST_... macro functions
////////////////////////////////////////////////////////////////////////

#define OUTPUT_TEST_ERROR                                                        \
  {                                                                              \
    va_list args;                                                                \
    va_start(args, szMsg);                                                       \
    WTestFramework::Error(szErrorText, szFile, iLine, szFunction, szMsg, args); \
    W_TEST_DEBUG_BREAK                                                          \
    va_end(args);                                                                \
    return false;                                                                \
  }

#define OUTPUT_TEST_ERROR_NO_BREAK                                               \
  {                                                                              \
    va_list args;                                                                \
    va_start(args, szMsg);                                                       \
    WTestFramework::Error(szErrorText, szFile, iLine, szFunction, szMsg, args); \
    va_end(args);                                                                \
    return false;                                                                \
  }

bool WTestBool(bool bCondition, const char* szErrorText, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  WTestFramework::s_iAssertCounter++;

  if (!bCondition)
  {
    // if the test breaks here, go one up in the callstack to see where it exactly failed
    OUTPUT_TEST_ERROR
  }

  return true;
}

bool WTestResult(WResult condition, const char* szErrorText, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  WTestFramework::s_iAssertCounter++;

  if (condition.Failed())
  {
    // if the test breaks here, go one up in the callstack to see where it exactly failed
    OUTPUT_TEST_ERROR
  }

  return true;
}

bool WTestDouble(double f1, double f2, double fEps, const char* szF1, const char* szF2, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  WTestFramework::s_iAssertCounter++;

  if (WMath::IsNaN(f1) || WMath::IsNaN(f2) || WMath::IsNaN(fEps))
  {
    char szErrorText[256];
    safeprintf(szErrorText, 256, "Failure: f1 = '%f', f2 = '%f', epsilon = '%f' (one of them is NaN)", f1, f2, fEps);

    OUTPUT_TEST_ERROR
  }

  const double fD = f1 - f2;

  if (fD < -fEps || fD > +fEps)
  {
    char szErrorText[256];
    safeprintf(szErrorText, 256, "Failure: '%s' (%.8f) does not equal '%s' (%.8f) within an epsilon of %.8f", szF1, f1, szF2, f2, fEps);

    OUTPUT_TEST_ERROR
  }

  return true;
}

bool WTestInt(WInt64 i1, WInt64 i2, const char* szI1, const char* szI2, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  WTestFramework::s_iAssertCounter++;

  if (i1 != i2)
  {
    char szErrorText[256];
    safeprintf(szErrorText, 256, "Failure: '%s' (%lli) does not equal '%s' (%lli)", szI1, i1, szI2, i2);

    OUTPUT_TEST_ERROR
  }

  return true;
}

bool WTestWString(std::wstring s1, std::wstring s2, const char* szWString1, const char* szWString2, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  WTestFramework::s_iAssertCounter++;

  if (s1 != s2)
  {
    char szErrorText[2048];
    safeprintf(szErrorText, 2048, "Failure: '%s' (%s) does not equal '%s' (%s)", szWString1, WStringUtf8(s1.c_str()).GetData(), szWString2, WStringUtf8(s2.c_str()).GetData());

    OUTPUT_TEST_ERROR
  }

  return true;
}

bool WTestString(WStringView s1, WStringView s2, const char* szString1, const char* szString2, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  WTestFramework::s_iAssertCounter++;

  if (s1 != s2)
  {
    WStringBuilder ss1 = s1;
    WStringBuilder ss2 = s2;

    char szErrorText[2048];
    safeprintf(szErrorText, 2048, "Failure: '%s' (%s) does not equal '%s' (%s)", szString1, ss1.GetData(), szString2, ss2.GetData());

    OUTPUT_TEST_ERROR
  }

  return true;
}

bool WTestVector(WVec4d v1, WVec4d v2, double fEps, const char* szCondition, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  WTestFramework::s_iAssertCounter++;

  char szErrorText[256];

  if (!WMath::IsEqual(v1.x, v2.x, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.x (%.8f) does not equal v2.x (%.8f) within an epsilon of %.8f", szCondition, v1.x, v2.x, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!WMath::IsEqual(v1.y, v2.y, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.y (%.8f) does not equal v2.y (%.8f) within an epsilon of %.8f", szCondition, v1.y, v2.y, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!WMath::IsEqual(v1.z, v2.z, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.z (%.8f) does not equal v2.z (%.8f) within an epsilon of %.8f", szCondition, v1.z, v2.z, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!WMath::IsEqual(v1.w, v2.w, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.w (%.8f) does not equal v2.w (%.8f) within an epsilon of %.8f", szCondition, v1.w, v2.w, fEps);

    OUTPUT_TEST_ERROR
  }

  return true;
}

bool WTestFiles(const char* szFile1, const char* szFile2, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  WTestFramework::s_iAssertCounter++;

  char szErrorText[s_iMaxErrorMessageLength];

  WFileReader ReadFile1;
  WFileReader ReadFile2;

  if (ReadFile1.Open(szFile1) == W_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile1);

    OUTPUT_TEST_ERROR
  }
  else if (ReadFile2.Open(szFile2) == W_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile2);

    OUTPUT_TEST_ERROR
  }

  else if (ReadFile1.GetFileSize() != ReadFile2.GetFileSize())
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File sizes do not match: '%s' (%llu Bytes) and '%s' (%llu Bytes)", szFile1, ReadFile1.GetFileSize(), szFile2, ReadFile2.GetFileSize());

    OUTPUT_TEST_ERROR
  }
  else
  {
    while (true)
    {
      WUInt8 uiTemp1[512];
      WUInt8 uiTemp2[512];
      const WUInt64 uiRead1 = ReadFile1.ReadBytes(uiTemp1, 512);
      const WUInt64 uiRead2 = ReadFile2.ReadBytes(uiTemp2, 512);

      if (uiRead1 != uiRead2)
      {
        safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: Files could not read same amount of data: '%s' and '%s'", szFile1, szFile2);

        OUTPUT_TEST_ERROR
      }
      else
      {
        if (uiRead1 == 0)
          break;

        if (memcmp(uiTemp1, uiTemp2, (size_t)uiRead1) != 0)
        {
          safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: Files contents do not match: '%s' and '%s'", szFile1, szFile2);

          OUTPUT_TEST_ERROR
        }
      }
    }
  }

  return true;
}

bool WTestTextFiles(const char* szFile1, const char* szFile2, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  WTestFramework::s_iAssertCounter++;

  char szErrorText[s_iMaxErrorMessageLength];

  WFileReader ReadFile1;
  WFileReader ReadFile2;

  if (ReadFile1.Open(szFile1) == W_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile1);

    OUTPUT_TEST_ERROR
  }
  else if (ReadFile2.Open(szFile2) == W_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile2);

    OUTPUT_TEST_ERROR
  }
  else
  {
    WStringBuilder sFile1;
    sFile1.ReadAll(ReadFile1);
    sFile1.ReplaceAll("\r\n", "\n");

    WStringBuilder sFile2;
    sFile2.ReadAll(ReadFile2);
    sFile2.ReplaceAll("\r\n", "\n");

    if (sFile1 != sFile2)
    {
      safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: Text files contents do not match: '%s' and '%s'", szFile1, szFile2);

      OUTPUT_TEST_ERROR
    }
  }

  return true;
}

bool WTestImage(WUInt32 uiImageNumber, WUInt32 uiMaxError, bool bIsDepthImage, bool bIsLineImage, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  char szErrorText[s_iMaxErrorMessageLength] = "";

  if (!WTestFramework::GetInstance()->CompareImages(uiImageNumber, uiMaxError, szErrorText, bIsDepthImage, bIsLineImage))
  {
    OUTPUT_TEST_ERROR_NO_BREAK
  }

  return true;
}
