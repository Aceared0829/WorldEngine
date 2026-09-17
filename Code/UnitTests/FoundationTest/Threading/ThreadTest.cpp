#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Time/Time.h>

namespace
{
  WInt32 g_iCrossThreadVariable = 0;
  const WUInt32 g_uiIncrementSteps = 160000;

  class TestThread3 : public WThread
  {
  public:
    TestThread3()
      : WThread("Test Thread")
    {
    }

    WMutex* m_pWaitMutex = nullptr;
    WMutex* m_pBlockedMutex = nullptr;

    virtual WUInt32 Run()
    {
      // test TryLock on a locked mutex
      W_TEST_BOOL(m_pBlockedMutex->TryLock().Failed());

      {
        // enter and leave the mutex once
        W_LOCK(*m_pWaitMutex);
      }

      W_PROFILE_SCOPE("Test Thread::Run");

      for (WUInt32 i = 0; i < g_uiIncrementSteps; i++)
      {
        WAtomicUtils::Increment(g_iCrossThreadVariable);

        WTime::Now();
        WThreadUtils::YieldTimeSlice();
        WTime::Now();
      }

      return 0;
    }
  };
} // namespace

W_CREATE_SIMPLE_TEST_GROUP(Threading);

W_CREATE_SIMPLE_TEST(Threading, Thread)
{
  g_iCrossThreadVariable = 0;


  W_TEST_BLOCK(WTestBlock::Enabled, "Thread")
  {
    TestThread3* pTestThread31 = nullptr;
    TestThread3* pTestThread32 = nullptr;

    /// the try-catch is necessary to quiet the static code analysis
    try
    {
      pTestThread31 = new TestThread3;
      pTestThread32 = new TestThread3;
    }
    catch (...)
    {
    }

    W_TEST_BOOL(pTestThread31 != nullptr);
    W_TEST_BOOL(pTestThread32 != nullptr);

    WMutex waitMutex, blockedMutex;
    pTestThread31->m_pWaitMutex = &waitMutex;
    pTestThread32->m_pWaitMutex = &waitMutex;

    pTestThread31->m_pBlockedMutex = &blockedMutex;
    pTestThread32->m_pBlockedMutex = &blockedMutex;

    // no one holds these mutexes yet, must succeed
    W_TEST_BOOL(blockedMutex.TryLock().Succeeded());
    W_TEST_BOOL(waitMutex.TryLock().Succeeded());

    // Both thread will increment the global variable via atomic operations
    pTestThread31->Start();
    pTestThread32->Start();

    // give the threads a bit of time to start
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(50));

    // allow the threads to run now
    waitMutex.Unlock();

    // Main thread will also increment the test variable
    WAtomicUtils::Increment(g_iCrossThreadVariable);

    // Join with both threads
    pTestThread31->Join();
    pTestThread32->Join();

    // we are holding the mutex, another TryLock should work
    W_TEST_BOOL(blockedMutex.TryLock().Succeeded());

    // The threads should have finished, no one holds the lock
    W_TEST_BOOL(waitMutex.TryLock().Succeeded());

    // Test deletion
    delete pTestThread31;
    delete pTestThread32;

    W_TEST_INT(g_iCrossThreadVariable, g_uiIncrementSteps * 2 + 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Thread Sleeping")
  {
    const WTime start = WTime::Now();

    WTime sleepTime(WTime::MakeFromSeconds(0.3));

    WThreadUtils::Sleep(sleepTime);

    const WTime stop = WTime::Now();

    const WTime duration = stop - start;

    // We test for 0.25 - 0.35 since the threading functions are a bit varying in their precision
    W_TEST_BOOL(duration.GetSeconds() > 0.25);
    W_TEST_BOOL_MSG(duration.GetSeconds() < 1.0, "This test can fail when the machine is under too much load and blocks the process for too long.");
  }
}
