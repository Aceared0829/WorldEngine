set (W_BUILD_UNITTESTS ON CACHE BOOL "Whether unit-test projects should be added to the solution")

W_requires(W_BUILD_UNITTESTS)

W_add_all_subdirs()