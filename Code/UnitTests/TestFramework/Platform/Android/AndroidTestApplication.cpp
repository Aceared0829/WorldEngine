#include <TestFramework/TestFrameworkPCH.h>

#if W_ENABLED(W_PLATFORM_ANDROID)

#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Android/Utils/AndroidJni.h>
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <Foundation/Utilities/CommandLineUtils.h>
#  include <TestFramework/Platform/Android/AndroidTestApplication.h>
#  include <TestFramework/Utilities/TestSetup.h>
#  include <android/log.h>
#  include <android/native_activity.h>
#  include <android_native_app_glue.h>

WAndroidTestApplication::WAndroidTestApplication(struct android_app* pApp)
  : m_pApp(pApp)
{
  pApp->userData = this;
  pApp->onAppCmd = WAndroidHandleCmd;
  WAndroidUtils::SetAndroidApp(pApp);
}

void WAndroidTestApplication::HandleCmd(int32_t cmd)
{
  switch (cmd)
  {
    case APP_CMD_INIT_WINDOW:
      if (m_pApp->window != nullptr)
      {
        // Retrieve command line arguments from Intent extras.
        WDynamicArray<WString> args;
        WDynamicArray<const char*> argv;
        {
          WJniAttachment jni;
          WJniObject activity = jni.GetActivity();
          WJniObject intent = activity.Call<WJniObject>("getIntent");
          if (!intent.IsNull())
          {
            WJniString argsExtra = intent.Call<WJniString>("getStringExtra", WJniString("args"));
            if (!argsExtra.IsNull())
            {
              const char* szArgs = argsExtra.GetData();
              __android_log_print(ANDROID_LOG_INFO, "WorldEngine", "Received arguments from Intent: '%s'", szArgs);
              WCommandLineUtils::SplitCommandLineString(szArgs, false, args, argv);
            }
          }
        }

        WAndroidMain(static_cast<int>(argv.GetCount()), argv.IsEmpty() ? nullptr : const_cast<char**>(argv.GetData()));
        m_bStarted = true;

        int width = ANativeWindow_getWidth(m_pApp->window);
        int height = ANativeWindow_getHeight(m_pApp->window);
        WLog::Info("Init Window: {}x{}", width, height);
      }
      break;
    default:
      break;
  }
}
void WAndroidTestApplication::AndroidRun()
{
  bool bRun = true;
  while (true)
  {
    struct android_poll_source* pSource = nullptr;
    int iIdent = 0;
    int iEvents = 0;
    while ((iIdent = ALooper_pollAll(0, nullptr, &iEvents, (void**)&pSource)) >= 0)
    {
      if (pSource != nullptr)
        pSource->process(m_pApp, pSource);
    }

    // APP_CMD_INIT_WINDOW has not triggered yet. Engine is not yet started.
    if (!m_bStarted)
      continue;

    if (bRun && WTestSetup::RunTests() != WTestAppRun::Continue)
    {
      bRun = false;
      ANativeActivity_finish(m_pApp->activity);
    }
    if (m_pApp->destroyRequested)
    {
      break;
    }
  }
}

void WAndroidTestApplication::WAndroidHandleCmd(struct android_app* pApp, int32_t cmd)
{
  WAndroidTestApplication* pAndroidApp = static_cast<WAndroidTestApplication*>(pApp->userData);
  pAndroidApp->HandleCmd(cmd);
}

#endif
