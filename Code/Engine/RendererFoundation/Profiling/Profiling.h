#pragma once

#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct GPUTimingScope;

#if W_ENABLED(W_USE_PROFILING) || defined(W_DOCS)

/// Sets profiling marker and GPU timings for the current scope.
class W_RENDERERFOUNDATION_DLL WProfilingScopeAndMarker : public WProfilingScope
{
public:
  static GPUTimingScope* Start(WGALCommandEncoder* pCommandEncoder, const char* szName);
  static void Stop(WGALCommandEncoder* pCommandEncoder, GPUTimingScope*& ref_pTimingScope);

  WProfilingScopeAndMarker(WGALCommandEncoder* pCommandEncoder, const char* szName);

  ~WProfilingScopeAndMarker();

protected:
  WGALCommandEncoder* m_pCommandEncoder;
  GPUTimingScope* m_pTimingScope;
};

/// Profiles the current scope using the given name and also inserts a marker with the given command encoder.
#  define W_PROFILE_AND_MARKER(GALCommandEncoder, ScopeName)                                                \
    WProfilingScopeAndMarker W_PP_CONCAT(_WProfilingScope, W_SOURCE_LINE)(GALCommandEncoder, ScopeName); \
    W_TRACY_PROFILE_SCOPE(ScopeName)

#else

#  define W_PROFILE_AND_MARKER(GALCommandEncoder, ScopeName) /*empty*/

#endif
