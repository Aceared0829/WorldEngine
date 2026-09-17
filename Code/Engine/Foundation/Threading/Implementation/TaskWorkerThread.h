#pragma once

#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>

#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>

/// \internal Internal task worker thread class.
class WTaskWorkerThread final : public WThread
{
  W_DISALLOW_COPY_AND_ASSIGN(WTaskWorkerThread);

  /// \name Execution
  ///@{

public:
  /// Tells the worker thread what tasks to execute and which thread index it has.
  WTaskWorkerThread(WWorkerThreadType::Enum threadType, WUInt32 uiThreadNumber);
  ~WTaskWorkerThread();

  /// Deactivates the thread. Returns failure, if the thread is currently still running.
  WResult DeactivateWorker();

  /// Broadcasts WThreadEvent::ClearThreadLocals on this thread.
  void BroadcastClearThreadLocalsEvent();

  void WaitForBroadcastClearTLS();

private:
  // Which types of tasks this thread should work on.
  WWorkerThreadType::Enum m_WorkerType;

  // Whether the thread is supposed to continue running.
  volatile bool m_bActive = true;
  bool m_bClearThreadLocalsEvent = false;

  // For display purposes.
  WUInt16 m_uiWorkerThreadNumber = 0xFFFF;

  ///@}

  /// \name Thread Utilization
  ///@{

public:
  /// Returns the last utilization value (0 - 1 range). Optionally returns how many tasks it executed recently.
  double GetThreadUtilization(WUInt32* pNumTasksExecuted = nullptr);

  /// Computes the thread utilization by dividing the thread active time by the time that has passed since the last update.
  void UpdateThreadUtilization(WTime timePassed);

private:
  bool m_bExecutingTask = false;
  WUInt16 m_uiLastNumTasksExecuted = 0;
  WUInt16 m_uiNumTasksExecuted = 0;
  WTime m_StartedWorkingTime;
  WTime m_ThreadActiveTime;
  double m_fLastThreadUtilization = 0.0;

  ///@}

  /// \name Idle State
  ///@{

public:
  /// If the thread is currently idle, this will wake it up and return W_SUCCESS.
  WTaskWorkerState WakeUpIfIdle();

private:
  // Puts the thread to sleep (idle state)
  void WaitForWork();

  virtual WUInt32 Run() override;

  // used to wake up idle threads, see m_WorkerState
  WThreadSignal m_WakeUpSignal;

  // used to indicate whether this thread is currently idle
  // if so, it can be woken up using m_WakeUpSignal
  // WAtomicBool m_bIsIdle = false;
  WAtomicInteger32 m_iWorkerState; // WTaskWorkerState

  ///@}
};

/// \internal Thread local state used by the task system (and for better debugging)
struct WTaskWorkerInfo
{
  WWorkerThreadType::Enum m_WorkerType = WWorkerThreadType::Unknown;
  bool m_bAllowNestedTasks = true;
  WInt32 m_iWorkerIndex = -1;
  const char* m_szTaskName = nullptr;
  WAtomicInteger32* m_pWorkerState = nullptr;
};

extern thread_local WTaskWorkerInfo tl_TaskWorkerInfo;
