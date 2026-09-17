######################################
### Jolt support
######################################

set (W_3RDPARTY_JOLT_SUPPORT ON CACHE BOOL "Whether to add support for the Jolt physics engine.")
mark_as_advanced(FORCE W_3RDPARTY_JOLT_SUPPORT)

######################################
### W_requires_jolt()
######################################

macro(W_requires_jolt)

	W_requires(W_3RDPARTY_JOLT_SUPPORT)

endmacro()
