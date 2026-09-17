#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/Project/ProjectCreation.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

namespace
{
  WString GetProjectTemplatesFolder()
  {
    WStringBuilder sFolder = WApplicationServices::GetSingleton()->GetApplicationDataFolder();
    sFolder.AppendPath("ProjectTemplates");
    return sFolder;
  }
} // namespace

void WProjectCreation::FindProjectTemplates(WDynamicArray<WString>& out_templateNames)
{
  out_templateNames.Clear();

  WFileSystemIterator fsIt;
  fsIt.StartSearch(GetProjectTemplatesFolder(), WFileSystemIteratorFlags::ReportFolders);

  WStringBuilder path;

  while (fsIt.IsValid())
  {
    fsIt.GetStats().GetFullPath(path);
    path.AppendPath("WProject");

    if (WOSFile::ExistsFile(path))
    {
      out_templateNames.PushBack(fsIt.GetStats().m_sName);
    }

    fsIt.Next();
  }
}

WResult WProjectCreation::FindProjectTemplate(WStringView sTemplateName, WStringBuilder& out_sProjectFile)
{
  if (sTemplateName.IsEmpty())
    return W_FAILURE;

  out_sProjectFile = GetProjectTemplatesFolder();
  out_sProjectFile.AppendPath(sTemplateName);
  out_sProjectFile.AppendPath("WProject");

  if (!WOSFile::ExistsFile(out_sProjectFile))
  {
    out_sProjectFile.Clear();
    return W_FAILURE;
  }

  return W_SUCCESS;
}

void WProjectCreation::FindPluginTemplates(const WPluginBundleSet& pluginBundles, WDynamicArray<WString>& out_templateNames)
{
  out_templateNames.Clear();

  for (auto it : pluginBundles.m_Plugins)
  {
    for (const WString& sTemplate : it.Value().m_EnabledInTemplates)
    {
      if (!out_templateNames.Contains(sTemplate))
      {
        out_templateNames.PushBack(sTemplate);
      }
    }
  }
}

WStatus WProjectCreation::CreateProject(const WProjectCreationOptions& options, const WPluginBundleSet& pluginBundles)
{
  WStringBuilder sTargetDir = options.m_sTargetDirectory;
  sTargetDir.MakeCleanPath();

  if (sTargetDir.IsEmpty() || !WPathUtils::IsAbsolutePath(sTargetDir))
    return WStatus(WFmt("The project path '{}' is not an absolute path.", sTargetDir));

  if (WOSFile::ExistsDirectory(sTargetDir))
  {
    // an existing but empty folder is fine - a folder with files in it might be a project, and creating
    // one on top of it would mix two projects into one
    WFileSystemIterator fsIt;
    fsIt.StartSearch(sTargetDir, WFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);

    if (fsIt.IsValid())
      return WStatus(WFmt("The directory '{}' already exists and is not empty.", sTargetDir));
  }

  // resolve the template before anything is written, so that a bad name does not leave a folder behind
  WStringBuilder sTemplateProjectFile;

  if (!options.m_sProjectTemplate.IsEmpty() && FindProjectTemplate(options.m_sProjectTemplate, sTemplateProjectFile).Failed())
  {
    WStringBuilder sAvailable;
    WDynamicArray<WString> templates;
    FindProjectTemplates(templates);

    for (const WString& sName : templates)
    {
      sAvailable.AppendWithSeparator(", ", "'", sName, "'");
    }

    if (sAvailable.IsEmpty())
      sAvailable = "<none>";

    return WStatus(WFmt("There is no project template called '{}'. Available templates: {}", options.m_sProjectTemplate, sAvailable));
  }

  if (WOSFile::CreateDirectoryStructure(sTargetDir).Failed())
    return WStatus(WFmt("Failed to create the directory '{}'.", sTargetDir));

  if (options.m_sProjectTemplate.IsEmpty())
  {
    WPluginBundleSet localSet = pluginBundles;

    if (!options.m_sPluginTemplate.IsEmpty())
    {
      localSet.SetFromTemplate(options.m_sPluginTemplate);
    }

    WStringBuilder sPluginSelection = sTargetDir;
    sPluginSelection.AppendPath("Editor/PluginSelection.ddl");

    WFileWriter file;
    if (file.Open(sPluginSelection).Failed())
      return WStatus(WFmt("Failed to write the plugin selection to '{}'.", sPluginSelection));

    WOpenDdlWriter ddl;
    ddl.SetOutputStream(&file);

    localSet.WriteStateToDDL(ddl);
  }
  else
  {
    WStringBuilder sSrcFolder = sTemplateProjectFile;
    sSrcFolder.PathParentDirectory();

    if (WOSFile::CopyFolder(sSrcFolder, sTargetDir).Failed())
      return WStatus(WFmt("Failed to copy the project template from '{}' to '{}'.", sSrcFolder, sTargetDir));

    // in case the template folder contained an AssetCache, delete it, so that the new project starts
    // out with no transformed assets, rather than with outputs of unknown age
    WStringBuilder sAssetCache(sTargetDir, "/AssetCache");
    if (WOSFile::ExistsDirectory(sAssetCache) && WOSFile::DeleteFolder(sAssetCache).Failed())
      return WStatus(WFmt("Failed to delete the copied asset cache '{}'.", sAssetCache));
  }

  return WStatus(W_SUCCESS);
}
