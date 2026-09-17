#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Communication/IpcChannel.h>
#  include <Foundation/Platform/Win/MessageLoop_Platform.h>
#  include <Foundation/Platform/Win/PipeChannel_Platform.h>

WMessageLoop_win::WMessageLoop_win()
{
  m_hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
  W_ASSERT_DEBUG(m_hPort != INVALID_HANDLE_VALUE, "Failed to create IO completion port!");
}

WMessageLoop_win::~WMessageLoop_win()
{
  StopUpdateThread();
  CloseHandle(m_hPort);
}


bool WMessageLoop_win::WaitForMessages(WInt32 iTimeout, WIpcChannel* pFilter)
{
  if (iTimeout < 0)
    iTimeout = INFINITE;

  IOItem item;
  if (!MatchCompletedIOItem(pFilter, &item))
  {
    if (!GetIOItem(iTimeout, &item))
      return false;

    if (ProcessInternalIOItem(item))
      return true;
  }

  if (item.pContext->pChannel != NULL)
  {
    if (pFilter != NULL && item.pChannel != pFilter)
    {
      m_CompletedIO.PushBack(item);
    }
    else
    {
      W_ASSERT_DEBUG(item.pContext->pChannel == item.pChannel, "");
      static_cast<WPipeChannel_win*>(item.pChannel)->OnIOCompleted(item.pContext, item.uiBytesTransfered, item.uiError);
    }
  }
  return true;
}

bool WMessageLoop_win::GetIOItem(WInt32 iTimeout, IOItem* pItem)
{
  memset(pItem, 0, sizeof(*pItem));
  ULONG_PTR key = 0;
  OVERLAPPED* overlapped = NULL;
  if (!GetQueuedCompletionStatus(m_hPort, &pItem->uiBytesTransfered, &key, &overlapped, iTimeout))
  {
    // nothing queued
    if (overlapped == NULL)
      return false;

    pItem->uiError = GetLastError();
    pItem->uiBytesTransfered = 0;
  }

  pItem->pChannel = reinterpret_cast<WIpcChannel*>(key);
  pItem->pContext = reinterpret_cast<IOContext*>(overlapped);
  return true;
}

bool WMessageLoop_win::ProcessInternalIOItem(const IOItem& item)
{
  if (reinterpret_cast<WMessageLoop_win*>(item.pContext) == this && reinterpret_cast<WMessageLoop_win*>(item.pChannel) == this)
  {
    // internal notification
    W_ASSERT_DEBUG(item.uiBytesTransfered == 0, "");
    InterlockedExchange(&m_iHaveWork, 0);
    return true;
  }
  return false;
}

bool WMessageLoop_win::MatchCompletedIOItem(WIpcChannel* pFilter, IOItem* pItem)
{
  for (WUInt32 i = 0; i < m_CompletedIO.GetCount(); i++)
  {
    if (pFilter == NULL || m_CompletedIO[i].pChannel == pFilter)
    {
      *pItem = m_CompletedIO[i];
      m_CompletedIO.RemoveAtAndCopy(i);
      return true;
    }
  }
  return false;
}

void WMessageLoop_win::WakeUp()
{
  if (InterlockedExchange(&m_iHaveWork, 1))
  {
    // already running
    return;
  }
  // wake up the loop
  BOOL res = PostQueuedCompletionStatus(m_hPort, 0, reinterpret_cast<ULONG_PTR>(this), reinterpret_cast<OVERLAPPED*>(this));
  W_IGNORE_UNUSED(res);
  W_ASSERT_DEBUG(res, "Could not PostQueuedCompletionStatus: {0}", WArgErrorCode(GetLastError()));
}

#endif
