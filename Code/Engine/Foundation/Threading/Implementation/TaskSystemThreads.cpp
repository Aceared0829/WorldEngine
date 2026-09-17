#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

WUInt32 WTaskSystem::GetWorkerThreadCount(WWorkerThreadType::Enum type)
{
  return s_pThreadState->m_uiMaxWorkersToUse[type];
}

WUInt32 WTaskSystem::GetNumAllocatedWorkerThreads(WWorkerThreadType::Enum type)
{
  return s_pThreadState->m_iAllocatedWorkers[type];
}

void WTaskSystem::SetWorkerThreadCount(WInt32 iShortTasks, WInt32 iLongTasks)
{
  WSystemInformation info = WSystemInformation::Get();

  // these settings are supposed to be a sensible default for most applications
  // an app can of course change that to optimize for its own usage
  //
  const WInt32 iCpuCores = info.GetCPUCoreCount();

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iShortTasks <= 0)
    iShortTasks = WMath::Clamp<WInt32>(iCpuCores - 2, 2, 8);

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iLongTasks <= 0)
    iLongTasks = WMath::Clamp<WInt32>(iCpuCores - 2, 2, 8);

  // plus there is always one additional 'file access' thread
  // and the main thread, of course

  WUInt32 uiShortTasks = static_cast<WUInt32>(WMath::Max<WInt32>(iShortTasks, 1));
  WUInt32 uiLongTasks = static_cast<WUInt32>(WMath::Max<WInt32>(iLongTasks, 1));

  // if nothing has changed, do nothing
  if (s_pThreadState->m_uiMaxWorkersToUse[WWorkerThreadType::ShortTasks] == uiShortTasks &&
      s_pThreadState->m_uiMaxWorkersToUse[WWorkerThreadType::LongTasks] == uiLongTasks)
    return;

  WLog::Dev("CPU core count: {}", iCpuCores);
  WLog::Dev("Setting worker thread count to {} (short) / {} (long).", uiShortTasks, uiLongTasks);

  StopWorkerThreads();

  // this only allocates pointers, i.e. the maximum possible number of threads that we may be able to realloc at runtime
  s_pThreadState->m_Workers[WWorkerThreadType::ShortTasks].SetCount(1024);
  s_pThreadState->m_Workers[WWorkerThreadType::LongTasks].SetCount(1024);
  s_pThreadState->m_Workers[WWorkerThreadType::FileAccess].SetCount(128);

  s_pThreadState->m_uiMaxWorkersToUse[WWorkerThreadType::ShortTasks] = uiShortTasks;
  s_pThreadState->m_uiMaxWorkersToUse[WWorkerThreadType::LongTasks] = uiLongTasks;
  s_pThreadState->m_uiMaxWorkersToUse[WWorkerThreadType::FileAccess] = 1;

  AllocateThreads(WWorkerThreadType::ShortTasks, s_pThreadState->m_uiMaxWorkersToUse[WWorkerThreadType::ShortTasks]);
  AllocateThreads(WWorkerThreadType::LongTasks, s_pThreadState->m_uiMaxWorkersToUse[WWorkerThreadType::LongTasks]);
  AllocateThreads(WWorkerThreadType::FileAccess, s_pThreadState->m_uiMaxWorkersToUse[WWorkerThreadType::FileAccess]);
}

void WTaskSystem::StopWorkerThreads()
{
  bool bWorkersStillRunning = true;

  // as long as any worker thread is still active, send the wake up signal
  while (bWorkersStillRunning)
  {
    bWorkersStillRunning = false;

    for (WUInt32 type = 0; type < WWorkerThreadType::ENUM_COUNT; ++type)
    {
      const WUInt32 uiNumThreads = s_pThreadState->m_iAllocatedWorkers[type];

      for (WUInt32 i = 0; i < uiNumThreads; ++i)
      {
        if (s_pThreadState->m_Workers[type][i]->DeactivateWorker().Failed())
        {
          bWorkersStillRunning = true;
        }
      }
    }

    // waste some time
    WThreadUtils::YieldTimeSlice();
  }

  for (WUInt32 type = 0; type < WWorkerThreadType::ENUM_COUNT; ++type)
  {
    const WUInt32 uiNumWorkers = s_pThreadState->m_iAllocatedWorkers[type];

    for (WUInt32 i = 0; i < uiNumWorkers; ++i)
    {
      s_pThreadState->m_Workers[type][i]->Join();
      W_DEFAULT_DELETE(s_pThreadState->m_Workers[type][i]);
    }

    s_pThreadState->m_iAllocatedWorkers[type] = 0;
    s_pThreadState->m_uiMaxWorkersToUse[type] = 0;
    s_pThreadState->m_Workers[type].Clear();
  }
}

