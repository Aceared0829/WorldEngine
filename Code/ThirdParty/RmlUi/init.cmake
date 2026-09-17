### RmlUi
set (W_BUILD_RMLUI ON CACHE BOOL "Whether support for RmlUi should be added")
mark_as_advanced(FORCE W_BUILD_RMLUI)

macro(W_requires_rmlui)
  W_requires_one_of(W_CMAKE_PLATFORM_WINDOWS W_CMAKE_PLATFORM_LINUX)
  W_requires(W_BUILD_RMLUI)
  if (W_CMAKE_PLATFORM_WINDOWS_UWP)
    return()
  endif()
endmacro()