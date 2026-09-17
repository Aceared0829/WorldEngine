W_cmake_init()

W_requires_renderer()
W_requires_desktop()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(LIBRARY ${PROJECT_NAME})

W_link_target_dx11(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

W_copy_plugin_bundle(${PROJECT_NAME} "SampleGame")
