#pragma once

#include <Foundation/Basics.h>

#if W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Basics.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Types/Bitflags.h>
#  include <Foundation/Types/Delegate.h>

struct WDirectoryWatcherImpl;

/// Which action has been performed on a file.
enum class WDirectoryWatcherAction
{
  None,           ///< Nothing happened
  Added,          ///< A file or directory was added, requires WDirectoryWatcher::Watch::Creates flag when creating WDirectoryWatcher.
  Removed,        ///< A file or directory was removed, requires WDirectoryWatcher::Watch::Deletes flag when creating WDirectoryWatcher.
  Modified,       ///< A file was modified. Both Reads and Writes can 'modify' the timestamps of a file, requires WDirectoryWatcher::Watch::Writes flag when creating WDirectoryWatcher.
  RenamedOldName, ///< A file or directory was renamed. First the old name is provided, requires WDirectoryWatcher::Watch::Renames flag when creating WDirectoryWatcher.
  RenamedNewName, ///< A file or directory was renamed. The new name is provided second, requires WDirectoryWatcher::Watch::Renames flag when creating WDirectoryWatcher.
};

enum class WDirectoryWatcherType
{
  File,
  Directory
};

/// Platform-abstracted file system monitoring for detecting directory changes
///
/// Provides efficient monitoring of file system events (create, modify, delete, rename) within
/// a specified directory. Uses native OS facilities (inotify on Linux, ReadDirectoryChangesW on Windows)
/// for minimal overhead polling. Essential for tools like asset hot-reloading, live file editing,
/// and build system dependency tracking.
///
/// Changes are queued internally and retrieved through polling via EnumerateChanges(). Supports
/// configurable event filtering and optional recursive subdirectory monitoring.
class W_FOUNDATION_DLL WDirectoryWatcher
{
public:
  /// What to watch out for.
  struct Watch
  {
    using StorageType = WUInt8;
    constexpr static WUInt8 Default = 0;

    /// Enum values
    enum Enum
    {
      Writes = W_BIT(0),         ///< Watch for writes. Will trigger WDirectoryWatcherAction::Modified events.
      Creates = W_BIT(1),        ///< Watch for newly created files. Will trigger WDirectoryWatcherAction::Added events.
      Deletes = W_BIT(2),        ///< Watch for deleted files. Will trigger WDirectoryWatcherAction::Removed events.
      Renames = W_BIT(3),        ///< Watch for renames. Will trigger WDirectoryWatcherAction::RenamedOldName and WDirectoryWatcherAction::RenamedNewName events.
      Subdirectories = W_BIT(4), ///< Watch files in subdirectories recursively.
    };

    struct Bits
    {
      StorageType Writes : 1;
      StorageType Creates : 1;
      StorageType Deletes : 1;
      StorageType Renames : 1;
      StorageType Subdirectories : 1;
    };
  };

  WDirectoryWatcher();
  WDirectoryWatcher(const WDirectoryWatcher&) = delete;
  WDirectoryWatcher(WDirectoryWatcher&&) noexcept = delete;
  ~WDirectoryWatcher();

  WDirectoryWatcher& operator=(const WDirectoryWatcher&) = delete;
  WDirectoryWatcher& operator=(WDirectoryWatcher&&) noexcept = delete;

  /// Opens the directory at \p absolutePath for watching. \p whatToWatch controls what exactly should be watched.
  ///
  /// \note A instance of WDirectoryWatcher can only watch one directory at a time.
  WResult OpenDirectory(WStringView sAbsolutePath, WBitflags<Watch> whatToWatch);

  /// Closes the currently watched directory if any.
  void CloseDirectory();

  /// Returns the opened directory, will be empty if no directory was opened.
  WStringView GetDirectory() const { return m_sDirectoryPath; }

  using EnumerateChangesFunction = WDelegate<void(WStringView sFilename, WDirectoryWatcherAction action, WDirectoryWatcherType type), 48>;

  /// Calls the callback \p func for each change since the last call. For each change the filename
  /// and the action, which was performed on the file, is passed to \p func.
  /// If waitUpToMilliseconds is greater than 0, blocks until either a change was observed or the timelimit is reached.
  ///
  /// \note There might be multiple changes on the same file reported.
  void EnumerateChanges(EnumerateChangesFunction func, WTime waitUpTo = WTime::MakeZero());

  /// Same as the other EnumerateChanges function, but enumerates multiple watchers.
  static void EnumerateChanges(WArrayPtr<WDirectoryWatcher*> watchers, EnumerateChangesFunction func, WTime waitUpTo = WTime::MakeZero());

private:
  WString m_sDirectoryPath;
  WDirectoryWatcherImpl* m_pImpl = nullptr;
};

W_DECLARE_FLAGS_OPERATORS(WDirectoryWatcher::Watch);

#endif
