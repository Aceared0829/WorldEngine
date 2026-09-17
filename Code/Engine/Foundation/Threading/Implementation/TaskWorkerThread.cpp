#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

thread_local WTaskWorkerInfo tl_TaskWorkerInfo;

static WString GenerateThreadName(WWorkerThreadType::Enum threadType, WUInt32 uiThreadNumber)
{
  WStringBuilder sTemp;
  sTemp.SetFormat("{} {}", WWorkerThreadType::GetThreadTypeName(threadType), uiThreadNumber);
  return sTemp;
}

WTaskWorkerThread::WTaskWorkerThread(WWorkerThreadType::Enum threadType, WUInt32 uiThreadNumber)
  // We need at least 256 kb of stack size, otherwise the shader compilation tasks will run out of stack space.
  : WThread(GenerateThreadName(threadType, uiThreadNumber), 256 * 1024)
{
  m_WorkerType = threadType;
  m_uiWorkerThreadNumber = uiThreadNumber & 0xFFFF;
}

WTaskWorkerThread::~WTaskWorkerThread() = default;

WResult WTaskWorkerThread::DeactivateWorker()
{
  m_bActive = false;

  if (GetThreadStatus() != WThread::Finished)
  {
    // if necessary, wake this thread up
    WakeUpIfIdle();

    return W_FAILURE;
  }

  return W_SUCCESS;
}

void WTaskWorkerThread::BroadcastClearThreadLocalsEvent()
{
  m_bClearThreadLocalsEvent = true;

  if (GetThreadStatus() != WThread::Finished)
  {
    // if necessary, wake this thread up
    WakeUpIfIdle();
  }
}

void WTaskWorkerThread::WaitForBroadcastClearTLS()
{
  while (m_bClearThreadLocalsEvent)
  {
    WakeUpIfIdle();
    WThreadUtils::YieldTimeSlice();
  }
}

WUInt32 WTaskWorkerThread::Run()
{
  W_ASSERT_DEBUG(
    m_WorkerType != WWorkerThreadType::Unknown && m_WorkerType != WWorkerThreadType::MainThread, "Worker threads cannot use this type");
  W_ASSERT_DEBUG(m_WorkerType < WWorkerThreadType::ENUM_COUNT, "Worker Thread Type is invalid: {0}", m_WorkerType);

  // once this thread is running, store the worker type in the thread_local variable
  // such that the WTaskSystem is able to look this up (e.g. in WaitForGroup) to know which types of tasks to help with
  tl_TaskWorkerInfo.m_WorkerType = m_WorkerType;
  tl_TaskWorkerInfo.m_iWorkerIndex = m_uiWorkerThreadNumber;
  tl_TaskWorkerInfo.m_pWorkerState = &m_iWorkerState;

  const bool bIsReserve = m_uiWorkerThreadNumber >= WTaskSystem::s_pThreadState->m_uiMaxWorkersToUse[m_WorkerType];

  WTaskPriority::Enum FirstPriority;
  WTaskPriority::Enum LastPriority;
  WTaskSystem::DetermineTasksToExecuteOnThread(FirstPriority, LastPriority);

  m_bExecutingTask = false;

  while (m_bActive)
  {
    if (m_bClearThreadLocalsEvent)
    {
      m_bClearThreadLocalsEvent = false;

      WThreadEvent e;
      e.m_pThread = this;
      e.m_Type = WThreadEvent::Type::ClearThreadLocals;
      WThread::s_ThreadEvents.Broadcast(e);
    }

    if (!m_bExecutingTask)
    {
      m_bExecutingTask = true;
      m_StartedWorkingTime = WTime::Now();
    }

    if (!WTaskSystem::ExecuteTask(FirstPriority, LastPriority, false, WTaskGroupID(), &m_iWorkerState))
    {
      WaitForWork();
    }
    else
    {
      ++m_uiNumTasksExecuted;

      if (bIsReserve)
      {
        W_VERIFY(m_iWorkerState.Set((int)WTaskWorkerState::Idle) == (int)WTaskWorkerState::Active, "Corrupt worker state");

        // if this thread is part of the reserve, then don't continue to process tasks indefinitely
        // instead, put this thread to sleep and wake up someone else
        // that someone else may be a thread at the front of the queue, it may also turn out to be this thread again
        // either way, if at some point we woke up more threads than the maximum desired, this will move the active threads
        // to the front of the list, because of the way WTaskSystem::WakeUpThreads() works
        WTaskSystem::WakeUpThreads(m_WorkerType, 1);

        WaitForWork();
      }
    }
  }

  return 0;
}

void WTaskWorkerThread::WaitForWork()
{
  // m_bIsIdle usually will be true here, but may also already have been reset to false
  // in that case m_WakeUpSignal will be raised already and the code below will just run through and continue

  m_ThreadActiveTime += WTime::Now() - m_StartedWorkingTime;
  m_bExecutingTask = false;
  m_WakeUpSignal.WaitForSignal();
  W_ASSERT_DEBUG(m_iWorkerState == (int)WTaskWorkerState::Active, "Worker state should have been reset to 'active'");
}

WTaskWorkerState WTaskWorkerThread::WakeUpIfIdle()
{
  WTaskWorkerState prev = (WTaskWorkerState)m_iWorkerState.CompareAndSwap((int)WTaskWorkerState::Idle, (int)WTaskWorkerState::Active);
  if (prev == WTaskWorkerState::Idle) // was idle before
  {
    m_WakeUpSignal.RaiseSignal();
  }

  return static_cast<WTaskWorkerState>(prev);
}

void WTaskWorkerThread::UpdateThreadUtilization(WTime timePassed)
{
  WTime tActive = m_ThreadActiveTime;

  // The thread keeps track of how much time it spends executing tasks.
  // Here we retrieve that time and resets it to zero.
  {
    m_ThreadActiveTime = WTime::MakeZero();

    if (m_bExecutingTask)
    {
      const WTime tNow = WTime::Now();
      tActive += tNow - m_StartedWorkingTime;
      m_StartedWorkingTime = tNow;
    }
  }

  m_fLastThreadUtilization = tActive.GetSeconds() / timePassed.GetSeconds();
  m_uiLastNumTasksExecuted = m_uiNumTasksExecuted;
  m_uiNumTasksExecuted = 0;
}

double WTaskWorkerThread::GetThreadUtilization(WUInt32* pNumTasksExecuted /*= nullptr*/)
{
  if (pNumTasksExecuted)
  {
    *pNumTasksExecuted = m_uiLastNumTasksExecuted;
  }

  return m_fLastThreadUtilization;
}
