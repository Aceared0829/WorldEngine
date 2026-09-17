#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_CORE_LIB
#    define W_CORE_DLL W_DECL_EXPORT
#    define W_CORE_DLL_FRIEND W_DECL_EXPORT_FRIEND
#  else
#    define W_CORE_DLL W_DECL_IMPORT
#    define W_CORE_DLL_FRIEND W_DECL_IMPORT_FRIEND
#  endif
#else
#  define W_CORE_DLL
#  define W_CORE_DLL_FRIEND
#endif
