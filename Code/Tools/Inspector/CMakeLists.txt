W_cmake_init()

W_requires_desktop()
W_requires_qt()

W_requires(W_3RDPARTY_ADS_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(APPLICATION ${PROJECT_NAME})

W_make_windowapp(${PROJECT_NAME})

W_add_output_W_prefix(${PROJECT_NAME})

W_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Core
  RendererCore
  GuiFoundation
  ads
)
