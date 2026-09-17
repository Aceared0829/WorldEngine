#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERDX11_LIB
#    define W_RENDERERDX11_DLL W_DECL_EXPORT
#  else
#    define W_RENDERERDX11_DLL W_DECL_IMPORT
#  endif
#else
#  define W_RENDERERDX11_DLL
#endif


#define W_GAL_DX11_RELEASE(d3dobj) \
  do                                \
  {                                 \
    if ((d3dobj) != nullptr)        \
    {                               \
      (d3dobj)->Release();          \
      (d3dobj) = nullptr;           \
    }                               \
  } while (0)
