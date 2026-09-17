W_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
# AssImp is in a sub-module, when it isn't checked out W_requires_assimp() doesn't exist, so early out here when it's not needed anyway
W_apply_build_filter(${PROJECT_NAME})

W_requires_assimp()

W_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  RendererCore

  PRIVATE
  ozz
  meshoptimizer
)

W_link_target_assimp(${PROJECT_NAME})
