W_cmake_init()

W_requires_vulkan()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_W_PREFIX)

# Add the include directories
target_include_directories(${PROJECT_NAME} 
    PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
