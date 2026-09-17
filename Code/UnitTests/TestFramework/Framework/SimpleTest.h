#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/TestBaseClass.h>

class W_TEST_DLL WSimpleTestGroup : public WTestBaseClass
{
public:
  using SimpleTestFunc = void (*)();

  WSimpleTestGroup(const char* szName)
    : m_szTestName(szName)
  {
  }

  void AddSimpleTest(const char* szName, SimpleTestFunc testFunc);

  virtual const char* GetTestName() const override { return m_szTestName; }

private:
  virtual void SetupSubTests() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;

private:
  struct SimpleTestEntry
  {
    const char* m_szName;
    SimpleTestFunc m_Func;
  };

  const char* m_szTestName;
  std::deque<SimpleTestEntry> m_SimpleTests;
};

class W_TEST_DLL WRegisterTestHelper : public WEnumerable<WRegisterTestHelper>
{
  W_DECLARE_ENUMERABLE_CLASS(WRegisterTestHelper);

public:
  WRegisterTestHelper() = default;
  virtual ~WRegisterTestHelper() = default;
  virtual void RegisterTest() = 0;
};

class W_TEST_DLL WRegisterSimpleTestHelper : public WRegisterTestHelper
{
public:
  WRegisterSimpleTestHelper(WSimpleTestGroup* pTestGroup, const char* szTestName, WSimpleTestGroup::SimpleTestFunc func)
  {
    m_pTestGroup = pTestGroup;
    m_szTestName = szTestName;
    m_Func = func;
  }

  virtual void RegisterTest() override { m_pTestGroup->AddSimpleTest(m_szTestName, m_Func); }

private:
  WSimpleTestGroup* m_pTestGroup;
  const char* m_szTestName;
  WSimpleTestGroup::SimpleTestFunc m_Func;
};

#define W_CREATE_SIMPLE_TEST_GROUP(GroupName) WSimpleTestGroup W_PP_CONCAT(g_SimpleTestGroup__, GroupName)(W_PP_STRINGIFY(GroupName));

#define W_CREATE_SIMPLE_TEST(GroupName, TestName)                                                                             \
  extern WSimpleTestGroup W_PP_CONCAT(g_SimpleTestGroup__, GroupName);                                                       \
  static void WSimpleTestFunction__##GroupName##_##TestName();                                                                \
  WRegisterSimpleTestHelper WRegisterSimpleTest__##GroupName##TestName(                                                      \
    &W_PP_CONCAT(g_SimpleTestGroup__, GroupName), W_PP_STRINGIFY(TestName), WSimpleTestFunction__##GroupName##_##TestName); \
  static void WSimpleTestFunction__##GroupName##_##TestName()
