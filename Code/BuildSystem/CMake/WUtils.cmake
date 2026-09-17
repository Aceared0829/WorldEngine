include(CheckIncludeFileCXX)

file(GLOB UTILS_FILES "${CMAKE_CURRENT_LIST_DIR}/CMakeUtils/*.cmake")

# automatically include all files in the CMakeUtils subfolder
foreach(UTILS_FILE ${UTILS_FILES})
	include("${UTILS_FILE}")
endforeach()

# #####################################
# ## W_pull_config_vars()
# #####################################
macro(W_pull_config_vars)
	get_property(W_BUILDTYPENAME_DEBUG GLOBAL PROPERTY W_BUILDTYPENAME_DEBUG)
	get_property(W_BUILDTYPENAME_DEV GLOBAL PROPERTY W_BUILDTYPENAME_DEV)
	get_property(W_BUILDTYPENAME_RELEASE GLOBAL PROPERTY W_BUILDTYPENAME_RELEASE)

	get_property(W_BUILDTYPENAME_DEBUG_UPPER GLOBAL PROPERTY W_BUILDTYPENAME_DEBUG_UPPER)
	get_property(W_BUILDTYPENAME_DEV_UPPER GLOBAL PROPERTY W_BUILDTYPENAME_DEV_UPPER)
	get_property(W_BUILDTYPENAME_RELEASE_UPPER GLOBAL PROPERTY W_BUILDTYPENAME_RELEASE_UPPER)

	get_property(W_DEV_BUILD_LINKERFLAGS GLOBAL PROPERTY W_DEV_BUILD_LINKERFLAGS)

	get_property(W_CMAKE_RELPATH GLOBAL PROPERTY W_CMAKE_RELPATH)
	get_property(W_CMAKE_RELPATH_CODE GLOBAL PROPERTY W_CMAKE_RELPATH_CODE)
	get_property(W_CONFIG_PATH_7ZA GLOBAL PROPERTY W_CONFIG_PATH_7ZA)

	get_property(W_CONFIG_QT_WINX64_URL GLOBAL PROPERTY W_CONFIG_QT_WINX64_URL)
	get_property(W_CONFIG_QT_WINX64_VERSION GLOBAL PROPERTY W_CONFIG_QT_WINX64_VERSION)

	get_property(W_CONFIG_DIRECTXSHADERCOMPILER_LINUXX64_VERSION GLOBAL PROPERTY W_CONFIG_DIRECTXSHADERCOMPILER_LINUXX64_VERSION)
	get_property(W_CONFIG_DIRECTXSHADERCOMPILER_LINUXX64_URL GLOBAL PROPERTY W_CONFIG_DIRECTXSHADERCOMPILER_LINUXX64_URL)
	get_property(W_CONFIG_DIRECTXSHADERCOMPILER_WINX64_VERSION GLOBAL PROPERTY W_CONFIG_DIRECTXSHADERCOMPILER_WINX64_VERSION)
	get_property(W_CONFIG_DIRECTXSHADERCOMPILER_WINX64_URL GLOBAL PROPERTY W_CONFIG_DIRECTXSHADERCOMPILER_WINX64_URL)

	get_property(W_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION GLOBAL PROPERTY W_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION)
	get_property(W_CONFIG_VULKAN_VALIDATIONLAYERS_ANDROID_URL GLOBAL PROPERTY W_CONFIG_VULKAN_VALIDATIONLAYERS_ANDROID_URL)
endmacro()

