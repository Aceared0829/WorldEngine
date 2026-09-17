W_cmake_init()

W_requires_qt()
W_requires(W_3RDPARTY_ADS_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ToolsFoundation
  ads
  Core
)

W_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network Svg COPY_DLLS)

