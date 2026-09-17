#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/ThreadWithDispatcher.h>

WThreadWithDispatcher::WThreadWithDispatcher(const char* szName /*= "WThreadWithDispatcher"*/, WUInt32 uiStackSize /*= 128 * 1024*/)
  : WThread(szName, uiStackSize)
{
}

WThreadWithDispatcher::~WThreadWithDispatcher() = default;

void WThreadWithDispatcher::Dispatch(DispatchFunction&& delegate)
{
  W_LOCK(m_QueueMutex);
  m_ActiveQueue.PushBack(std::move(delegate));
}

void WThreadWithDispatcher::DispatchQueue()
{
  {
    W_LOCK(m_QueueMutex);
    std::swap(m_ActiveQueue, m_CurrentlyBeingDispatchedQueue);
  }

  for (const auto& pDelegate : m_CurrentlyBeingDispatchedQueue)
  {
    pDelegate();
  }

  m_CurrentlyBeingDispatchedQueue.Clear();
}
