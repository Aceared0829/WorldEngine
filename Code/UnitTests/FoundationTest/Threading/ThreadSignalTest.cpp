#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  class TestThread2 : public WThread
  {
  public:
    TestThread2()
      : WThread("Test Thread")
    {
    }

    WThreadSignal* m_pSignalAuto = nullptr;
    WThreadSignal* m_pSignalManual = nullptr;
    WAtomicInteger32* m_pCounter = nullptr;
    bool m_bTimeout = false;

    virtual WUInt32 Run()
    {
      m_pCounter->Decrement();

      m_pSignalAuto->WaitForSignal();

      m_pCounter->Increment();

      if (m_bTimeout)
      {
        m_pSignalManual->WaitForSignal(WTime::MakeFromSeconds(0.5));
      }
      else
      {
        m_pSignalManual->WaitForSignal();
      }

      m_pCounter->Increment();

      return 0;
    }
  };
} // namespace

W_CREATE_SIMPLE_TEST(Threading, ThreadSignal)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Wait No Timeout")
  {
    constexpr WUInt32 uiNumThreads = 32;

    WUniquePtr<TestThread2> pTestThread2s[uiNumThreads];
    WAtomicInteger32 iCounter = uiNumThreads;
    WThreadSignal sigAuto(WThreadSignal::Mode::AutoReset);
    WThreadSignal sigManual(WThreadSignal::Mode::ManualReset);

    for (WUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i] = W_DEFAULT_NEW(TestThread2);
      pTestThread2s[i]->m_pCounter = &iCounter;
      pTestThread2s[i]->m_pSignalAuto = &sigAuto;
      pTestThread2s[i]->m_pSignalManual = &sigManual;
      pTestThread2s[i]->Start();
    }

    // wait until all threads are in waiting state
    while (iCounter > 0)
    {
      WThreadUtils::YieldTimeSlice();
    }

    for (WUInt32 t = 0; t < uiNumThreads; ++t)
    {
      const WInt32 iExpected = t + 1;

      sigAuto.RaiseSignal();

      for (WUInt32 a = 0; a < 1000; ++a)
      {
        WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));

        if (iCounter >= iExpected)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      W_TEST_INT(iCounter, iExpected);
      W_TEST_BOOL(iCounter <= iExpected); // THIS test must never fail!
    }

    // wake up the rest
    {
      sigManual.RaiseSignal();

      for (WUInt32 a = 0; a < 1000; ++a)
      {
        WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));

        if (iCounter >= (WInt32)uiNumThreads * 2)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      W_TEST_INT(iCounter, (WInt32)uiNumThreads * 2);
      W_TEST_BOOL(iCounter <= (WInt32)uiNumThreads * 2); // THIS test must never fail!
    }

    for (WUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i]->Join();
    }
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "Wait With Timeout")
  {
    constexpr WUInt32 uiNumThreads = 16;

    WUniquePtr<TestThread2> pTestThread2s[uiNumThreads];
    WAtomicInteger32 iCounter = uiNumThreads;
    WThreadSignal sigAuto(WThreadSignal::Mode::AutoReset);
    WThreadSignal sigManual(WThreadSignal::Mode::ManualReset);

    for (WUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i] = W_DEFAULT_NEW(TestThread2);
      pTestThread2s[i]->m_pCounter = &iCounter;
      pTestThread2s[i]->m_pSignalAuto = &sigAuto;
      pTestThread2s[i]->m_pSignalManual = &sigManual;
      pTestThread2s[i]->m_bTimeout = true;
      pTestThread2s[i]->Start();
    }

    // wait until all threads are in waiting state
    while (iCounter > 0)
    {
      WThreadUtils::YieldTimeSlice();
    }

    // raise the signal N times
    for (WUInt32 t = 0; t < uiNumThreads; ++t)
    {
      sigAuto.RaiseSignal();

      for (WUInt32 a = 0; a < 1000; ++a)
      {
        WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));

        if (iCounter >= (WInt32)t + 1)
          break;
      }
    }

    // due to the wait timeout in the thread, testing this exact value here would be unreliable
    // W_TEST_INT(iCounter, (WInt32)uiNumThreads);

    // just wait for the rest
    {
      for (WUInt32 a = 0; a < 100; ++a)
      {
        WThreadUtils::Sleep(WTime::MakeFromMilliseconds(50));

        if (iCounter >= (WInt32)uiNumThreads * 2)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      W_TEST_INT(iCounter, (WInt32)uiNumThreads * 2);
      W_TEST_BOOL(iCounter <= (WInt32)uiNumThreads * 2); // THIS test must never fail!
    }

    for (WUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i]->Join();
    }
  }
}
