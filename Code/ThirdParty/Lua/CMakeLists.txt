W_cmake_init()



W_requires(W_3RDPARTY_LUA_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(LIBRARY ${PROJECT_NAME} NO_W_PREFIX NO_WARNINGS_AS_ERRORS ALL_SYMBOLS_VISIBLE)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_LUA_SUPPORT)

# Tell lua to use posix compliant APIs in case we are on a posix platform.
if(W_CMAKE_PLATFORM_POSIX)
  target_compile_definitions(${PROJECT_NAME} PRIVATE LUA_USE_POSIX)
endif()

if(W_CMAKE_COMPILER_MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /wd4701 /wd4702)
endif()
