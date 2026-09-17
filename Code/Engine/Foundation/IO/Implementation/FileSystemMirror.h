#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/UniquePtr.h>

// A general problem when implementing a directory watcher is, that moving a folder out of the watched directory only communicates
// Which folder was moved (but not to where, nor its contents). This means when a folder is moved out of view,
// this needs to be treated as a delete. At the point of the move, it is no longer possible to query the contents of the folder.
// So a in memory copy of the file system is required in order to correctly implement a directory watcher.
template <typename T>
class WFileSystemMirror
{
public:
  enum class Type
  {
    File,
    Directory
  };

  struct DirEntry
  {
    WMap<WString, DirEntry> m_subDirectories;
    WMap<WString, T> m_files;
  };

  WFileSystemMirror();
  ~WFileSystemMirror();

  // Adds the directory, and all files in it recursively.
  WResult AddDirectory(WStringView sPath, bool* out_pDirectoryExistsAlready = nullptr);

  // Adds a file. Creates directories if they do not exist.
  WResult AddFile(WStringView sPath, const T& value, bool* out_pFileExistsAlready, T* out_pOldValue);

  // Removes a file.
  WResult RemoveFile(WStringView sPath);

  // Removes a directory. Deletes any files & directories inside.
  WResult RemoveDirectory(WStringView sPath);

  // Moves a directory. Any files & folders inside are moved with it.
  WResult MoveDirectory(WStringView sFromPath, WStringView sToPath);

  using EnumerateFunc = WDelegate<void(const WStringBuilder& path, Type type)>;

  // Enumerates the files & directories under the given path
  WResult Enumerate(WStringView sPath, EnumerateFunc callbackFunc);

  // On success, out_Type will contains the type of the object (file or folder).
  WResult GetType(WStringView sPath, Type& out_type);

private:
  DirEntry* FindDirectory(WStringBuilder& path);

private:
  DirEntry m_TopLevelDir;
  WString m_sTopLevelDirPath;
};

namespace
{
  void EnsureTrailingSlash(WStringBuilder& ref_sBuilder)
  {
    if (!ref_sBuilder.EndsWith("/"))
    {
      ref_sBuilder.Append("/");
    }
  }

  void RemoveTrailingSlash(WStringBuilder& ref_sBuilder)
  {
    if (ref_sBuilder.EndsWith("/"))
    {
      ref_sBuilder.Shrink(0, 1);
    }
  }
} // namespace

template <typename T>
WFileSystemMirror<T>::WFileSystemMirror() = default;

template <typename T>
WFileSystemMirror<T>::~WFileSystemMirror() = default;

template <typename T>
WResult WFileSystemMirror<T>::AddDirectory(WStringView sPath, bool* out_pDirectoryExistsAlready)
{
  WStringBuilder currentDirAbsPath = sPath;
  currentDirAbsPath.MakeCleanPath();
  EnsureTrailingSlash(currentDirAbsPath);

  if (m_sTopLevelDirPath.IsEmpty())
  {
    m_sTopLevelDirPath = currentDirAbsPath;
    currentDirAbsPath.Shrink(0, 1); // remove trailing /

    DirEntry* currentDir = &m_TopLevelDir;

    WHybridArray<DirEntry*, 16> m_dirStack;

    WFileSystemIterator files;
    files.StartSearch(currentDirAbsPath.GetData(), WFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);
    for (; files.IsValid(); files.Next())
    {
      const WFileStats& stats = files.GetStats();

      // In case we are done with a directory, move back up
      while (currentDirAbsPath != stats.m_sParentPath)
      {
        W_ASSERT_DEV(m_dirStack.GetCount() > 0, "Unexpected file iteration order");
        currentDir = m_dirStack.PeekBack();
        m_dirStack.PopBack();
        currentDirAbsPath.PathParentDirectory();
        RemoveTrailingSlash(currentDirAbsPath);
      }

      if (stats.m_bIsDirectory)
      {
        m_dirStack.PushBack(currentDir);
        WStringBuilder subdirName = stats.m_sName;
        EnsureTrailingSlash(subdirName);
        auto insertIt = currentDir->m_subDirectories.Insert(subdirName, DirEntry());
        currentDir = &insertIt.Value();
        currentDirAbsPath.AppendPath(stats.m_sName);
      }
      else
      {
        currentDir->m_files.Insert(std::move(stats.m_sName), T{});
      }
    }
    if (out_pDirectoryExistsAlready != nullptr)
    {
      *out_pDirectoryExistsAlready = false;
    }
  }
  else
  {
    DirEntry* parentDir = FindDirectory(currentDirAbsPath);
    if (parentDir == nullptr)
    {
      return W_FAILURE;
    }

    if (out_pDirectoryExistsAlready != nullptr)
    {
      *out_pDirectoryExistsAlready = currentDirAbsPath.IsEmpty();
    }

    while (!currentDirAbsPath.IsEmpty())
    {
      const char* dirEnd = currentDirAbsPath.FindSubString("/");
      WStringView subdirName(currentDirAbsPath.GetData(), dirEnd + 1);
      auto insertIt = parentDir->m_subDirectories.Insert(subdirName, DirEntry());
      parentDir = &insertIt.Value();
      currentDirAbsPath.Shrink(WStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()), 0);
    }
  }

  return W_SUCCESS;
}

