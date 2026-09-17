#pragma once

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_FMODPLUGIN_LIB
#    define W_FMODPLUGIN_DLL W_DECL_EXPORT
#  else
#    define W_FMODPLUGIN_DLL W_DECL_IMPORT
#  endif
#else
#  define W_FMODPLUGIN_DLL
#endif

// Forward declarations

namespace FMOD
{
  namespace Studio
  {
    class Bank;
    class System;
    class EventInstance;
    class EventDescription;
  } // namespace Studio

  class System;
} // namespace FMOD