# #####################################
# ## W_pull_output_vars(LIB_OUTPUT_DIR DLL_OUTPUT_DIR)
# #####################################
macro(W_pull_output_vars LIB_OUTPUT_DIR DLL_OUTPUT_DIR)
	W_pull_all_vars()
	W_pull_config_vars()

	set(SUB_DIR "")
	set(PLATFORM_PREFIX "")
	set(PLATFORM_POSTFIX "")
	set(ARCH "x${W_CMAKE_ARCHITECTURE_POSTFIX}")

	# PLATFORM-TODO (build output path hook? add more variables?)
	if(W_CMAKE_PLATFORM_WINDOWS_UWP)
		# UWP has deployment problems if all applications output to the same path.
		set(SUB_DIR "/${TARGET_NAME}")
		set(PLATFORM_PREFIX "uwp_")

		if(${ARCH} STREQUAL "x32")
			set(ARCH "x86")
		endif()

		if(${ARCH} STREQUAL "xArm32")
			set(ARCH "arm")
		endif()

		if(${ARCH} STREQUAL "xArm64")
			set(ARCH "arm64")
		endif()

	elseif(W_CMAKE_PLATFORM_WINDOWS_DESKTOP)
		set(PLATFORM_POSTFIX "_win10")

	elseif(W_CMAKE_PLATFORM_WEB)
		set(PLATFORM_POSTFIX "_wasm")

	elseif(W_CMAKE_PLATFORM_ANDROID)
		set(PLATFORM_POSTFIX "_android")
	endif()

	string(TOLOWER ${W_CMAKE_GENERATOR_PREFIX} LOWER_GENERATOR_PREFIX)

	set(PRE_PATH "${W_CMAKE_PLATFORM_PREFIX}${W_OUTPUT_DIRECTORY_PLATFORM_POSTFIX}${W_CMAKE_GENERATOR_PREFIX}${W_CMAKE_COMPILER_POSTFIX}")
	set(OUTPUT_DEBUG "${PRE_PATH}${W_BUILDTYPENAME_DEBUG}${W_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")
	set(OUTPUT_RELEASE "${PRE_PATH}${W_BUILDTYPENAME_RELEASE}${W_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")
	set(OUTPUT_DEV "${PRE_PATH}${W_BUILDTYPENAME_DEV}${W_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")

	set(OUTPUT_DLL_DEBUG "${DLL_OUTPUT_DIR}/${OUTPUT_DEBUG}")
	set(OUTPUT_LIB_DEBUG "${LIB_OUTPUT_DIR}/${OUTPUT_DEBUG}")

	set(OUTPUT_DLL_RELEASE "${DLL_OUTPUT_DIR}/${OUTPUT_RELEASE}")
	set(OUTPUT_LIB_RELEASE "${LIB_OUTPUT_DIR}/${OUTPUT_RELEASE}")

	set(OUTPUT_DLL_DEV "${DLL_OUTPUT_DIR}/${OUTPUT_DEV}")
	set(OUTPUT_LIB_DEV "${LIB_OUTPUT_DIR}/${OUTPUT_DEV}")

endmacro()

# #####################################
# ## W_set_target_output_dirs(<target> <lib-output-dir> <dll-output-dir>)
# #####################################
function(W_set_target_output_dirs TARGET_NAME LIB_OUTPUT_DIR DLL_OUTPUT_DIR)
	if(W_DO_NOT_SET_OUTPUT_DIRS)
		return()
	endif()

	W_pull_output_vars("${LIB_OUTPUT_DIR}" "${DLL_OUTPUT_DIR}")

	# If we can't use generator expressions the non-generator expression version of the
	# output directory should point to the version matching CMAKE_BUILD_TYPE. This is the case for
	# add_custom_command BYPRODUCTS for example needed by Ninja.
	if("${CMAKE_BUILD_TYPE}" STREQUAL ${W_BUILDTYPENAME_DEBUG})
		set_target_properties(${TARGET_NAME} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEBUG}"
			LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEBUG}"
			ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_DEBUG}"
		)
	elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${W_BUILDTYPENAME_RELEASE})
		set_target_properties(${TARGET_NAME} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_RELEASE}"
			LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_RELEASE}"
			ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_RELEASE}"
		)
	elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${W_BUILDTYPENAME_DEV})
		set_target_properties(${TARGET_NAME} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEV}"
			LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEV}"
			ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_DEV}"
		)
	else()
		message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")
	endif()

	set_target_properties(${TARGET_NAME} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_${W_BUILDTYPENAME_DEBUG_UPPER} "${OUTPUT_DLL_DEBUG}"
		LIBRARY_OUTPUT_DIRECTORY_${W_BUILDTYPENAME_DEBUG_UPPER} "${OUTPUT_DLL_DEBUG}"
		ARCHIVE_OUTPUT_DIRECTORY_${W_BUILDTYPENAME_DEBUG_UPPER} "${OUTPUT_LIB_DEBUG}"
	)

	set_target_properties(${TARGET_NAME} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_${W_BUILDTYPENAME_RELEASE_UPPER} "${OUTPUT_DLL_RELEASE}"
		LIBRARY_OUTPUT_DIRECTORY_${W_BUILDTYPENAME_RELEASE_UPPER} "${OUTPUT_DLL_RELEASE}"
		ARCHIVE_OUTPUT_DIRECTORY_${W_BUILDTYPENAME_RELEASE_UPPER} "${OUTPUT_LIB_RELEASE}"
	)

	set_target_properties(${TARGET_NAME} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_${W_BUILDTYPENAME_DEV_UPPER} "${OUTPUT_DLL_DEV}"
		LIBRARY_OUTPUT_DIRECTORY_${W_BUILDTYPENAME_DEV_UPPER} "${OUTPUT_DLL_DEV}"
		ARCHIVE_OUTPUT_DIRECTORY_${W_BUILDTYPENAME_DEV_UPPER} "${OUTPUT_LIB_DEV}"
	)
endfunction()

