#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Memory/Policies/AllocPolicyGuarding.h>

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

struct AlloctionMetaData
{
  AlloctionMetaData()
  {
    m_uiSize = 0;

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_magic); ++i)
    {
      m_magic[i] = 0x12345678;
    }
  }

  ~AlloctionMetaData()
  {
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_magic); ++i)
    {
      W_ASSERT_DEV(m_magic[i] == 0x12345678, "Magic value has been overwritten. This might be the result of a buffer underrun!");
    }
  }

  size_t m_uiSize;
  WUInt32 m_magic[32];
};

WAllocPolicyGuarding::WAllocPolicyGuarding(WAllocator* pParent)
{
  W_IGNORE_UNUSED(pParent);

  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  m_uiPageSize = sysInfo.dwPageSize;
}


void* WAllocPolicyGuarding::Allocate(size_t uiSize, size_t uiAlign)
{
  W_ASSERT_DEV(WMath::IsPowerOf2((WUInt32)uiAlign), "Alignment must be power of two");
  uiAlign = WMath::Max<size_t>(uiAlign, W_ALIGNMENT_MINIMUM);

  size_t uiAlignedSize = WMemoryUtils::AlignSize(uiSize, uiAlign);
  size_t uiTotalSize = uiAlignedSize + sizeof(AlloctionMetaData);

  // align to full pages and add one page in front and one in back
  size_t uiPageSize = m_uiPageSize;
  size_t uiFullPageSize = WMemoryUtils::AlignSize(uiTotalSize, uiPageSize);
  void* pMemory = VirtualAlloc(nullptr, uiFullPageSize + 2 * uiPageSize, MEM_RESERVE, PAGE_NOACCESS);
  W_ASSERT_DEV(pMemory != nullptr, "Could not reserve memory pages. Error Code '{0}'", WArgErrorCode(::GetLastError()));

  // add one page and commit the payload pages
  pMemory = WMemoryUtils::AddByteOffset(pMemory, uiPageSize);
  void* ptr = VirtualAlloc(pMemory, uiFullPageSize, MEM_COMMIT, PAGE_READWRITE);
  W_ASSERT_DEV(ptr != nullptr, "Could not commit memory pages. Error Code '{0}'", WArgErrorCode(::GetLastError()));

  // store information in meta data
  AlloctionMetaData* metaData = WMemoryUtils::AddByteOffset(static_cast<AlloctionMetaData*>(ptr), uiFullPageSize - uiTotalSize);
  WMemoryUtils::Construct<SkipTrivialTypes>(metaData, 1);
  metaData->m_uiSize = uiAlignedSize;

  // finally add offset to the actual payload
  ptr = WMemoryUtils::AddByteOffset(metaData, sizeof(AlloctionMetaData));
  return ptr;
}

// deactivate analysis warning for VirtualFree flags, it is needed for the specific functionality
W_MSVC_ANALYSIS_WARNING_PUSH
W_MSVC_ANALYSIS_WARNING_DISABLE(6250)

void WAllocPolicyGuarding::Deallocate(void* pPtr)
{
  WLock<WMutex> lock(m_Mutex);

  if (!m_AllocationsToFreeLater.CanAppend())
  {
    void* pMemory = m_AllocationsToFreeLater.PeekFront();
    W_VERIFY(::VirtualFree(pMemory, 0, MEM_RELEASE), "Could not free memory pages. Error Code '{0}'", WArgErrorCode(::GetLastError()));

    m_AllocationsToFreeLater.PopFront();
  }

  // Retrieve info from meta data first.
  AlloctionMetaData* metaData = WMemoryUtils::AddByteOffset(static_cast<AlloctionMetaData*>(pPtr), -((std::ptrdiff_t)sizeof(AlloctionMetaData)));
  size_t uiAlignedSize = metaData->m_uiSize;

  WMemoryUtils::Destruct(metaData, 1);

  // Decommit the pages but do not release the memory yet so use-after-free can be detected.
  size_t uiPageSize = m_uiPageSize;
  size_t uiTotalSize = uiAlignedSize + sizeof(AlloctionMetaData);
  size_t uiFullPageSize = WMemoryUtils::AlignSize(uiTotalSize, uiPageSize);
  pPtr = WMemoryUtils::AddByteOffset(pPtr, ((std::ptrdiff_t)uiAlignedSize) - uiFullPageSize);

  W_VERIFY(
    ::VirtualFree(pPtr, uiFullPageSize, MEM_DECOMMIT), "Could not decommit memory pages. Error Code '{0}'", WArgErrorCode(::GetLastError()));

  // Finally store the allocation so we can release it later
  void* pMemory = WMemoryUtils::AddByteOffset(pPtr, -((std::ptrdiff_t)uiPageSize));
  m_AllocationsToFreeLater.PushBack(pMemory);
}

W_MSVC_ANALYSIS_WARNING_POP

#endif
