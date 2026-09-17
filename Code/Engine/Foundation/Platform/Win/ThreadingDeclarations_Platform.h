#pragma once

// Deactivate Doxygen document generation for the following block.
/// \cond

#include <Foundation/Platform/Win/Utils/MinWindows.h>

#if W_ENABLED(W_PLATFORM_32BIT)
struct alignas(4) WMutexHandle
{
  WUInt8 data[24];
};
#else
struct alignas(8) WMutexHandle
{
  WUInt8 data[40];
};
#endif


#if W_ENABLED(W_PLATFORM_32BIT)
struct alignas(4) WConditionVariableHandle
{
  WUInt8 data[4];
};
#else
struct alignas(8) WConditionVariableHandle
{
  WUInt8 data[8];
};
#endif



using WThreadHandle = WMinWindows::HANDLE;
using WThreadID = WMinWindows::DWORD;
using WOSThreadEntryPoint = WMinWindows::DWORD(__stdcall*)(void* lpThreadParameter);
using WSemaphoreHandle = WMinWindows::HANDLE;

#define W_THREAD_CLASS_ENTRY_POINT WMinWindows::DWORD __stdcall WThreadClassEntryPoint(void* lpThreadParameter);

struct WConditionVariableData
{
  WConditionVariableHandle m_ConditionVariable;
};

/// \endcond
