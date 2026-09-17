W_cmake_init()

W_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(APPLICATION ${PROJECT_NAME})

# Package required data files for deployment
# This ensures the application has access to shaders and sample data
W_package_files(${PROJECT_NAME} "${W_ROOT}/Data/Samples/ShaderExplorer" "Data/Samples/ShaderExplorer")
W_package_files(${PROJECT_NAME} "${W_ROOT}/Data/Base/Shaders" "Data/Base/Shaders")

# Package shader cache if available (Vulkan shaders for Android/Web platforms)
if(EXISTS "${W_ROOT}/Output/ShaderCache/VULKAN")
    W_package_files(${PROJECT_NAME} "${W_ROOT}/Output/ShaderCache/VULKAN" "Output/ShaderCache/VULKAN")
endif()

if(W_CMAKE_PLATFORM_ANDROID)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
        InspectorPlugin
        # To support Fileserve we need to link against the plugin which does unfortunately force loading of it so Fileserve can't be disabled at runtime.
        # See comments in ShaderExplorer.h for details.
        # FileservePlugin
    )
endif()

W_add_renderers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  RendererCore
)
