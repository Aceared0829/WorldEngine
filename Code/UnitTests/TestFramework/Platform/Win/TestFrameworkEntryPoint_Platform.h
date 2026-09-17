#pragma once

#include <Foundation/Platform/Win/Utils/MinWindows.h>
#define W_TESTFRAMEWORK_ENTRY_POINT_CODE_INJECTION                                             \
  extern "C"                                                                                    \
  {                                                                                             \
    _declspec(dllexport) WMinWindows::DWORD NvOptimusEnablement = 0x00000001;                  \
    _declspec(dllexport) WMinWindows::DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; \
  }

#include <TestFramework/Platform/Default/TestFrameworkEntryPoint_Default.h>
