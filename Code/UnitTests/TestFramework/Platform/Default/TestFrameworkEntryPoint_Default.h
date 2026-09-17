#pragma once

#include <Foundation/Basics.h>

#ifndef W_TESTFRAMEWORK_ENTRY_POINT_CODE_INJECTION
#  define W_TESTFRAMEWORK_ENTRY_POINT_CODE_INJECTION
#endif

/// Macro to define the application entry point for all test applications
#define W_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)                    \
  /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */           \
  W_TESTFRAMEWORK_ENTRY_POINT_CODE_INJECTION                                             \
  W_APPLICATION_ENTRY_POINT_CODE_INJECTION                                               \
  int main(int argc, char** argv)                                                         \
  {                                                                                       \
    WTestSetup::InitTestFramework(szTestName, szNiceTestName, argc, (const char**)argv); \
    /* Execute custom init code here by using the BEGIN/END macros directly */

#define W_TESTFRAMEWORK_ENTRY_POINT_END()                        \
  while (WTestSetup::RunTests() == WTestAppRun::Continue)       \
  {                                                               \
  }                                                               \
  const WInt32 iFailedTests = WTestSetup::GetFailedTestCount(); \
  WTestSetup::DeInitTestFramework();                             \
  return iFailedTests;                                            \
  }

#define W_TESTFRAMEWORK_ENTRY_POINT(szTestName, szNiceTestName)             \
  W_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)             \
  /* Execute custom init code here by using the BEGIN/END macros directly */ \
  W_TESTFRAMEWORK_ENTRY_POINT_END()
