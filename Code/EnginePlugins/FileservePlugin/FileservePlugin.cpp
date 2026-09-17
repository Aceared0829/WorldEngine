#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveDataDir.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(FileservePlugin, FileservePluginMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WFileSystem::RegisterDataDirectoryFactory(WDataDirectory::FileserveType::Factory, 100.0f);

    if (WStartup::HasApplicationTag("tool") ||
        WStartup::HasApplicationTag("testframework")) // the testframework configures a fileserve client itself
      return;

    WFileserveClient* fs = WFileserveClient::GetSingleton();

    if (fs == nullptr)
    {
      fs = W_DEFAULT_NEW(WFileserveClient);
      W_IGNORE_UNUSED(fs);

      // on sandboxed platforms we must go through fileserve, so we enforce a fileserve connection
      // on unrestricted platforms, we use fileserve, if a connection can be established,
      // but if the connection times out, we fall back to regular file accesses
#if W_DISABLED(W_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
      if (fs->SearchForServerAddress().Failed())
      {
        fs->WaitForServerInfo().IgnoreResult();
      }
#endif
    }
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (WStartup::HasApplicationTag("tool") ||
        WStartup::HasApplicationTag("testframework"))
      return;

    if (WFileserveClient::GetSingleton() != nullptr)
    {
      WFileserveClient* pSingleton = WFileserveClient::GetSingleton();
      W_DEFAULT_DELETE(pSingleton);
    }
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on


W_STATICLINK_FILE(FileServePlugin, FileServePlugin_FileservePlugin);
