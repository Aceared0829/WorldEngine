W_cmake_init()

W_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(APPLICATION ${PROJECT_NAME})

W_make_windowapp(${PROJECT_NAME})

W_add_output_W_prefix(${PROJECT_NAME})

W_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network Svg)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
)

add_dependencies(${PROJECT_NAME}
  EditorEngineProcess
  EditorPluginScene
  EditorPluginAssets
  TexConv
)

if (W_BUILD_FMOD)
	add_dependencies(${PROJECT_NAME} EditorPluginFmod FmodPlugin)
endif()

if (W_BUILD_PHYSX)
	add_dependencies(${PROJECT_NAME} EditorPluginPhysX PhysXPlugin)
endif()
