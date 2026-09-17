# #####################################
# ## W_include_WExport()
# #####################################

macro(W_include_WExport)
	# Create a modified version of the WExport.cmake file,
	# where the absolute paths to the original locations are replaced
	# with the absolute paths to this installation
	W_get_export_location(EXP_FILE)
	set(IMP_FILE "${CMAKE_BINARY_DIR}/WExport.cmake")
	set(EXPINFO_FILE "${W_OUTPUT_DIRECTORY_DLL}/WExportInfo.cmake")

	# read the file that contains the original paths
	include(${EXPINFO_FILE})

	# read the WExport file into a string
	file(READ ${EXP_FILE} IMP_CONTENT)

	# replace the original paths with our paths
	string(REPLACE ${EXPINP_OUTPUT_DIRECTORY_DLL} ${W_OUTPUT_DIRECTORY_DLL} IMP_CONTENT "${IMP_CONTENT}")
	string(REPLACE ${EXPINP_OUTPUT_DIRECTORY_LIB} ${W_OUTPUT_DIRECTORY_LIB} IMP_CONTENT "${IMP_CONTENT}")
	string(REPLACE ${EXPINP_SOURCE_DIR} ${W_SDK_DIR} IMP_CONTENT "${IMP_CONTENT}")

	# write the modified WExport file to disk
	file(WRITE ${IMP_FILE} "${IMP_CONTENT}")

	# include the modified file, so that the CMake targets become known
	include(${IMP_FILE})
endmacro()

# #####################################
# ## W_configure_external_project()
# #####################################
macro(W_configure_external_project)

	if (W_SDK_DIR STREQUAL "")
		file(RELATIVE_PATH W_SUBMODULE_PREFIX_PATH ${CMAKE_SOURCE_DIR} ${W_SDK_DIR})
	else()
		set(W_SUBMODULE_PREFIX_PATH "")
	endif()

	set_property(GLOBAL PROPERTY W_SUBMODULE_PREFIX_PATH ${W_SUBMODULE_PREFIX_PATH})

	if(W_SUBMODULE_PREFIX_PATH STREQUAL "")
		set(W_SUBMODULE_MODE FALSE)
	else()
		set(W_SUBMODULE_MODE TRUE)
	endif()

	set_property(GLOBAL PROPERTY W_SUBMODULE_MODE ${W_SUBMODULE_MODE})

	W_build_filter_init()

	W_set_build_types()
endmacro()