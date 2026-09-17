#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/IO/FileEnums.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Timestamp.h>

struct WOSFileData;

#include <OSFileDecl_Platform.h>

/// Defines in which mode to open a file.
struct WFileOpenMode
{
  enum Enum
  {
    None,   ///< None, only used internally.
    Read,   ///< Open file for reading.
    Write,  ///< Open file for writing (already existing data is discarded).
    Append, ///< Open file for appending (writing, but always only at the end, already existing data is preserved).
  };
};

/// Holds the stats for a file.
struct W_FOUNDATION_DLL WFileStats
{
  WFileStats();
  ~WFileStats();

  /// Stores the concatenated m_sParentPath and m_sName in \a path.
  void GetFullPath(WStringBuilder& ref_sPath) const;

  /// Path to the parent folder.
  /// Append m_sName to m_sParentPath to obtain the full path.
  WStringBuilder m_sParentPath;

  /// The name of the file or folder that the stats are for. Does not include the parent path to it.
  /// Append m_sName to m_sParentPath to obtain the full path.
  WString m_sName;

  /// The last modification time as an UTC timestamp since Unix epoch.
  WTimestamp m_LastModificationTime;

  /// The size of the file in bytes.
  WUInt64 m_uiFileSize = 0;

  /// Whether the file object is a file or folder.
  bool m_bIsDirectory = false;
};

#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS) || defined(W_DOCS)

struct WFileIterationData;

struct WFileSystemIteratorFlags
{
  using StorageType = WUInt8;

  enum Enum : WUInt8
  {
    Recursive = W_BIT(0),
    ReportFiles = W_BIT(1),
    ReportFolders = W_BIT(2),

    ReportFilesRecursive = Recursive | ReportFiles,
    ReportFoldersRecursive = Recursive | ReportFolders,
    ReportFilesAndFoldersRecursive = Recursive | ReportFiles | ReportFolders,

    Default = ReportFilesAndFoldersRecursive,
  };

  struct Bits
  {
    StorageType Recursive : 1;
    StorageType ReportFiles : 1;
    StorageType ReportFolders : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WFileSystemIteratorFlags);

/// An WFileSystemIterator allows to iterate over all files in a certain directory.
///
/// The search can be recursive, and it can contain wildcards (* and ?) to limit the search to specific file types.
class W_FOUNDATION_DLL WFileSystemIterator
{
  W_DISALLOW_COPY_AND_ASSIGN(WFileSystemIterator);

public:
  WFileSystemIterator();
  ~WFileSystemIterator();

  /// Starts a search at the given folder. Use * and ? as wildcards.
  ///
  /// To iterate all files from one folder, use '/Some/Folder'
  /// To iterate over all files of a certain type (in one folder) use '/Some/Folder/*.ext'
  /// Only the final path segment can use placeholders, folders in between must be fully named.
  /// If bRecursive is false, the iterator will only iterate over the files in the start folder, and will not recurse into subdirectories.
  /// If bReportFolders is false, only files will be reported, folders will be skipped (though they will be recursed into, if bRecursive is true).
  ///
  /// If W_SUCCESS is returned, the iterator points to a valid file, and the functions GetCurrentPath() and GetStats() will return
  /// the information about that file. To advance to the next file, use Next() or SkipFolder().
  /// When no iteration is possible (the directory does not exist or the wild-cards are used incorrectly), W_FAILURE is returned.
  void StartSearch(WStringView sSearchTerm, WBitflags<WFileSystemIteratorFlags> flags = WFileSystemIteratorFlags::Default); // [tested]

  /// The same as StartSearch() but executes the same search on multiple folders.
  ///
  /// The search term is appended to each start folder and they are searched one after the other.
  void StartMultiFolderSearch(WArrayPtr<WString> startFolders, WStringView sSearchTerm, WBitflags<WFileSystemIteratorFlags> flags = WFileSystemIteratorFlags::Default);

  /// Returns the search string with which StartSearch() was called.
  ///
  /// If StartMultiFolderSearch() is used, every time a new top-level folder is entered, StartSearch() is executed. In this case GetCurrentSearchTerm() can be used to know in which top-level folder the search is currently running.
  const WStringView GetCurrentSearchTerm() const { return m_sSearchTerm; }

