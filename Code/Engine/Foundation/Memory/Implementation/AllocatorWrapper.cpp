#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/AllocatorWrapper.h>

static thread_local WAllocator* s_pAllocator = nullptr;

WLocalAllocatorWrapper::WLocalAllocatorWrapper(WAllocator* pAllocator)
{
  s_pAllocator = pAllocator;
}

void WLocalAllocatorWrapper::Reset()
{
  s_pAllocator = nullptr;
}

WAllocator* WLocalAllocatorWrapper::GetAllocator()
{
  return s_pAllocator;
}
