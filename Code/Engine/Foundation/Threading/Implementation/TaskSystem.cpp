#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

WMutex WTaskSystem::s_TaskSystemMutex;
WUniquePtr<WTaskSystemState> WTaskSystem::s_pState;
WUniquePtr<WTaskSystemThreadState> WTaskSystem::s_pThreadState;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, TaskSystem)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ThreadUtils",
    "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    if (WStartup::HasApplicationTag("NoTaskSystem"))
      return;

    WTaskSystem::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WTaskSystem::Shutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

void WTaskSystem::Startup()
{
  s_pThreadState = W_DEFAULT_NEW(WTaskSystemThreadState);
  s_pState = W_DEFAULT_NEW(WTaskSystemState);

  tl_TaskWorkerInfo.m_WorkerType = WWorkerThreadType::MainThread;
  tl_TaskWorkerInfo.m_iWorkerIndex = 0;

  // initialize with the default number of worker threads
  SetWorkerThreadCount();
}

void WTaskSystem::Shutdown()
{
  if (s_pThreadState == nullptr)
    return;

  StopWorkerThreads();

  s_pState.Clear();
  s_pThreadState.Clear();
}

void WTaskSystem::SetTargetFrameTime(WTime targetFrameTime)
{
  s_pState->m_TargetFrameTime = targetFrameTime;
}

void WTaskSystem::BroadcastClearThreadLocalsEvent()
{
  for (WUInt32 i = 0; i < WWorkerThreadType::ENUM_COUNT; ++i)
  {
    for (WTaskWorkerThread* pWorker : s_pThreadState->m_Workers[i])
    {
      if (pWorker)
      {
        pWorker->BroadcastClearThreadLocalsEvent();
      }
    }
  }

  // make sure they have all sent the event
  for (WUInt32 i = 0; i < WWorkerThreadType::ENUM_COUNT; ++i)
  {
    for (WTaskWorkerThread* pWorker : s_pThreadState->m_Workers[i])
    {
      if (pWorker)
      {
        pWorker->WaitForBroadcastClearTLS();
      }
    }
  }
}

W_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystem);
