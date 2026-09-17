######################################
### Jolt support
######################################

set (W_3RDPARTY_KRAUT_SUPPORT ON CACHE BOOL "Whether to add support for procedurally generated trees with Kraut.")
mark_as_advanced(FORCE W_3RDPARTY_KRAUT_SUPPORT)

######################################
### W_requires_kraut()
######################################

macro(W_requires_kraut)

	W_requires(W_3RDPARTY_KRAUT_SUPPORT)

endmacro()
