#include <TestFramework/TestFrameworkPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <TestFramework/Framework/TestResults.h>

////////////////////////////////////////////////////////////////////////
// WTestOutput public functions
////////////////////////////////////////////////////////////////////////

const char* const WTestOutput::s_Names[] = {
  "StartOutput", "BeginBlock", "EndBlock", "ImportantInfo", "Details", "Success", "Message", "Warning", "Error", "Duration", "FinalResult"};

const char* WTestOutput::ToString(Enum type)
{
  return s_Names[type];
}

WTestOutput::Enum WTestOutput::FromString(const char* szName)
{
  for (WUInt32 i = 0; i < AllOutputTypes; ++i)
  {
    if (strcmp(szName, s_Names[i]) == 0)
      return (WTestOutput::Enum)i;
  }
  return InvalidType;
}


////////////////////////////////////////////////////////////////////////
// WTestResultData public functions
////////////////////////////////////////////////////////////////////////

void WTestResultData::Reset()
{
  m_bExecuted = false;
  m_bSuccess = false;
  m_iTestAsserts = 0;
  m_fTestDuration = 0.0;
  m_iFirstOutput = -1;
  m_iLastOutput = -1;
  m_sCustomStatus.clear();
}

void WTestResultData::AddOutput(WInt32 iOutputIndex)
{
  if (m_iFirstOutput == -1)
  {
    m_iFirstOutput = iOutputIndex;
    m_iLastOutput = iOutputIndex;
  }
  else
  {
    m_iLastOutput = iOutputIndex;
  }
}


////////////////////////////////////////////////////////////////////////
// WTestResultData public functions
////////////////////////////////////////////////////////////////////////

WTestConfiguration::WTestConfiguration()

  = default;


////////////////////////////////////////////////////////////////////////
// WTestFrameworkResult public functions
////////////////////////////////////////////////////////////////////////

void WTestFrameworkResult::Clear()
{
  m_Tests.clear();
  m_Errors.clear();
  m_TestOutput.clear();
}

void WTestFrameworkResult::SetupTests(const std::deque<WTestEntry>& tests, const WTestConfiguration& config)
{
  m_Config = config;
  Clear();

  const WUInt32 uiTestCount = (WUInt32)tests.size();
  for (WUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_Tests.push_back(WTestResult(tests[uiTestIdx].m_szTestName));

    const WUInt32 uiSubTestCount = (WUInt32)tests[uiTestIdx].m_SubTests.size();
    for (WUInt32 uiSubTestIdx = 0; uiSubTestIdx < uiSubTestCount; ++uiSubTestIdx)
    {
      m_Tests[uiTestIdx].m_SubTests.push_back(WSubTestResult(tests[uiTestIdx].m_SubTests[uiSubTestIdx].m_szSubTestName));
    }
  }
}

void ::WTestFrameworkResult::Reset()
{
  const WUInt32 uiTestCount = (WUInt32)m_Tests.size();
  for (WUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_Tests[uiTestIdx].Reset();
  }
  m_Errors.clear();
  m_TestOutput.clear();
}

