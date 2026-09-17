#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#if W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Application/Config/FileSystemConfig.h>
#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/Threading/TaskSystem.h>

class WTask;

/// Event fired by WFileSystemWatcher::m_Events.
struct WFileSystemWatcherEvent
{
  enum class Type
  {
    FileAdded,
    FileRemoved,
    FileChanged,
    DirectoryAdded,
    DirectoryRemoved,
  };

  WStringView m_sPath;
  Type m_Type;
};

/// Creates a file system watcher for the given filesystem config and fires any changes on a worker task via an event.
class W_TOOLSFOUNDATION_DLL WFileSystemWatcher
{
public:
  WFileSystemWatcher(const WApplicationFileSystemConfig& fileSystemConfig);
  ~WFileSystemWatcher();

  /// Once called, file system watchers are created for each data directory and changes are observed.
  void Initialize();

  /// Waits for all pending tasks to complete and then stops observing changes and destroys file system watchers.
  void Deinitialize();

  /// Needs to be called at regular intervals (e.g. each frame) to restart background tasks.
  void MainThreadTick();

public:
  WEvent<const WFileSystemWatcherEvent&, WMutex> m_Events;

private:
  // On file move / rename operations we want the new file to be seen first before the old file delete event so that we can correctly detect this as a move instead of a delete operation. We achieve this by delaying each event by a fixed number of frames.
  static constexpr WUInt32 s_AddedFrameDelay = 5;
  static constexpr WUInt32 s_RemovedFrameDelay = 10;
  // Sometimes moving a file triggers a modified event on the old file. To prevent this from triggering the removal to be seen before the addition, we also delay modified events by the same amount as remove events.
  static constexpr WUInt32 s_ModifiedFrameDelay = 10;

  struct WatcherResult
  {
    WString m_sFile;
    WDirectoryWatcherAction m_Action;
    WDirectoryWatcherType m_Type;
  };

  struct PendingUpdate
  {
    WString m_sAbsPath;
    WUInt32 m_uiFrameDelay = 0;
  };

  /// Handles a single change notification by a directory watcher.
  void HandleWatcherChange(const WatcherResult& res);
  /// Handles update delays to allow compacting multiple changes.
  void NotifyChanges();
  /// Adds a change with the given delay to the container. If the entry is already present, only its delay is increased.
  void AddEntry(WDynamicArray<PendingUpdate>& container, const WStringView sAbsPath, WUInt32 uiFrameDelay);
  /// Reduces the delay counter of every item in the container. If a delay reaches zero, it is removed and the callback is fired.
  void ConsumeEntry(WDynamicArray<PendingUpdate>& container, WFileSystemWatcherEvent::Type type, const WDelegate<void(const WString& sAbsPath, WFileSystemWatcherEvent::Type type)>& consume);

private:
  // Immutable data after StartInitialize
  WApplicationFileSystemConfig m_FileSystemConfig;

  // Watchers
  mutable WMutex m_WatcherMutex;
  WHybridArray<WDirectoryWatcher*, 6> m_Watchers;
  WSharedPtr<WTask> m_pWatcherTask;
  WSharedPtr<WTask> m_pNotifyTask;
  WTaskGroupID m_WatcherGroup;
  WTaskGroupID m_NotifyGroup;
  WAtomicBool m_bShutdown = false;

  // Pending ops
  WHybridArray<PendingUpdate, 4> m_FileAdded;
  WHybridArray<PendingUpdate, 4> m_FileRemoved;
  WHybridArray<PendingUpdate, 4> m_FileChanged;
  WHybridArray<PendingUpdate, 4> m_DirectoryAdded;
  WHybridArray<PendingUpdate, 4> m_DirectoryRemoved;
};

#endif