template <typename T>
WResult WFileSystemMirror<T>::AddFile(WStringView sPath0, const T& value, bool* out_pFileExistsAlready, T* out_pOldValue)
{
  WStringBuilder sPath = sPath0;
  DirEntry* dir = FindDirectory(sPath);
  if (dir == nullptr)
  {
    return W_FAILURE; // file not under top level directory
  }

  const char* szSlashPos = sPath.FindSubString("/");

  while (szSlashPos != nullptr)
  {
    WStringView subdirName(sPath.GetData(), szSlashPos + 1);
    auto insertIt = dir->m_subDirectories.Insert(subdirName, DirEntry());
    dir = &insertIt.Value();
    sPath.Shrink(WStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()), 0);
    szSlashPos = sPath.FindSubString("/");
  }

  auto it = dir->m_files.Find(sPath);
  // Do not add the file twice
  if (!it.IsValid())
  {
    dir->m_files.Insert(sPath, value);
    if (out_pFileExistsAlready != nullptr)
    {
      *out_pFileExistsAlready = false;
    }
  }
  else
  {
    if (out_pFileExistsAlready != nullptr)
    {
      *out_pFileExistsAlready = true;
    }
    if (out_pOldValue != nullptr)
    {
      *out_pOldValue = it.Value();
    }
    it.Value() = value;
  }
  return W_SUCCESS;
}

template <typename T>
WResult WFileSystemMirror<T>::RemoveFile(WStringView sPath0)
{
  WStringBuilder sPath = sPath0;
  DirEntry* dir = FindDirectory(sPath);
  if (dir == nullptr)
  {
    return W_FAILURE; // file not under top level directory
  }

  if (sPath.FindSubString("/") != nullptr)
  {
    return W_FAILURE; // file does not exist
  }

  if (dir->m_files.GetCount() == 0)
  {
    return W_FAILURE; // there are no files in this directory
  }

  auto it = dir->m_files.Find(sPath);
  if (!it.IsValid())
  {
    return W_FAILURE; // file does not exist
  }

  dir->m_files.Remove(it);
  return W_SUCCESS;
}

