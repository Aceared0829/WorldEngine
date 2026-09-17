#pragma once

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RMLUIPLUGIN_LIB
#    define W_RMLUIPLUGIN_DLL W_DECL_EXPORT
#  else
#    define W_RMLUIPLUGIN_DLL W_DECL_IMPORT
#  endif
#else
#  define W_RMLUIPLUGIN_DLL
#endif

#ifndef RMLUI_USE_CUSTOM_RTTI
#  define RMLUI_USE_CUSTOM_RTTI
#endif
