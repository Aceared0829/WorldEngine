#include <ToolsFoundation/ToolsFoundationDLL.h>

#if W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER)

#  include <ToolsFoundation/FileSystem/FileSystemWatcher.h>

#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Threading/DelegateTask.h>

////////////////////////////////////////////////////////////////////////
// WAssetWatcher
////////////////////////////////////////////////////////////////////////

WFileSystemWatcher::WFileSystemWatcher(const WApplicationFileSystemConfig& fileSystemConfig)
{
  m_FileSystemConfig = fileSystemConfig;
}


WFileSystemWatcher::~WFileSystemWatcher() = default;

void WFileSystemWatcher::Initialize()
{
  W_PROFILE_SCOPE("Initialize");

  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    WStringBuilder sTemp;
    if (WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
    {
      WLog::Error("Failed to init directory watcher for dir '{0}'", dd.m_sDataDirSpecialPath);
      continue;
    }

    WDirectoryWatcher* pWatcher = W_DEFAULT_NEW(WDirectoryWatcher);
    WResult res = pWatcher->OpenDirectory(sTemp, WDirectoryWatcher::Watch::Deletes | WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Renames | WDirectoryWatcher::Watch::Subdirectories);

    if (res.Failed())
    {
      W_DEFAULT_DELETE(pWatcher);
      WLog::Error("Failed to init directory watcher for dir '{0}'", sTemp);
      continue;
    }

    m_Watchers.PushBack(pWatcher);
  }

  m_pWatcherTask = W_DEFAULT_NEW(WDelegateTask<void>, "Watcher Changes", WTaskNesting::Never, [this]()
    {
      WTempHybridArray<WatcherResult, 16> watcherResults;
      for (WDirectoryWatcher* pWatcher : m_Watchers)
      {
        pWatcher->EnumerateChanges([pWatcher, &watcherResults](WStringView sFilename, WDirectoryWatcherAction action, WDirectoryWatcherType type)
          { watcherResults.PushBack({sFilename, action, type}); });
      }
      for (const WatcherResult& res : watcherResults)
      {
        HandleWatcherChange(res);
      } //
    });
  // This is a separate task as these trigger callbacks which can potentially take a long time and we can't have the watcher changes task be blocked for so long or notifications might get lost.
  m_pNotifyTask = W_DEFAULT_NEW(WDelegateTask<void>, "Watcher Notify", WTaskNesting::Never, [this]()
    { NotifyChanges(); });
}

void WFileSystemWatcher::Deinitialize()
{
  m_bShutdown = true;
  WTaskGroupID watcherGroup;
  WTaskGroupID notifyGroup;
  {
    W_LOCK(m_WatcherMutex);
    watcherGroup = m_WatcherGroup;
    notifyGroup = m_NotifyGroup;
  }
  WTaskSystem::WaitForGroup(watcherGroup);
  WTaskSystem::WaitForGroup(notifyGroup);
  {
    W_LOCK(m_WatcherMutex);
    m_pWatcherTask.Clear();
    m_pNotifyTask.Clear();
    for (WDirectoryWatcher* pWatcher : m_Watchers)
    {
      W_DEFAULT_DELETE(pWatcher);
    }
    m_Watchers.Clear();
  }
}

void WFileSystemWatcher::MainThreadTick()
{
  W_PROFILE_SCOPE("WAssetWatcherTick");
  W_LOCK(m_WatcherMutex);
  if (!m_bShutdown && m_pWatcherTask && WTaskSystem::IsTaskGroupFinished(m_WatcherGroup))
  {
    m_WatcherGroup = WTaskSystem::StartSingleTask(m_pWatcherTask, WTaskPriority::LongRunningHighPriority);
  }
  if (!m_bShutdown && m_pNotifyTask && WTaskSystem::IsTaskGroupFinished(m_NotifyGroup))
  {
    m_NotifyGroup = WTaskSystem::StartSingleTask(m_pNotifyTask, WTaskPriority::LongRunningHighPriority);
  }
}


