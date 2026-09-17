#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>


WTaskGroupID WTaskSystem::CreateTaskGroup(WTaskPriority::Enum priority, WOnTaskGroupFinishedCallback callback)
{
  W_LOCK(s_TaskSystemMutex);

  WUInt32 i = 0;

  // this search could be speed up with a stack of free groups
  for (; i < s_pState->m_TaskGroups.GetCount(); ++i)
  {
    if (!s_pState->m_TaskGroups[i].m_bInUse)
    {
      goto foundtaskgroup;
    }
  }

  // no free group found, create a new one
  s_pState->m_TaskGroups.ExpandAndGetRef();
  s_pState->m_TaskGroups[i].m_uiTaskGroupIndex = static_cast<WUInt16>(i);

foundtaskgroup:

  s_pState->m_TaskGroups[i].Reuse(priority, callback);

  WTaskGroupID id;
  id.m_pTaskGroup = &s_pState->m_TaskGroups[i];
  id.m_uiGroupCounter = s_pState->m_TaskGroups[i].m_uiGroupCounter;
  return id;
}

void WTaskSystem::AddTaskToGroup(WTaskGroupID groupID, const WSharedPtr<WTask>& pTask)
{
  W_ASSERT_DEBUG(pTask != nullptr, "Cannot add nullptr tasks.");
  W_ASSERT_DEV(pTask->IsTaskFinished(), "The given task is not finished! Cannot reuse a task before it is done.");
  W_ASSERT_DEBUG(!pTask->m_sTaskName.IsEmpty(), "Every task should have a name");

  WTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  pTask->Reset();
  pTask->m_BelongsToGroup = groupID;
  groupID.m_pTaskGroup->m_Tasks.PushBack(pTask);
}

void WTaskSystem::AddTaskGroupDependency(WTaskGroupID groupID, WTaskGroupID dependsOn)
{
  W_ASSERT_DEBUG(dependsOn.IsValid(), "Invalid dependency");
  W_ASSERT_DEBUG(groupID.m_pTaskGroup != dependsOn.m_pTaskGroup || groupID.m_uiGroupCounter != dependsOn.m_uiGroupCounter, "Group cannot depend on itselfs");

  WTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  groupID.m_pTaskGroup->m_DependsOnGroups.PushBack(dependsOn);
}

void WTaskSystem::AddTaskGroupDependencyBatch(WArrayPtr<const WTaskGroupDependency> batch)
{
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  // lock here once to reduce the overhead of WTaskGroup::DebugCheckTaskGroup inside AddTaskGroupDependency
  W_LOCK(s_TaskSystemMutex);
#endif

  for (const WTaskGroupDependency& dep : batch)
  {
    AddTaskGroupDependency(dep.m_TaskGroup, dep.m_DependsOn);
  }
}

void WTaskSystem::StartTaskGroup(WTaskGroupID groupID)
{
  W_ASSERT_DEV(s_pThreadState->m_Workers[WWorkerThreadType::ShortTasks].GetCount() > 0, "No worker threads started.");

  WTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  WInt32 iActiveDependencies = 0;

  {
    W_LOCK(s_TaskSystemMutex);

    WTaskGroup& tg = *groupID.m_pTaskGroup;

    tg.m_bStartedByUser = true;

    for (WUInt32 i = 0; i < tg.m_DependsOnGroups.GetCount(); ++i)
    {
      if (!IsTaskGroupFinished(tg.m_DependsOnGroups[i]))
      {
        WTaskGroup& Dependency = *tg.m_DependsOnGroups[i].m_pTaskGroup;

        // add this task group to the list of dependencies, such that when that group finishes, this task group can get woken up
        Dependency.m_OthersDependingOnMe.PushBack(groupID);

        // count how many other groups need to finish before this task group can be executed
        ++iActiveDependencies;
      }
    }

    if (iActiveDependencies != 0)
    {
      // atomic integers are quite slow, so do not use them in the loop, where they are not yet needed
      tg.m_iNumActiveDependencies = iActiveDependencies;
    }
  }

  if (iActiveDependencies == 0)
  {
    ScheduleGroupTasks(groupID.m_pTaskGroup, false);
  }
}

