W_cmake_init()

# Writing external process dumps is supported on Windows and Linux.
W_requires_one_of(W_CMAKE_PLATFORM_WINDOWS W_CMAKE_PLATFORM_LINUX)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(APPLICATION ${PROJECT_NAME})

W_add_output_W_prefix(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Foundation
)
