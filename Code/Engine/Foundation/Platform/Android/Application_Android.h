#pragma once

#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#if W_ENABLED(W_PLATFORM_ANDROID)

#  include <Foundation/Basics.h>
#  include <Foundation/Strings/String.h>

class WApplication;
struct AInputEvent;

class WAndroidApplication
{
public:
  WAndroidApplication(struct android_app* pApp, WApplication* pEzApp);
  ~WAndroidApplication();
  void AndroidRun();
  void HandleCmd(int32_t cmd);
  int32_t HandleInput(AInputEvent* pEvent);
  void HandleIdent(WInt32 iIdent);

private:
  struct android_app* m_pApp;
  WApplication* m_pEzApp;
  bool m_bStarted = false;
};

#endif
