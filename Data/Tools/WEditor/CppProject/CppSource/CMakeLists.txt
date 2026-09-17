#W-version 4
cmake_minimum_required(VERSION 3.21)

# Set CMake policies
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "4.1")
  cmake_policy(SET CMP0194 OLD)
endif()

project (CppProject VERSION 1.0 LANGUAGES C CXX)

if(PROJECT_IS_TOP_LEVEL)

	set (W_SDK_DIR "" CACHE PATH "The WorldEngine SDK root directory.")

	message(STATUS "W_SDK_DIR is set to '${W_SDK_DIR}'")

	# Where the SDK's binaries are. The plugin has to be built into the same directories, or the editor
	# does not find it. The editor passes them in through CMakeUserPresets.json, because an SDK can be
	# built into a directory other than the default one below.
	if (NOT W_OUTPUT_DIRECTORY_LIB)
		set (W_OUTPUT_DIRECTORY_LIB "${W_SDK_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files." FORCE)
	endif()

	if (NOT W_OUTPUT_DIRECTORY_DLL)
		set (W_OUTPUT_DIRECTORY_DLL "${W_SDK_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files." FORCE)
	endif()

	include("${W_SDK_DIR}/WCMakeConfig.cmake")
	get_property(W_CMAKE_RELPATH GLOBAL PROPERTY W_CMAKE_RELPATH)
	get_property(W_CMAKE_RELPATH_CODE GLOBAL PROPERTY W_CMAKE_RELPATH_CODE)

	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${W_SDK_DIR}/${W_CMAKE_RELPATH}")

	set(W_ROOT ${W_SDK_DIR})
	set_property(GLOBAL PROPERTY W_ROOT ${W_ROOT})

	include("WUtils")
	include("WPlatform")

	W_include_WExport()

	W_configure_external_project()

endif()

# Include optional user extensions for custom build configuration
# This file can be used to add custom CMake logic without modifying this template
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CMakeExtensions.txt")
	include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeExtensions.txt")
endif()

add_subdirectory("CppProjectPlugin")
add_subdirectory("CppProjectGame")
