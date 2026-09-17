#pragma once

#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/Implementation/MessageLoop.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

class WIpcChannel;
struct IOContext;

class W_FOUNDATION_DLL WMessageLoop_win : public WMessageLoop
{
public:
  struct IOItem
  {
    W_DECLARE_POD_TYPE();

    WIpcChannel* pChannel;
    IOContext* pContext;
    DWORD uiBytesTransfered;
    DWORD uiError;
  };

public:
  WMessageLoop_win();
  ~WMessageLoop_win();

  HANDLE GetPort() const { return m_hPort; }

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(WInt32 iTimeout, WIpcChannel* pFilter) override;

  bool GetIOItem(WInt32 iTimeout, IOItem* pItem);
  bool ProcessInternalIOItem(const IOItem& item);
  bool MatchCompletedIOItem(WIpcChannel* pFilter, IOItem* pItem);

private:
  WDynamicArray<IOItem> m_CompletedIO;
  LONG m_iHaveWork = 0;
  HANDLE m_hPort = INVALID_HANDLE_VALUE;
};

using WMessageLoop_Platform = WMessageLoop_win;

#endif
