W_cmake_init()

W_requires_desktop()
W_requires_vulkan()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

W_create_target(LIBRARY ${PROJECT_NAME})

if (W_CMAKE_PLATFORM_WINDOWS)
    W_link_target_dx11(${PROJECT_NAME})
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  RendererCore
  SPIRV-Reflect
)

W_link_target_dxc(${PROJECT_NAME})

if (W_CMAKE_PLATFORM_WINDOWS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()

if(W_CMAKE_COMPILER_CLANG OR W_CMAKE_COMPILER_GCC)
    if(W_CMAKE_PLATFORM_WINDOWS_DESKTOP AND W_CMAKE_ARCHITECTURE_64BIT)
        set_source_files_properties("${W_VULKAN_DIR}/Source/SPIRV-Reflect/spirv_reflect.c" PROPERTIES COMPILE_FLAGS "-Wno-switch")
    elseif(W_CMAKE_PLATFORM_LINUX AND W_CMAKE_ARCHITECTURE_64BIT)
        set_source_files_properties("${W_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.c" PROPERTIES COMPILE_FLAGS "-Wno-switch")
    else()
        message(FATAL_ERROR "TODO: SPIR-V is not yet supported on this platform and/or architecture.")
    endif()
endif()

if(W_CMAKE_COMPILER_CLANG)
   target_compile_options(${PROJECT_NAME} PRIVATE -fms-extensions)
endif()
