W_cmake_init()

W_requires(W_3RDPARTY_TINYEXR_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_W_PREFIX)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_TINYEXR_SUPPORT)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
