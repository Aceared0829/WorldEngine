#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_ANDROID)

#  include <Foundation/Application/Application.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Android/Application_Android.h>
#  include <android/log.h>
#  include <android_native_app_glue.h>

static void WAndroidHandleCmd(struct android_app* pApp, int32_t cmd)
{
  WAndroidApplication* pAndroidApp = static_cast<WAndroidApplication*>(pApp->userData);
  pAndroidApp->HandleCmd(cmd);
}

static int32_t WAndroidHandleInput(struct android_app* pApp, AInputEvent* pEvent)
{
  WAndroidApplication* pAndroidApp = static_cast<WAndroidApplication*>(pApp->userData);
  return pAndroidApp->HandleInput(pEvent);
}

WAndroidApplication::WAndroidApplication(struct android_app* pApp, WApplication* pEzApp)
  : m_pApp(pApp)
  , m_pEzApp(pEzApp)
{
  pApp->userData = this;
  pApp->onAppCmd = WAndroidHandleCmd;
  pApp->onInputEvent = WAndroidHandleInput;
  // #TODO: acquire sensors, set app->onAppCmd, set app->onInputEvent
}

WAndroidApplication::~WAndroidApplication() {}

void WAndroidApplication::AndroidRun()
{
  bool bRun = true;
  while (true)
  {
    struct android_poll_source* pSource = nullptr;
    int iIdent = 0;
    int iEvents = 0;
    while ((iIdent = ALooper_pollOnce(m_bStarted ? 0 : -1, nullptr, &iEvents, (void**)&pSource)) >= 0)
    {
      if (pSource != nullptr)
        pSource->process(m_pApp, pSource);

      HandleIdent(iIdent);
    }

    // APP_CMD_INIT_WINDOW has not triggered yet. Engine is not yet started.
    if (!m_bStarted)
      continue;

    if (bRun)
    {
      m_pEzApp->Run();

      if (m_pEzApp->ShouldApplicationQuit())
      {
        bRun = false;
        ANativeActivity_finish(m_pApp->activity);
      }
    }

    if (m_pApp->destroyRequested)
    {
      break;
    }
  }
}

void WAndroidApplication::HandleCmd(int32_t cmd)
{
  switch (cmd)
  {
    case APP_CMD_INIT_WINDOW:
      if (m_pApp->window != nullptr)
      {
        W_VERIFY(WRun_Startup(m_pEzApp).Succeeded(), "Failed to startup engine");
        m_bStarted = true;

        int width = ANativeWindow_getWidth(m_pApp->window);
        int height = ANativeWindow_getHeight(m_pApp->window);
        WLog::Info("Init Window: {}x{}", width, height);
      }
      break;
    case APP_CMD_TERM_WINDOW:
      m_pEzApp->QuitApplication();
      break;
    default:
      break;
  }
  WAndroidUtils::s_AppCommandEvent.Broadcast(cmd);
}

int32_t WAndroidApplication::HandleInput(AInputEvent* pEvent)
{
  WAndroidInputEvent event;
  event.m_pEvent = pEvent;
  event.m_bHandled = false;

  WAndroidUtils::s_InputEvent.Broadcast(event);
  return event.m_bHandled ? 1 : 0;
}

void WAndroidApplication::HandleIdent(WInt32 iIdent)
{
  // #TODO:
}

W_FOUNDATION_DLL void WAndroidRun(struct android_app* pApp, WApplication* pEzApp)
{
  WAndroidApplication androidApp(pApp, pEzApp);

  // This call will loop until APP_CMD_INIT_WINDOW is emitted which triggers WRun_Startup
  androidApp.AndroidRun();

  WRun_Shutdown(pEzApp);

  const int iReturnCode = pEzApp->GetReturnCode();
  if (iReturnCode != 0)
  {
    const char* szReturnCode = pEzApp->TranslateReturnCode();
    if (szReturnCode != nullptr && szReturnCode[0] != '\0')
      __android_log_print(ANDROID_LOG_ERROR, "WorldEngine", "Return Code: '%s'", szReturnCode);
  }
}

#endif
