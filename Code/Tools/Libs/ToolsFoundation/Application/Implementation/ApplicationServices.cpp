#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Project/ToolsProject.h>

W_IMPLEMENT_SINGLETON(WApplicationServices);

static WApplicationServices g_instance;

WApplicationServices::WApplicationServices()
  : m_SingletonRegistrar(this)
{
}

WString WApplicationServices::GetApplicationUserDataFolder() const
{
  WStringBuilder path = WOSFile::GetUserDataFolder();
  path.AppendPath("WorldEngine Project", WApplication::GetApplicationInstance()->GetApplicationName());
  path.MakeCleanPath();

  return path;
}

WString WApplicationServices::GetApplicationDataFolder() const
{
  WStringBuilder sAppDir(">sdk/Data/Tools/", WApplication::GetApplicationInstance()->GetApplicationName());

  WStringBuilder result;
  WFileSystem::ResolveSpecialDirectory(sAppDir, result).IgnoreResult();
  result.MakeCleanPath();

  return result;
}

WString WApplicationServices::GetApplicationPreferencesFolder() const
{
  return GetApplicationUserDataFolder();
}

WString WApplicationServices::GetProjectPreferencesFolder() const
{
  return GetProjectPreferencesFolder(WToolsProject::GetSingleton()->GetProjectDirectory());
}

WString WApplicationServices::GetProjectPreferencesFolder(WStringView sProjectFilePath) const
{
  WStringBuilder path = GetApplicationUserDataFolder();

  sProjectFilePath.TrimWordEnd("WProject");
  sProjectFilePath.TrimWordEnd("WRemoteProject");
  sProjectFilePath.Trim("/\\");

  WStringBuilder ProjectName = sProjectFilePath;

  WStringBuilder ProjectPath = ProjectName;
  ProjectPath.PathParentDirectory();

  const WUInt64 uiPathHash = WHashingUtils::StringHash(ProjectPath.GetData());

  ProjectName = ProjectName.GetFileName();

  path.AppendFormat("/Projects/{}_{}", uiPathHash, ProjectName);

  path.MakeCleanPath();
  return path;
}

WString WApplicationServices::GetDocumentPreferencesFolder(const WDocument* pDocument) const
{
  WStringBuilder path = GetProjectPreferencesFolder();

  WStringBuilder sGuid;
  WConversionUtils::ToString(pDocument->GetGuid(), sGuid);

  path.AppendPath(sGuid);

  path.MakeCleanPath();
  return path;
}

WString WApplicationServices::GetPrecompiledToolsFolder(bool bUsePrecompiledTools) const
{
  if (bUsePrecompiledTools)
  {
    // Don't derive this from the application directory through a fixed number of "../" hops -- that assumes
    // a specific output directory depth (e.g. the default "Output/Bin/<Config>") and breaks for custom
    // build layouts (e.g. a custom -WorkspaceDir with an extra output folder level). The SDK root is already
    // known reliably (auto-detected by searching upwards for "WSdkRoot.txt"), so anchor to that instead.
    WFileSystem::DetectSdkRootDirectory().IgnoreResult();

    WStringBuilder sPath = WFileSystem::GetSdkRootDirectory();
    sPath.AppendPath("Data/Tools/Precompiled");
    sPath.MakeCleanPath();
    return sPath;
  }

  WStringBuilder sPath = WOSFile::GetApplicationDirectory();
  sPath.MakeCleanPath();
  return sPath;
}

WString WApplicationServices::GetSampleProjectsFolder() const
{
  WStringBuilder sPath = WOSFile::GetApplicationDirectory();

  sPath.AppendPath("../../../Data/Samples");

  sPath.MakeCleanPath();

  return sPath;
}
