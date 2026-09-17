# #####################################
# ## Output directories
# #####################################
set(W_OUTPUT_DIRECTORY_LIB "${CMAKE_SOURCE_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files.")
set(W_OUTPUT_DIRECTORY_DLL "${CMAKE_SOURCE_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files.")

mark_as_advanced(FORCE W_OUTPUT_DIRECTORY_LIB)
mark_as_advanced(FORCE W_OUTPUT_DIRECTORY_DLL)

set(W_OUTPUT_DIRECTORY_PLATFORM_POSTFIX "" CACHE STRING "Platform postfix for output directory")
mark_as_advanced(FORCE W_OUTPUT_DIRECTORY_PLATFORM_POSTFIX)

# #####################################
# ## PCH support
# #####################################
set(W_USE_PCH ON CACHE BOOL "Whether to use Precompiled Headers.")

mark_as_advanced(FORCE W_USE_PCH)

# #####################################
# ## Folder Unity files
# #####################################
set(W_ENABLE_FOLDER_UNITY_FILES ON CACHE BOOL "Whether unity cpp files should be created per folder")

mark_as_advanced(FORCE W_ENABLE_FOLDER_UNITY_FILES)

# #####################################
# ## PVS Studio support
# #####################################
set(W_ENABLE_PVS_STUDIO_HEADER_IN_UNITY_FILES ON CACHE BOOL "Adds the necessary comment to the generated unity files for PVS checking")

mark_as_advanced(FORCE W_ENABLE_PVS_STUDIO_HEADER_IN_UNITY_FILES)

# #####################################
# ## Static analysis support
# #####################################
set(W_ENABLE_COMPILER_STATIC_ANALYSIS OFF CACHE BOOL "Enables static analysis in the compiler options")

mark_as_advanced(FORCE W_ENABLE_COMPILER_STATIC_ANALYSIS)

# #####################################
# ## SSE level
# #####################################

set(W_MIN_REQUIRED_SSE_LEVEL_VALUES "SSE2;SSE41;AVX")
set(W_MIN_REQUIRED_SSE_LEVEL "SSE41" CACHE STRING "Sets the minimum required SSE level")
set_property(CACHE W_MIN_REQUIRED_SSE_LEVEL PROPERTY STRINGS ${W_MIN_REQUIRED_SSE_LEVEL_VALUES})

mark_as_advanced(FORCE W_MIN_REQUIRED_SSE_LEVEL)
