# #####################################
# ## Vulkan support
# #####################################

set(W_BUILD_VULKAN ON CACHE BOOL "Whether to build Vulkan renderer support.")

# #####################################
# ## W_requires_vulkan()
# #####################################
macro(W_requires_vulkan)
	W_requires(W_CMAKE_PLATFORM_SUPPORTS_VULKAN)
	W_requires(W_BUILD_VULKAN)
	find_package(WVulkan REQUIRED)
endmacro()

# #####################################
# ## W_link_target_dxc(<target>)
# #####################################
function(W_link_target_dxc TARGET_NAME)
	W_requires_vulkan()

	find_package(WVulkan REQUIRED)

	if(WVULKAN_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE WVulkan::DXC)

		get_target_property(_dll_location WVulkan::DXC IMPORTED_LOCATION)

		if(NOT _dll_location STREQUAL "")
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:WVulkan::DXC> $<TARGET_FILE_DIR:${TARGET_NAME}>)
		endif()

		unset(_dll_location)
	endif()
endfunction()
