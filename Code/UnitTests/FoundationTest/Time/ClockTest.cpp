#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Clock.h>

class WSimpleTimeStepSmoother : public WTimeStepSmoothing
{
public:
  virtual WTime GetSmoothedTimeStep(WTime rawTimeStep, const WClock* pClock) override { return WTime::MakeFromSeconds(0.42); }

  virtual void Reset(const WClock* pClock) override {}
};

W_CREATE_SIMPLE_TEST(Time, Clock)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor / Reset")
  {
    WClock c("Test");                                 // calls 'Reset' internally

    W_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr); // after constructor

    W_TEST_DOUBLE(c.GetAccumulatedTime().GetSeconds(), 0.0, 0.0);
    W_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 0.0, 0.0);
    W_TEST_DOUBLE(c.GetSpeed(), 1.0, 0.0);
    W_TEST_BOOL(c.GetPaused() == false);
    W_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.001, 0.0); // to ensure the tests fail if somebody changes these constants
    W_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.1, 0.0);   // to ensure the tests fail if somebody changes these constants
    W_TEST_BOOL(c.GetTimeDiff() > WTime::MakeFromSeconds(0.0));

    WSimpleTimeStepSmoother s;

    c.SetTimeStepSmoothing(&s);

    W_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.Reset(false);

    // does NOT reset which time step smoother to use
    W_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.Reset(true);
    W_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr); // after constructor
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetPaused / GetPaused")
  {
    WClock c("Test");
    W_TEST_BOOL(!c.GetPaused());

    c.SetPaused(true);
    W_TEST_BOOL(c.GetPaused());

    c.SetPaused(false);
    W_TEST_BOOL(!c.GetPaused());

    c.SetPaused(true);
    W_TEST_BOOL(c.GetPaused());

    c.Reset(false);
    W_TEST_BOOL(!c.GetPaused());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Updates while Paused / Unpaused")
  {
    WClock c("Test");

    c.SetPaused(false);

    const WTime t0 = c.GetAccumulatedTime();

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    c.Update();

    const WTime t1 = c.GetAccumulatedTime();
    W_TEST_BOOL(t0 < t1);

    c.SetPaused(true);

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    c.Update();

    const WTime t2 = c.GetAccumulatedTime();
    W_TEST_BOOL(t1 == t2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFixedTimeStep / GetFixedTimeStep")
  {
    WClock c("Test");

    W_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 0.0, 0.0);

    c.SetFixedTimeStep(WTime::MakeFromSeconds(1.0 / 60.0));

    W_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 1.0 / 60.0, 0.000001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Updates with fixed time step")
  {
    WClock c("Test");
    c.SetFixedTimeStep(WTime::MakeFromSeconds(1.0 / 60.0));
    c.Update();

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

    c.Update();
    W_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(50));

    c.Update();
    W_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);

    c.Update();
    W_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetAccumulatedTime / GetAccumulatedTime")
  {
    WClock c("Test");

    c.SetAccumulatedTime(WTime::MakeFromSeconds(23.42));

    W_TEST_DOUBLE(c.GetAccumulatedTime().GetSeconds(), 23.42, 0.000001);

    c.Update(); // by default after a SetAccumulatedTime the time diff should always be > 0

    W_TEST_BOOL(c.GetTimeDiff().GetSeconds() > 0.0);

    const WTime t0 = c.GetAccumulatedTime();

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(5));
    c.Update();

    const WTime t1 = c.GetAccumulatedTime();

    W_TEST_BOOL(t1 > t0);
    W_TEST_BOOL(c.GetTimeDiff().GetSeconds() > 0.0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetSpeed / GetSpeed / GetTimeDiff")
  {
    WClock c("Test");
    W_TEST_DOUBLE(c.GetSpeed(), 1.0, 0.0);

    c.SetFixedTimeStep(WTime::MakeFromSeconds(0.01));

    c.SetSpeed(10.0);
    W_TEST_DOUBLE(c.GetSpeed(), 10.0, 0.000001);

    c.Update();
    const WTime t0 = c.GetTimeDiff();
    W_TEST_DOUBLE(t0.GetSeconds(), 0.1, 0.00001);

    c.SetSpeed(0.1);

    c.Update();
    const WTime t1 = c.GetTimeDiff();
    W_TEST_DOUBLE(t1.GetSeconds(), 0.001, 0.00001);

    c.Reset(false);

    c.Update();
    const WTime t2 = c.GetTimeDiff();
    W_TEST_DOUBLE(t2.GetSeconds(), 0.01, 0.00001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetMinimumTimeStep / GetMinimumTimeStep")
  {
    WClock c("Test");
    W_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.001, 0.0); // to ensure the tests fail if somebody changes these constants

    double smallestValue = WMath::MaxValue<double>();
    for (WUInt32 i = 0; i < 10; i++)
    {
      c.Update();
      smallestValue = WMath::Min(smallestValue, c.GetTimeDiff().GetSeconds());
      if (WMath::IsEqual(smallestValue, c.GetMinimumTimeStep().GetSeconds(), 0.0000000001))
        break;
    }
    W_TEST_DOUBLE_MSG(smallestValue, c.GetMinimumTimeStep().GetSeconds(), 0.0000000001, "After 10 itterations, c.GetTimeDiff() did not reach the minimum step");

    c.SetMinimumTimeStep(WTime::MakeFromSeconds(0.1));
    c.SetMaximumTimeStep(WTime::MakeFromSeconds(1.0));

    W_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.1, 0.0);

    c.Update();
    c.Update();

    W_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMinimumTimeStep().GetSeconds(), 0.0000000001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetMaximumTimeStep / GetMaximumTimeStep")
  {
    WClock c("Test");
    W_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.1, 0.0); // to ensure the tests fail if somebody changes these constants

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(200));
    c.Update();

    W_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMaximumTimeStep().GetSeconds(), 0.0000000001);

    c.SetMaximumTimeStep(WTime::MakeFromSeconds(0.2));

    W_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.2, 0.0);

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(400));
    c.Update();

    W_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMaximumTimeStep().GetSeconds(), 0.0000000001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetTimeStepSmoothing / GetTimeStepSmoothing")
  {
    WClock c("Test");

    W_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr);

    WSimpleTimeStepSmoother s;
    c.SetTimeStepSmoothing(&s);

    W_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.SetMaximumTimeStep(WTime::MakeFromSeconds(10.0)); // this would limit the time step even after smoothing
    c.Update();

    W_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 0.42, 0.0);
  }
}