  /// Returns the current path in which files are searched. Changes when 'Next' moves in or out of a sub-folder.
  ///
  /// You can use this to get the full path of the current file, by appending this value and the filename from 'GetStats'
  const WStringBuilder& GetCurrentPath() const { return m_sCurPath; } // [tested]

  /// Returns the file stats of the current object that the iterator points to.
  const WFileStats& GetStats() const { return m_CurFile; } // [tested]

  /// Advances the iterator to the next file object. Might recurse into sub-folders.
  void Next(); // [tested]

  /// The same as 'Next' only that the current folder will not be recursed into.
  void SkipFolder(); // [tested]

  /// Returns true if the iterator currently points to a valid file entry.
  bool IsValid() const;

private:
  WInt32 InternalNext();

  /// The current path of the folder, in which the iterator currently is.
  WStringBuilder m_sCurPath;

  WBitflags<WFileSystemIteratorFlags> m_Flags;

  /// The stats about the file that the iterator currently points to.
  WFileStats m_CurFile;

  /// Platform specific data, required by the implementation.
  WFileIterationData m_Data;

  WString m_sSearchTerm;
  WString m_sMultiSearchTerm;
  WUInt32 m_uiCurrentStartFolder = 0;
  WHybridArray<WString, 8> m_StartFolders;
};

#endif

/// This is an abstraction for the most important file operations.
///
/// Instances of WOSFile can be used for reading and writing files.
/// All paths must be absolute paths, relative paths and current working directories are not supported,
/// since that cannot be guaranteed to work equally on all platforms under all circumstances.
/// A few static functions allow to query the most important data about files, to delete files and create directories.
class W_FOUNDATION_DLL WOSFile
{
  W_DISALLOW_COPY_AND_ASSIGN(WOSFile);

public:
  WOSFile();
  ~WOSFile();

  /// Opens a file for reading or writing. Returns W_SUCCESS if the file could be opened successfully.
  ///
  /// \param sFile Absolute path to the file to open
  /// \param openMode How to open the file (Read, Write, Append)
  /// \param fileShareMode How the file can be shared with other processes (platform-specific)
  WResult Open(WStringView sFile, WFileOpenMode::Enum openMode, WFileShareMode::Enum fileShareMode = WFileShareMode::Default); // [tested]

  /// Returns true if a file is currently open.
  bool IsOpen() const; // [tested]

  /// Closes the file, if it is currently opened.
  void Close(); // [tested]

  /// Writes the given number of bytes from the buffer into the file. Returns true if all data was successfully written.
  WResult Write(const void* pBuffer, WUInt64 uiBytes); // [tested]

  /// Reads up to the given number of bytes from the file. Returns the actual number of bytes that was read.
  WUInt64 Read(void* pBuffer, WUInt64 uiBytes); // [tested]

  /// Reads the entire file content into the given array
  WUInt64 ReadAll(WDynamicArray<WUInt8>& out_fileContent); // [tested]

  /// Returns the name of the file that is currently opened. Returns an empty string, if no file is open.
  WStringView GetOpenFileName() const { return m_sFileName; } // [tested]

  /// Returns the position in the file at which read/write operations will occur.
  WUInt64 GetFilePosition() const; // [tested]

  /// Sets the position where in the file to read/write next.
  void SetFilePosition(WInt64 iDistance, WFileSeekMode::Enum pos) const; // [tested]

  /// Returns the current total size of the file.
  WUInt64 GetFileSize() const; // [tested]

  /// This will return the platform specific file data (handles etc.), if you really want to be able to wreak havoc.
  const WOSFileData& GetFileData() const { return m_FileData; }

  /// Returns the processes current working directory (CWD).
  ///
  /// The value typically depends on the directory from which the application was launched.
  /// Since this is a process wide global variable, other code can modify it at any time.
  ///
  /// \note W does not use the CWD for any file resolution. This function is provided to enable
  /// tools to work with relative paths from the command-line, but every application has to implement
  /// such behavior individually.
  static const WString GetCurrentWorkingDirectory(); // [tested]

  /// If szPath is a relative path, this function prepends GetCurrentWorkingDirectory().
  ///
  /// In either case, MakeCleanPath() is used before the string is returned.
  static const WString MakePathAbsoluteWithCWD(WStringView sPath); // [tested]

