#include <Core/CorePCH.h>

#if W_ENABLED(W_PLATFORM_LINUX)

#  if W_DISABLED(W_SUPPORTS_GLFW)
#    include <Core/Platform/NoImpl/InputDevice_NoImpl.inl>
#  endif

#endif
