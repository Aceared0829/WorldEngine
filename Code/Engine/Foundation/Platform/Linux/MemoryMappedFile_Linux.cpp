#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_LINUX)

#  include <linux/version.h>

#  if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 22)
#    define W_POSIX_MMAP_MAP_POPULATE
#  endif

#  include <Foundation/Platform/Posix/MemoryMappedFile_Posix.inl>

#endif
