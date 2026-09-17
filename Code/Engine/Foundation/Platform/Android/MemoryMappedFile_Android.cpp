#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_ANDROID)
#  define W_POSIX_MMAP_SKIPUNLINK
#  include <Foundation/Platform/Posix/MemoryMappedFile_Posix.inl>
#endif