# #####################################
# ## W_set_default_target_output_dirs(<target>)
# #####################################
function(W_set_default_target_output_dirs TARGET_NAME)
	W_set_target_output_dirs("${TARGET_NAME}" "${W_OUTPUT_DIRECTORY_LIB}" "${W_OUTPUT_DIRECTORY_DLL}")
endfunction()

# #####################################
# ## W_write_configuration_txt()
# #####################################
function(W_write_configuration_txt)
	if(W_NO_TXT_FILES)
		return()
	endif()
	# Clear Targets.txt and Tests.txt
	file(WRITE ${CMAKE_BINARY_DIR}/Targets.txt "")
	file(WRITE ${CMAKE_BINARY_DIR}/Tests.txt "")

	W_pull_all_vars()
	W_pull_config_vars()

	# Write configuration to file, as this is done at configure time we must pin the configuration in place (Dev is used because all build machines use this).
	file(WRITE ${CMAKE_BINARY_DIR}/Configuration.txt "")
	set(CONFIGURATION_DESC "${W_CMAKE_PLATFORM_PREFIX}${W_CMAKE_GENERATOR_PREFIX}${W_CMAKE_COMPILER_POSTFIX}${W_BUILDTYPENAME_DEV}${W_CMAKE_ARCHITECTURE_POSTFIX}")
	file(APPEND ${CMAKE_BINARY_DIR}/Configuration.txt ${CONFIGURATION_DESC})
endfunction()

# #####################################
# ## W_add_target_folder_as_include_dir(<target> <path-to-target>)
# #####################################
function(W_add_target_folder_as_include_dir TARGET_NAME TARGET_FOLDER)
	get_filename_component(PARENT_DIR ${TARGET_FOLDER} DIRECTORY)

	# target_include_directories(${TARGET_NAME} PRIVATE "${TARGET_FOLDER}")
	target_include_directories(${TARGET_NAME} PUBLIC "${PARENT_DIR}")
endfunction()

# #####################################
# ## W_set_common_target_definitions(<target>)
# #####################################
function(W_set_common_target_definitions TARGET_NAME)
	W_pull_all_vars()
	W_pull_config_vars()

	# set the BUILDSYSTEM_COMPILE_ENGINE_AS_DLL definition
	if(W_COMPILE_ENGINE_AS_DLL)
		target_compile_definitions(${TARGET_NAME} PUBLIC BUILDSYSTEM_COMPILE_ENGINE_AS_DLL)
	endif()

	target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_MAJOR=${W_CMAKE_SDKVERSION_MAJOR})
	target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_MINOR=${W_CMAKE_SDKVERSION_MINOR})
	target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_PATCH=${W_CMAKE_SDKVERSION_PATCH})

	set(ORIGINAL_BUILD_TYPE "$<IF:$<STREQUAL:${W_CMAKE_GENERATOR_CONFIGURATION},${W_BUILDTYPENAME_DEBUG}>,Debug,$<IF:$<STREQUAL:${W_CMAKE_GENERATOR_CONFIGURATION},${W_BUILDTYPENAME_DEV}>,Dev,Shipping>>")

	# set the BUILDSYSTEM_BUILDTYPE definition
	target_compile_definitions(${TARGET_NAME} PRIVATE "BUILDSYSTEM_BUILDTYPE=\"${ORIGINAL_BUILD_TYPE}\"")
	target_compile_definitions(${TARGET_NAME} PUBLIC "BUILDSYSTEM_BUILDTYPE_${ORIGINAL_BUILD_TYPE}")

	# set the BUILDSYSTEM_BUILDING_XYZ_LIB definition
	string(TOUPPER ${TARGET_NAME} PROJECT_NAME_UPPER)
	target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_BUILDING_${PROJECT_NAME_UPPER}_LIB)

	if(W_BUILD_VULKAN AND W_CMAKE_PLATFORM_SUPPORTS_VULKAN)
		target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_ENABLE_VULKAN_SUPPORT)
	endif()

	if(W_BUILD_D3D11 AND (W_CMAKE_PLATFORM_SUPPORTS_D3D11 OR W_BUILD_EXPERIMENTAL_DXVK))
		target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_ENABLE_D3D11_SUPPORT)
	endif()

	if(W_DEFAULT_RENDERER)
		target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_DEFAULT_RENDERER="${W_DEFAULT_RENDERER}")
	endif()

	if(W_BUILD_EXPERIMENTAL_WEBGPU)
		target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_ENABLE_WEBGPU_SUPPORT)
	endif()

	# set the BUILDSYSTEM_MIN_REQUIRED_SSE_LEVEL
	if (W_MIN_REQUIRED_SSE_LEVEL STREQUAL "SSE2")
		target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_MIN_REQUIRED_SSE_LEVEL=20)
	elseif (W_MIN_REQUIRED_SSE_LEVEL STREQUAL "SSE41")
		target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_MIN_REQUIRED_SSE_LEVEL=41)
	elseif (W_MIN_REQUIRED_SSE_LEVEL STREQUAL "AVX")
		target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_MIN_REQUIRED_SSE_LEVEL=50)
	endif()

	# on Windows, make sure to use the Unicode API
	target_compile_definitions(${TARGET_NAME} PUBLIC UNICODE _UNICODE)
