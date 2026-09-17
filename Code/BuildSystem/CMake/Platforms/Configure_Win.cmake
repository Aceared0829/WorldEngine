include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: Windows")

set_property(GLOBAL PROPERTY W_CMAKE_PLATFORM_WINDOWS ON)
set_property(GLOBAL PROPERTY W_CMAKE_PLATFORM_WINDOWS_DESKTOP ON)
set_property(GLOBAL PROPERTY W_CMAKE_PLATFORM_SUPPORTS_VULKAN ON)
set_property(GLOBAL PROPERTY W_CMAKE_PLATFORM_SUPPORTS_D3D11 ON)
set_property(GLOBAL PROPERTY W_CMAKE_PLATFORM_SUPPORTS_WEBGPU ON)
set_property(GLOBAL PROPERTY W_CMAKE_PLATFORM_SUPPORTS_EDITOR ON)

if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    set(W_CMAKE_WINDOWS_SDK_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
else()
    set(W_CMAKE_WINDOWS_SDK_VERSION ${CMAKE_SYSTEM_VERSION})
    string(REGEX MATCHALL "\\." NUMBER_OF_DOTS "${W_CMAKE_WINDOWS_SDK_VERSION}")
    list(LENGTH NUMBER_OF_DOTS NUMBER_OF_DOTS)

    if(NUMBER_OF_DOTS EQUAL 2)
        set(W_CMAKE_WINDOWS_SDK_VERSION "${W_CMAKE_WINDOWS_SDK_VERSION}.0")
    endif()
endif()

set_property(GLOBAL PROPERTY W_CMAKE_WINDOWS_SDK_VERSION ${W_CMAKE_WINDOWS_SDK_VERSION})

# #####################################
# ## General settings
# #####################################
set(W_COMPILE_ENGINE_AS_DLL ON CACHE BOOL "Whether to compile the code as a shared libraries (DLL).")
mark_as_advanced(FORCE W_COMPILE_ENGINE_AS_DLL)

macro(W_platform_pull_properties)

	get_property(W_CMAKE_PLATFORM_WINDOWS GLOBAL PROPERTY W_CMAKE_PLATFORM_WINDOWS)
	get_property(W_CMAKE_PLATFORM_WINDOWS_UWP GLOBAL PROPERTY W_CMAKE_PLATFORM_WINDOWS_UWP)
	get_property(W_CMAKE_PLATFORM_WINDOWS_DESKTOP GLOBAL PROPERTY W_CMAKE_PLATFORM_WINDOWS_DESKTOP)
	get_property(W_CMAKE_WINDOWS_SDK_VERSION GLOBAL PROPERTY W_CMAKE_WINDOWS_SDK_VERSION)

endmacro()

macro (W_platformhook_set_build_flags_clang TARGET_NAME)
    # Disable the warning that clang doesn't support pragma optimize.
    target_compile_options(${TARGET_NAME} PRIVATE -Wno-ignored-pragma-optimize -Wno-pragma-pack)
endmacro()

macro(W_platform_detect_generator)
	string(FIND ${CMAKE_VERSION} "MSVC" VERSION_CONTAINS_MSVC)

	if(${VERSION_CONTAINS_MSVC} GREATER -1)
		message(STATUS "CMake was called from Visual Studio Open Folder workflow")
		set_property(GLOBAL PROPERTY W_CMAKE_INSIDE_VS ON)
	endif()

    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        # Visual Studio (All VS generators define MSVC)
        message(STATUS "Generator is MSVC (W_CMAKE_GENERATOR_MSVC)")

        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_MSVC ON)
        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_PREFIX "Vs")
        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_CONFIGURATION $<CONFIGURATION>)
    elseif(CMAKE_GENERATOR MATCHES "Ninja") # Ninja makefiles. Only makefile format supported by Visual Studio Open Folder
        message(STATUS "Buildsystem is Ninja (W_CMAKE_GENERATOR_NINJA)")

        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_NINJA ON)
        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_PREFIX "Ninja")
        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})
    else()
        message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Windows! Please extend W_platform_detect_generator()")
    endif()
