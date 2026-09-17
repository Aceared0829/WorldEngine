W_cmake_init()

W_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(APPLICATION ${PROJECT_NAME})

W_make_windowapp(${PROJECT_NAME})

W_add_output_W_prefix(${PROJECT_NAME})

W_add_renderers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  EditorEngineProcessFramework
)

# For SetProcessDpiAwareness
if (W_CMAKE_PLATFORM_WINDOWS_DESKTOP)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Shcore.lib
  )
endif()

