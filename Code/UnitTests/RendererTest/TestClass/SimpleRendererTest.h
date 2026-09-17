#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <RendererTest/TestClass/TestClass.h>
#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/SimpleTest.h>

class WSimpleRendererTestGroup : public WGraphicsTest
{
public:
  using SimpleRendererTestFunc = void (*)();

  WSimpleRendererTestGroup(const char* szName)
    : m_szTestName(szName)
  {
  }

  void AddSimpleRendererTest(const char* szName, SimpleRendererTestFunc testFunc);
  virtual const char* GetTestName() const override { return m_szTestName; }

private:
  virtual void SetupSubTests() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

private:
  struct SimpleRendererTestEntry
  {
    const char* m_szName;
    SimpleRendererTestFunc m_Func;
  };

  const char* m_szTestName;
  std::deque<SimpleRendererTestEntry> m_SimpleRendererTests;
};

class WRegisterSimpleRendererTestHelper : public WRegisterTestHelper
{
public:
  WRegisterSimpleRendererTestHelper(WSimpleRendererTestGroup* pTestGroup, const char* szTestName, WSimpleRendererTestGroup::SimpleRendererTestFunc func)
  {
    m_pTestGroup = pTestGroup;
    m_szTestName = szTestName;
    m_Func = func;
  }

  virtual void RegisterTest() override { m_pTestGroup->AddSimpleRendererTest(m_szTestName, m_Func); }

private:
  WSimpleRendererTestGroup* m_pTestGroup;
  const char* m_szTestName;
  WSimpleRendererTestGroup::SimpleRendererTestFunc m_Func;
};

#define W_CREATE_SIMPLE_RENDERER_TEST_GROUP(GroupName) WSimpleRendererTestGroup W_PP_CONCAT(g_SimpleRendererTestGroup__, GroupName)(W_PP_STRINGIFY(GroupName));

#define W_CREATE_SIMPLE_RENDERER_TEST(GroupName, TestName)                                                                                    \
  extern WSimpleRendererTestGroup W_PP_CONCAT(g_SimpleRendererTestGroup__, GroupName);                                                       \
  static void WSimpleRendererTestFunction__##GroupName##_##TestName();                                                                        \
  WRegisterSimpleRendererTestHelper WRegisterSimpleRendererTest__##GroupName##TestName(                                                      \
    &W_PP_CONCAT(g_SimpleRendererTestGroup__, GroupName), W_PP_STRINGIFY(TestName), WSimpleRendererTestFunction__##GroupName##_##TestName); \
  static void WSimpleRendererTestFunction__##GroupName##_##TestName()