void WTaskSystem::StartTaskGroupBatch(WArrayPtr<const WTaskGroupID> batch)
{
  W_LOCK(s_TaskSystemMutex);

  for (const WTaskGroupID& group : batch)
  {
    StartTaskGroup(group);
  }
}

bool WTaskSystem::IsTaskGroupFinished(WTaskGroupID group)
{
  // if the counters differ, the task group has been reused since the GroupID was created, so that group has finished
  return (group.m_pTaskGroup == nullptr) || (group.m_pTaskGroup->m_uiGroupCounter != group.m_uiGroupCounter);
}

void WTaskSystem::ScheduleGroupTasks(WTaskGroup* pGroup, bool bHighPriority)
{
  if (pGroup->m_Tasks.IsEmpty())
  {
    pGroup->m_iNumRemainingTasks = 1;

    // "finish" one task -> will finish the task group and kick off dependent groups
    TaskHasFinished(nullptr, pGroup);
    return;
  }

  WInt32 iRemainingTasks = 0;

  // add all the tasks to the task list, so that they will be processed
  {
    W_LOCK(s_TaskSystemMutex);


    // store how many tasks from this groups still need to be processed

    for (auto pTask : pGroup->m_Tasks)
    {
      iRemainingTasks += WMath::Max(1u, pTask->m_uiMultiplicity);
      pTask->m_iRemainingRuns = WMath::Max(1u, pTask->m_uiMultiplicity);
    }

    pGroup->m_iNumRemainingTasks = iRemainingTasks;


    for (WUInt32 task = 0; task < pGroup->m_Tasks.GetCount(); ++task)
    {
      auto& pTask = pGroup->m_Tasks[task];

      for (WUInt32 mult = 0; mult < WMath::Max(1u, pTask->m_uiMultiplicity); ++mult)
      {
        TaskData td;
        td.m_pBelongsToGroup = pGroup;
        td.m_pTask = pTask;
        td.m_pTask->m_bTaskIsScheduled = true;
        td.m_uiInvocation = mult;

        if (bHighPriority)
          s_pState->m_Tasks[pGroup->m_Priority].PushFront(td);
        else
          s_pState->m_Tasks[pGroup->m_Priority].PushBack(td);
      }
    }

    // send the proper thread signal, to make sure one of the correct worker threads is awake
    switch (pGroup->m_Priority)
    {
      case WTaskPriority::EarlyThisFrame:
      case WTaskPriority::ThisFrame:
      case WTaskPriority::LateThisFrame:
      case WTaskPriority::EarlyNextFrame:
      case WTaskPriority::NextFrame:
      case WTaskPriority::LateNextFrame:
      case WTaskPriority::In2Frames:
      case WTaskPriority::In3Frames:
      case WTaskPriority::In4Frames:
      case WTaskPriority::In5Frames:
      case WTaskPriority::In6Frames:
      case WTaskPriority::In7Frames:
      case WTaskPriority::In8Frames:
      case WTaskPriority::In9Frames:
      {
        WakeUpThreads(WWorkerThreadType::ShortTasks, iRemainingTasks);
        break;
      }

      case WTaskPriority::LongRunning:
      case WTaskPriority::LongRunningHighPriority:
      {
        WakeUpThreads(WWorkerThreadType::LongTasks, iRemainingTasks);
        break;
      }

      case WTaskPriority::FileAccess:
      case WTaskPriority::FileAccessHighPriority:
      {
        WakeUpThreads(WWorkerThreadType::FileAccess, iRemainingTasks);
        break;
      }

      case WTaskPriority::SomeFrameMainThread:
      case WTaskPriority::ThisFrameMainThread:
      case WTaskPriority::ENUM_COUNT:
        // nothing to do for these enum values
        break;
    }
  }
}

void WTaskSystem::DependencyHasFinished(WTaskGroup* pGroup)
{
  // remove one dependency from the group
  if (pGroup->m_iNumActiveDependencies.Decrement() == 0)
  {
    // if there are no remaining dependencies, kick off all tasks in this group
    ScheduleGroupTasks(pGroup, true);
  }
}

