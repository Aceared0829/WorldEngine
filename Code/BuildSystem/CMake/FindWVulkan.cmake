# find the folder into which the Vulkan SDK has been installed

# early out, if this target has been created before
if(WVulkan_FOUND)
	return()
endif()

W_pull_compiler_and_architecture_vars()
W_pull_config_vars()

get_property(W_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY W_SUBMODULE_PREFIX_PATH)

if (COMMAND W_platformhook_find_vulkan)
	W_platformhook_find_vulkan()
else()
	message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
endif()
