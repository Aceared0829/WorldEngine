set (W_3RDPARTY_MINIAUDIO_SUPPORT ON CACHE BOOL "Whether to add support for MiniAudio.")
mark_as_advanced(FORCE W_3RDPARTY_MINIAUDIO_SUPPORT)

macro(W_requires_miniaudio)
	W_requires(W_3RDPARTY_MINIAUDIO_SUPPORT)
endmacro()
