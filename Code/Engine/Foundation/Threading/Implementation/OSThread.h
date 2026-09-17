#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

/// Implementation of a thread.
///
/// Since the thread class needs a platform specific entry-point it is usually
/// recommended to use the WThread class instead as the base for long running threads.
class W_FOUNDATION_DLL WOSThread
{
public:
  /// Initializes the thread instance (e.g. thread creation etc.)
  ///
  /// Note that the thread won't start execution until Start() is called. Please note that szName must be valid until Start() has been
  /// called!
  WOSThread(WOSThreadEntryPoint threadEntryPoint, void* pUserData = nullptr, WStringView sName = "WOSThread", WUInt32 uiStackSize = 128 * 1024);

  /// Destructor.
  virtual ~WOSThread();

  /// Starts the thread
  void Start(); // [tested]

  /// Waits in the calling thread until the thread has finished execution (e.g. returned from the thread function)
  void Join(); // [tested]

  /// Returns the thread ID of the thread object, may be used in comparison operations with WThreadUtils::GetCurrentThreadID() for
  /// example.
  const WThreadID& GetThreadID() const { return m_ThreadID; }

  /// Returns how many WOSThreads are currently active.
  static WInt32 GetThreadCount() { return s_iThreadCount; }

protected:
  WThreadHandle m_hHandle;
  WThreadID m_ThreadID;

  WOSThreadEntryPoint m_EntryPoint;

  void* m_pUserData;

  WString m_sName;

  WUInt32 m_uiStackSize;


private:
  /// Stores how many WOSThread are currently active.
  static WAtomicInteger32 s_iThreadCount;

  W_DISALLOW_COPY_AND_ASSIGN(WOSThread);
};
