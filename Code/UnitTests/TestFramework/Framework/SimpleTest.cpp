#include <TestFramework/TestFrameworkPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <TestFramework/Framework/TestFramework.h>

W_ENUMERABLE_CLASS_IMPLEMENTATION(WRegisterTestHelper);

void WSimpleTestGroup::AddSimpleTest(const char* szName, SimpleTestFunc testFunc)
{
  SimpleTestEntry e;
  e.m_szName = szName;
  e.m_Func = testFunc;

  for (WUInt32 i = 0; i < m_SimpleTests.size(); ++i)
  {
    if ((strcmp(m_SimpleTests[i].m_szName, e.m_szName) == 0) && (m_SimpleTests[i].m_Func == e.m_Func))
      return;
  }

  m_SimpleTests.push_back(e);
}

void WSimpleTestGroup::SetupSubTests()
{
  for (WUInt32 i = 0; i < m_SimpleTests.size(); ++i)
  {
    AddSubTest(m_SimpleTests[i].m_szName, i);
  }
}

WTestAppRun WSimpleTestGroup::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  // until the block name is properly set, use the test name instead
  WTestFramework::s_szTestBlockName = m_SimpleTests[iIdentifier].m_szName;

  W_PROFILE_SCOPE(m_SimpleTests[iIdentifier].m_szName);
  m_SimpleTests[iIdentifier].m_Func();

  WTestFramework::s_szTestBlockName = "";
  return WTestAppRun::Quit;
}

WResult WSimpleTestGroup::InitializeSubTest(WInt32 iIdentifier)
{
  // initialize everything up to 'core'
  WStartup::StartupCoreSystems();
  return W_SUCCESS;
}

WResult WSimpleTestGroup::DeInitializeSubTest(WInt32 iIdentifier)
{
  // shut down completely
  WStartup::ShutdownCoreSystems();
  WMemoryTracker::DumpMemoryLeaks();
  return W_SUCCESS;
}