bool WTestFrameworkResult::WriteJsonToFile(const char* szFileName) const
{
  WStartup::StartupCoreSystems();
  W_SCOPE_EXIT(WStartup::ShutdownCoreSystems());

  {
    WStringBuilder jsonFilename;
    if (WPathUtils::IsAbsolutePath(szFileName))
    {
      // Make sure we can access raw absolute file paths
      if (WFileSystem::AddDataDirectory("", "jsonoutput", ":", WDataDirUsage::AllowWrites).Failed())
        return false;

      jsonFilename = szFileName;
    }
    else
    {
      // If this is a relative path, we use the Wtest/ data directory to make sure that this works properly with the fileserver.
      if (WFileSystem::AddDataDirectory(">Wtest/", "jsonoutput", ":", WDataDirUsage::AllowWrites).Failed())
        return false;

      jsonFilename = ":";
      jsonFilename.AppendPath(szFileName);
    }

    WFileWriter file;
    if (file.Open(jsonFilename).Failed())
    {
      return false;
    }
    WStandardJSONWriter js;
    js.SetOutputStream(&file);

    js.BeginObject();
    {
      js.BeginObject("configuration");
      {
        js.AddVariableUInt64("m_uiInstalledMainMemory", m_Config.m_uiInstalledMainMemory);
        js.AddVariableUInt32("m_uiMemoryPageSize", m_Config.m_uiMemoryPageSize);
        js.AddVariableUInt32("m_uiCPUCoreCount", m_Config.m_uiCPUCoreCount);
        js.AddVariableBool("m_b64BitOS", m_Config.m_b64BitOS);
        js.AddVariableBool("m_b64BitApplication", m_Config.m_b64BitApplication);
        js.AddVariableString("m_sPlatformName", m_Config.m_sPlatformName.c_str());
        js.AddVariableString("m_sBuildConfiguration", m_Config.m_sBuildConfiguration.c_str());
        js.AddVariableInt64("m_iDateTime", m_Config.m_iDateTime);
        js.AddVariableInt32("m_iRCSRevision", m_Config.m_iRCSRevision);
        js.AddVariableString("m_sHostName", m_Config.m_sHostName.c_str());
      }
      js.EndObject();

      // Output Messages
      js.BeginArray("messages");
      {
        WUInt32 uiMessages = GetOutputMessageCount();
        for (WUInt32 uiMessageIdx = 0; uiMessageIdx < uiMessages; ++uiMessageIdx)
        {
          const WTestOutputMessage* pMessage = GetOutputMessage(uiMessageIdx);
          js.BeginObject();
          {
            js.AddVariableString("m_Type", WTestOutput::ToString(pMessage->m_Type));
            js.AddVariableString("m_sMessage", pMessage->m_sMessage.c_str());
            if (pMessage->m_iErrorIndex != -1)
              js.AddVariableInt32("m_iErrorIndex", pMessage->m_iErrorIndex);
          }
          js.EndObject();
        }
      }
      js.EndArray();

      // Error Messages
      js.BeginArray("errors");
      {
        WUInt32 uiMessages = GetErrorMessageCount();
        for (WUInt32 uiMessageIdx = 0; uiMessageIdx < uiMessages; ++uiMessageIdx)
        {
          const WTestErrorMessage* pMessage = GetErrorMessage(uiMessageIdx);
          js.BeginObject();
          {
            js.AddVariableString("m_sError", pMessage->m_sError.c_str());
            js.AddVariableString("m_sBlock", pMessage->m_sBlock.c_str());
            js.AddVariableString("m_sFile", pMessage->m_sFile.c_str());
            js.AddVariableString("m_sFunction", pMessage->m_sFunction.c_str());
            js.AddVariableInt32("m_iLine", pMessage->m_iLine);
            js.AddVariableString("m_sMessage", pMessage->m_sMessage.c_str());
          }
          js.EndObject();
        }
      }
      js.EndArray();

      // Tests
      js.BeginArray("tests");
      {
        WUInt32 uiTests = GetTestCount();
        for (WUInt32 uiTestIdx = 0; uiTestIdx < uiTests; ++uiTestIdx)
        {
          const WTestResultData& testResult = GetTestResultData(uiTestIdx, WInvalidIndex);
          js.BeginObject();
          {
            js.AddVariableString("m_sName", testResult.m_sName.c_str());
            js.AddVariableBool("m_bExecuted", testResult.m_bExecuted);
            js.AddVariableBool("m_bSuccess", testResult.m_bSuccess);
            js.AddVariableInt32("m_iTestAsserts", testResult.m_iTestAsserts);
            js.AddVariableDouble("m_fTestDuration", testResult.m_fTestDuration);
            js.AddVariableInt32("m_iFirstOutput", testResult.m_iFirstOutput);
            js.AddVariableInt32("m_iLastOutput", testResult.m_iLastOutput);

            // Sub Tests
            js.BeginArray("subTests");
            {
              WUInt32 uiSubTests = GetSubTestCount(uiTestIdx);
              for (WUInt32 uiSubTestIdx = 0; uiSubTestIdx < uiSubTests; ++uiSubTestIdx)
              {
                const WTestResultData& subTestResult = GetTestResultData(uiTestIdx, uiSubTestIdx);
                js.BeginObject();
                {
                  js.AddVariableString("m_sName", subTestResult.m_sName.c_str());
                  js.AddVariableBool("m_bExecuted", subTestResult.m_bExecuted);
                  js.AddVariableBool("m_bSuccess", subTestResult.m_bSuccess);
                  js.AddVariableInt32("m_iTestAsserts", subTestResult.m_iTestAsserts);
                  js.AddVariableDouble("m_fTestDuration", subTestResult.m_fTestDuration);
                  js.AddVariableInt32("m_iFirstOutput", subTestResult.m_iFirstOutput);
                  js.AddVariableInt32("m_iLastOutput", subTestResult.m_iLastOutput);
                }
                js.EndObject();
              }
            }
            js.EndArray(); // subTests
          }
          js.EndObject();
        }
      }
      js.EndArray(); // tests
    }
    js.EndObject();
  }

  return true;
}

