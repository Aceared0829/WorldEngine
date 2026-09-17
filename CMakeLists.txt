cmake_minimum_required(VERSION 3.20)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-source builds are not supported. Please choose a different binary directory.")
	return()
endif()

# Re-run CMake when HEAD changes (e.g. after git pull), but not on git fetch.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git/HEAD")
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/.git/HEAD")
endif()

# Set CMake policies
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "4.1")
  cmake_policy(SET CMP0194 OLD)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/WCMakeConfig.cmake")
get_property(W_CMAKE_RELPATH GLOBAL PROPERTY W_CMAKE_RELPATH)
get_property(W_CMAKE_RELPATH_CODE GLOBAL PROPERTY W_CMAKE_RELPATH_CODE)

# Required to find cmake modules like 'FindDirectX.cmake'
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/${W_CMAKE_RELPATH}")

include("WUtils")

file(RELATIVE_PATH W_SUBMODULE_PREFIX_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set_property(GLOBAL PROPERTY W_SUBMODULE_PREFIX_PATH ${W_SUBMODULE_PREFIX_PATH})

if(W_SUBMODULE_PREFIX_PATH STREQUAL "")
	set(W_SUBMODULE_MODE FALSE)
else()
	set(W_SUBMODULE_MODE TRUE)

	if (NOT DEFINED W_SUBMODULE_ROOT_IDE_FOLDER)
		set(W_SUBMODULE_ROOT_IDE_FOLDER "WorldEngine")
	endif()
endif()

set_property(GLOBAL PROPERTY W_SUBMODULE_MODE ${W_SUBMODULE_MODE})

# Set a global variable pointing to the root of the W source tree
set(W_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set_property(GLOBAL PROPERTY W_ROOT ${W_ROOT})

W_build_filter_init()

W_detect_project_name(DETECTED_NAME)

if(NOT W_SUBMODULE_MODE)
	W_detect_languages()

	set(W_SOLUTION_NAME ${DETECTED_NAME} CACHE STRING "The name of the solution")
	project(${W_SOLUTION_NAME}
		VERSION 1.0
		DESCRIPTION "An open-source game engine"
		HOMEPAGE_URL "https://ezengine.net"
		LANGUAGES ${W_LANGUAGES})

	# Enable CTest support for unit testing
	enable_testing()
endif()

include("WPlatform")

W_set_build_types()
W_check_build_type()
W_write_configuration_txt()

if(W_CUSTOM_TOOLCHAIN_FILE)
	message(STATUS "Including W_CUSTOM_TOOLCHAIN_FILE: '${W_CUSTOM_TOOLCHAIN_FILE}'")
	include("${W_CUSTOM_TOOLCHAIN_FILE}")
endif()

W_pull_all_vars()

set(W_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Code")

W_find_qt()
W_init_projects()

get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY W_BUILD_FILTER_SELECTED)
if (NOT ${SELECTED_FILTER_NAME} MATCHES "Everything")
	set(CMAKE_SUPPRESS_REGENERATION true)
endif()
add_subdirectory(${W_CMAKE_RELPATH_CODE})
add_subdirectory(Data)

W_add_external_projects_folder("_1")
W_add_external_projects_folder("_2")
W_add_external_projects_folder("_3")

if(NOT W_SUBMODULE_MODE)
	if(TARGET Editor)
		# sets the Editor project as the default startup project in Visual Studio
		set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Editor)

		get_property(EXTERNAL_PROJECTS GLOBAL PROPERTY "EXTERNAL_PROJECTS")
		if(EXTERNAL_PROJECTS)
		  message(STATUS "External Projects (WEditor): ${EXTERNAL_PROJECTS}")
		  add_dependencies(Editor ${EXTERNAL_PROJECTS})

		  message(STATUS "External Projects (WPlayer): ${EXTERNAL_PROJECTS}")
		  add_dependencies(Player ${EXTERNAL_PROJECTS})
		  endif()		
	endif()
endif()

# finalizers may run into problems in split solutions
# however, they aren't needed there, since CI will always build all projects
# and finalizers are typically only needed to set up build dependencies
if (${SELECTED_FILTER_NAME} MATCHES "Everything")
W_finalize_projects()
endif()

get_property(EXPORT_PROJECTS GLOBAL PROPERTY "EXPORT_PROJECTS")
W_get_export_location(W_EXPORT_LOCATION)
export(TARGETS ${EXPORT_PROJECTS} FILE "${W_EXPORT_LOCATION}")

set(EXPORT_INFO "
set(EXPINP_OUTPUT_DIRECTORY_DLL ${W_OUTPUT_DIRECTORY_DLL})
set(EXPINP_OUTPUT_DIRECTORY_LIB ${W_OUTPUT_DIRECTORY_LIB})
set(EXPINP_BINARY_DIR ${CMAKE_BINARY_DIR})
set(EXPINP_SOURCE_DIR ${CMAKE_SOURCE_DIR})
")

file(WRITE "${W_OUTPUT_DIRECTORY_DLL}/WExportInfo.cmake" ${EXPORT_INFO})
