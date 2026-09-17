#pragma once

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERDOCPLUGIN_LIB
#    define W_RENDERDOCPLUGIN_DLL W_DECL_EXPORT
#  else
#    define W_RENDERDOCPLUGIN_DLL W_DECL_IMPORT
#  endif
#else
#  define W_RENDERDOCPLUGIN_DLL
#endif
