#pragma once

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_ANGELSCRIPTPLUGIN_LIB
#    define W_ANGELSCRIPTPLUGIN_DLL W_DECL_EXPORT
#  else
#    define W_ANGELSCRIPTPLUGIN_DLL W_DECL_IMPORT
#  endif
#else
#  define W_ANGELSCRIPTPLUGIN_DLL
#endif

#define AS_CHECK(x)                                  \
  if (int res = (x); res < 0)                        \
  {                                                  \
    W_REPORT_FAILURE("AngelScript error: {}", res); \
  }

enum WAsUserData
{
  ScriptInstancePtr = 0,
  RttiPtr = 1,
  FuncFlags = 2,
};
