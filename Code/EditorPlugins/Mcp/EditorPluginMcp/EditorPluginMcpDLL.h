#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_EDITORPLUGINMCP_LIB
#    define W_EDITORPLUGINMCP_DLL W_DECL_EXPORT
#  else
#    define W_EDITORPLUGINMCP_DLL W_DECL_IMPORT
#  endif
#else
#  define W_EDITORPLUGINMCP_DLL
#endif
