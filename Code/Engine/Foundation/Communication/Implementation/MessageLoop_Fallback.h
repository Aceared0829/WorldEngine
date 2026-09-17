#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Implementation/MessageLoop.h>

class W_FOUNDATION_DLL WMessageLoop_Fallback : public WMessageLoop
{
public:
  WMessageLoop_Fallback();
  ~WMessageLoop_Fallback();

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(WInt32 iTimeout, WIpcChannel* pFilter) override;

private:
};
