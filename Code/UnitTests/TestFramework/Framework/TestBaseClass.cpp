#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Framework/TestFramework.h>

W_ENUMERABLE_CLASS_IMPLEMENTATION(WTestBaseClass);

const char* WTestBaseClass::GetSubTestName(WInt32 iIdentifier) const
{
  const WInt32 entryIndex = FindEntryForIdentifier(iIdentifier);

  if (entryIndex < 0)
  {
    WLog::Error("Tried to access retrieve sub-test name using invalid identifier.");
    return "";
  }

  return m_Entries[entryIndex].m_szName;
}

void WTestBaseClass::UpdateConfiguration(WTestConfiguration& ref_config) const
{
  // If the configuration hasn't been set yet this is the first instance of WTestBaseClass being called
  // to fill in the configuration and we thus have to do so.
  // Derived classes can have more information (e.g.GPU info) and there is no way to know which instance
  // of WTestBaseClass may have additional information so we ask all of them and each one early outs
  // if the information it knows about is already present.
  if (ref_config.m_uiInstalledMainMemory == 0)
  {
    const WSystemInformation& pSysInfo = WSystemInformation::Get();
    ref_config.m_uiInstalledMainMemory = pSysInfo.GetInstalledMainMemory();
    ref_config.m_uiMemoryPageSize = pSysInfo.GetMemoryPageSize();
    ref_config.m_uiCPUCoreCount = pSysInfo.GetCPUCoreCount();
    ref_config.m_sPlatformName = pSysInfo.GetPlatformName();
    ref_config.m_b64BitOS = pSysInfo.Is64BitOS();
    ref_config.m_b64BitApplication = W_ENABLED(W_PLATFORM_64BIT);
    ref_config.m_sBuildConfiguration = pSysInfo.GetBuildConfiguration();
    ref_config.m_iDateTime = WTimestamp::CurrentTimestamp().GetInt64(WSIUnitOfTime::Second);
    ref_config.m_iRCSRevision = WTestFramework::GetInstance()->GetSettings().m_iRevision;
    ref_config.m_sHostName = pSysInfo.GetHostName();
  }
}

void WTestBaseClass::MapImageNumberToString(const char* szTestName, const WSubTestEntry& subTest, WUInt32 uiImageNumber, WStringBuilder& out_sString) const
{
  out_sString.SetFormat("{0}_{1}_{2}", szTestName, subTest.m_szSubTestName, WArgI(uiImageNumber, 3, true));
  out_sString.ReplaceAll(" ", "_");
}

void WTestBaseClass::ClearSubTests()
{
  m_Entries.clear();
}

void WTestBaseClass::AddSubTest(const char* szName, WInt32 iIdentifier)
{
  W_ASSERT_DEV(szName != nullptr, "Sub test name must not be nullptr");

  TestEntry e;
  e.m_szName = szName;
  e.m_iIdentifier = iIdentifier;

  m_Entries.push_back(e);
}

WResult WTestBaseClass::DoTestInitialization()
{
  try
  {
    if (InitializeTest() == W_FAILURE)
    {
      WTestFramework::Output(WTestOutput::Error, "Test Initialization failed.");
      return W_FAILURE;
    }
  }
  catch (...)
  {
    WTestFramework::Output(WTestOutput::Error, "Exception during test initialization.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

void WTestBaseClass::DoTestDeInitialization()
{
  try

  {
    if (DeInitializeTest() == W_FAILURE)
      WTestFramework::Output(WTestOutput::Error, "Test DeInitialization failed.");
  }
  catch (...)
  {
    WTestFramework::Output(WTestOutput::Error, "Exception during test de-initialization.");
  }
}

WResult WTestBaseClass::DoSubTestInitialization(WInt32 iIdentifier)
{
  try
  {
    if (InitializeSubTest(iIdentifier) == W_FAILURE)
    {
      WTestFramework::Output(WTestOutput::Error, "Sub-Test Initialization failed, skipping Test.");
      return W_FAILURE;
    }
  }
  catch (...)
  {
    WTestFramework::Output(WTestOutput::Error, "Exception during sub-test initialization.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

void WTestBaseClass::DoSubTestDeInitialization(WInt32 iIdentifier)
{
  try
  {
    if (DeInitializeSubTest(iIdentifier) == W_FAILURE)
      WTestFramework::Output(WTestOutput::Error, "Sub-Test De-Initialization failed.");
  }
  catch (...)
  {
    WTestFramework::Output(WTestOutput::Error, "Exception during sub-test de-initialization.");
  }
}

WTestAppRun WTestBaseClass::DoSubTestRun(WInt32 iIdentifier, double& fDuration, WUInt32 uiInvocationCount)
{
  fDuration = 0.0;

  WTestAppRun ret = WTestAppRun::Quit;

  try
  {
    WTime StartTime = WTime::Now();

    ret = RunSubTest(iIdentifier, uiInvocationCount);

    fDuration = (WTime::Now() - StartTime).GetMilliseconds();
  }
  catch (...)
  {
    const WInt32 iEntry = FindEntryForIdentifier(iIdentifier);

    if (iEntry >= 0)
      WTestFramework::Output(WTestOutput::Error, "Exception during sub-test '%s'.", m_Entries[iEntry].m_szName);
    else
      WTestFramework::Output(WTestOutput::Error, "Exception during unknown sub-test.");
  }

  return ret;
}

WInt32 WTestBaseClass::FindEntryForIdentifier(WInt32 iIdentifier) const
{
  for (WInt32 i = 0; i < (WInt32)m_Entries.size(); ++i)
  {
    if (m_Entries[i].m_iIdentifier == iIdentifier)
    {
      return i;
    }
  }

  return -1;
}