void WTaskSystem::AllocateThreads(WWorkerThreadType::Enum type, WUInt32 uiAddThreads)
{
  W_ASSERT_DEBUG(uiAddThreads > 0, "Invalid number of threads to allocate");

  {
    // prevent concurrent thread allocation
    W_LOCK(s_TaskSystemMutex);

    WUInt32 uiNextThreadIdx = s_pThreadState->m_iAllocatedWorkers[type];

    W_ASSERT_ALWAYS(uiNextThreadIdx + uiAddThreads <= s_pThreadState->m_Workers[type].GetCount(), "Max number of worker threads ({}) exceeded.",
      s_pThreadState->m_Workers[type].GetCount());

    for (WUInt32 i = 0; i < uiAddThreads; ++i)
    {
      s_pThreadState->m_Workers[type][uiNextThreadIdx] = W_DEFAULT_NEW(WTaskWorkerThread, (WWorkerThreadType::Enum)type, uiNextThreadIdx);
      s_pThreadState->m_Workers[type][uiNextThreadIdx]->Start();

      ++uiNextThreadIdx;
    }

    // let others access the new threads now
    s_pThreadState->m_iAllocatedWorkers[type] = uiNextThreadIdx;
  }

  WLog::Dev("Allocated {} additional '{}' worker threads ({} total)", uiAddThreads, WWorkerThreadType::GetThreadTypeName(type),
    s_pThreadState->m_iAllocatedWorkers[type]);
}

void WTaskSystem::WakeUpThreads(WWorkerThreadType::Enum type, WUInt32 uiNumThreadsToWakeUp)
{
  // together with WTaskWorkerThread::Run() this function will make sure to keep the number
  // of active threads close to m_uiMaxWorkersToUse
  //
  // threads that go into the 'blocked' state will raise the number of threads that get activated
  // and when they are unblocked, together they may exceed the 'maximum' number of active threads
  // but over time the threads at the end of the list will put themselves to sleep again

  auto* s = WTaskSystem::s_pThreadState.Borrow();

  const WUInt32 uiTotalThreads = s_pThreadState->m_iAllocatedWorkers[type];
  WUInt32 uiAllowedActiveThreads = s_pThreadState->m_uiMaxWorkersToUse[type];

  for (WUInt32 threadIdx = 0; threadIdx < uiTotalThreads; ++threadIdx)
  {
    switch (s->m_Workers[type][threadIdx]->WakeUpIfIdle())
    {
      case WTaskWorkerState::Idle:
      {
        // was idle before -> now it is active
        if (--uiNumThreadsToWakeUp == 0)
          return;

        [[fallthrough]];
      }

      case WTaskWorkerState::Active:
      {
        // already active
        if (--uiAllowedActiveThreads == 0)
          return;

        break;
      }

      default:
        break;
    }
  }

  // if the loop above did not find enough threads to wake up
  if (uiNumThreadsToWakeUp > 0 && uiAllowedActiveThreads > 0)
  {
    // the new threads will start not-idle and take on some work
    AllocateThreads(type, WMath::Min(uiNumThreadsToWakeUp, uiAllowedActiveThreads));
  }
}

WWorkerThreadType::Enum WTaskSystem::GetCurrentThreadWorkerType()
{
  return tl_TaskWorkerInfo.m_WorkerType;
}

double WTaskSystem::GetThreadUtilization(WWorkerThreadType::Enum type, WUInt32 uiThreadIndex, WUInt32* pNumTasksExecuted /*= nullptr*/)
{
  return s_pThreadState->m_Workers[type][uiThreadIndex]->GetThreadUtilization(pNumTasksExecuted);
}

void WTaskSystem::DetermineTasksToExecuteOnThread(WTaskPriority::Enum& out_FirstPriority, WTaskPriority::Enum& out_LastPriority)
{
  switch (tl_TaskWorkerInfo.m_WorkerType)
  {
    case WWorkerThreadType::MainThread:
    {
      out_FirstPriority = WTaskPriority::ThisFrameMainThread;
      out_LastPriority = WTaskPriority::SomeFrameMainThread;
      break;
    }

    case WWorkerThreadType::FileAccess:
    {
      out_FirstPriority = WTaskPriority::FileAccessHighPriority;
      out_LastPriority = WTaskPriority::FileAccess;
      break;
    }

    case WWorkerThreadType::LongTasks:
    {
      out_FirstPriority = WTaskPriority::LongRunningHighPriority;
      out_LastPriority = WTaskPriority::LongRunning;
      break;
    }

    case WWorkerThreadType::ShortTasks:
    {
      out_FirstPriority = WTaskPriority::EarlyThisFrame;
      out_LastPriority = WTaskPriority::In9Frames;
      break;
    }

    case WWorkerThreadType::Unknown:
    {
      // probably a thread not launched through W
      out_FirstPriority = WTaskPriority::EarlyThisFrame;
      out_LastPriority = WTaskPriority::In9Frames;
      break;
    }

    default:
    {
      W_ASSERT_NOT_IMPLEMENTED;
      break;
    }
  }
}