WUInt32 WTestFrameworkResult::GetTestCount(WTestResultQuery::Enum countQuery) const
{
  WUInt32 uiAccumulator = 0;
  const WUInt32 uiTests = (WUInt32)m_Tests.size();

  if (countQuery == WTestResultQuery::Count)
    return uiTests;

  if (countQuery == WTestResultQuery::Errors)
    return (WUInt32)m_Errors.size();

  for (WUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    switch (countQuery)
    {
      case WTestResultQuery::Executed:
        uiAccumulator += m_Tests[uiTest].m_Result.m_bExecuted ? 1 : 0;
        break;
      case WTestResultQuery::Success:
        uiAccumulator += m_Tests[uiTest].m_Result.m_bSuccess ? 1 : 0;
        break;
      default:
        break;
    }
  }
  return uiAccumulator;
}

WUInt32 WTestFrameworkResult::GetSubTestCount(WUInt32 uiTestIndex, WTestResultQuery::Enum countQuery) const
{
  if (uiTestIndex >= (WUInt32)m_Tests.size())
    return 0;

  const WTestResult& test = m_Tests[uiTestIndex];
  WUInt32 uiAccumulator = 0;
  const WUInt32 uiSubTests = (WUInt32)test.m_SubTests.size();

  if (countQuery == WTestResultQuery::Count)
    return uiSubTests;

  if (countQuery == WTestResultQuery::Errors)
  {
    for (WInt32 iOutputIdx = test.m_Result.m_iFirstOutput; iOutputIdx <= test.m_Result.m_iLastOutput && iOutputIdx != -1; ++iOutputIdx)
    {
      if (m_TestOutput[iOutputIdx].m_Type == WTestOutput::Error)
        uiAccumulator++;
    }
    return uiAccumulator;
  }

  for (WUInt32 uiSubTest = 0; uiSubTest < uiSubTests; ++uiSubTest)
  {
    switch (countQuery)
    {
      case WTestResultQuery::Executed:
        uiAccumulator += test.m_SubTests[uiSubTest].m_Result.m_bExecuted ? 1 : 0;
        break;
      case WTestResultQuery::Success:
        uiAccumulator += test.m_SubTests[uiSubTest].m_Result.m_bSuccess ? 1 : 0;
        break;
      default:
        break;
    }
  }
  return uiAccumulator;
}

WUInt32 WTestFrameworkResult::GetTestIndexByName(const char* szTestName) const
{
  const WUInt32 uiTestCount = GetTestCount();
  for (WUInt32 i = 0; i < uiTestCount; ++i)
  {
    if (m_Tests[i].m_Result.m_sName.compare(szTestName) == 0)
      return i;
  }

  return WInvalidIndex;
}

WUInt32 WTestFrameworkResult::GetSubTestIndexByName(WUInt32 uiTestIndex, const char* szSubTestName) const
{
  if (uiTestIndex >= GetTestCount())
    return WInvalidIndex;

  const WUInt32 uiSubTestCount = GetSubTestCount(uiTestIndex);
  for (WUInt32 i = 0; i < uiSubTestCount; ++i)
  {
    if (m_Tests[uiTestIndex].m_SubTests[i].m_Result.m_sName.compare(szSubTestName) == 0)
      return i;
  }

  return WInvalidIndex;
}

double WTestFrameworkResult::GetTotalTestDuration() const
{
  double fTotalTestDuration = 0.0;
  const WUInt32 uiTests = (WUInt32)m_Tests.size();
  for (WUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    fTotalTestDuration += m_Tests[uiTest].m_Result.m_fTestDuration;
  }
  return fTotalTestDuration;
}

const WTestResultData& WTestFrameworkResult::GetTestResultData(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex) const
{
  return (uiSubTestIndex == WInvalidIndex) ? m_Tests[uiTestIndex].m_Result : m_Tests[uiTestIndex].m_SubTests[uiSubTestIndex].m_Result;
}

void WTestFrameworkResult::TestOutput(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, WTestOutput::Enum type, const char* szMsg)
{
  if (uiTestIndex != WInvalidIndex)
  {
    m_Tests[uiTestIndex].m_Result.AddOutput((WUInt32)m_TestOutput.size());
    if (uiSubTestIndex != WInvalidIndex)
    {
      m_Tests[uiTestIndex].m_SubTests[uiSubTestIndex].m_Result.AddOutput((WUInt32)m_TestOutput.size());
    }
  }

  m_TestOutput.push_back(WTestOutputMessage());
  WTestOutputMessage& outputMessage = *m_TestOutput.rbegin();
  outputMessage.m_Type = type;
  outputMessage.m_sMessage.assign(szMsg);
}

