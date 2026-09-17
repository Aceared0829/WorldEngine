include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: Linux")

set_property(GLOBAL PROPERTY W_CMAKE_PLATFORM_LINUX ON)
set_property(GLOBAL PROPERTY W_CMAKE_PLATFORM_POSIX ON)
set_property(GLOBAL PROPERTY W_CMAKE_PLATFORM_SUPPORTS_VULKAN ON)

# #####################################
# ## General settings
# #####################################
set(W_COMPILE_ENGINE_AS_DLL ON CACHE BOOL "Whether to compile the code as a shared libraries (DLL).")
mark_as_advanced(FORCE W_COMPILE_ENGINE_AS_DLL)

# #####################################
# ## Experimental Editor support on Linux
# #####################################
set (W_EXPERIMENTAL_EDITOR_ON_LINUX OFF CACHE BOOL "Wether or not to build the editor on linux")

if (W_EXPERIMENTAL_EDITOR_ON_LINUX)
    set_property(GLOBAL PROPERTY W_CMAKE_PLATFORM_SUPPORTS_EDITOR ON)
endif()

macro(W_platform_pull_properties)

    get_property(W_CMAKE_PLATFORM_LINUX GLOBAL PROPERTY W_CMAKE_PLATFORM_LINUX)

endmacro()

macro(W_platformhook_set_build_flags_clang TARGET_NAME)
	target_compile_options(${TARGET_NAME} PRIVATE -fPIC)

	# Look for the super fast ld compatible linker called "mold". If present we want to use it.
	find_program(MOLD_PATH "mold")

	# We want to use the llvm linker lld by default
	# Unless the user has specified a different linker
	get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)

	if("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY")
		if(NOT("${CMAKE_EXE_LINKER_FLAGS}" MATCHES "fuse-ld="))
			if(MOLD_PATH)
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=${MOLD_PATH}")
			else()
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=lld")
			endif()
		endif()

		# Reporting missing symbols at linktime
		target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
	elseif("${TARGET_TYPE}" STREQUAL "EXECUTABLE")
		if(NOT("${CMAKE_SHARED_LINKER_FLAGS}" MATCHES "fuse-ld="))
			if(MOLD_PATH)
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=${MOLD_PATH}")
			else()
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=lld")
			endif()
		endif()

		# Reporting missing symbols at linktime
		target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
	endif()
endmacro()

macro(W_platformhook_set_application_properties TARGET_NAME)

    # We need to link against pthread and rt last or linker errors will occur.
	target_link_libraries(${TARGET_NAME} PRIVATE pthread rt)

	# Set RPATH so the executable can find shared libraries in its own directory
	set_target_properties(${TARGET_NAME} PROPERTIES
		BUILD_RPATH "$ORIGIN"
		INSTALL_RPATH "$ORIGIN"
	)

endmacro()

macro(W_platform_detect_generator)
    if(CMAKE_GENERATOR MATCHES "Unix Makefiles") # Unix Makefiles (for QtCreator etc.)
        message(STATUS "Buildsystem is Make (W_CMAKE_GENERATOR_MAKE)")

        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_MAKE ON)
        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_PREFIX "Make")
        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

    elseif(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
        message(STATUS "Buildsystem is Ninja (W_CMAKE_GENERATOR_NINJA)")

        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_NINJA ON)
        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_PREFIX "Ninja")
        set_property(GLOBAL PROPERTY W_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})
    else()
        message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Linux! Please extend W_platform_detect_generator()")
    endif()
endmacro()


macro(W_platformhook_set_library_properties TARGET_NAME)
    # c = libc.so (the C standard library)
    # m = libm.so (the C standard library math portion)
    # pthread = libpthread.so (thread support)
    # rt = librt.so (compiler runtime functions)
    target_link_libraries(${TARGET_NAME} PRIVATE pthread rt c m)

    get_property(W_CMAKE_COMPILER_GCC GLOBAL PROPERTY W_CMAKE_COMPILER_GCC)
    if(W_CMAKE_COMPILER_GCC)
        # Workaround for: https://bugs.launchpad.net/ubuntu/+source/gcc-5/+bug/1568899
	target_link_libraries(${TARGET_NAME} PRIVATE -lgcc)
    endif()

    # Set RPATH so shared libraries can find other shared libraries in the same directory
    set_target_properties(${TARGET_NAME} PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endmacro()

macro(W_platformhook_find_vulkan)
    if(W_CMAKE_ARCHITECTURE_64BIT AND W_CMAKE_ARCHITECTURE_X86)
        set(W_DXC_DIR "${W_ROOT}/Workspace/shared/DXC-LinuxX64-${W_CONFIG_DIRECTXSHADERCOMPILER_LINUXX64_VERSION}")
        W_download_and_extract("${W_CONFIG_DIRECTXSHADERCOMPILER_LINUXX64_URL}" "${W_DXC_DIR}" "DXC-LinuxX64-${W_CONFIG_DIRECTXSHADERCOMPILER_LINUXX64_VERSION}")
    else()
        message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(WVulkan DEFAULT_MSG W_DXC_DIR)
    
	if(W_CMAKE_ARCHITECTURE_64BIT AND W_CMAKE_ARCHITECTURE_X86)
		add_library(WVulkan::DXC SHARED IMPORTED)
		set_target_properties(WVulkan::DXC PROPERTIES IMPORTED_LOCATION "${W_DXC_DIR}/lib/libdxcompiler.so")
		set_target_properties(WVulkan::DXC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${W_DXC_DIR}/include/dxc")
	else()
		message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
	endif()
    
endmacro()
