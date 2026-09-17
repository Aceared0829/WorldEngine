W_cmake_init()

W_requires(W_3RDPARTY_RECAST_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Recast
  GameEngine
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Utilities
)

W_copy_plugin_bundle(${PROJECT_NAME} "AI")
