

set (W_BUILD_SAMPLES ON CACHE BOOL "Whether sample projects should be added to the solution")

W_requires(W_BUILD_SAMPLES)

W_add_all_subdirs()