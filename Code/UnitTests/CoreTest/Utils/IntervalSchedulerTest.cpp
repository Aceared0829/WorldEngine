#include <CoreTest/CoreTestPCH.h>

#include <Core/Utils/IntervalScheduler.h>

W_CREATE_SIMPLE_TEST_GROUP(Utils);

namespace
{
  struct TestWork
  {
    float m_IntervalMs = 0.0f;
    WUInt32 m_Counter = 0;

    void Run()
    {
      ++m_Counter;
    }
  };
} // namespace

W_CREATE_SIMPLE_TEST(Utils, IntervalScheduler)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constant workload")
  {
    float intervals[] = {10, 20, 60, 60, 60};

    WTempHybridArray<TestWork, 32> works;
    WIntervalScheduler<TestWork*> scheduler;

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(intervals); ++i)
    {
      auto& work = works.ExpandAndGetRef();
      work.m_IntervalMs = intervals[i];

      scheduler.AddOrUpdateWork(&work, WTime::MakeFromMilliseconds(work.m_IntervalMs));
    }

    constexpr WUInt32 uiNumIterations = 60;
    constexpr WTime timeStep = WTime::MakeFromMilliseconds(10);

    WUInt32 wrongDelta = 0;
    for (WUInt32 i = 0; i < uiNumIterations; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(timeStep, [&](TestWork* pWork, WTime deltaTime)
        {
        if (i > 10)
        {
          const double deltaMs = deltaTime.GetMilliseconds();
          const double variance = pWork->m_IntervalMs * 0.3;
          const double midValue = pWork->m_IntervalMs + 1.0 - variance;
          if (WMath::IsEqual<double>(deltaMs, midValue, variance) == false)
          {
            ++wrongDelta;
          }
        }

        pWork->Run();
        ++fNumWorks; });

      W_TEST_FLOAT(fNumWorks, 2.5f, 0.5f);

      for (auto& work : works)
      {
        W_TEST_BOOL(scheduler.GetInterval(&work) == WTime::MakeFromMilliseconds(work.m_IntervalMs));
      }
    }

    // 3 wrong deltas for ~120 scheduled works is ok
    W_TEST_BOOL(wrongDelta <= 3);

    for (auto& work : works)
    {
      const float expectedCounter = static_cast<float>(uiNumIterations * timeStep.GetMilliseconds()) / WMath::Max(work.m_IntervalMs, 10.0f);

      // check for roughly expected or a little bit more
      W_TEST_FLOAT(static_cast<float>(work.m_Counter), expectedCounter + 3.0f, 4.0f);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constant workload (bigger delta)")
  {
    float intervals[] = {10, 20, 60, 60, 60};

    WTempHybridArray<TestWork, 32> works;
    WIntervalScheduler<TestWork*> scheduler;

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(intervals); ++i)
    {
      auto& work = works.ExpandAndGetRef();
      work.m_IntervalMs = intervals[i];

      scheduler.AddOrUpdateWork(&work, WTime::MakeFromMilliseconds(work.m_IntervalMs));
    }

    constexpr WUInt32 uiNumIterations = 60;
    constexpr WTime timeStep = WTime::MakeFromMilliseconds(20);

    WUInt32 wrongDelta = 0;
    for (WUInt32 i = 0; i < uiNumIterations; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(timeStep, [&](TestWork* pWork, WTime deltaTime)
        {
        if (i > 10)
        {
          const double deltaMs = deltaTime.GetMilliseconds();
          const double variance = WMath::Max(pWork->m_IntervalMs, 20.0f) * 0.3;
          const double midValue = WMath::Max(pWork->m_IntervalMs, 20.0f) + 1.0 - variance;
          if (WMath::IsEqual<double>(deltaMs, midValue, variance) == false)
          {
            ++wrongDelta;
          }
        }

        pWork->Run();
        ++fNumWorks; });

      W_TEST_FLOAT(fNumWorks, 3.5f, 0.5f);

      for (auto& work : works)
      {
        W_TEST_BOOL(scheduler.GetInterval(&work) == WTime::MakeFromMilliseconds(work.m_IntervalMs));
      }
    }

    // 3 wrong deltas for ~150 scheduled works is ok
    W_TEST_BOOL(wrongDelta <= 3);

    for (auto& work : works)
    {
      const float expectedCounter = static_cast<float>(uiNumIterations * timeStep.GetMilliseconds()) / WMath::Max(work.m_IntervalMs, 20.0f);

      // check for roughly expected or a little bit more
      W_TEST_FLOAT(static_cast<float>(work.m_Counter), expectedCounter + 2.0f, 3.0f);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Dynamic workload")
  {
    WTempHybridArray<TestWork, 32> works;

    WIntervalScheduler<TestWork*> scheduler;

    for (WUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works.ExpandAndGetRef();
      scheduler.AddOrUpdateWork(&work, WTime::MakeFromMilliseconds(i));
    }

    for (WUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(WTime::MakeFromMilliseconds(10), [&](TestWork* pWork, WTime deltaTime)
        {
        pWork->Run();
        ++fNumWorks; });

      W_TEST_FLOAT(fNumWorks, 15.5f, 0.5f);
    }

    for (WUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works.ExpandAndGetRef();
      scheduler.AddOrUpdateWork(&work, WTime::MakeFromMilliseconds(20 + i));
    }

    float fPrevNumWorks = 15.5f;
    for (WUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0.0f;
      scheduler.Update(WTime::MakeFromMilliseconds(10), [&](TestWork* pWork, WTime deltaTime)
        {
        pWork->Run();
        ++fNumWorks; });

      // fNumWork will slowly ramp up until it reaches the new workload of 22 or 23 per update
      W_TEST_BOOL(fNumWorks + 1.0f >= fPrevNumWorks);
      W_TEST_BOOL(fNumWorks <= 23.0f);

      fPrevNumWorks = fNumWorks;
    }

    for (WUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i];
      scheduler.RemoveWork(&work);
    }

    scheduler.Update(WTime::MakeFromMilliseconds(10), WIntervalScheduler<TestWork*>::RunWorkCallback());

    for (WUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i + 16];
      W_TEST_BOOL(scheduler.GetInterval(&work) == WTime::MakeFromMilliseconds(20 + i));

      scheduler.AddOrUpdateWork(&work, WTime::MakeFromMilliseconds(100 + i));
    }

    scheduler.Update(WTime::MakeFromMilliseconds(10), WIntervalScheduler<TestWork*>::RunWorkCallback());

    for (WUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i + 16];
      W_TEST_BOOL(scheduler.GetInterval(&work) == WTime::MakeFromMilliseconds(100 + i));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Update/Remove during schedule")
  {
    WTempHybridArray<TestWork, 32> works;

    WIntervalScheduler<TestWork*> scheduler;

    for (WUInt32 i = 0; i < 32; ++i)
    {
      auto& work = works.ExpandAndGetRef();
      work.m_IntervalMs = static_cast<float>((i & 1u));

      scheduler.AddOrUpdateWork(&work, WTime::MakeFromMilliseconds(i));
    }

    WUInt32 uiNumWorks = 0;
    scheduler.Update(WTime::MakeFromMilliseconds(33),
      [&](TestWork* pWork, WTime deltaTime)
      {
        pWork->Run();
        ++uiNumWorks;

        if (pWork->m_IntervalMs == 0.0f)
        {
          scheduler.RemoveWork(pWork);
        }
        else
        {
          scheduler.AddOrUpdateWork(pWork, WTime::MakeFromMilliseconds(50));
        }
      });

    W_TEST_INT(uiNumWorks, 32);
    for (WUInt32 i = 0; i < 32; ++i)
    {
      const WUInt32 uiExpectedCounter = 1;
      W_TEST_INT(works[i].m_Counter, uiExpectedCounter);
    }

    uiNumWorks = 0;
    scheduler.Update(WTime::MakeFromMilliseconds(100),
      [&](TestWork* pWork, WTime deltaTime)
      {
        W_TEST_FLOAT(pWork->m_IntervalMs, 1.0f, WMath::DefaultEpsilon<float>());

        pWork->Run();
        ++uiNumWorks;
      });

    W_TEST_INT(uiNumWorks, 16);
    for (WUInt32 i = 0; i < 32; ++i)
    {
      const WUInt32 uiExpectedCounter = 1 + (i & 1);
      W_TEST_INT(works[i].m_Counter, uiExpectedCounter);
    }
  }
}
