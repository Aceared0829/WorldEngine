W_cmake_init()


W_requires_d3d()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(LIBRARY ${PROJECT_NAME})

W_link_target_dx11(${PROJECT_NAME})

W_enable_strict_warnings(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  RendererFoundation
)

if(W_3RDPARTY_GLFW_SUPPORT)

  target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

endif()