void WFileSystemWatcher::NotifyChanges()
{
  auto NotifyChange = [this](const WString& sAbsPath, WFileSystemWatcherEvent::Type type)
  {
    WFileSystemWatcherEvent e;
    e.m_sPath = sAbsPath;
    e.m_Type = type;
    m_Events.Broadcast(e);
  };

  // Files
  ConsumeEntry(m_FileAdded, WFileSystemWatcherEvent::Type::FileAdded, NotifyChange);
  ConsumeEntry(m_FileChanged, WFileSystemWatcherEvent::Type::FileChanged, NotifyChange);
  ConsumeEntry(m_FileRemoved, WFileSystemWatcherEvent::Type::FileRemoved, NotifyChange);

  // Directories
  ConsumeEntry(m_DirectoryAdded, WFileSystemWatcherEvent::Type::DirectoryAdded, NotifyChange);
  ConsumeEntry(m_DirectoryRemoved, WFileSystemWatcherEvent::Type::DirectoryRemoved, NotifyChange);
}

void WFileSystemWatcher::HandleWatcherChange(const WatcherResult& res)
{
  switch (res.m_Action)
  {
    case WDirectoryWatcherAction::None:
      W_ASSERT_DEV(false, "None event should never happen");
      break;
    case WDirectoryWatcherAction::RenamedNewName:
    case WDirectoryWatcherAction::Added:
    {
      if (res.m_Type == WDirectoryWatcherType::Directory)
      {
        AddEntry(m_DirectoryAdded, res.m_sFile, s_AddedFrameDelay);
      }
      else
      {
        AddEntry(m_FileAdded, res.m_sFile, s_AddedFrameDelay);
      }
    }
    break;
    case WDirectoryWatcherAction::RenamedOldName:
    case WDirectoryWatcherAction::Removed:
    {
      if (res.m_Type == WDirectoryWatcherType::Directory)
      {
        AddEntry(m_DirectoryRemoved, res.m_sFile, s_RemovedFrameDelay);
      }
      else
      {
        AddEntry(m_FileRemoved, res.m_sFile, s_RemovedFrameDelay);
      }
    }
    break;
    case WDirectoryWatcherAction::Modified:
    {
      if (res.m_Type == WDirectoryWatcherType::Directory)
      {
        // Can a directory even be modified? In any case, we ignore this change.
        // UpdateEntry(m_DirectoryRemoved, res.sFile, s_RemovedFrameDelay);
      }
      else
      {
        AddEntry(m_FileChanged, res.m_sFile, s_ModifiedFrameDelay);
      }
    }
    break;
  }
}

void WFileSystemWatcher::AddEntry(WDynamicArray<PendingUpdate>& container, const WStringView sAbsPath, WUInt32 uiFrameDelay)
{
  W_LOCK(m_WatcherMutex);
  for (PendingUpdate& update : container)
  {
    if (update.m_sAbsPath == sAbsPath)
    {
      update.m_uiFrameDelay = uiFrameDelay;
      return;
    }
  }
  PendingUpdate& update = container.ExpandAndGetRef();
  update.m_uiFrameDelay = uiFrameDelay;
  update.m_sAbsPath = sAbsPath;
}

void WFileSystemWatcher::ConsumeEntry(WDynamicArray<PendingUpdate>& container, WFileSystemWatcherEvent::Type type, const WDelegate<void(const WString& sAbsPath, WFileSystemWatcherEvent::Type type)>& consume)
{
  WTempHybridArray<PendingUpdate, 16> updates;
  {
    W_LOCK(m_WatcherMutex);
    for (WUInt32 i = container.GetCount(); i > 0; --i)
    {
      PendingUpdate& update = container[i - 1];
      --update.m_uiFrameDelay;
      if (update.m_uiFrameDelay == 0)
      {
        updates.PushBack(update);
        container.RemoveAtAndSwap(i - 1);
      }
    }
  }
  for (const PendingUpdate& update : updates)
  {
    consume(update.m_sAbsPath, type);
  }
}

#endif