endmacro()

macro (W_platformhook_make_windowapp TARGET_NAME)
    set_property(TARGET ${TARGET_NAME} PROPERTY WIN32_EXECUTABLE ON)
endmacro()

macro(W_platformhook_find_vulkan)
    if(W_CMAKE_ARCHITECTURE_64BIT AND W_CMAKE_ARCHITECTURE_X86)
        set(W_DXC_DIR "${W_ROOT}/Workspace/shared/DXC-WinX64-${W_CONFIG_DIRECTXSHADERCOMPILER_WINX64_VERSION}")
        W_download_and_extract("${W_CONFIG_DIRECTXSHADERCOMPILER_WINX64_URL}" "${W_DXC_DIR}" "DXC-WinX64-${W_CONFIG_DIRECTXSHADERCOMPILER_WINX64_VERSION}")
    else()
        message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(WVulkan DEFAULT_MSG W_DXC_DIR)

    if(W_CMAKE_ARCHITECTURE_64BIT AND W_CMAKE_ARCHITECTURE_X86)
        add_library(WVulkan::DXC SHARED IMPORTED)
        set_target_properties(WVulkan::DXC PROPERTIES IMPORTED_LOCATION "${W_DXC_DIR}/bin/x64/dxcompiler.dll")
        set_target_properties(WVulkan::DXC PROPERTIES IMPORTED_IMPLIB "${W_DXC_DIR}/lib/x64/dxcompiler.lib")
        set_target_properties(WVulkan::DXC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${W_DXC_DIR}/inc")
    else()
        message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
    endif() 
endmacro()

macro(W_platformhook_find_qt)
	if(W_CMAKE_COMPILER_CLANG)
		# The qt6 interface compile options contain msvc specific flags which don't exist for clang.
		set_target_properties(Qt6::Platform PROPERTIES INTERFACE_COMPILE_OPTIONS "")
		
		# Qt6 link options include '-NXCOMPAT' which does not exist on clang.
		get_target_property(QtLinkOptions Qt6::PlatformCommonInternal INTERFACE_LINK_OPTIONS)
		string(REPLACE "-NXCOMPAT;" "" QtLinkOptions "${QtLinkOptions}")
		set_target_properties(Qt6::PlatformCommonInternal PROPERTIES INTERFACE_LINK_OPTIONS "${QtLinkOptions}")
	endif()
endmacro()


macro(W_platformhook_download_qt)

	# Currently only implemented for x64
	if(W_CMAKE_ARCHITECTURE_64BIT)
		if(W_CMAKE_ARCHITECTURE_64BIT)
			set(W_SDK_VERSION "${W_CONFIG_QT_WINX64_VERSION}")
			set(W_SDK_URL "${W_CONFIG_QT_WINX64_URL}")
		endif()

		# Reset W_QT_DIR if it points to an auto-managed Qt package that no longer matches
		# the configured version, so the correct version gets downloaded automatically.
		# User-specified custom paths (not matching the "Qt6-" naming convention) are left alone.
		set(W_EXPECTED_QT_DIR "${CMAKE_BINARY_DIR}/../${W_SDK_VERSION}")
		if(NOT "${W_QT_DIR}" STREQUAL "${W_EXPECTED_QT_DIR}" AND "${W_QT_DIR}" MATCHES "Qt6-")
			set(W_QT_DIR "W_QT_DIR-NOTFOUND" CACHE PATH "Directory of the Qt installation" FORCE)
		endif()

		if((W_QT_DIR STREQUAL "W_QT_DIR-NOTFOUND") OR(W_QT_DIR STREQUAL ""))
			W_download_and_extract("${W_SDK_URL}" "${CMAKE_BINARY_DIR}/.." "${W_SDK_VERSION}")

			set(W_QT_DIR "${CMAKE_BINARY_DIR}/../${W_SDK_VERSION}" CACHE PATH "Directory of the Qt installation" FORCE)
		endif()
	endif()

endmacro()