void WTestFrameworkResult::TestError(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, const char* szError, const char* szBlock, const char* szFile,
  WInt32 iLine, const char* szFunction, const char* szMsg)
{
  // In case there is no message set, we use the error as the message.
  TestOutput(uiTestIndex, uiSubTestIndex, WTestOutput::Error, szError);
  m_TestOutput.rbegin()->m_iErrorIndex = (WInt32)m_Errors.size();

  m_Errors.push_back(WTestErrorMessage());
  WTestErrorMessage& errorMessage = *m_Errors.rbegin();
  errorMessage.m_sError.assign(szError);
  errorMessage.m_sBlock.assign(szBlock);
  errorMessage.m_sFile.assign(szFile);
  errorMessage.m_iLine = iLine;
  errorMessage.m_sFunction.assign(szFunction);
  errorMessage.m_sMessage.assign(szMsg);
}

void WTestFrameworkResult::TestResult(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, bool bSuccess, double fDuration)
{
  WTestResultData& Result = (uiSubTestIndex == WInvalidIndex) ? m_Tests[uiTestIndex].m_Result : m_Tests[uiTestIndex].m_SubTests[uiSubTestIndex].m_Result;

  Result.m_bExecuted = true;
  Result.m_bSuccess = bSuccess;
  Result.m_fTestDuration = fDuration;

  // Accumulate sub-test duration onto test duration to get duration feedback while the sub-tests are running.
  // Final time will be set again once the entire test finishes and currently these times are identical as
  // init and de-init times aren't measured at the moment due to missing timer when engine is shut down.
  if (uiSubTestIndex != WInvalidIndex)
  {
    m_Tests[uiTestIndex].m_Result.m_fTestDuration += fDuration;
  }
}

void WTestFrameworkResult::AddAsserts(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, int iCount)
{
  if (uiTestIndex != WInvalidIndex)
  {
    m_Tests[uiTestIndex].m_Result.m_iTestAsserts += iCount;
  }

  if (uiSubTestIndex != WInvalidIndex)
  {
    m_Tests[uiTestIndex].m_SubTests[uiSubTestIndex].m_Result.m_iTestAsserts += iCount;
  }
}

void WTestFrameworkResult::SetCustomStatus(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, const char* szCustomStatus)
{
  if (uiTestIndex != WInvalidIndex && uiSubTestIndex != WInvalidIndex)
  {
    m_Tests[uiTestIndex].m_SubTests[uiSubTestIndex].m_Result.m_sCustomStatus = szCustomStatus;
  }
}

WUInt32 WTestFrameworkResult::GetOutputMessageCount(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, WTestOutput::Enum type) const
{
  if (uiTestIndex == WInvalidIndex && type == WTestOutput::AllOutputTypes)
    return (WUInt32)m_TestOutput.size();

  WInt32 iStartIdx = 0;
  WInt32 iEndIdx = (WInt32)m_TestOutput.size() - 1;

  if (uiTestIndex != WInvalidIndex)
  {
    const WTestResultData& result = GetTestResultData(uiTestIndex, uiSubTestIndex);
    iStartIdx = result.m_iFirstOutput;
    iEndIdx = result.m_iLastOutput;

    // If no messages have been output (yet) for the given test we early-out here.
    if (iStartIdx == -1)
      return 0;

    // If all message types should be counted we can simply return the range.
    if (type == WTestOutput::AllOutputTypes)
      return iEndIdx - iStartIdx + 1;
  }

  WUInt32 uiAccumulator = 0;
  for (WInt32 uiOutputMessageIdx = iStartIdx; uiOutputMessageIdx <= iEndIdx; ++uiOutputMessageIdx)
  {
    if (m_TestOutput[uiOutputMessageIdx].m_Type == type)
      uiAccumulator++;
  }
  return uiAccumulator;
}

const WTestOutputMessage* WTestFrameworkResult::GetOutputMessage(WUInt32 uiOutputMessageIdx) const
{
  return &m_TestOutput[uiOutputMessageIdx];
}

WUInt32 WTestFrameworkResult::GetErrorMessageCount(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex) const
{
  // If no test is given we can simply return the total error count.
  if (uiTestIndex == WInvalidIndex)
  {
    return (WUInt32)m_Errors.size();
  }

  return GetOutputMessageCount(uiTestIndex, uiSubTestIndex, WTestOutput::Error);
}

const WTestErrorMessage* WTestFrameworkResult::GetErrorMessage(WUInt32 uiErrorMessageIdx) const
{
  return &m_Errors[uiErrorMessageIdx];
}


////////////////////////////////////////////////////////////////////////
// WTestFrameworkResult public functions
////////////////////////////////////////////////////////////////////////

void WTestFrameworkResult::WTestResult::Reset()
{
  m_Result.Reset();
  const WUInt32 uiSubTestCount = (WUInt32)m_SubTests.size();
  for (WUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
  {
    m_SubTests[uiSubTest].m_Result.Reset();
  }
}