  /// Checks whether the given file exists.
  static bool ExistsFile(WStringView sFile); // [tested]

  /// Checks whether the given directory exists.
  static bool ExistsDirectory(WStringView sDirectory); // [tested]

  /// If the given file already exists, determines a file path that doesn't exist yet.
  ///
  /// If the original file already exists, sSuffix is appended and then a number starting at 1.
  /// Loops until it finds a filename that is not yet taken.
  static void FindFreeFilename(WStringBuilder& inout_sPath, WStringView sSuffix = "-");

  /// Deletes the given file. Returns W_SUCCESS, if the file was deleted or did not exist in the first place. Returns W_FAILURE
  static WResult DeleteFile(WStringView sFile); // [tested]

  /// Creates the given directory structure (meaning all directories in the path, that do not exist). Returns false, if any directory could not
  /// be created.
  static WResult CreateDirectoryStructure(WStringView sDirectory); // [tested]

  /// Renames / Moves an existing directory. The file / directory at szFrom must exist. The parent directory of szTo must exist.
  /// Returns W_FAILURE if the move failed.
  static WResult MoveFileOrDirectory(WStringView sFrom, WStringView sTo);

  /// Copies the source file into the destination file.
  static WResult CopyFile(WStringView sSource, WStringView sDestination); // [tested]

#if W_ENABLED(W_SUPPORTS_FILE_STATS) || defined(W_DOCS)
  /// Gets the stats about the given file or folder. Returns false, if the stats could not be determined.
  static WResult GetFileStats(WStringView sFileOrFolder, WFileStats& out_stats); // [tested]

#  if (W_ENABLED(W_SUPPORTS_CASE_INSENSITIVE_PATHS) && W_ENABLED(W_SUPPORTS_UNRESTRICTED_FILE_ACCESS)) || defined(W_DOCS)
  /// Useful on systems that are not strict about the casing of file names. Determines the correct name of a file.
  static WResult GetFileCasing(WStringView sFileOrFolder, WStringBuilder& out_sCorrectSpelling); // [tested]
#  endif

#endif

#if (W_ENABLED(W_SUPPORTS_FILE_ITERATORS) && W_ENABLED(W_SUPPORTS_FILE_STATS)) || defined(W_DOCS)

  /// Returns the WFileStats for all files and folders in the given folder
  static void GatherAllItemsInFolder(WDynamicArray<WFileStats>& out_itemList, WStringView sFolder, WBitflags<WFileSystemIteratorFlags> flags = WFileSystemIteratorFlags::Default);

  /// Copies \a szSourceFolder to \a szDestinationFolder. Overwrites existing files.
  ///
  /// If \a out_FilesCopied is provided, the destination path of every successfully copied file is appended to it.
  static WResult CopyFolder(WStringView sSourceFolder, WStringView sDestinationFolder, WDynamicArray<WString>* out_pFilesCopied = nullptr);

  /// Deletes all files recursively in \a szFolder.
  static WResult DeleteFolder(WStringView sFolder);

#endif

  /// Returns the full path to the application binary.
  static WStringView GetApplicationPath();

  /// Returns the path to the directory in which the application binary is located.
  static WStringView GetApplicationDirectory();

  /// Returns the folder into which user data may be safely written.
  /// Append a sub-folder for your application.
  ///
  /// On Windows this is the '%appdata%' directory.
  /// On Posix systems this is the '~' (home) directory.
  ///
  /// If szSubFolder is specified, it will be appended to the result.
  static WString GetUserDataFolder(WStringView sSubFolder = {});

  /// Returns the folder into which temp data may be written.
  ///
  /// On Windows this is the '%localappdata%/Temp' directory.
  /// On Posix systems this is the '~/.cache' directory.
  ///
  /// If szSubFolder is specified, it will be appended to the result.
  static WString GetTempDataFolder(WStringView sSubFolder = {});

