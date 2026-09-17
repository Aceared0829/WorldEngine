#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_EDITORPLUGINVISUALSCRIPT_LIB
#    define W_EDITORPLUGINVISUALSCRIPT_DLL W_DECL_EXPORT
#  else
#    define W_EDITORPLUGINVISUALSCRIPT_DLL W_DECL_IMPORT
#  endif
#else
#  define W_EDITORPLUGINVISUALSCRIPT_DLL
#endif
