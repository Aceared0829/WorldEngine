W_cmake_init()

W_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  GameEngine
  RendererCore
  Utilities
  ParticlePlugin
  ProcGenPlugin
  VisualScriptPlugin  
)

if (W_3RDPARTY_KRAUT_SUPPORT)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    KrautPlugin
  )
endif()

if (W_3RDPARTY_ANGELSCRIPT_SUPPORT)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    AngelScriptPlugin
  )
endif()

if (W_BUILD_RMLUI AND (W_CMAKE_PLATFORM_WINDOWS OR W_CMAKE_PLATFORM_LINUX))
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    RmlUiPlugin
  )
endif()

W_add_renderers(${PROJECT_NAME})

W_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)


