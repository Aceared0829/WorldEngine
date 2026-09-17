W_cmake_init()

W_requires(W_CMAKE_PLATFORM_ANDROID)

W_requires(W_3RDPARTY_PERFETTO_SUPPORT)

# Build the Perfetto amalgamated SDK as a static library.

find_package(Threads REQUIRED)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
W_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_W_PREFIX NO_WARNINGS_AS_ERRORS)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_PERFETTO_SUPPORT)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/sdk")
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

