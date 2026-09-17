#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERCORE_LIB
#    define W_RENDERERCORE_DLL W_DECL_EXPORT
#  else
#    define W_RENDERERCORE_DLL W_DECL_IMPORT
#  endif
#else
#  define W_RENDERERCORE_DLL
#endif

#define W_EMBED_FONT_FILE W_ON
