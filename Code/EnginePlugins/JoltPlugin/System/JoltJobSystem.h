#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <Jolt/Jolt.h>

#include <Jolt/Core/FixedSizeFreeList.h>
#include <Jolt/Core/JobSystemWithBarrier.h>

class WJoltJobSystem final : public JPH::JobSystemWithBarrier
{
public:
  WJoltJobSystem(WUInt32 uiMaxJobs, WUInt32 uiMaxBarriers);

  virtual int GetMaxConcurrency() const override;
  virtual JPH::JobHandle CreateJob(const char* szName, JPH::ColorArg color, const JobFunction& jobFunction, WUInt32 uiNumDependencies = 0) override;

  virtual void QueueJob(Job* pJob) override;
  virtual void QueueJobs(Job** pJobs, WUInt32 uiNumJobs) override;
  virtual void FreeJob(Job* pJob) override;

private:
  static void OnTaskFinished(const WSharedPtr<WTask>& task);

  class CustomJob : public JPH::JobSystem::Job
  {
  public:
    CustomJob(const char* szJobName, JPH::ColorArg color, JPH::JobSystem* pJobSystem, const JobFunction& jobFunction, WUInt32 uiNumDependencies)
      : Job(szJobName, color, pJobSystem, jobFunction, uiNumDependencies)
    {
    }

    WUInt32 m_uiJobIndex = WInvalidIndex;
  };

  class WJoltTask : public WTask
  {
  public:
    CustomJob* m_pJob = nullptr;

    virtual void Execute() override;
  };


  using AvailableJobs = JPH::FixedSizeFreeList<CustomJob>;
  AvailableJobs m_Jobs;

  WDynamicArray<WSharedPtr<WJoltTask>> m_Tasks;
};
