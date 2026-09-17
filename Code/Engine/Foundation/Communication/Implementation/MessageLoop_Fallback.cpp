#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/MessageLoop_Fallback.h>
#include <Foundation/Communication/IpcChannel.h>

WMessageLoop_Fallback::WMessageLoop_Fallback() = default;

WMessageLoop_Fallback::~WMessageLoop_Fallback()
{
  StopUpdateThread();
}

void WMessageLoop_Fallback::WakeUp()
{
  // nothing to do
}

bool WMessageLoop_Fallback::WaitForMessages(WInt32 iTimeout, WIpcChannel* pFilter)
{
  W_IGNORE_UNUSED(pFilter);

  // nothing to do

  if (iTimeout < 0)
  {
    // if timeout is 'indefinite' wait a little
    WThreadUtils::YieldTimeSlice();
  }

  return false;
}
