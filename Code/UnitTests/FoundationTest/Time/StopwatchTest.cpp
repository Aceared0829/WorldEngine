#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Stopwatch.h>

W_CREATE_SIMPLE_TEST(Time, Stopwatch)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "General Functionality")
  {
    WStopwatch sw;

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(50));

    sw.StopAndReset();
    sw.Resume();

    const WTime t0 = sw.Checkpoint();

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

    const WTime t1 = sw.Checkpoint();

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(20));

    const WTime t2 = sw.Checkpoint();

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(30));

    const WTime t3 = sw.Checkpoint();

    const WTime tTotal1 = sw.GetRunningTotal();

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

    sw.Pause(); // freeze the current running total

    const WTime tTotal2 = sw.GetRunningTotal();

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10)); // should not affect the running total anymore

    const WTime tTotal3 = sw.GetRunningTotal();


    // these tests are deliberately written such that they cannot fail,
    // even when the OS is under heavy load

    W_TEST_BOOL(t0 > WTime::MakeFromMilliseconds(5));
    W_TEST_BOOL(t1 > WTime::MakeFromMilliseconds(5));
    W_TEST_BOOL(t2 > WTime::MakeFromMilliseconds(5));
    W_TEST_BOOL(t3 > WTime::MakeFromMilliseconds(5));


    W_TEST_BOOL(t1 + t2 + t3 <= tTotal1);
    W_TEST_BOOL(t0 + t1 + t2 + t3 > tTotal1);

    W_TEST_BOOL(tTotal1 < tTotal2);
    W_TEST_BOOL(tTotal1 < tTotal3);
    W_TEST_BOOL(tTotal2 == tTotal3);
  }
}
