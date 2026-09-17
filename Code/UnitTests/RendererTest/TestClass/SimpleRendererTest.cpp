#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

void WSimpleRendererTestGroup::AddSimpleRendererTest(const char* szName, SimpleRendererTestFunc testFunc)
{
  SimpleRendererTestEntry e;
  e.m_szName = szName;
  e.m_Func = testFunc;

  for (WUInt32 i = 0; i < m_SimpleRendererTests.size(); ++i)
  {
    if ((strcmp(m_SimpleRendererTests[i].m_szName, e.m_szName) == 0) && (m_SimpleRendererTests[i].m_Func == e.m_Func))
      return;
  }

  m_SimpleRendererTests.push_back(e);
}

void WSimpleRendererTestGroup::SetupSubTests()
{
  for (WUInt32 i = 0; i < m_SimpleRendererTests.size(); ++i)
  {
    AddSubTest(m_SimpleRendererTests[i].m_szName, i);
  }
}

WTestAppRun WSimpleRendererTestGroup::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  // until the block name is properly set, use the test name instead
  WTestFramework::s_szTestBlockName = m_SimpleRendererTests[iIdentifier].m_szName;

  W_PROFILE_SCOPE(m_SimpleRendererTests[iIdentifier].m_szName);
  m_SimpleRendererTests[iIdentifier].m_Func();

  WTestFramework::s_szTestBlockName = "";
  return WTestAppRun::Quit;
}
