#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Thread.h>

WEvent<const WThreadEvent&, WMutex> WThread::s_ThreadEvents;

thread_local WThread* g_pCurrentThread = nullptr;

const WThread* WThread::GetCurrentThread()
{
  return g_pCurrentThread;
}

WThread::WThread(WStringView sName /*= "WThread"*/, WUInt32 uiStackSize /*= 128 * 1024*/)
  : WOSThread(WThreadClassEntryPoint, this, sName, uiStackSize)
  , m_sName(sName)
{
  WThreadEvent e;
  e.m_pThread = this;
  e.m_Type = WThreadEvent::Type::ThreadCreated;
  WThread::s_ThreadEvents.Broadcast(e, 255);
}

WThread::~WThread()
{
  W_ASSERT_DEV(!IsRunning(), "Thread deletion while still running detected!");

  WThreadEvent e;
  e.m_pThread = this;
  e.m_Type = WThreadEvent::Type::ThreadDestroyed;
  WThread::s_ThreadEvents.Broadcast(e, 255);
}

WUInt32 RunThread(WThread* pThread)
{
  if (pThread == nullptr)
    return 0;

  g_pCurrentThread = pThread;
  WProfilingSystem::SetThreadName(pThread->m_sName.GetView());

  {
    WThreadEvent e;
    e.m_pThread = pThread;
    e.m_Type = WThreadEvent::Type::StartingExecution;
    WThread::s_ThreadEvents.Broadcast(e, 255);
  }

  pThread->m_ThreadStatus = WThread::Running;

  // Run the worker thread function
  WUInt32 uiReturnCode = pThread->Run();

  {
    WThreadEvent e;
    e.m_pThread = pThread;
    e.m_Type = WThreadEvent::Type::FinishedExecution;
    WThread::s_ThreadEvents.Broadcast(e, 255);
  }

  pThread->m_ThreadStatus = WThread::Finished;

  WProfilingSystem::RemoveThread();

  return uiReturnCode;
}
