# #####################################
# ## Embree support
# #####################################

set(W_BUILD_EMBREE OFF CACHE BOOL "Whether support for Intel Embree should be added")

# #####################################
# ## W_requires_embree()
# #####################################
macro(W_requires_embree)
	W_requires(W_CMAKE_PLATFORM_WINDOWS)
	W_requires(W_BUILD_EMBREE)
endmacro()

# #####################################
# ## W_link_target_embree(<target>)
# #####################################
function(W_link_target_embree TARGET_NAME)
	W_requires_embree()

	find_package(WEmbree REQUIRED)

	if(WEMBREE_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE WEmbree::WEmbree)

		target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_EMBREE_SUPPORT)

		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:WEmbree::WEmbree> $<TARGET_FILE_DIR:${TARGET_NAME}>
		)
	endif()
endfunction()
