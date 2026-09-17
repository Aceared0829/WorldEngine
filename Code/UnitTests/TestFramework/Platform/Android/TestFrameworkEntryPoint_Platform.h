#pragma once

#include <TestFramework/Platform/Android/AndroidTestApplication.h>
#include <android/log.h>

#define W_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)                                                \
  int WAndroidMain(int argc, char** argv);                                                                           \
  W_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                                           \
  extern "C" void android_main(struct android_app* app)                                                               \
  {                                                                                                                   \
    WAndroidTestApplication androidApp(app);                                                                         \
    androidApp.AndroidRun();                                                                                          \
    const WInt32 iFailedTests = WTestSetup::GetFailedTestCount();                                                   \
    WTestSetup::DeInitTestFramework();                                                                               \
    __android_log_print(ANDROID_LOG_ERROR, "WorldEngine", "Test framework exited with return code: '%d'", iFailedTests); \
  }                                                                                                                   \
                                                                                                                      \
  int WAndroidMain(int argc, char** argv)                                                                            \
  {                                                                                                                   \
    WTestSetup::InitTestFramework(szTestName, szNiceTestName, argc, (const char**)argv);                             \
    /* Execute custom init code here by using the BEGIN/END macros directly */

#define W_TESTFRAMEWORK_ENTRY_POINT_END() \
  return 0;                                \
  }

#define W_TESTFRAMEWORK_ENTRY_POINT(szTestName, szNiceTestName)             \
  W_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)             \
  /* Execute custom init code here by using the BEGIN/END macros directly */ \
  W_TESTFRAMEWORK_ENTRY_POINT_END()
