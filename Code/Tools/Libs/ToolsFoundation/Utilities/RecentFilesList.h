#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// Maintains a list of recently used files and the container window ID they previously resided in.
class W_TOOLSFOUNDATION_DLL WRecentFilesList
{
public:
  WRecentFilesList(WUInt32 uiMaxElements) { m_uiMaxElements = uiMaxElements; }

  /// Struct that defines the file and container window of the recent file list.
  struct RecentFile
  {
    RecentFile()
      : m_iContainerWindow(0)
    {
    }
    RecentFile(WStringView sFile, WInt32 iContainerWindow)
      : m_File(sFile)
      , m_iContainerWindow(iContainerWindow)
    {
    }

    WString m_File;
    WInt32 m_iContainerWindow;
  };
  /// Moves the inserted file to the front with the given container ID.
  void Insert(WStringView sFile, WInt32 iContainerWindow);

  /// Returns all files in the list.
  const WDeque<RecentFile>& GetFileList() const { return m_Files; }

  /// Clears the list.
  void Clear() { m_Files.Clear(); }

  /// Saves the recent files list to the given file. Uses a simple text file format (one line per item).
  void Save(WStringView sFile);

  /// Loads the recent files list from the given file. Uses a simple text file format (one line per item).
  void Load(WStringView sFile);

private:
  WUInt32 m_uiMaxElements;
  WDeque<RecentFile> m_Files;
};