endfunction()

# #####################################
# ## W_set_project_ide_folder(<target> <path-to-target>)
# #####################################
function(W_set_project_ide_folder TARGET_NAME PROJECT_SOURCE_DIR)
	# globally enable sorting targets into folders in IDEs
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	get_filename_component(PARENT_FOLDER ${PROJECT_SOURCE_DIR} PATH)
	get_filename_component(FOLDER_NAME ${PARENT_FOLDER} NAME)

	set(IDE_FOLDER "${FOLDER_NAME}")

	set(CMAKE_SOURCE_DIR_PREFIX "${CMAKE_SOURCE_DIR}/")
	cmake_path(IS_PREFIX CMAKE_SOURCE_DIR_PREFIX ${PROJECT_SOURCE_DIR} NORMALIZE FOLDER_IN_TREE)
	if(FOLDER_IN_TREE)
		set(IDE_FOLDER "")
		string(REPLACE ${CMAKE_SOURCE_DIR_PREFIX} "" PARENT_FOLDER ${PROJECT_SOURCE_DIR})

		get_filename_component(PARENT_FOLDER "${PARENT_FOLDER}" PATH)
		get_filename_component(FOLDER_NAME "${PARENT_FOLDER}" NAME)

		get_filename_component(PARENT_FOLDER2 "${PARENT_FOLDER}" PATH)

		while(NOT ${PARENT_FOLDER2} STREQUAL "")
			set(IDE_FOLDER "${FOLDER_NAME}/${IDE_FOLDER}")

			get_filename_component(PARENT_FOLDER "${PARENT_FOLDER}" PATH)
			get_filename_component(FOLDER_NAME "${PARENT_FOLDER}" NAME)

			get_filename_component(PARENT_FOLDER2 "${PARENT_FOLDER}" PATH)
		endwhile()
	endif()

	get_property(W_SUBMODULE_MODE GLOBAL PROPERTY W_SUBMODULE_MODE)

	if(W_SUBMODULE_MODE)
		set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "${W_SUBMODULE_ROOT_IDE_FOLDER}/${IDE_FOLDER}")
	else()
		set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER ${IDE_FOLDER})
	endif()
endfunction()

# #####################################
# ## W_add_output_W_prefix(<target>)
# #####################################
function(W_add_output_W_prefix TARGET_NAME)
	set_target_properties(${TARGET_NAME} PROPERTIES IMPORT_PREFIX "W")
	set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "W")
endfunction()

# #####################################
# ## W_make_windowapp(<target>)
#
# Turns the target application from a 'console app' into a 'window app', which means it doesn't
# show a command prompt with the log output on systems that differentiate between the these app types.
# #####################################
function(W_make_windowapp TARGET_NAME)
	set_property(TARGET ${TARGET_NAME} PROPERTY WIN32_EXECUTABLE ON)
	target_compile_definitions(${TARGET_NAME} PRIVATE W_WINDOWAPP=1)

	if (COMMAND W_platformhook_make_windowapp)
		W_platformhook_make_windowapp(${TARGET_NAME})
	endif()
endfunction()

# #####################################
# ## W_gather_subfolders(<abs-path-to-folder> <out-sub-folders>)
# #####################################
function(W_gather_subfolders START_FOLDER RESULT_FOLDERS)
	set(ALL_FILES "")
	set(ALL_DIRS "")

	file(GLOB_RECURSE ALL_FILES RELATIVE "${START_FOLDER}" "${START_FOLDER}/*")

	foreach(FILE ${ALL_FILES})
		get_filename_component(FILE_PATH ${FILE} DIRECTORY)

		list(APPEND ALL_DIRS ${FILE_PATH})
	endforeach()

	list(REMOVE_DUPLICATES ALL_DIRS)

	set(${RESULT_FOLDERS} ${ALL_DIRS} PARENT_SCOPE)
endfunction()

