#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/System/Process.h>
#include <Foundation/Time/Time.h>

class WStreamWriter;
class WThread;

/// This class encapsulates a profiling scope.
///
/// The constructor creates a new scope in the profiling system and the destructor pops the scope.
/// You shouldn't need to use this directly, just use the macro W_PROFILE_SCOPE provided below.
class W_FOUNDATION_DLL WProfilingScope
{
public:
  WProfilingScope(WStringView sName, const char* szFunctionName, WTime timeout);
  ~WProfilingScope();

protected:
  WStringView m_sName;
  const char* m_szFunction;
  WTime m_BeginTime;
  WTime m_Timeout;
};

/// This class implements a profiling scope similar to WProfilingScope, but with additional sub-scopes which can be added easily without
/// introducing actual C++ scopes.
///
/// The constructor pushes one surrounding scope on the stack and then a nested scope as the first section.
/// The function StartNextSection() will end the nested scope and start a new inner scope.
/// This allows to end one scope and start a new one, without having to add actual C++ scopes for starting/stopping profiling scopes.
///
/// You shouldn't need to use this directly, just use the macro W_PROFILE_LIST_SCOPE provided below.
class WProfilingListScope
{
public:
  W_FOUNDATION_DLL WProfilingListScope(WStringView sListName, WStringView sFirstSectionName, const char* szFunctionName);
  W_FOUNDATION_DLL ~WProfilingListScope();

  W_FOUNDATION_DLL static void StartNextSection(WStringView sNextSectionName);

protected:
  static thread_local WProfilingListScope* s_pCurrentList;

  WProfilingListScope* m_pPreviousList;

  WStringView m_sListName;
  const char* m_szListFunction;
  WTime m_ListBeginTime;

  WStringView m_sCurSectionName;
  WTime m_CurSectionBeginTime;
};

/// Helper functionality of the profiling system.
class W_FOUNDATION_DLL WProfilingSystem
{
public:
  struct ThreadInfo
  {
    WUInt64 m_uiThreadId;
    WString m_sName;
  };

  struct CPUScope
  {
    W_DECLARE_POD_TYPE();

    static constexpr WUInt32 NAME_SIZE = 40;

    const char* m_szFunctionName;
    WTime m_BeginTime;
    WTime m_EndTime;
    char m_szName[NAME_SIZE];
  };

  struct CPUScopesBufferFlat
  {
    WDynamicArray<CPUScope> m_Data;
    WUInt64 m_uiThreadId = 0;
  };

  /// Helper struct to hold GPU profiling data.
  struct GPUScope
  {
    W_DECLARE_POD_TYPE();

    static constexpr WUInt32 NAME_SIZE = 48;

    WTime m_BeginTime;
    WTime m_EndTime;
    char m_szName[NAME_SIZE];
  };

  struct W_FOUNDATION_DLL ProfilingData
  {
    WUInt32 m_uiFramesThreadID = 0;
    WUInt32 m_uiProcessSortIndex = 0;
    WOsProcessID m_uiProcessID = 0;

    WHybridArray<ThreadInfo, 16> m_ThreadInfos;

    WDynamicArray<CPUScopesBufferFlat> m_AllEventBuffers;

    WUInt64 m_uiFrameCount = 0;
    WDynamicArray<WTime> m_FrameStartTimes;

    WDynamicArray<WDynamicArray<GPUScope>> m_GPUScopes;

    /// Writes profiling data as JSON to the output stream.
    WResult Write(WStreamWriter& ref_outputStream) const;

    void Clear();

    /// Concatenates all given ProfilingData instances into one merge struct
    static void Merge(ProfilingData& out_merged, WArrayPtr<const ProfilingData*> inputs);
  };

public:
  static void Clear();

  static void Capture(WProfilingSystem::ProfilingData& out_capture, bool bClearAfterCapture = false);

  /// Scopes are discarded if their duration is shorter than the specified threshold. Default is 0.1ms.
  static void SetDiscardThreshold(WTime threshold);

  using ScopeTimeoutDelegate = WDelegate<void(WStringView sName, WStringView sFunctionName, WTime duration)>;

