#if W_ENABLED(W_COMPILER_MSVC) && W_ENABLED(W_PLATFORM_ARCH_X86)

extern "C"
{
  // The main purpose of this little hack here is to have Mutex::Lock and Mutex::Unlock inline-able without including windows.h
  // The hack however does only work on the MSVC compiler. See fall back code below.

  // First define two functions which are binary compatible with EnterCriticalSection and LeaveCriticalSection
  __declspec(dllimport) void __stdcall WWinEnterCriticalSection(WMutexHandle* pHandle);
  __declspec(dllimport) void __stdcall WWinLeaveCriticalSection(WMutexHandle* pHandle);
  __declspec(dllimport) WMinWindows::BOOL __stdcall WWinTryEnterCriticalSection(WMutexHandle* pHandle);

  // Now redirect them through linker flags to the correct implementation
#  if W_ENABLED(W_PLATFORM_32BIT)
#    pragma comment(linker, "/alternatename:__imp__WWinEnterCriticalSection@4=__imp__EnterCriticalSection@4")
#    pragma comment(linker, "/alternatename:__imp__WWinLeaveCriticalSection@4=__imp__LeaveCriticalSection@4")
#    pragma comment(linker, "/alternatename:__imp__WWinTryEnterCriticalSection@4=__imp__TryEnterCriticalSection@4")
#  else
#    pragma comment(linker, "/alternatename:__imp_WWinEnterCriticalSection=__imp_EnterCriticalSection")
#    pragma comment(linker, "/alternatename:__imp_WWinLeaveCriticalSection=__imp_LeaveCriticalSection")
#    pragma comment(linker, "/alternatename:__imp_WWinTryEnterCriticalSection=__imp_TryEnterCriticalSection")
#  endif
}

inline void WMutex::Lock()
{
  WWinEnterCriticalSection(&m_hHandle);
  ++m_iLockCount;
}

inline void WMutex::Unlock()
{
  --m_iLockCount;
  WWinLeaveCriticalSection(&m_hHandle);
}

inline WResult WMutex::TryLock()
{
  if (WWinTryEnterCriticalSection(&m_hHandle) != 0)
  {
    ++m_iLockCount;
    return W_SUCCESS;
  }

  return W_FAILURE;
}

#else

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

inline void WMutex::Lock()
{
  EnterCriticalSection((CRITICAL_SECTION*)&m_hHandle);
  ++m_iLockCount;
}

inline void WMutex::Unlock()
{
  --m_iLockCount;
  LeaveCriticalSection((CRITICAL_SECTION*)&m_hHandle);
}

inline WResult WMutex::TryLock()
{
  if (TryEnterCriticalSection((CRITICAL_SECTION*)&m_hHandle) != 0)
  {
    ++m_iLockCount;
    return W_SUCCESS;
  }

  return W_FAILURE;
}

#endif
