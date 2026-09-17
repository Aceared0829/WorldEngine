#include <Core/CorePCH.h>

#if W_ENABLED(W_PLATFORM_OSX)

#  if W_DISABLED(W_SUPPORTS_GLFW)
#    include <Core/Platform/NoImpl/Window_NoImpl.inl>
#  endif

#endif
