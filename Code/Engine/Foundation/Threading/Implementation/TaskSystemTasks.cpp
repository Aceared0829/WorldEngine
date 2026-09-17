#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>

WTaskGroupID WTaskSystem::StartSingleTask(const WSharedPtr<WTask>& pTask, WTaskPriority::Enum priority, WTaskGroupID dependency,
  WOnTaskGroupFinishedCallback callback /*= WOnTaskGroupFinishedCallback()*/)
{
  WTaskGroupID Group = CreateTaskGroup(priority, callback);
  AddTaskGroupDependency(Group, dependency);
  AddTaskToGroup(Group, pTask);
  StartTaskGroup(Group);
  return Group;
}

WTaskGroupID WTaskSystem::StartSingleTask(
  const WSharedPtr<WTask>& pTask, WTaskPriority::Enum priority, WOnTaskGroupFinishedCallback callback /*= WOnTaskGroupFinishedCallback()*/)
{
  WTaskGroupID Group = CreateTaskGroup(priority, callback);
  AddTaskToGroup(Group, pTask);
  StartTaskGroup(Group);
  return Group;
}

void WTaskSystem::TaskHasFinished(WSharedPtr<WTask>&& pTask, WTaskGroup* pGroup)
{
  // call task finished callback and deallocate the task (if last reference)
  if (pTask && pTask->m_iRemainingRuns == 0)
  {
    if (pTask->m_OnTaskFinished.IsValid())
    {
      pTask->m_OnTaskFinished(pTask);
    }

    // make sure to clear the task sharedptr BEFORE we mark the task (group) as finished,
    // so that if this is the last reference, the task gets deallocated first
    pTask.Clear();
  }

  if (pGroup->m_iNumRemainingTasks.Decrement() == 0)
  {
    // If this was the last task that had to be finished from this group, make sure all dependent groups are started

    WUInt32 groupCounter = 0;
    {
      // see WTaskGroup::WaitForFinish() for why we need this lock here
      // without it, there would be a race condition between these two places, reading and writing m_uiGroupCounter and waiting/signaling
      // m_CondVarGroupFinished
      W_LOCK(pGroup->m_CondVarGroupFinished);

      groupCounter = pGroup->m_uiGroupCounter;

      // set this task group to be finished such that no one tries to append further dependencies
      pGroup->m_uiGroupCounter += 2;
    }

    {
      W_LOCK(s_TaskSystemMutex);

      // unless an outside reference is held onto a task, this will deallocate the tasks
      pGroup->m_Tasks.Clear();

      for (WUInt32 dep = 0; dep < pGroup->m_OthersDependingOnMe.GetCount(); ++dep)
      {
        DependencyHasFinished(pGroup->m_OthersDependingOnMe[dep].m_pTaskGroup);
      }
    }

    // wake up all threads that are waiting for this group
    pGroup->m_CondVarGroupFinished.SignalAll();

    if (pGroup->m_OnFinishedCallback.IsValid())
    {
      WTaskGroupID id;
      id.m_pTaskGroup = pGroup;
      id.m_uiGroupCounter = groupCounter;
      pGroup->m_OnFinishedCallback(id);
    }

    // set this task available for reuse
    pGroup->m_bInUse = false;
  }
}

