#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Time/Time.h>

W_CREATE_SIMPLE_TEST_GROUP(Time);

W_CREATE_SIMPLE_TEST(Time, Timer)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Basics")
  {
    WTime TestTime = WTime::Now();

    W_TEST_BOOL(TestTime.GetMicroseconds() > 0.0);

    volatile WUInt32 testValue = 0;
    for (WUInt32 i = 0; i < 42000; ++i)
    {
      testValue += 23;
    }

    WTime TestTime2 = WTime::Now();

    W_TEST_BOOL(TestTime2.GetMicroseconds() > 0.0);

    TestTime2 -= TestTime;

    W_TEST_BOOL(TestTime2.GetMicroseconds() > 0.0);
  }
}
