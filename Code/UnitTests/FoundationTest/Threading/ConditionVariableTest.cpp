#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ConditionVariable.h>

#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  class TestThread : public WThread
  {
  public:
    TestThread()
      : WThread("Test Thread")
    {
    }

    WConditionVariable* m_pCV = nullptr;
    WAtomicInteger32* m_pCounter = nullptr;

    virtual WUInt32 Run()
    {
      W_LOCK(*m_pCV);

      m_pCounter->Decrement();

      m_pCV->UnlockWaitForSignalAndLock();

      m_pCounter->Increment();
      return 0;
    }
  };

  class TestThreadTimeout : public WThread
  {
  public:
    TestThreadTimeout()
      : WThread("Test Thread Timeout")
    {
    }

    WConditionVariable* m_pCV = nullptr;
    WConditionVariable* m_pCVTimeout = nullptr;
    WAtomicInteger32* m_pCounter = nullptr;

    virtual WUInt32 Run()
    {
      // make sure all threads are put to sleep first
      {
        W_LOCK(*m_pCV);
        m_pCounter->Decrement();
        m_pCV->UnlockWaitForSignalAndLock();
      }

      // this condition will never be met during the test
      // it should always run into the timeout
      W_LOCK(*m_pCVTimeout);
      m_pCVTimeout->UnlockWaitForSignalAndLock(WTime::MakeFromSeconds(0.5));

      m_pCounter->Increment();
      return 0;
    }
  };
} // namespace

W_CREATE_SIMPLE_TEST(Threading, ConditionalVariable)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Wait No Timeout")
  {
    constexpr WUInt32 uiNumThreads = 32;

    WUniquePtr<TestThread> pTestThreads[uiNumThreads];
    WAtomicInteger32 iCounter = uiNumThreads;
    WConditionVariable cv;

    for (WUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i] = W_DEFAULT_NEW(TestThread);
      pTestThreads[i]->m_pCounter = &iCounter;
      pTestThreads[i]->m_pCV = &cv;
      pTestThreads[i]->Start();
    }

    // wait until all threads are in waiting state
    while (true)
    {
      // We need to lock here as otherwise we could signal
      // while a thread hasn't reached the wait yet.
      W_LOCK(cv);
      if (iCounter == 0)
        break;

      WThreadUtils::YieldTimeSlice();
    }

    for (WUInt32 t = 0; t < uiNumThreads / 2; ++t)
    {
      const WInt32 iExpected = iCounter + 1;

      cv.SignalOne();

      for (WUInt32 a = 0; a < 1000; ++a)
      {
        WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));

        if (iCounter >= iExpected)
          break;
      }

      // Theoretically this could fail, if the OS doesn't wake up any other thread in time but with 1000 tries that is very unlikely.
      // On some platforms like posix it is not guaranteed that exactly one thread is woken up, so we check that at least one thread was woken up.
      W_TEST_BOOL(iCounter >= iExpected);
    }

    // wake up the rest
    {
      cv.SignalAll();

      for (WUInt32 a = 0; a < 1000; ++a)
      {
        WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));

        if (iCounter >= (WInt32)uiNumThreads)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      W_TEST_INT(iCounter, (WInt32)uiNumThreads);
      W_TEST_BOOL(iCounter <= (WInt32)uiNumThreads); // THIS test must never fail!
    }

    for (WUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i]->Join();
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Wait With timeout")
  {
    constexpr WUInt32 uiNumThreads = 16;

    WUniquePtr<TestThreadTimeout> pTestThreads[uiNumThreads];
    WAtomicInteger32 iCounter = uiNumThreads;
    WConditionVariable cv;
    WConditionVariable cvt;

    for (WUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i] = W_DEFAULT_NEW(TestThreadTimeout);
      pTestThreads[i]->m_pCounter = &iCounter;
      pTestThreads[i]->m_pCV = &cv;
      pTestThreads[i]->m_pCVTimeout = &cvt;
      pTestThreads[i]->Start();
    }

    // wait until all threads are in waiting state
    while (true)
    {
      // We need to lock here as otherwise we could signal
      // while a thread hasn't reached the wait yet.
      W_LOCK(cv);
      if (iCounter == 0)
        break;

      WThreadUtils::YieldTimeSlice();
    }

    // open the flood gates
    cv.SignalAll();

    // all threads should run into their timeout now
    for (WUInt32 a = 0; a < 100; ++a)
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(50));

      if (iCounter >= (WInt32)uiNumThreads)
        break;
    }

    // theoretically this could fail, if the OS doesn't wake up any other thread in time
    // but with 100 tries that is very unlikely
    W_TEST_INT(iCounter, (WInt32)uiNumThreads);
    W_TEST_BOOL(iCounter <= (WInt32)uiNumThreads); // THIS test must never fail!

    for (WUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i]->Join();
    }
  }
}