WTaskSystem::TaskData WTaskSystem::GetNextTask(WTaskPriority::Enum FirstPriority, WTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait,
  const WTaskGroupID& WaitingForGroup, WAtomicInteger32* pWorkerState)
{
  // this is the central function that selects tasks for the worker threads to work on

  W_ASSERT_DEV(FirstPriority >= WTaskPriority::EarlyThisFrame && LastPriority < WTaskPriority::ENUM_COUNT, "Priority Range is invalid: {0} to {1}",
    FirstPriority, LastPriority);

  W_LOCK(s_TaskSystemMutex);

  // go through all the task lists that this thread is willing to work on
  for (WUInt32 prio = FirstPriority; prio <= (WUInt32)LastPriority; ++prio)
  {
    for (auto it = s_pState->m_Tasks[prio].GetIterator(); it.IsValid(); ++it)
    {
      if (!bOnlyTasksThatNeverWait || (it->m_pTask->m_NestingMode == WTaskNesting::Never) || it->m_pBelongsToGroup == WaitingForGroup.m_pTaskGroup)
      {
        TaskData td = *it;

        s_pState->m_Tasks[prio].Remove(it);
        return td;
      }
    }
  }

  if (pWorkerState)
  {
    W_VERIFY(pWorkerState->Set((int)WTaskWorkerState::Idle) == (int)WTaskWorkerState::Active, "Corrupt Worker State");
  }

  return TaskData();
}

bool WTaskSystem::ExecuteTask(WTaskPriority::Enum FirstPriority, WTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait,
  const WTaskGroupID& WaitingForGroup, WAtomicInteger32* pWorkerState)
{
  // const WWorkerThreadType::Enum workerType = (tl_TaskWorkerInfo.m_WorkerType == WWorkerThreadType::Unknown) ? WWorkerThreadType::ShortTasks :
  // tl_TaskWorkerInfo.m_WorkerType;

  WTaskSystem::TaskData td = GetNextTask(FirstPriority, LastPriority, bOnlyTasksThatNeverWait, WaitingForGroup, pWorkerState);

  if (td.m_pTask == nullptr)
    return false;

  if (bOnlyTasksThatNeverWait && td.m_pTask->m_NestingMode != WTaskNesting::Never)
  {
    W_ASSERT_DEV(td.m_pBelongsToGroup == WaitingForGroup.m_pTaskGroup, "");
  }

  tl_TaskWorkerInfo.m_bAllowNestedTasks = td.m_pTask->m_NestingMode != WTaskNesting::Never;
  tl_TaskWorkerInfo.m_szTaskName = td.m_pTask->m_sTaskName;
  td.m_pTask->Run(td.m_uiInvocation);
  tl_TaskWorkerInfo.m_bAllowNestedTasks = true;
  tl_TaskWorkerInfo.m_szTaskName = nullptr;

  // notify the group, that a task is finished, which might trigger other tasks to be executed
  TaskHasFinished(std::move(td.m_pTask), td.m_pBelongsToGroup);

  return true;
}


WResult WTaskSystem::CancelTask(const WSharedPtr<WTask>& pTask, WOnTaskRunning::Enum onTaskRunning)
{
  if (pTask->IsTaskFinished())
    return W_SUCCESS;

  // pTask may actually finish between here and the lock below
  // in that case we will return failure, as in we had to 'wait' for a task,
  // but it will be handled correctly

  W_PROFILE_SCOPE("CancelTask");

  // we set the cancel flag, to make sure that tasks that support canceling will terminate asap
  pTask->m_bCancelExecution = true;

  {
    W_LOCK(s_TaskSystemMutex);

    // if the task is still in the queue of its group, it had not yet been scheduled
    if (!pTask->m_bTaskIsScheduled && pTask->m_BelongsToGroup.m_pTaskGroup->m_Tasks.RemoveAndSwap(pTask))
    {
      // we set the task to finished, even though it was not executed
      pTask->m_iRemainingRuns = 0;
      return W_SUCCESS;
    }

    // check if the task has already been scheduled for execution
    // if so, remove it from the work queue
    {
      for (WUInt32 i = 0; i < WTaskPriority::ENUM_COUNT; ++i)
      {
        auto it = s_pState->m_Tasks[i].GetIterator();

        while (it.IsValid())
        {
          if (it->m_pTask == pTask)
          {
            // we set the task to finished, even though it was not executed
            pTask->m_iRemainingRuns = 0;

            // tell the system that one task of that group is 'finished', to ensure its dependencies will get scheduled
            TaskHasFinished(std::move(it->m_pTask), it->m_pBelongsToGroup);

            s_pState->m_Tasks[i].Remove(it);
            return W_SUCCESS;
          }

          ++it;
        }
      }
    }
  }

  // if we made it here, the task was already running
  // thus we just wait for it to finish

  if (onTaskRunning == WOnTaskRunning::WaitTillFinished)
  {
    WaitForCondition([pTask]()
      { return pTask->IsTaskFinished(); });
  }

  return W_FAILURE;
}


