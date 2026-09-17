# find the folder in which Embree is located

# early out, if this target has been created before
if(TARGET WEmbree::WEmbree)
	return()
endif()

find_path(W_EMBREE_DIR include/embree3/rtcore.h
	PATHS
	${W_ROOT}/Code/ThirdParty/embree
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(EMBREE_LIB_PATH "${W_EMBREE_DIR}/vc141win64")
else()
	set(EMBREE_LIB_PATH "${W_EMBREE_DIR}/vc141win32")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WEmbree DEFAULT_MSG W_EMBREE_DIR)

if(WEMBREE_FOUND)
	add_library(WEmbree::WEmbree SHARED IMPORTED)
	set_target_properties(WEmbree::WEmbree PROPERTIES IMPORTED_LOCATION "${EMBREE_LIB_PATH}/embree3.dll")
	set_target_properties(WEmbree::WEmbree PROPERTIES IMPORTED_IMPLIB "${EMBREE_LIB_PATH}/embree3.lib")
	set_target_properties(WEmbree::WEmbree PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${W_EMBREE_DIR}/include")
endif()

mark_as_advanced(FORCE W_EMBREE_DIR)

unset(EMBREE_LIB_PATH)