WResult WTaskSystem::CancelGroup(WTaskGroupID group, WOnTaskRunning::Enum onTaskRunning)
{
  if (WTaskSystem::IsTaskGroupFinished(group))
    return W_SUCCESS;

  W_PROFILE_SCOPE("CancelGroup");

  W_LOCK(s_TaskSystemMutex);

  WResult res = W_SUCCESS;

  auto TasksCopy = group.m_pTaskGroup->m_Tasks;

  // first cancel ALL the tasks in the group, without waiting for anything
  for (WUInt32 task = 0; task < TasksCopy.GetCount(); ++task)
  {
    if (CancelTask(TasksCopy[task], WOnTaskRunning::ReturnWithoutBlocking) == W_FAILURE)
    {
      res = W_FAILURE;
    }
  }

  // if all tasks could be removed without problems, we do not need to try it again with blocking

  if (onTaskRunning == WOnTaskRunning::WaitTillFinished && res == W_FAILURE)
  {
    // now cancel the tasks in the group again, this time wait for those that are already running
    for (WUInt32 task = 0; task < TasksCopy.GetCount(); ++task)
    {
      CancelTask(TasksCopy[task], WOnTaskRunning::WaitTillFinished).IgnoreResult();
    }
  }

  return res;
}

void WTaskSystem::WaitForGroup(WTaskGroupID group)
{
  W_PROFILE_SCOPE("WTaskSystem::WaitForGroup");

  W_ASSERT_DEV(tl_TaskWorkerInfo.m_bAllowNestedTasks, "The executing task '{}' is flagged to never wait for other tasks but does so anyway. Remove the flag or remove the wait-dependency.", tl_TaskWorkerInfo.m_szTaskName);

  const auto ThreadTaskType = tl_TaskWorkerInfo.m_WorkerType;
  const bool bAllowSleep = ThreadTaskType != WWorkerThreadType::MainThread;

  while (!WTaskSystem::IsTaskGroupFinished(group))
  {
    if (!HelpExecutingTasks(group))
    {
      if (bAllowSleep)
      {
        const WWorkerThreadType::Enum typeToWakeUp = (ThreadTaskType == WWorkerThreadType::Unknown) ? WWorkerThreadType::ShortTasks : ThreadTaskType;

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          W_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)WTaskWorkerState::Blocked) == (int)WTaskWorkerState::Active, "Corrupt worker state");
        }

        WakeUpThreads(typeToWakeUp, 1);

        group.m_pTaskGroup->WaitForFinish(group);

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          W_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)WTaskWorkerState::Active) == (int)WTaskWorkerState::Blocked, "Corrupt worker state");
        }

        break;
      }
      else
      {
        WThreadUtils::YieldTimeSlice();
      }
    }
  }
}

void WTaskSystem::WaitForCondition(WDelegate<bool()> condition)
{
  W_PROFILE_SCOPE("WaitForCondition");

  W_ASSERT_DEV(tl_TaskWorkerInfo.m_bAllowNestedTasks, "The executing task '{}' is flagged to never wait for other tasks but does so anyway. Remove the flag or remove the wait-dependency.", tl_TaskWorkerInfo.m_szTaskName);

  const auto ThreadTaskType = tl_TaskWorkerInfo.m_WorkerType;
  const bool bAllowSleep = ThreadTaskType != WWorkerThreadType::MainThread;

  while (!condition())
  {
    if (!HelpExecutingTasks(WTaskGroupID()))
    {
      if (bAllowSleep)
      {
        const WWorkerThreadType::Enum typeToWakeUp = (ThreadTaskType == WWorkerThreadType::Unknown) ? WWorkerThreadType::ShortTasks : ThreadTaskType;

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          W_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)WTaskWorkerState::Blocked) == (int)WTaskWorkerState::Active, "Corrupt worker state");
        }

        WakeUpThreads(typeToWakeUp, 1);

        while (!condition())
        {
          // TODO: busy loop for now
          WThreadUtils::YieldTimeSlice();
        }

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          W_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)WTaskWorkerState::Active) == (int)WTaskWorkerState::Blocked, "Corrupt worker state");
        }

        break;
      }
      else
      {
        WThreadUtils::YieldTimeSlice();
      }
    }
  }
}