# #####################################
# ## W_glob_source_files(<path-to-folder> <out-files>)
# #####################################
function(W_glob_source_files ROOT_DIR RESULT_ALL_SOURCES)
	file(GLOB_RECURSE RELEVANT_FILES
		"${ROOT_DIR}/*.cpp"
		"${ROOT_DIR}/*.cc"
		"${ROOT_DIR}/*.h"
		"${ROOT_DIR}/*.hpp"
		"${ROOT_DIR}/*.inl"
		"${ROOT_DIR}/*.c"
		"${ROOT_DIR}/*.cs"
		"${ROOT_DIR}/*.ui"
		"${ROOT_DIR}/*.qrc"
		"${ROOT_DIR}/*.def"
		"${ROOT_DIR}/*.ico"
		"${ROOT_DIR}/*.rc"
		"${ROOT_DIR}/*.s"
		"${ROOT_DIR}/*.asm"
		"${ROOT_DIR}/*.cmake"
		"${ROOT_DIR}/*.natvis"
		"${ROOT_DIR}/*.txt"
		"${ROOT_DIR}/*.md"
		"${ROOT_DIR}/*.WPluginBundle"
		"${ROOT_DIR}/*.ddl"
		"${ROOT_DIR}/*.WPermVar"
		"${ROOT_DIR}/*.WShader"
		"${ROOT_DIR}/*.WShaderTemplate"
		"${ROOT_DIR}/*.rml"
		"${ROOT_DIR}/*.rcss"
	)

	set(${RESULT_ALL_SOURCES} ${RELEVANT_FILES} PARENT_SCOPE)
endfunction()

