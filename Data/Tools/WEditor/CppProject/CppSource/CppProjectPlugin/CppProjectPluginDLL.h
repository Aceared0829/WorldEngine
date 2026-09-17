#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_CPPPROJECTPLUGIN_LIB
#    define W_CPPPROJECTPLUGIN_DLL W_DECL_EXPORT
#  else
#    define W_CPPPROJECTPLUGIN_DLL W_DECL_IMPORT
#  endif
#else
#  define W_CPPPROJECTPLUGIN_DLL
#endif