template <typename T>
WResult WFileSystemMirror<T>::RemoveDirectory(WStringView sPath)
{
  WStringBuilder parentPath = sPath;
  WStringBuilder dirName = sPath;
  parentPath.PathParentDirectory();
  EnsureTrailingSlash(parentPath);
  dirName.Shrink(parentPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(dirName);

  DirEntry* parentDir = FindDirectory(parentPath);
  if (parentDir == nullptr || !parentPath.IsEmpty())
  {
    return W_FAILURE;
  }

  if (!parentDir->m_subDirectories.Remove(dirName))
  {
    return W_FAILURE;
  }

  return W_SUCCESS;
}

template <typename T>
WResult WFileSystemMirror<T>::MoveDirectory(WStringView sFromPath0, WStringView sToPath0)
{
  WStringBuilder sFromPath = sFromPath0;
  WStringBuilder sFromName = sFromPath0;
  sFromPath.PathParentDirectory();
  EnsureTrailingSlash(sFromPath);
  sFromName.Shrink(sFromPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(sFromName);


  WStringBuilder sToPath = sToPath0;
  WStringBuilder sToName = sToPath0;
  sToPath.PathParentDirectory();
  EnsureTrailingSlash(sToPath);
  sToName.Shrink(sToPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(sToName);

  DirEntry* moveFromDir = FindDirectory(sFromPath);
  if (!moveFromDir)
  {
    return W_FAILURE;
  }
  W_ASSERT_DEV(sFromPath.IsEmpty(), "move from directory should fully exist");

  DirEntry* moveToDir = FindDirectory(sToPath);
  if (!moveToDir)
  {
    return W_FAILURE;
  }

  if (!sToPath.IsEmpty())
  {
    do
    {
      const char* dirEnd = sToPath.FindSubString("/");
      WStringView subdirName(sToPath.GetData(), dirEnd + 1);
      auto insertIt = moveToDir->m_subDirectories.Insert(subdirName, DirEntry());
      moveToDir = &insertIt.Value();
      sToPath.Shrink(0, WStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()));
    } while (!sToPath.IsEmpty());
  }

  DirEntry movedDir;
  {
    auto fromIt = moveFromDir->m_subDirectories.Find(sFromName);
    if (!fromIt.IsValid())
    {
      return W_FAILURE;
    }

    movedDir = std::move(fromIt.Value());
    moveFromDir->m_subDirectories.Remove(fromIt);
  }

  moveToDir->m_subDirectories.Insert(sToName, std::move(movedDir));

  return W_SUCCESS;
}

namespace
{
  template <typename T>
  struct WDirEnumerateState
  {
    typename WFileSystemMirror<T>::DirEntry* dir;
    typename WMap<WString, typename WFileSystemMirror<T>::DirEntry>::Iterator subDirIt;
  };
} // namespace

template <typename T>
WResult WFileSystemMirror<T>::Enumerate(WStringView sPath0, EnumerateFunc callbackFunc)
{
  WHybridArray<WDirEnumerateState<T>, 16> dirStack;
  WStringBuilder sPath = sPath0;
  if (!sPath.EndsWith("/"))
  {
    sPath.Append("/");
  }
  DirEntry* dirToEnumerate = FindDirectory(sPath);
  if (dirToEnumerate == nullptr)
  {
    return W_FAILURE;
  }
  if (!sPath.IsEmpty())
  {
    return W_FAILURE; // requested folder to enumerate doesn't exist
  }
  DirEntry* currentDir = dirToEnumerate;
  typename WMap<WString, WFileSystemMirror::DirEntry>::Iterator currentSubDirIt = currentDir->m_subDirectories.GetIterator();
  sPath = sPath0;

  while (currentDir != nullptr)
  {
    if (currentSubDirIt.IsValid())
    {
      DirEntry* nextDir = &currentSubDirIt.Value();
      sPath.AppendPath(currentSubDirIt.Key());
      currentSubDirIt.Next();
      dirStack.PushBack({currentDir, currentSubDirIt});
      currentDir = nextDir;
    }
    else
    {
      WStringBuilder sFilePath;
      for (auto& file : currentDir->m_files)
      {
        sFilePath = sPath;
        sFilePath.AppendPath(file.Key());
        callbackFunc(sFilePath, Type::File);
      }

      if (currentDir != dirToEnumerate)
      {
        if (sPath.EndsWith("/") && sPath.GetElementCount() > 1)
        {
          sPath.Shrink(0, 1);
        }
        callbackFunc(sPath, Type::Directory);
      }

      if (dirStack.IsEmpty())
      {
        currentDir = nullptr;
      }
      else
      {
        currentDir = dirStack.PeekBack().dir;
        currentSubDirIt = dirStack.PeekBack().subDirIt;
        dirStack.PopBack();
        sPath.PathParentDirectory();
        if (sPath.GetElementCount() > 1 && sPath.EndsWith("/"))
        {
          sPath.Shrink(0, 1);
        }
      }
    }
  }

  return W_SUCCESS;
}

template <typename T>
WResult WFileSystemMirror<T>::GetType(WStringView sPath0, Type& out_type)
{
  WStringBuilder sPath = sPath0;
  DirEntry* dir = FindDirectory(sPath);
  if (dir == nullptr)
  {
    return W_FAILURE; // file not under top level directory
  }

  auto it = dir->m_files.Find(sPath);
  if (it.IsValid())
  {
    out_type = WFileSystemMirror::Type::File;
    return W_SUCCESS;
  }

  auto itDir = dir->m_subDirectories.Find(sPath);
  if (itDir.IsValid())
  {
    out_type = WFileSystemMirror::Type::Directory;
    return W_SUCCESS;
  }

  return W_FAILURE;
}

template <typename T>
typename WFileSystemMirror<T>::DirEntry* WFileSystemMirror<T>::FindDirectory(WStringBuilder& path)
{
  if (!path.StartsWith(m_sTopLevelDirPath))
  {
    return nullptr;
  }
  path.TrimWordStart(m_sTopLevelDirPath);

  DirEntry* currentDir = &m_TopLevelDir;

  bool found = false;
  do
  {
    found = false;
    for (auto& dir : currentDir->m_subDirectories)
    {
      if (path.StartsWith(dir.Key()))
      {
        currentDir = &dir.Value();
        path.TrimWordStart(dir.Key());
        path.TrimWordStart("/");
        found = true;
        break;
      }
    }
  } while (found);

  return currentDir;
}
