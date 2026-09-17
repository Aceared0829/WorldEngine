W_cmake_init()

W_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(LIBRARY ${PROJECT_NAME})

# No 'Network' component: the MCP server speaks plain sockets now, in the Mcp library, so that the same
# server can run inside the engine process, which has no Qt. Qt is still needed for the tools themselves.
W_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  Mcp
)

W_copy_plugin_bundle(${PROJECT_NAME} "Mcp")
