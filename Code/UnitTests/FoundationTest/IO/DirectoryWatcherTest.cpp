#include <FoundationTest/FoundationTestPCH.h>

#if W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Threading/ThreadUtils.h>

namespace DirectoryWatcherTestHelpers
{

  struct ExpectedEvent
  {
    ~ExpectedEvent(){}; // NOLINT: To make it non-pod

    const char* path;
    WDirectoryWatcherAction action;
    WDirectoryWatcherType type;

    bool operator==(const ExpectedEvent& other) const
    {
      return WStringView(path) == WStringView(other.path) && action == other.action && type == other.type;
    }
  };

  struct ExpectedEventStorage
  {
    WString path;
    WDirectoryWatcherAction action;
    WDirectoryWatcherType type;
  };

  void TickWatcher(WDirectoryWatcher& ref_watcher)
  {
    ref_watcher.EnumerateChanges([&](WStringView sPath, WDirectoryWatcherAction action, WDirectoryWatcherType type) {},
      WTime::MakeFromMilliseconds(100));
  }
} // namespace DirectoryWatcherTestHelpers


void DirectoryWatcherTest()
{
  using namespace DirectoryWatcherTestHelpers;

  WStringBuilder tmp, tmp2;
  WStringBuilder sTestRootPath = WTestFramework::GetInstance()->GetAbsOutputPath();
  sTestRootPath.AppendPath("DirectoryWatcher/");

  auto CheckExpectedEvents = [&](WDirectoryWatcher& ref_watcher, WArrayPtr<ExpectedEvent> events)
  {
    WDynamicArray<ExpectedEventStorage> firedEvents;
    WUInt32 i = 0;
    ref_watcher.EnumerateChanges([&](WStringView sPath, WDirectoryWatcherAction action, WDirectoryWatcherType type)
      {
      tmp = sPath;
      tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
      firedEvents.PushBack({tmp, action, type});
      if (i < events.GetCount())
      {
        W_TEST_BOOL_MSG(tmp == events[i].path, "Expected event at index %d path mismatch: '%s' vs '%s'", i, tmp.GetData(), events[i].path);
        W_TEST_BOOL_MSG(action == events[i].action, "Expected event at index %d action", i);
        W_TEST_BOOL_MSG(type == events[i].type, "Expected event at index %d type mismatch", i);
      }
      i++; },
      WTime::MakeFromMilliseconds(100));
    W_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CheckExpectedEventsUnordered = [&](WDirectoryWatcher& ref_watcher, WArrayPtr<ExpectedEvent> events)
  {
    WDynamicArray<ExpectedEventStorage> firedEvents;
    WUInt32 i = 0;
    WDynamicArray<bool> eventFired;
    eventFired.SetCount(events.GetCount());
    ref_watcher.EnumerateChanges([&](WStringView sPath, WDirectoryWatcherAction action, WDirectoryWatcherType type)
      {
        tmp = sPath;
        tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
        firedEvents.PushBack({tmp, action, type});
        auto index = events.IndexOf({tmp, action, type});
        W_TEST_BOOL_MSG(index != WInvalidIndex, "Event %d (%s, %d, %d) not found in expected events list", i, tmp.GetData(), (int)action, (int)type);
        if (index != WInvalidIndex)
        {
          eventFired[index] = true;
        }
        i++;
        //
      },
      WTime::MakeFromMilliseconds(100));
    for (auto& fired : eventFired)
    {
      W_TEST_BOOL(fired);
    }
    W_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CheckExpectedEventsMultiple = [&](WArrayPtr<WDirectoryWatcher*> watchers, WArrayPtr<ExpectedEvent> events)
  {
    WDynamicArray<ExpectedEventStorage> firedEvents;
    WUInt32 i = 0;
    WDirectoryWatcher::EnumerateChanges(
      watchers, [&](WStringView sPath, WDirectoryWatcherAction action, WDirectoryWatcherType type)
      {
        tmp = sPath;
        tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
        firedEvents.PushBack({tmp, action, type});
        if (i < events.GetCount())
        {
          W_TEST_BOOL_MSG(tmp == events[i].path, "Expected event at index %d path mismatch: '%s' vs '%s'", i, tmp.GetData(), events[i].path);
          W_TEST_BOOL_MSG(action == events[i].action, "Expected event at index %d action", i);
          W_TEST_BOOL_MSG(type == events[i].type, "Expected event at index %d type mismatch", i);
        }
        i++;
        //
      },
      WTime::MakeFromMilliseconds(100));
    W_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CreateFile = [&](const char* szRelPath)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);

    WOSFile file;
    W_TEST_BOOL(file.Open(tmp, WFileOpenMode::Write).Succeeded());
    W_TEST_BOOL(file.Write("Hello World", 11).Succeeded());
  };

  auto ModifyFile = [&](const char* szRelPath)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);

    WOSFile file;
    W_TEST_BOOL(file.Open(tmp, WFileOpenMode::Append).Succeeded());
    W_TEST_BOOL(file.Write("Hello World", 11).Succeeded());
  };

  auto DeleteFile = [&](const char* szRelPath)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    W_TEST_BOOL(WOSFile::DeleteFile(tmp).Succeeded());
  };

  auto CreateDirectory = [&](const char* szRelPath)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    W_TEST_BOOL(WOSFile::CreateDirectoryStructure(tmp).Succeeded());
  };

  auto Rename = [&](const char* szFrom, const char* szTo)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szFrom);

    tmp2 = sTestRootPath;
    tmp2.AppendPath(szTo);

    W_TEST_BOOL(WOSFile::MoveFileOrDirectory(tmp, tmp2).Succeeded());
  };

  auto DeleteDirectory = [&](const char* szRelPath, bool bTest = true)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    tmp.MakeCleanPath();

    if (bTest)
    {
      W_TEST_BOOL(WOSFile::DeleteFolder(tmp).Succeeded());
    }
    else
    {
      WOSFile::DeleteFolder(tmp).IgnoreResult();
    }
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "git")
  {
    WOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    W_TEST_BOOL(WOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    CreateFile("index");

    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Deletes | WDirectoryWatcher::Watch::Renames | WDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("index.lock");
    DeleteFile("index");
    Rename("index.lock", "index");

    ExpectedEvent expectedEvents[] = {
      {"index.lock", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
      {"index", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
      {"index.lock", WDirectoryWatcherAction::RenamedOldName, WDirectoryWatcherType::File},
      {"index", WDirectoryWatcherAction::RenamedNewName, WDirectoryWatcherType::File},

    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Simple Create File")
  {
    WOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    W_TEST_BOOL(WOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Writes).Succeeded());

    CreateFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Simple delete file")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Deletes).Succeeded());

    DeleteFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Simple modify file")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Writes).Succeeded());

    CreateFile("test.file");

    TickWatcher(watcher);

    ModifyFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", WDirectoryWatcherAction::Modified, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("test.file");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Simple rename file")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Renames | WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes | WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("test.file");
    Rename("test.file", "supertest.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
      {"test.file", WDirectoryWatcherAction::RenamedOldName, WDirectoryWatcherType::File},
      {"supertest.file", WDirectoryWatcherAction::RenamedNewName, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("supertest.file");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Change file casing")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Renames | WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes | WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("rename.file");
    Rename("rename.file", "Rename.file");

    ExpectedEvent expectedEvents[] = {
      {"rename.file", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
      {"rename.file", WDirectoryWatcherAction::RenamedOldName, WDirectoryWatcherType::File},
      {"Rename.file", WDirectoryWatcherAction::RenamedNewName, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("Rename.file");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Windows check for correct handling of pending file remove event #1")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Renames | WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes | WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("rename.file");
    DeleteFile("rename.file");

    ExpectedEvent expectedEvents[] = {
      {"rename.file", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
      {"rename.file", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Windows check for correct handling of pending file remove event #2")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Renames | WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes | WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("rename.file");
    DeleteFile("rename.file");
    CreateFile("Rename.file");
    DeleteFile("Rename.file");

    ExpectedEvent expectedEvents[] = {
      {"rename.file", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
      {"rename.file", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
      {"Rename.file", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
      {"Rename.file", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Simple create directory")
  {
    WOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    W_TEST_BOOL(WOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Creates).Succeeded());

    CreateDirectory("testDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Simple delete directory")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Deletes).Succeeded());

    DeleteDirectory("testDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Simple rename directory")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Renames).Succeeded());

    CreateDirectory("testDir");
    Rename("testDir", "supertestDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", WDirectoryWatcherAction::RenamedOldName, WDirectoryWatcherType::Directory},
      {"supertestDir", WDirectoryWatcherAction::RenamedNewName, WDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteDirectory("supertestDir");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Change directory casing")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Renames | WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes | WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("renameDir");
    Rename("renameDir", "RenameDir");

    ExpectedEvent expectedEvents[] = {
      {"renameDir", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"renameDir", WDirectoryWatcherAction::RenamedOldName, WDirectoryWatcherType::Directory},
      {"RenameDir", WDirectoryWatcherAction::RenamedNewName, WDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteDirectory("RenameDir");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Windows check for correct handling of pending directory remove event #1")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Renames | WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes | WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("renameDir");
    DeleteDirectory("renameDir");

    ExpectedEvent expectedEvents[] = {
      {"renameDir", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"renameDir", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Windows check for correct handling of pending directory remove event #2")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Renames | WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes | WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("renameDir");
    DeleteDirectory("renameDir");
    CreateDirectory("RenameDir");
    DeleteDirectory("RenameDir");

    ExpectedEvent expectedEvents[] = {
      {"renameDir", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"renameDir", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::Directory},
      {"RenameDir", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"RenameDir", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Subdirectory Create File")
  {
    tmp = sTestRootPath;
    tmp.AppendPath("subdir");
    W_TEST_BOOL(WOSFile::CreateDirectoryStructure(tmp).Succeeded());

    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Subdirectory delete file")
  {

    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Deletes | WDirectoryWatcher::Watch::Subdirectories).Succeeded());

    DeleteFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Subdirectory modify file")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("subdir/test.file");

    TickWatcher(watcher);

    ModifyFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", WDirectoryWatcherAction::Modified, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GUI Create Folder & file")
  {
    DeleteDirectory("sub", false);
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes |
                            WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("New Folder");

    ExpectedEvent expectedEvents1[] = {
      {"New Folder", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    Rename("New Folder", "sub");

    CreateFile("sub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/bla", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/bla");
    DeleteFile("sub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/bla", WDirectoryWatcherAction::Modified, WDirectoryWatcherType::File},
      {"sub/bla", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GUI Create Folder & file fast")
  {
    DeleteDirectory("sub", false);
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes |
                            WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("New Folder");
    Rename("New Folder", "sub");

    ExpectedEvent expectedEvents1[] = {
      {"New Folder", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    CreateFile("sub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/bla", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/bla");
    DeleteFile("sub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/bla", WDirectoryWatcherAction::Modified, WDirectoryWatcherType::File},
      {"sub/bla", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GUI Create Folder & file fast subdir")
  {
    DeleteDirectory("sub", false);

    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes |
                            WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("New Folder/subsub");
    Rename("New Folder", "sub");

    TickWatcher(watcher);

    CreateFile("sub/subsub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/subsub/bla", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/subsub/bla");
    DeleteFile("sub/subsub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/subsub/bla", WDirectoryWatcherAction::Modified, WDirectoryWatcherType::File},
      {"sub/subsub/bla", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);

    DeleteDirectory("sub");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GUI Delete Folder")
  {
    DeleteDirectory("sub2", false);
    DeleteDirectory("../sub2", false);

    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes |
                            WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"sub2", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"sub2/file1", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
      {"sub2/subsub2", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents1);

    Rename("sub2", "../sub2");

    ExpectedEvent expectedEvents2[] = {
      {"sub2/subsub2/file2.txt", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
      {"sub2/subsub2", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::Directory},
      {"sub2/file1", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
      {"sub2", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::Directory},
    };
    // Issue here: After moving sub2 out of view, it remains in m_pathToWd
    CheckExpectedEvents(watcher, expectedEvents2);

    Rename("../sub2", "sub2");

    ExpectedEvent expectedEvents3[] = {
      {"sub2", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"sub2/file1", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
      {"sub2/subsub2", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents3);

    DeleteDirectory("sub2");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Create, Delete, Create")
  {
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes |
                            WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"sub2", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"sub2/file1", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
      {"sub2/subsub2", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents1);

    DeleteDirectory("sub2");

    ExpectedEvent expectedEvents2[] = {
      {"sub2/file1", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
      {"sub2/subsub2/file2.txt", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
      {"sub2/subsub2", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::Directory},
      {"sub2", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::Directory},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents2);

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"sub2", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"sub2/file1", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
      {"sub2/subsub2", WDirectoryWatcherAction::Added, WDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents3);

    DeleteDirectory("sub2");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GUI Create file & delete")
  {
    DeleteDirectory("sub", false);
    WDirectoryWatcher watcher;
    W_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes |
                            WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    CreateFile("file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"file2.txt", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    Rename("file2.txt", "datei2.txt");

    ExpectedEvent expectedEvents2[] = {
      {"file2.txt", WDirectoryWatcherAction::RenamedOldName, WDirectoryWatcherType::File},
      {"datei2.txt", WDirectoryWatcherAction::RenamedNewName, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    DeleteFile("datei2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"datei2.txt", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Enumerate multiple")
  {
    DeleteDirectory("watch1", false);
    DeleteDirectory("watch2", false);
    DeleteDirectory("watch3", false);
    WDirectoryWatcher watchers[3];

    WDirectoryWatcher* pWatchers[] = {watchers + 0, watchers + 1, watchers + 2};

    CreateDirectory("watch1");
    CreateDirectory("watch2");
    CreateDirectory("watch3");

    WStringBuilder watchPath;

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch1");
    W_TEST_BOOL(watchers[0].OpenDirectory(
                              watchPath,
                              WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes |
                                WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch2");
    W_TEST_BOOL(watchers[1].OpenDirectory(
                              watchPath,
                              WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes |
                                WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch3");
    W_TEST_BOOL(watchers[2].OpenDirectory(
                              watchPath,
                              WDirectoryWatcher::Watch::Creates | WDirectoryWatcher::Watch::Deletes |
                                WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    CreateFile("watch1/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"watch1/file2.txt", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents1);

    CreateFile("watch2/file2.txt");

    ExpectedEvent expectedEvents2[] = {
      {"watch2/file2.txt", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents2);

    CreateFile("watch3/file2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"watch3/file2.txt", WDirectoryWatcherAction::Added, WDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents3);

    ModifyFile("watch1/file2.txt");
    ModifyFile("watch2/file2.txt");

    ExpectedEvent expectedEvents4[] = {
      {"watch1/file2.txt", WDirectoryWatcherAction::Modified, WDirectoryWatcherType::File},
      {"watch2/file2.txt", WDirectoryWatcherAction::Modified, WDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents4);

    DeleteFile("watch1/file2.txt");
    DeleteFile("watch2/file2.txt");
    DeleteFile("watch3/file2.txt");

    ExpectedEvent expectedEvents5[] = {
      {"watch1/file2.txt", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
      {"watch2/file2.txt", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
      {"watch3/file2.txt", WDirectoryWatcherAction::Removed, WDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents5);
  }

  WOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
}

W_CREATE_SIMPLE_TEST(IO, DirectoryWatcher)
{
  DirectoryWatcherTest();
}

#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
W_CREATE_SIMPLE_TEST(IO, DirectoryWatcherNonNTFS)
{
  auto* pForceNonNTFS = static_cast<WCVarBool*>(WCVar::FindCVarByName("Platform.DirectoryWatcher.ForceNonNTFS"));
  *pForceNonNTFS = true;
  DirectoryWatcherTest();
  *pForceNonNTFS = false;
}
#  endif

#endif