  /// Returns the folder into which the user may want to store documents.
  /// Append a sub-folder for your application.
  ///
  /// On Windows this is the 'Documents' directory.
  /// On Posix systems this is the '~' (home) directory.
  ///
  /// If szSubFolder is specified, it will be appended to the result.
  static WString GetUserDocumentsFolder(WStringView sSubFolder = {});


public:
  /// Describes the types of events that WOSFile sends.
  struct EventType
  {
    enum Enum
    {
      None,
      FileOpen,        ///< A file has been (attempted) to open.
      FileClose,       ///< An open file has been closed.
      FileExists,      ///< A check whether a file exists has been done.
      DirectoryExists, ///< A check whether a directory exists has been done.
      FileDelete,      ///< A file was attempted to be deleted.
      FileRead,        ///< From an open file data was read.
      FileWrite,       ///< Data was written to an open file.
      MakeDir,         ///< A path has been created (recursive directory creation).
      FileCopy,        ///< A file has been copied to another location.
      FileStat,        ///< The stats of a file are queried
      FileCasing,      ///< The exact spelling of a file/path is requested
    };
  };

  /// The data that is sent through the event interface.
  struct EventData
  {
    /// The type of information that is sent.
    EventType::Enum m_EventType = EventType::None;

    /// A unique ID for each file access. Reads and writes to the same open file use the same ID. If the same file is opened multiple times,
    /// different IDs are used.
    WInt32 m_iFileID = 0;

    /// The name of the file that was operated upon.
    WStringView m_sFile;

    /// If a second file was operated upon (FileCopy), that is the second file name.
    WStringView m_sFile2;

    /// Mode that a file has been opened in.
    WFileOpenMode::Enum m_FileMode = WFileOpenMode::None;

    /// Whether the operation succeeded (reading, writing, etc.)
    bool m_bSuccess = true;

    /// How long the operation took.
    WTime m_Duration;

    /// How many bytes were transfered (reading, writing)
    WUInt64 m_uiBytesAccessed = 0;
  };

  using Event = WEvent<const EventData&, WMutex>;

  /// Allows to register a function as an event receiver. All receivers will be notified in the order that they registered.
  static void AddEventHandler(Event::Handler handler) { s_FileEvents.AddEventHandler(handler); }

  /// Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveEventHandler(Event::Handler handler) { s_FileEvents.RemoveEventHandler(handler); }

private:
  /// Manages all the Event Handlers for the OSFile events.
  static Event s_FileEvents;

  // *** Internal Functions that do the platform specific work ***

  WResult InternalOpen(WStringView sFile, WFileOpenMode::Enum OpenMode, WFileShareMode::Enum FileShareMode);
  void InternalClose();
  WResult InternalWrite(const void* pBuffer, WUInt64 uiBytes);
  WUInt64 InternalRead(void* pBuffer, WUInt64 uiBytes);
  WUInt64 InternalGetFilePosition() const;
  void InternalSetFilePosition(WInt64 iDistance, WFileSeekMode::Enum Pos) const;

  static bool InternalExistsFile(WStringView sFile);
  static bool InternalExistsDirectory(WStringView sDirectory);
  static WResult InternalDeleteFile(WStringView sFile);
  static WResult InternalDeleteDirectory(WStringView sDirectory);
  static WResult InternalCreateDirectory(WStringView sFile);
  static WResult InternalMoveFileOrDirectory(WStringView sDirectoryFrom, WStringView sDirectoryTo);

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  static WResult InternalGetFileStats(WStringView sFileOrFolder, WFileStats& out_Stats);
#endif

  // *************************************************************

  /// Stores the mode with which the file was opened.
  WFileOpenMode::Enum m_FileMode;

  /// [internal] On win32 when a file is already open, and this is true, WOSFile will wait until the file becomes available
  bool m_bRetryOnSharingViolation = true;

  /// Stores the (cleaned up) filename that was used to open the file.
  WStringBuilder m_sFileName;

  /// Stores the value of s_FileCounter when the WOSFile is created.
  WInt32 m_iFileID;

  /// Platform specific data about the open file.
  WOSFileData m_FileData;

  /// The application binary's path.
  static WString64 s_sApplicationPath;

  /// The path where user data is stored on this OS
  static WString64 s_sUserDataPath;

  /// The path where temp data is stored on this OS
  static WString64 s_sTempDataPath;

  /// The path where user data documents are stored on this OS
  static WString64 s_sUserDocumentsPath;

  /// Counts how many different files are touched.225
  static WAtomicInteger32 s_iFileCounter;
};