# #####################################
# ## W_add_all_subdirs()
# #####################################
function(W_add_all_subdirs)
	# find all cmake files below this directory
	file(GLOB SUB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")

	foreach(VAR ${SUB_DIRS})
		get_filename_component(RES ${VAR} DIRECTORY)

		add_subdirectory(${RES})
	endforeach()
endfunction()

# #####################################
# ## W_cmake_init()
# #####################################
macro(W_cmake_init)
	W_pull_all_vars()
endmacro()

# #####################################
# ## W_requires(<variable>)
# #####################################
macro(W_requires)
	if(${ARGC} EQUAL 0)
		return()
	endif()

	set(ALL_ARGS "${ARGN}")

	foreach(arg IN LISTS ALL_ARGS)
		if(NOT ${arg})
			return()
		endif()
	endforeach()
endmacro()

# #####################################
# ## W_requires_one_of(<variable1> (<variable2>) (<variable3>) ...)
# #####################################
macro(W_requires_one_of)
	if(${ARGC} EQUAL 0)
		message(FATAL_ERROR "W_requires_one_of needs at least one argument")
	endif()

	set(ALL_ARGS "${ARGN}")

	set(VALID 0)

	foreach(arg IN LISTS ALL_ARGS)
		if(${arg})
			set(VALID 1)
		endif()
	endforeach()

	if(NOT VALID)
		return()
	endif()
endmacro()

# #####################################
# ## W_requires_desktop()
# #####################################
macro(W_requires_desktop)
	W_requires_one_of(W_CMAKE_PLATFORM_WINDOWS_DESKTOP W_CMAKE_PLATFORM_LINUX)
endmacro()

# #####################################
# ## W_requires_editor()
# #####################################
macro(W_requires_editor)
	W_requires_qt()
	W_requires_renderer()
	if(NOT W_CMAKE_PLATFORM_SUPPORTS_EDITOR)
		return()
	endif()
endmacro()

# #####################################
# ## W_add_external_folder(<project-number>)
# #####################################
function(W_add_external_projects_folder PROJECT_NUMBER)
	set(CACHE_VAR_NAME "W_EXTERNAL_PROJECT${PROJECT_NUMBER}")

	set(${CACHE_VAR_NAME} "" CACHE PATH "A folder outside the W repository that should be parsed for CMakeLists.txt files to include projects into the W solution.")

	set(CACHE_VAR_VALUE ${${CACHE_VAR_NAME}})

	if(NOT CACHE_VAR_VALUE)
		return()
	endif()

	set_property(GLOBAL PROPERTY "GATHER_EXTERNAL_PROJECTS" TRUE)
	add_subdirectory(${CACHE_VAR_VALUE} "${CMAKE_BINARY_DIR}/ExternalProject${PROJECT_NUMBER}")
	set_property(GLOBAL PROPERTY "GATHER_EXTERNAL_PROJECTS" FALSE)
endfunction()

# #####################################
# ## W_init_projects()
# #####################################
# By defining W_SOURCE_DIR before calling this function
# you can change the location that will be scanned for projects.
function(W_init_projects)
	# find all init.cmake files below this directory or the given source directory if any.
	if(W_SOURCE_DIR)
		file(GLOB_RECURSE INIT_FILES "${W_SOURCE_DIR}/init.cmake")
	else()
		file(GLOB_RECURSE INIT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/init.cmake")
	endif()

	foreach(INIT_FILE ${INIT_FILES})
		message(STATUS "Including '${INIT_FILE}'")
		include("${INIT_FILE}")
	endforeach()
endfunction()

# #####################################
# ## W_finalize_projects()
# #####################################
# By defining W_SOURCE_DIR before calling this function
# you can change the location that will be scanned for projects.
function(W_finalize_projects)
	# find all finalize.cmake files below this directory or the given source directory if any.
	if(W_SOURCE_DIR)
		file(GLOB_RECURSE FINALIZE_FILES "${W_SOURCE_DIR}/finalize.cmake")
	else()
		file(GLOB_RECURSE FINALIZE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/finalize.cmake")
	endif()

	# TODO: also finalize external projects
	foreach(FINALIZE_FILE ${FINALIZE_FILES})
		message(STATUS "Including '${FINALIZE_FILE}'")
		include("${FINALIZE_FILE}")
	endforeach()
endfunction()

# #####################################
# ## W_build_filter_init()
# #####################################

# The build filter is intended to only build a subset of WorldEngine.
# The build filters are configured through cmake files in the 'BuildFilters' directory.
function(W_build_filter_init)
	file(GLOB_RECURSE FILTER_FILES "${W_ROOT}/Code/BuildSystem/CMake/BuildFilters/*.BuildFilter")

	get_property(W_BUILD_FILTER_NAMES GLOBAL PROPERTY W_BUILD_FILTER_NAMES)

	foreach(VAR ${FILTER_FILES})
		cmake_path(GET VAR STEM FILTER_NAME)
		list(APPEND W_BUILD_FILTER_NAMES "${FILTER_NAME}")

		message(STATUS "Reading build filter '${FILTER_NAME}'")
		include(${VAR})
	endforeach()

	list(REMOVE_DUPLICATES W_BUILD_FILTER_NAMES)
	set_property(GLOBAL PROPERTY W_BUILD_FILTER_NAMES ${W_BUILD_FILTER_NAMES})

	set(W_BUILD_FILTER "Everything" CACHE STRING "Which projects to include in the solution.")

	get_property(W_BUILD_FILTER_NAMES GLOBAL PROPERTY W_BUILD_FILTER_NAMES)
	set_property(CACHE W_BUILD_FILTER PROPERTY STRINGS ${W_BUILD_FILTER_NAMES})
	set_property(GLOBAL PROPERTY W_BUILD_FILTER_SELECTED ${W_BUILD_FILTER})
endfunction()

# #####################################
# ## W_project_build_filter_index(<PROJECT_NAME> <OUT_INDEX>)
# #####################################
function(W_project_build_filter_index PROJECT_NAME OUT_INDEX)
	get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY W_BUILD_FILTER_SELECTED)
	set(FILTER_VAR_NAME "W_BUILD_FILTER_${SELECTED_FILTER_NAME}")
	get_property(FILTER_PROJECTS GLOBAL PROPERTY ${FILTER_VAR_NAME})

	list(LENGTH FILTER_PROJECTS LIST_LENGTH)

	if(${LIST_LENGTH} GREATER 1)
		list(FIND FILTER_PROJECTS ${PROJECT_NAME} FOUND_INDEX)
		set(${OUT_INDEX} ${FOUND_INDEX} PARENT_SCOPE)
	else()
		set(${OUT_INDEX} 0 PARENT_SCOPE)
	endif()
endfunction()

# #####################################
# ## W_apply_build_filter(<PROJECT_NAME>)
# #####################################
macro(W_apply_build_filter PROJECT_NAME)
	W_project_build_filter_index(${PROJECT_NAME} PROJECT_INDEX)

	if(${PROJECT_INDEX} EQUAL -1)
		get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY W_BUILD_FILTER_SELECTED)
		message(STATUS "Project '${PROJECT_NAME}' excluded by build filter '${SELECTED_FILTER_NAME}'.")
		return()
	endif()
endmacro()

# #####################################
# ## W_set_build_types()
# #####################################
function(W_set_build_types)
	W_pull_config_vars()

	set(CMAKE_CONFIGURATION_TYPES "${W_BUILDTYPENAME_DEBUG};${W_BUILDTYPENAME_DEV};${W_BUILDTYPENAME_RELEASE}" CACHE STRING "" FORCE)

	if (W_BUILDTYPE_ONLY)
		set(CMAKE_CONFIGURATION_TYPES "${W_BUILDTYPE_ONLY}" CACHE STRING "" FORCE)
	endif()

	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "W build config types" FORCE)

	set(CMAKE_BUILD_TYPE ${W_BUILDTYPENAME_DEV} CACHE STRING "The default build type")
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})

	set(CMAKE_EXE_LINKER_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_EXE_LINKER_FLAGS_${W_BUILDTYPENAME_DEV_UPPER} ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_EXE_LINKER_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_EXE_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_SHARED_LINKER_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_SHARED_LINKER_FLAGS_${W_BUILDTYPENAME_DEV_UPPER} ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_SHARED_LINKER_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_SHARED_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_STATIC_LINKER_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_STATIC_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_STATIC_LINKER_FLAGS_${W_BUILDTYPENAME_DEV_UPPER} ${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_STATIC_LINKER_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_STATIC_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_MODULE_LINKER_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_MODULE_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_MODULE_LINKER_FLAGS_${W_BUILDTYPENAME_DEV_UPPER} ${CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_MODULE_LINKER_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_MODULE_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

	# Fix for cl : Command line warning D9025 : overriding '/Ob0' with '/Ob1'
	# We are adding /Ob1 to debug inside ./CMakeUtils/WUtilsCppFlags.cmake
	if(W_CMAKE_COMPILER_GCC)
		string(REPLACE "/Ob0" "/Ob1" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
		string(REPLACE "/Ob0" "/Ob1" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
	endif ()

	set(CMAKE_CXX_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_CXX_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS_${W_BUILDTYPENAME_DEV_UPPER} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_CXX_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_C_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_C_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_C_FLAGS_${W_BUILDTYPENAME_DEV_UPPER} ${CMAKE_C_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_C_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_C_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_CSharp_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_CSharp_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_CSharp_FLAGS_${W_BUILDTYPENAME_DEV_UPPER} ${CMAKE_CSharp_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_CSharp_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_CSharp_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_RC_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_RC_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_RC_FLAGS_${W_BUILDTYPENAME_DEV_UPPER} ${CMAKE_RC_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_RC_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_RC_FLAGS_RELEASE} CACHE STRING "" FORCE)

	mark_as_advanced(FORCE CMAKE_EXE_LINKER_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_EXE_LINKER_FLAGS_${W_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_EXE_LINKER_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_SHARED_LINKER_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_SHARED_LINKER_FLAGS_${W_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_SHARED_LINKER_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_STATIC_LINKER_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_STATIC_LINKER_FLAGS_${W_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_STATIC_LINKER_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_MODULE_LINKER_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_MODULE_LINKER_FLAGS_${W_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_MODULE_LINKER_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_CXX_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_CXX_FLAGS_${W_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_CXX_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_C_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_C_FLAGS_${W_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_C_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_CSharp_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_CSharp_FLAGS_${W_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_CSharp_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_RC_FLAGS_${W_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_RC_FLAGS_${W_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_RC_FLAGS_${W_BUILDTYPENAME_RELEASE_UPPER})
endfunction()

# #####################################
# ## W_create_link(<source> <destination-folder> <destination-name>)
# #####################################
function(W_create_link SOURCE DEST_FOLDER DEST_NAME)
	if(NOT EXISTS "${DEST_FOLDER}")
		file(MAKE_DIRECTORY ${DEST_FOLDER})
	endif()

	# Set DESTINATION to the full path of the link.
	set(DESTINATION "${DEST_FOLDER}/${DEST_NAME}")

	# We re-create the link every time because it could become a dead link when shared between workspaces.
	if(EXISTS "${DESTINATION}")
		file(REMOVE ${DESTINATION})
	endif()

	file(CREATE_LINK ${SOURCE} ${DESTINATION} RESULT OUT_RESULT SYMBOLIC)

	if (NOT ${OUT_RESULT} EQUAL 0)
		if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
		    # On Windows, fall back to creation directory junctions as those don't require admin access.
			file(TO_NATIVE_PATH ${SOURCE} SOURCE)
			file(TO_NATIVE_PATH ${DESTINATION} DESTINATION)

			execute_process(
				COMMAND "cmd" "/C" "mklink" "/D" "/J" "${DESTINATION}" "${SOURCE}" RESULT_VARIABLE OUT_RESULT2
			)
			if(NOT OUT_RESULT2 EQUAL 0)
				message(FATAL_ERROR "Could not create symlink '${DESTINATION}' pointing to '${SOURCE}': ${OUT_RESULT2}")
			endif()
		else()
			message(FATAL_ERROR "Failed to run: file(CREATE_LINK ${SOURCE} ${DEST_FOLDER}/${DEST_NAME} RESULT OUT_RESULT SYMBOLIC) \nRe-run with admin rights:\n${OUT_RESULT}")
		endif()
	endif ()
endfunction()

# #####################################
# ## W_download_and_extract(<url-to-download> <dest-folder-path> <dest-filename-without-extension>)
# #####################################
function(W_download_and_extract URL DEST_FOLDER DEST_FILENAME)
	if(${URL} MATCHES ".tar.gz$")
		set(PKG_TYPE "tar.gz")
	elseif(${URL} MATCHES ".tar.xz$")
		set(PKG_TYPE "tar.xz")
	else()
		get_filename_component(PKG_TYPE ${URL} LAST_EXT)
		string(REGEX REPLACE "^\\." "" PKG_TYPE "${PKG_TYPE}")
	endif()

	set(FULL_FILENAME "${DEST_FILENAME}.${PKG_TYPE}")
	set(PKG_FILE "${DEST_FOLDER}/${FULL_FILENAME}")
	set(EXTRACT_MARKER "${PKG_FILE}.extracted")

	if(EXISTS "${EXTRACT_MARKER}")
		return()
	endif()

	# if the "URL" is actually a file path
	if(NOT "${URL}" MATCHES "http*")
		set(PKG_FILE "${URL}")
	endif()

	if(NOT EXISTS "${PKG_FILE}")
		message(STATUS "Downloading '${FULL_FILENAME}'...")
		file(DOWNLOAD ${URL} "${PKG_FILE}" SHOW_PROGRESS STATUS DOWNLOAD_STATUS)

		list(GET DOWNLOAD_STATUS 0 DOWNLOAD_STATUS_CODE)

		if(NOT DOWNLOAD_STATUS_CODE EQUAL 0)
			message(FATAL_ERROR "Download failed: ${DOWNLOAD_STATUS}")
			return()
		endif()
	endif()

	W_pull_config_vars()

	message(STATUS "Extracting '${FULL_FILENAME}'...")

	if(${PKG_TYPE} MATCHES "7z")
		set(FULL_7ZA_PATH "${W_ROOT}/${W_CONFIG_PATH_7ZA}")
		execute_process(COMMAND "${FULL_7ZA_PATH}"
			x "${PKG_FILE}"
			-aoa
			WORKING_DIRECTORY "${DEST_FOLDER}"
			COMMAND_ERROR_IS_FATAL ANY
			RESULT_VARIABLE CMD_STATUS)
	else()
		execute_process(COMMAND ${CMAKE_COMMAND}
			-E tar -xf "${PKG_FILE}"
			WORKING_DIRECTORY "${DEST_FOLDER}"
			COMMAND_ERROR_IS_FATAL ANY
			RESULT_VARIABLE CMD_STATUS)
	endif()

	if(NOT CMD_STATUS EQUAL 0)
		message(FATAL_ERROR "Extracting package '${FULL_FILENAME}' failed.")
		return()
	endif()

	file(TOUCH ${EXTRACT_MARKER})
endfunction()

# #####################################
# ## W_get_export_location()
# #####################################
function(W_get_export_location DST_VAR)
	W_pull_config_vars()
	W_pull_output_vars("" "${W_OUTPUT_DIRECTORY_DLL}")

	if(GENERATOR_IS_MULTI_CONFIG OR (CMAKE_GENERATOR MATCHES "Visual Studio"))
		set("${DST_VAR}" "${W_OUTPUT_DIRECTORY_DLL}/WExport.cmake" PARENT_SCOPE)
	else()
		if("${CMAKE_BUILD_TYPE}" STREQUAL ${W_BUILDTYPENAME_DEBUG})
			set("${DST_VAR}" "${W_OUTPUT_DIRECTORY_DLL}/${OUTPUT_DEBUG}/WExport.cmake" PARENT_SCOPE)
		elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${W_BUILDTYPENAME_RELEASE})
			set("${DST_VAR}" "${W_OUTPUT_DIRECTORY_DLL}/${OUTPUT_RELEASE}/WExport.cmake" PARENT_SCOPE)
		elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${W_BUILDTYPENAME_DEV})
			set("${DST_VAR}" "${W_OUTPUT_DIRECTORY_DLL}/${OUTPUT_DEV}/WExport.cmake" PARENT_SCOPE)
		else()
			message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")
		endif()
	endif()
endfunction()

# #####################################
# ## W_package_files(TARGET_NAME SRC_FOLDER DST_FOLDER)
# #####################################
#
# Embeds all files in SRC_FOLDER (recursively) into the application package (e.g. APK or WASM)
# and puts it into the virtual folder DST_FOLDER.
#
# It is platform-specific whether this does anyting.
# Internally it just forwards to W_platformhook_package_files.
# #####################################
function(W_package_files TARGET_NAME SRC_FOLDER DST_FOLDER)

	if (COMMAND W_platformhook_package_files)
		W_platformhook_package_files(${TARGET_NAME} ${SRC_FOLDER} ${DST_FOLDER})
	endif()

endfunction()


# #####################################
# ## W_copy_plugin_bundle(TARGET_NAME FILE_NAME)
# #####################################
#
# Copies the given file with the .WPluginBundle extension from the current source directory into the target directory.
#
# #####################################
function(W_copy_plugin_bundle TARGET_NAME FILE_NAME)

	add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${FILE_NAME}.WPluginBundle" $<TARGET_FILE_DIR:${TARGET_NAME}>
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)

endfunction()
