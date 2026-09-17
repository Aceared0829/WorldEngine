#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

static WInt32 s_iDataDirCounter = 0;
static WMap<WString, WInt32, WCompareHelper<WString>, WStaticsAllocatorWrapper> s_KnownDataDirs;

static void FileSystemEventHandler(const WFileSystem::FileEvent& e)
{
  switch (e.m_EventType)
  {
    case WFileSystem::FileEventType::AddDataDirectorySucceeded:
    {
      bool bExisted = false;
      auto it = s_KnownDataDirs.FindOrAdd(e.m_sFileOrDirectory, &bExisted);

      if (!bExisted)
      {
        it.Value() = s_iDataDirCounter;
        ++s_iDataDirCounter;
      }

      WStringBuilder sName;
      sName.SetFormat("IO/DataDirs/Dir{0}", WArgI(it.Value(), 2, true));

      WStats::SetStat(sName.GetData(), e.m_sFileOrDirectory);
    }
    break;

    case WFileSystem::FileEventType::RemoveDataDirectory:
    {
      auto it = s_KnownDataDirs.Find(e.m_sFileOrDirectory);

      if (!it.IsValid())
        break;

      WStringBuilder sName;
      sName.SetFormat("IO/DataDirs/Dir{0}", WArgI(it.Value(), 2, true));

      WStats::RemoveStat(sName.GetData());
    }
    break;

    default:
      break;
  }
}

void AddFileSystemEventHandler()
{
  WFileSystem::RegisterEventHandler(FileSystemEventHandler);
}

void RemoveFileSystemEventHandler()
{
  WFileSystem::UnregisterEventHandler(FileSystemEventHandler);
}
