#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>

class WProcessMessage;
class WIpcChannel;
class WLoopThread;

/// Internal sub-system used by WIpcChannel.
///
/// This sub-system creates a background thread as soon as the first WIpcChannel
/// is added to it. This class should never be needed to be accessed outside
/// of WIpcChannel implementations.
class W_FOUNDATION_DLL WMessageLoop
{
  W_DECLARE_SINGLETON(WMessageLoop);

public:
  WMessageLoop();
  virtual ~WMessageLoop() = default;
  ;

  /// Needs to be called by newly created channels' constructors.
  void AddChannel(WIpcChannel* pChannel);

  void RemoveChannel(WIpcChannel* pChannel);

protected:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, MessageLoop);
  friend class WLoopThread;
  friend class WIpcChannel;

  void StartUpdateThread();
  void StopUpdateThread();
  void RunLoop();
  bool ProcessTasks();
  void Quit();

  /// Wake up the message loop when new work comes in.
  virtual void WakeUp() = 0;
  /// Waits until a new message has been processed (sent, received).
  /// \param timeout If negative, wait indefinitely.
  /// \param pFilter If not null, wait for a message for the specific channel.
  /// \return Returns whether a message was received or the timeout was reached.
  virtual bool WaitForMessages(WInt32 iTimeout, WIpcChannel* pFilter) = 0;

  WThreadID m_ThreadId = 0;
  mutable WMutex m_Mutex;
  bool m_bShouldQuit = false;
  bool m_bCallTickFunction = false;
  class WLoopThread* m_pUpdateThread = nullptr;

  WMutex m_TasksMutex;
  WDynamicArray<WIpcChannel*> m_ConnectQueue;
  WDynamicArray<WIpcChannel*> m_DisconnectQueue;
  WDynamicArray<WIpcChannel*> m_SendQueue;

  // Thread local copies of the different queues for the ProcessTasks method
  WDynamicArray<WIpcChannel*> m_ConnectQueueTask;
  WDynamicArray<WIpcChannel*> m_DisconnectQueueTask;
  WDynamicArray<WIpcChannel*> m_SendQueueTask;

  WDynamicArray<WIpcChannel*> m_AllAddedChannels;
};
