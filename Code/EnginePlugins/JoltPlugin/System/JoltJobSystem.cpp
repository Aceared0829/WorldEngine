#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/Types/SharedPtr.h>
#include <JoltPlugin/System/JoltJobSystem.h>

WJoltJobSystem::WJoltJobSystem(WUInt32 uiMaxJobs, WUInt32 uiMaxBarriers)
{
  JobSystemWithBarrier::Init(uiMaxBarriers);

  m_Jobs.Init(uiMaxJobs, uiMaxJobs);

  m_Tasks.SetCount(uiMaxJobs);
  for (WUInt32 i = 0; i < m_Tasks.GetCount(); ++i)
  {
    m_Tasks[i] = W_DEFAULT_NEW(WJoltTask);
    m_Tasks[i]->ConfigureTask("Jolt", WTaskNesting::Never, &WJoltJobSystem::OnTaskFinished);
  }
}

int WJoltJobSystem::GetMaxConcurrency() const
{
  return WTaskSystem::GetWorkerThreadCount(WWorkerThreadType::ShortTasks);
}

JPH::JobHandle WJoltJobSystem::CreateJob(const char* szName, JPH::ColorArg color, const JobFunction& jobFunction, WUInt32 uiNumDependencies)
{
  // Loop until we can get a job from the free list
  WUInt32 index;
  for (;;)
  {
    index = m_Jobs.ConstructObject(szName, color, this, jobFunction, uiNumDependencies);
    if (index != AvailableJobs::cInvalidObjectIndex)
      break;

    W_ASSERT_DEBUG(false, "No Jolt jobs available!");
    WThreadUtils::YieldTimeSlice();
  }

  CustomJob* job = &m_Jobs.Get(index);
  job->m_uiJobIndex = index;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  {
    WStringBuilder name("Jolt-", szName);
    m_Tasks[index]->ConfigureTask(name, WTaskNesting::Never, &WJoltJobSystem::OnTaskFinished);
  }
#endif

  // Construct handle to keep a reference, the job is queued below and may immediately complete
  JobHandle handle(job);

  // If there are no dependencies, queue the job now
  if (uiNumDependencies == 0)
    QueueJob(job);

  // Return the handle
  return handle;
}

void WJoltJobSystem::FreeJob(Job* pJob)
{
  m_Jobs.DestructObject(static_cast<CustomJob*>(pJob));
}

void WJoltJobSystem::OnTaskFinished(const WSharedPtr<WTask>& task)
{
  WJoltTask* pTask = static_cast<WJoltTask*>(task.Borrow());

  auto* pJob = static_cast<JPH::JobSystem::Job*>(pTask->m_pJob);
  pTask->m_pJob = nullptr;

  // doing this here prevens a race condition in reusing tasks
  pJob->Release();
}

void WJoltJobSystem::QueueJob(Job* pJob)
{
  auto* pMyJob = static_cast<CustomJob*>(pJob);
  pMyJob->AddRef();

  const auto& pTask = m_Tasks[pMyJob->m_uiJobIndex];

  pTask->m_pJob = pMyJob;

  WTaskSystem::StartSingleTask(pTask, WTaskPriority::EarlyThisFrame);
}

void WJoltJobSystem::QueueJobs(Job** pJob, WUInt32 uiNum_Jobs)
{
  for (WUInt32 i = 0; i < uiNum_Jobs; ++i)
  {
    QueueJob(pJob[i]);
  }
}

void WJoltJobSystem::WJoltTask::Execute()
{
  m_pJob->Execute();
}
