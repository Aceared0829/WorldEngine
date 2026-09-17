#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/Singleton.h>

class TestSingleton
{
  W_DECLARE_SINGLETON(TestSingleton);

public:
  TestSingleton()
    : m_SingletonRegistrar(this)
  {
  }

  WInt32 m_iValue = 41;
};

W_IMPLEMENT_SINGLETON(TestSingleton);

class SingletonInterface
{
public:
  virtual WInt32 GetValue() = 0;
};

class TestSingletonOfInterface : public SingletonInterface
{
  W_DECLARE_SINGLETON_OF_INTERFACE(TestSingletonOfInterface, SingletonInterface);

public:
  TestSingletonOfInterface()
    : m_SingletonRegistrar(this)
  {
  }

  virtual WInt32 GetValue() { return 23; }
};

W_IMPLEMENT_SINGLETON(TestSingletonOfInterface);


W_CREATE_SIMPLE_TEST(Configuration, Singleton)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Singleton Registration")
  {
    {
      TestSingleton* pSingleton = WSingletonRegistry::GetSingletonInstance<TestSingleton>();
      W_TEST_BOOL(pSingleton == nullptr);
    }

    {
      TestSingleton g_Singleton;

      {
        TestSingleton* pSingleton = WSingletonRegistry::GetSingletonInstance<TestSingleton>();
        W_TEST_BOOL(pSingleton == &g_Singleton);
        W_TEST_INT(pSingleton->m_iValue, 41);
      }
    }

    {
      TestSingleton* pSingleton = WSingletonRegistry::GetSingletonInstance<TestSingleton>();
      W_TEST_BOOL(pSingleton == nullptr);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Singleton of Interface")
  {
    {
      SingletonInterface* pSingleton = WSingletonRegistry::GetSingletonInstance<SingletonInterface>();
      W_TEST_BOOL(pSingleton == nullptr);
    }

    {
      TestSingletonOfInterface g_Singleton;

      {
        SingletonInterface* pSingleton = WSingletonRegistry::GetSingletonInstance<SingletonInterface>();
        W_TEST_BOOL(pSingleton == &g_Singleton);
        W_TEST_INT(pSingleton->GetValue(), 23);
      }

      {
        TestSingletonOfInterface* pSingleton = WSingletonRegistry::GetSingletonInstance<TestSingletonOfInterface>();
        W_TEST_BOOL(pSingleton == &g_Singleton);
        W_TEST_INT(pSingleton->GetValue(), 23);
      }

      {
        SingletonInterface* pSingleton = WSingletonRegistry::GetRequiredSingletonInstance<SingletonInterface>();
        W_TEST_BOOL(pSingleton == &g_Singleton);
        W_TEST_INT(pSingleton->GetValue(), 23);
      }

      {
        TestSingletonOfInterface* pSingleton = WSingletonRegistry::GetRequiredSingletonInstance<TestSingletonOfInterface>();
        W_TEST_BOOL(pSingleton == &g_Singleton);
        W_TEST_INT(pSingleton->GetValue(), 23);
      }
    }

    {
      SingletonInterface* pSingleton = WSingletonRegistry::GetSingletonInstance<SingletonInterface>();
      W_TEST_BOOL(pSingleton == nullptr);
    }

    {
      TestSingletonOfInterface* pSingleton = WSingletonRegistry::GetSingletonInstance<TestSingletonOfInterface>();
      W_TEST_BOOL(pSingleton == nullptr);
    }
  }
}
