#pragma once

#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/TestFrameworkDLL.h>
#include <deque>
#include <string>
#include <vector>

struct W_TEST_DLL WTestOutput
{
  /// Defines the type of output message for WTestOutputMessage.
  enum Enum
  {
    InvalidType = -1,
    StartOutput = 0,
    BeginBlock,
    EndBlock,
    ImportantInfo,
    Details,
    Success,
    Message,
    Warning,
    Error,
    ImageDiffFile,
    Duration,
    FinalResult,
    AllOutputTypes
  };

  static const char* const s_Names[];
  static const char* ToString(Enum type);
  static Enum FromString(const char* szName);
};

/// A message of type WTestOutput::Enum, stored in WResult.
struct W_TEST_DLL WTestErrorMessage
{
  WTestErrorMessage() = default;

  std::string m_sError;
  std::string m_sBlock;
  std::string m_sFile;
  WInt32 m_iLine = -1;
  std::string m_sFunction;
  std::string m_sMessage;
};

/// A message of type WTestOutput::Enum, stored in WResult.
struct W_TEST_DLL WTestOutputMessage
{
  WTestOutputMessage() = default;

  WTestOutput::Enum m_Type = WTestOutput::ImportantInfo;
  std::string m_sMessage;
  WInt32 m_iErrorIndex = -1;
};

struct W_TEST_DLL WTestResultQuery
{
  /// Defines what information should be accumulated over the sub-tests in WTestEntry::GetSubTestCount.
  enum Enum
  {
    Count,
    Executed,
    Success,
    Errors,
  };
};

/// Stores the results of a test run. Used by both WTestEntry and WSubTestEntry.
struct W_TEST_DLL WTestResultData
{
  WTestResultData() = default;

  void Reset();
  void AddOutput(WInt32 iOutputIndex);

  std::string m_sName;
  bool m_bExecuted = false;     ///< Whether the test was executed. If false, the test was either deactivated or the test process crashed before
                                ///< executing it.
  bool m_bSuccess = false;      ///< Whether the test succeeded or not.
  int m_iTestAsserts = 0;       ///< Asserts that were checked. For tests this includes the count of all of their sub-tests as well.
  double m_fTestDuration = 0.0; ///< Duration of the test/sub-test. For tests, this includes the duration of all their sub-tests as well.
  WInt32 m_iFirstOutput = -1;  ///< First output message. For tests, this range includes all messages of their sub-tests as well.
  WInt32 m_iLastOutput = -1;   ///< Last output message. For tests, this range includes all messages of their sub-tests as well.
  std::string m_sCustomStatus;  ///< If this is not empty, the UI will display this instead of "Pending"
};

struct W_TEST_DLL WTestConfiguration
{
  WTestConfiguration();

  WUInt64 m_uiInstalledMainMemory = 0;
  WUInt32 m_uiMemoryPageSize = 0;
  WUInt32 m_uiCPUCoreCount = 0;
  bool m_b64BitOS = false;
  bool m_b64BitApplication = false;
  std::string m_sPlatformName;
  std::string m_sBuildConfiguration; ///< Debug, Release, etc
  WInt64 m_iDateTime = 0;           ///< in seconds since Linux epoch
  WInt32 m_iRCSRevision = -1;
  std::string m_sHostName;
};

class W_TEST_DLL WTestFrameworkResult
{
public:
  WTestFrameworkResult() = default;

  // Manage tests
  void Clear();
  void SetupTests(const std::deque<WTestEntry>& tests, const WTestConfiguration& config);
  void Reset();
  bool WriteJsonToFile(const char* szFileName) const;

  // Result access
  WUInt32 GetTestCount(WTestResultQuery::Enum countQuery = WTestResultQuery::Count) const;
  WUInt32 GetSubTestCount(WUInt32 uiTestIndex, WTestResultQuery::Enum countQuery = WTestResultQuery::Count) const;
  WUInt32 GetTestIndexByName(const char* szTestName) const;
  WUInt32 GetSubTestIndexByName(WUInt32 uiTestIndex, const char* szSubTestName) const;
  double GetTotalTestDuration() const;
  const WTestResultData& GetTestResultData(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex) const;

  // Test output
  void TestOutput(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, WTestOutput::Enum type, const char* szMsg);
  void TestError(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, const char* szError, const char* szBlock, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg);
  void TestResult(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, bool bSuccess, double fDuration);
  void AddAsserts(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, int iCount);
  void SetCustomStatus(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, const char* szCustomStatus);

  // Messages / Errors
  WUInt32 GetOutputMessageCount(WUInt32 uiTestIndex = WInvalidIndex, WUInt32 uiSubTestIndex = WInvalidIndex, WTestOutput::Enum type = WTestOutput::AllOutputTypes) const;
  const WTestOutputMessage* GetOutputMessage(WUInt32 uiOutputMessageIdx) const;

  WUInt32 GetErrorMessageCount(WUInt32 uiTestIndex = WInvalidIndex, WUInt32 uiSubTestIndex = WInvalidIndex) const;
  const WTestErrorMessage* GetErrorMessage(WUInt32 uiErrorMessageIdx) const;

private:
  struct WSubTestResult
  {
    WSubTestResult() = default;
    WSubTestResult(const char* szName) { m_Result.m_sName = szName; }

    WTestResultData m_Result;
  };

  struct WTestResult
  {
    WTestResult() = default;
    WTestResult(const char* szName) { m_Result.m_sName = szName; }

    void Reset();

    WTestResultData m_Result;
    std::deque<WSubTestResult> m_SubTests;
  };

private:
  WTestConfiguration m_Config;
  std::deque<WTestResult> m_Tests;
  std::deque<WTestErrorMessage> m_Errors;
  std::deque<WTestOutputMessage> m_TestOutput;
};
