W_requires(W_CMAKE_PLATFORM_LINUX)

set (W_3RDPARTY_TRACELOGGING_LTTNG_SUPPORT OFF CACHE BOOL "Whether to add support for tracelogging via lttng.")
mark_as_advanced(FORCE W_3RDPARTY_TRACELOGGING_LTTNG_SUPPORT)