W_requires(W_CMAKE_PLATFORM_ANDROID)

set (W_3RDPARTY_PERFETTO_SUPPORT ON CACHE BOOL "Whether to add support for Perfetto tracing on Android.")
mark_as_advanced(FORCE W_3RDPARTY_PERFETTO_SUPPORT)
