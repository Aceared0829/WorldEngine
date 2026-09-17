#pragma once

#include <TestFramework/TestFrameworkDLL.h>

struct android_app;

int WAndroidMain(int argc, char** argv);

// A small wrapper class around the android message loop to wait for window creation before starting tests.
class W_TEST_DLL WAndroidTestApplication
{
public:
  WAndroidTestApplication(struct android_app* pApp);
  void HandleCmd(int32_t cmd);
  void AndroidRun();

  static void WAndroidHandleCmd(struct android_app* pApp, int32_t cmd);

private:
  struct android_app* m_pApp = nullptr;
  bool m_bStarted = false;
};
