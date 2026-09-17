#pragma once

#include <Foundation/Threading/TaskSystem.h>

class WTaskSystemThreadState
{
private:
  friend class WTaskSystem;
  friend class WTaskWorkerThread;

  // The arrays of all the active worker threads.
  WDynamicArray<WTaskWorkerThread*> m_Workers[WWorkerThreadType::ENUM_COUNT];

  // the number of allocated (non-null) worker threads in m_Workers
  WAtomicInteger32 m_iAllocatedWorkers[WWorkerThreadType::ENUM_COUNT];

  // the maximum number of worker threads that should be non-idle (and not blocked) at any time
  WUInt32 m_uiMaxWorkersToUse[WWorkerThreadType::ENUM_COUNT] = {};
};

class WTaskSystemState
{
private:
  friend class WTaskSystem;

  // The target frame time used by FinishFrameTasks()
  WTime m_TargetFrameTime = WTime::MakeFromSeconds(1.0 / 40.0); // => 25 ms

  // The deque can grow without relocating existing data, therefore the WTaskGroupID's can store pointers directly to the data
  WDeque<WTaskGroup> m_TaskGroups;

  // The lists of all scheduled tasks, for each priority.
  WList<WTaskSystem::TaskData> m_Tasks[WTaskPriority::ENUM_COUNT];
};
