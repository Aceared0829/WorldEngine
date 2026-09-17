#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_TESTFRAMEWORK_LIB
#    define W_TEST_DLL W_DECL_EXPORT
#  else
#    define W_TEST_DLL W_DECL_IMPORT
#  endif
#else
#  define W_TEST_DLL
#endif

enum class WTestAppRun
{
  Continue,
  Quit
};
