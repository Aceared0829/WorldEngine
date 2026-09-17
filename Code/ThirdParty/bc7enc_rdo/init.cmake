set (W_3RDPARTY_BC7ENC_RDO_SUPPORT ON CACHE BOOL "Whether to add support for the bc7 compression.")
mark_as_advanced(FORCE W_3RDPARTY_BC7ENC_RDO_SUPPORT)

macro(W_requires_bc7enc_rdo)
	
	W_requires(W_3RDPARTY_BC7ENC_RDO_SUPPORT)

endmacro()