bool WTaskSystem::HelpExecutingTasks(const WTaskGroupID& WaitingForGroup)
{
  const bool bOnlyTasksThatNeverWait = tl_TaskWorkerInfo.m_WorkerType != WWorkerThreadType::MainThread;

  WTaskPriority::Enum FirstPriority;
  WTaskPriority::Enum LastPriority;
  DetermineTasksToExecuteOnThread(FirstPriority, LastPriority);

  return ExecuteTask(FirstPriority, LastPriority, bOnlyTasksThatNeverWait, WaitingForGroup, nullptr);
}

void WTaskSystem::ReprioritizeFrameTasks()
{
  // There should usually be no 'this frame tasks' left at this time
  // however, while we waited to enter the lock, such tasks might have appeared
  // In this case we move them into the highest-priority 'this frame' queue, to ensure they will be executed asap
  for (WUInt32 i = (WUInt32)WTaskPriority::ThisFrame; i <= (WUInt32)WTaskPriority::LateThisFrame; ++i)
  {
    auto it = s_pState->m_Tasks[i].GetIterator();

    // move all 'this frame' tasks into the 'early this frame' queue
    while (it.IsValid())
    {
      s_pState->m_Tasks[WTaskPriority::EarlyThisFrame].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_pState->m_Tasks[i].Clear();
  }

  for (WUInt32 i = (WUInt32)WTaskPriority::EarlyNextFrame; i <= (WUInt32)WTaskPriority::LateNextFrame; ++i)
  {
    auto it = s_pState->m_Tasks[i].GetIterator();

    // move all 'next frame' tasks into the 'this frame' queues
    while (it.IsValid())
    {
      s_pState->m_Tasks[i - 3].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_pState->m_Tasks[i].Clear();
  }

  for (WUInt32 i = (WUInt32)WTaskPriority::In2Frames; i <= (WUInt32)WTaskPriority::In9Frames; ++i)
  {
    auto it = s_pState->m_Tasks[i].GetIterator();

    // move all 'in N frames' tasks into the 'in N-1 frames' queues
    // moves 'In2Frames' into 'LateNextFrame'
    while (it.IsValid())
    {
      s_pState->m_Tasks[i - 1].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_pState->m_Tasks[i].Clear();
  }
}

void WTaskSystem::ExecuteSomeFrameTasks(WTime smoothFrameTime)
{
  W_PROFILE_SCOPE("ExecuteSomeFrameTasks");

  // 'SomeFrameMainThread' tasks are usually used to upload resources that have been loaded in the background
  // they do not need to be executed right away, but the earlier, the better

  // as long as the frame time is short enough, execute tasks that need to be done on the main thread
  // on fast machines that means that these tasks are finished as soon as possible and users will see the results quickly

  // if the frame time spikes, we can skip this a few times, to try to prevent further slow downs
  // however in such instances, the 'frame time threshold' will increase and thus the chance that we skip this entirely becomes lower over
  // time that guarantees some progress, even if the frame rate is constantly low

  static WTime s_FrameTimeThreshold = smoothFrameTime;
  static WTime s_LastExecution; // initializes to zero -> very large frame time difference at first

  WTime CurTime = WTime::Now();
  WTime LastTime = s_LastExecution;
  s_LastExecution = CurTime;

  // as long as we have a smooth frame rate, execute as many of these tasks, as possible
  while (CurTime - LastTime < smoothFrameTime)
  {
    if (!ExecuteTask(WTaskPriority::SomeFrameMainThread, WTaskPriority::SomeFrameMainThread, false, WTaskGroupID(), nullptr))
    {
      // nothing left to do, reset the threshold
      s_FrameTimeThreshold = smoothFrameTime;
      return;
    }

    CurTime = WTime::Now();
  }

  WUInt32 uiNumTasksTodo = 0;

  {
    W_LOCK(s_TaskSystemMutex);
    uiNumTasksTodo = s_pState->m_Tasks[WTaskPriority::SomeFrameMainThread].GetCount();
  }

  if (uiNumTasksTodo == 0)
    return;

  if (CurTime - LastTime < s_FrameTimeThreshold) // the accumulating threshold has caught up with us
  {
    // don't reset the threshold, from now on we execute at least one task per frame

    ExecuteTask(WTaskPriority::SomeFrameMainThread, WTaskPriority::SomeFrameMainThread, false, WTaskGroupID(), nullptr);
  }
  else
  {
    // increase the threshold slightly every time we skip the work
    // this means that when the frame rate is too low, we can ignore these tasks for a few frames
    // and thus prevent decreasing the frame rate even further
    // however we increase the time threshold, at which we skip this, further and further
    // therefore at some point we will start executing these tasks, no matter how low the frame rate is
    //
    // this gives us some buffer to smooth out performance drops
    s_FrameTimeThreshold += WTime::MakeFromMilliseconds(0.2);
  }

  // if the queue is really full, we have to guarantee more progress
  {
    if (uiNumTasksTodo > 100)
      ExecuteTask(WTaskPriority::SomeFrameMainThread, WTaskPriority::SomeFrameMainThread, false, WTaskGroupID(), nullptr);

    if (uiNumTasksTodo > 75)
      ExecuteTask(WTaskPriority::SomeFrameMainThread, WTaskPriority::SomeFrameMainThread, false, WTaskGroupID(), nullptr);

    if (uiNumTasksTodo > 50)
      ExecuteTask(WTaskPriority::SomeFrameMainThread, WTaskPriority::SomeFrameMainThread, false, WTaskGroupID(), nullptr);
  }
}


void WTaskSystem::FinishFrameTasks()
{
  W_ASSERT_DEV(WThreadUtils::IsMainThread(), "This function must be executed on the main thread.");

  // make sure all 'main thread' and 'short' tasks are either finished or being worked on by other threads already
  {
    while (true)
    {
      // Prefer to work on main-thread tasks
      if (ExecuteTask(WTaskPriority::ThisFrameMainThread, WTaskPriority::ThisFrameMainThread, false, WTaskGroupID(), nullptr))
      {
        continue;
      }

      // if there are none, help out with the other tasks for this frame
      if (ExecuteTask(WTaskPriority::EarlyThisFrame, WTaskPriority::LateThisFrame, false, WTaskGroupID(), nullptr))
      {
        continue;
      }

      break;
    }
  }

  // all the important tasks for this frame should be finished or worked on by now
  // so we can now re-prioritize the tasks for the next frame
  {
    W_LOCK(s_TaskSystemMutex);

    ReprioritizeFrameTasks();
  }

  ExecuteSomeFrameTasks(s_pState->m_TargetFrameTime);

  // Update the thread utilization
  {
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    const WTime tNow = WTime::Now();
    static WTime s_LastFrameUpdate = tNow;
    const WTime tDiff = tNow - s_LastFrameUpdate;

    // prevent division by zero (inside ComputeThreadUtilization)
    if (tDiff > WTime::MakeFromSeconds(0.0))
    {
      s_LastFrameUpdate = tNow;

      for (WUInt32 type = 0; type < WWorkerThreadType::ENUM_COUNT; ++type)
      {
        const WUInt32 uiNumWorkers = s_pThreadState->m_iAllocatedWorkers[type];

        for (WUInt32 t = 0; t < uiNumWorkers; ++t)
        {
          s_pThreadState->m_Workers[type][t]->UpdateThreadUtilization(tDiff);
        }
      }
    }
#endif
  }
}
