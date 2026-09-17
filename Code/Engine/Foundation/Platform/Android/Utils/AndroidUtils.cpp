#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_ANDROID)
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <android_native_app_glue.h>

android_app* WAndroidUtils::s_app;
JavaVM* WAndroidUtils::s_vm;
jobject WAndroidUtils::s_na;
WEvent<WAndroidInputEvent&> WAndroidUtils::s_InputEvent;
WEvent<WInt32> WAndroidUtils::s_AppCommandEvent;

void WAndroidUtils::SetAndroidApp(android_app* app)
{
  s_app = app;
  SetAndroidJavaVM(s_app->activity->vm);
  SetAndroidNativeActivity(s_app->activity->clazz);
}

android_app* WAndroidUtils::GetAndroidApp()
{
  return s_app;
}

void WAndroidUtils::SetAndroidJavaVM(JavaVM* vm)
{
  s_vm = vm;
}

JavaVM* WAndroidUtils::GetAndroidJavaVM()
{
  return s_vm;
}

void WAndroidUtils::SetAndroidNativeActivity(jobject nativeActivity)
{
  s_na = nativeActivity;
}

jobject WAndroidUtils::GetAndroidNativeActivity()
{
  return s_na;
}

#endif
