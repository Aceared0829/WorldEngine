# #####################################
# ## W_requires_renderer()
# #####################################

macro(W_requires_renderer)
	# PLATFORM-TODO

	# if we know which backend the user wants, require that
	# otherwise require the platform specific renderer
	# if nothing else is known, this platform doesn't support renderers
	if(W_BUILD_EXPERIMENTAL_WEBGPU)
		W_requires_webgpu()
	elseif(W_BUILD_VULKAN)
		W_requires_vulkan()
	elseif(W_BUILD_D3D11)
		W_requires_d3d()
	else()
		message(STATUS "No renderer available on this platform.")
		return()
	endif()
endmacro()

# #####################################
# ## W_DEFAULT_RENDERER
# #####################################

set(W_DEFAULT_RENDERER "" CACHE STRING "The renderer to use by default when none is specified on the command line. Leave empty to auto-select (DX11 if built, else Vulkan).")

# #####################################
# ## W_add_renderers(<target>)
# ## Add all required libraries and dependencies to the given target so it has access to all available renderers.
# #####################################
function(W_add_renderers TARGET_NAME)
	# PLATFORM-TODO
	if(W_BUILD_VULKAN AND W_CMAKE_PLATFORM_SUPPORTS_VULKAN)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererVulkan
		)

		if (TARGET ShaderCompilerVulkan)
			add_dependencies(${TARGET_NAME}
				ShaderCompilerVulkan
			)
		endif()
	endif()

	if(W_BUILD_EXPERIMENTAL_WEBGPU)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererWebGPU
		)

		if (TARGET ShaderCompilerWebGPU)
			add_dependencies(${TARGET_NAME}
				ShaderCompilerWebGPU
			)
		endif()
	endif()

	if(W_BUILD_D3D11 AND (W_CMAKE_PLATFORM_SUPPORTS_D3D11 OR W_BUILD_EXPERIMENTAL_DXVK))
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererDX11
		)
		W_link_target_dx11(${TARGET_NAME})

		if (TARGET ShaderCompilerHLSL)
			add_dependencies(${TARGET_NAME}
				ShaderCompilerHLSL
			)
		endif()
	endif()
endfunction()