  /// Sets a callback that is triggered when a profiling scope takes longer than desired.
  static void SetScopeTimeoutCallback(ScopeTimeoutDelegate callback);

  /// Should be called once per frame to capture the timestamp of the new frame.
  static void StartNewFrame();

  /// Adds a new scoped event for the calling thread in the profiling system
  static void AddCPUScope(WStringView sName, const char* szFunctionName, WTime beginTime, WTime endTime, WTime scopeTimeout);

  /// Get current frame counter
  static WUInt64 GetFrameCount();

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ProfilingSystem);
  friend WUInt32 RunThread(WThread* pThread);

  static void Initialize();
  /// Removes profiling data of dead threads.
  static void Reset();

  /// Sets the name of the current thread.
  static void SetThreadName(WStringView sThreadName);
  /// Removes the current thread from the profiling system.
  ///  Needs to be called before the thread exits to be able to release profiling memory of dead threads on Reset.
  static void RemoveThread();

public:
  /// Initialized internal data structures for GPU profiling data. Needs to be called before adding any data.
  static void InitializeGPUData(WUInt32 uiGpuCount = 1);

  /// Adds a GPU profiling scope in the internal event ringbuffer.
  static void AddGPUScope(WStringView sName, WTime beginTime, WTime endTime, WUInt32 uiGpuIndex = 0);
};

#if W_ENABLED(W_USE_PROFILING) || defined(W_DOCS)

/// Profiles the current scope using the given name.
///
/// It is allowed to nest W_PROFILE_SCOPE, also with W_PROFILE_LIST_SCOPE. However W_PROFILE_SCOPE should start and end within the same list scope
/// section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa WProfilingScope
/// \sa W_PROFILE_LIST_SCOPE
#  define W_PROFILE_SCOPE(ScopeName) \
    WProfilingScope W_PP_CONCAT(_WProfilingScope, W_SOURCE_LINE)(ScopeName, W_SOURCE_FUNCTION, WTime::MakeZero())

/// Same as W_PROFILE_SCOPE but if the scope takes longer than 'Timeout', the WProfilingSystem's timeout callback is executed.
///
/// This can be used to log an error or save a callstack, etc. when a scope exceeds an expected amount of time.
///
/// \sa WProfilingSystem::SetScopeTimeoutCallback()
#  define W_PROFILE_SCOPE_WITH_TIMEOUT(ScopeName, Timeout) \
    WProfilingScope W_PP_CONCAT(_WProfilingScope, W_SOURCE_LINE)(ScopeName, W_SOURCE_FUNCTION, Timeout)

/// Profiles the current scope using the given name as the overall list scope name and the section name for the first section in the list.
///
/// Use W_PROFILE_LIST_NEXT_SECTION to start a new section in the list scope.
///
/// It is allowed to nest W_PROFILE_SCOPE, also with W_PROFILE_LIST_SCOPE. However W_PROFILE_SCOPE should start and end within the same list scope
/// section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa WProfilingListScope
/// \sa W_PROFILE_LIST_NEXT_SECTION
#  define W_PROFILE_LIST_SCOPE(ListName, FirstSectionName) \
    WProfilingListScope W_PP_CONCAT(_WProfilingScope, W_SOURCE_LINE)(ListName, FirstSectionName, W_SOURCE_FUNCTION)

/// Starts a new section in a W_PROFILE_LIST_SCOPE
///
/// \sa WProfilingListScope
/// \sa W_PROFILE_LIST_SCOPE
#  define W_PROFILE_LIST_NEXT_SECTION(NextSectionName) \
    WProfilingListScope::StartNextSection(NextSectionName)

/// Used to indicate that a frame is finished and another starts.
#  define W_PROFILER_FRAME_MARKER()

#else
#  define W_PROFILE_SCOPE(ScopeName)
#  define W_PROFILE_SCOPE_WITH_TIMEOUT(ScopeName, Timeout)
#  define W_PROFILE_LIST_SCOPE(ListName, FirstSectionName)
#  define W_PROFILE_LIST_NEXT_SECTION(NextSectionName)
#  define W_PROFILER_FRAME_MARKER()
#endif

// Let Tracy override the macros.
#include <Foundation/Profiling/Profiling_Tracy.h>
