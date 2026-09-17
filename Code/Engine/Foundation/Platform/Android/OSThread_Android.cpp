#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_ANDROID)
#  define W_POSIX_THREAD_SETNAME
#  include <Foundation/Platform/Posix/OSThread_Posix.inl>
#endif
