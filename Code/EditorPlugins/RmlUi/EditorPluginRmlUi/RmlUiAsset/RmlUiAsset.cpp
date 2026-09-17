#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Project/ToolsProject.h>

WStringView FindNextHREF(WStringView& ref_sRml)
{
  while (true)
  {
    const char* szCurrent = ref_sRml.FindSubString("href");
    if (szCurrent == nullptr)
      return WStringView();

    szCurrent += 4; // skip "href"

    const char* szStart = nullptr;
    const char* szEnd = nullptr;
    while (*szCurrent != '\0')
    {
      if (*szCurrent == '\"')
      {
        if (szStart == nullptr)
          szStart = szCurrent + 1;
        else
        {
          szEnd = szCurrent;
          break;
        }
      }
      ++szCurrent;
    }

    if (szStart == nullptr || szEnd == nullptr)
    {
      // malformed or reached end of string
      ref_sRml.SetStartPosition(szCurrent);
      return WStringView();
    }

    ref_sRml.SetStartPosition(szEnd + 1);

    const WStringView href = WStringView(szStart, szEnd);
    if (href.HasExtension(".rcss") || href.HasExtension(".rml"))
      return href;

    // non-matching extension; continue searching
  }
}

WStringView FindNextSrcValue(WStringView& ref_sContent)
{
  while (true)
  {
    const char* szCurrent = ref_sContent.FindSubString("src");
    if (szCurrent == nullptr)
      return WStringView();

    szCurrent += 3;

    szCurrent = WStringUtils::SkipCharacters(szCurrent, WStringUtils::IsWhiteSpace);

    if (*szCurrent == '=')
    {
      // HTML attribute: src="value" or src='value'
      ++szCurrent;
      const char cQuote = *szCurrent;
      if (cQuote == '"' || cQuote == '\'')
      {
        ++szCurrent;
        const char* szStart = szCurrent;
        while (*szCurrent != '\0' && *szCurrent != cQuote)
          ++szCurrent;
        if (*szCurrent == cQuote)
        {
          ref_sContent.SetStartPosition(szCurrent + 1);
          return WStringView(szStart, szCurrent);
        }
      }
      ref_sContent.SetStartPosition(szCurrent);
      continue;
    }
    else if (*szCurrent == ':')
    {
      // CSS property: src: value;
      ++szCurrent;
      szCurrent = WStringUtils::SkipCharacters(szCurrent, WStringUtils::IsWhiteSpace);
      const char* szStart = szCurrent;
      while (*szCurrent != '\0' && *szCurrent != ';' && *szCurrent != '}' && *szCurrent != '\n' && *szCurrent != '\r')
        ++szCurrent;
      const char* szEnd = szCurrent;
      WStringUtils::Trim(szStart, szEnd, nullptr, " \t");
      ref_sContent.SetStartPosition(szCurrent);
      if (szEnd > szStart)
        return WStringView(szStart, szEnd);
      continue;
    }
    else
    {
      // "src" was part of another word, skip past it
      ref_sContent.SetStartPosition(szCurrent);
      continue;
    }
  }
}

static WStringView SanitizePath(WStringView sPath)
{
  sPath.Trim("/\\", "");
  return sPath;
}

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRmlUiAssetDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WRmlUiAssetDocument::WRmlUiAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WRmlUiAssetProperties>(sDocumentPath, WAssetDocEngineConnection::Simple)
{
}

void WRmlUiAssetDocument::OpenExternalEditor()
{
  WStringBuilder sFile(GetProperties()->m_sRmlFile);

  if (!WFileSystem::ExistsFile(sFile))
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation(WFmt("Can't find the file '{}'.\nTo create an RML file click the button next to 'RmlFile'.", sFile));

    ShowDocumentStatus("RML file doesn't exist.");
    return;
  }

  WStringBuilder sFileAbs;
  if (WFileSystem::ResolvePath(sFile, &sFileAbs, nullptr).Failed())
    return;

  {
    QStringList args;

    args.append(WMakeQString(WToolsProject::GetSingleton()->GetProjectDirectory()));
    args.append(sFileAbs.GetData());

    if (WQtUiServices::OpenInVsCode(args).Failed())
    {
      // try again with a different program
      WQtUiServices::OpenFileInDefaultProgram(sFileAbs).IgnoreResult();
    }
  }
}

WTransformStatus WRmlUiAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  WRmlUiAssetProperties* pProp = GetProperties();

  WRmlUiResourceDescriptor desc;
  desc.m_sRmlFile = pProp->m_sRmlFile;
  desc.m_ScaleMode = pProp->m_ScaleMode;
  desc.m_ReferenceResolution = pProp->m_ReferenceResolution;

  desc.m_DependencyFile.AddFileDependency(pProp->m_sRmlFile);

  W_SUCCEED_OR_RETURN(FindDependencies(desc.m_DependencyFile, pProp->m_sRmlFile));

  W_SUCCEED_OR_RETURN(desc.Save(stream));

  return WStatus(W_SUCCESS);
}

WTransformStatus WRmlUiAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  WStatus status = WAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

WStatus WRmlUiAssetDocument::FindDependencies(WDependencyFile& ref_Dependencies, WStringView sFilePath) const
{
  WSet<WString> visited;
  return FindDependencies(ref_Dependencies, sFilePath, visited);
}

WStatus WRmlUiAssetDocument::FindDependencies(WDependencyFile& ref_Dependencies, WStringView sFilePath, WSet<WString>& ref_visited) const
{
  WString sFilePathStr = sFilePath;
  if (ref_visited.Contains(sFilePathStr))
    return W_SUCCESS;
  ref_visited.Insert(sFilePathStr);

  WStringBuilder sContent;
  {
    WFileReader reader;
    if (reader.Open(sFilePath).Failed())
      return WStatus(WFmt("Failed to read file: '{}'", sFilePath));

    sContent.ReadAll(reader);
  }

  const WStringView sFileDir = sFilePath.GetFileDirectory();

  WStringBuilder sTemp;
  WStringView sContentView = sContent;

  while (true)
  {
    const WStringView href = SanitizePath(FindNextHREF(sContentView));
    if (href.IsEmpty())
      break;

    if (WFileSystem::ExistsFile(href))
    {
      ref_Dependencies.AddFileDependency(href);
      W_SUCCEED_OR_RETURN(FindDependencies(ref_Dependencies, href, ref_visited));
      continue;
    }

    sTemp.SetPath(sFileDir, href);
    if (WFileSystem::ExistsFile(sTemp))
    {
      ref_Dependencies.AddFileDependency(sTemp);
      W_SUCCEED_OR_RETURN(FindDependencies(ref_Dependencies, sTemp, ref_visited));
      continue;
    }
  }

  return W_SUCCESS;
}

void WRmlUiAssetDocument::FindPackageDependencies(WSet<WString>& ref_packageDeps, WStringView sFilePath, WSet<WString>& ref_visited) const
{
  WString sFilePathStr = sFilePath;
  if (ref_visited.Contains(sFilePathStr))
    return;
  ref_visited.Insert(sFilePathStr);

  WStringBuilder sContent;
  {
    WFileReader reader;
    if (reader.Open(sFilePath).Failed())
      return;
    sContent.ReadAll(reader);
  }

  const WStringView sFileDir = sFilePath.GetFileDirectory();
  WStringBuilder sTemp;

  // Recurse into referenced rcss/rml files
  {
    WStringView sContentView = sContent;
    while (true)
    {
      const WStringView href = SanitizePath(FindNextHREF(sContentView));
      if (href.IsEmpty())
        break;

      if (WFileSystem::ExistsFile(href))
      {
        FindPackageDependencies(ref_packageDeps, href, ref_visited);
      }
      else
      {
        sTemp.SetPath(sFileDir, href);
        if (WFileSystem::ExistsFile(sTemp))
          FindPackageDependencies(ref_packageDeps, sTemp, ref_visited);
      }
    }
  }

  // Collect source file references (images, fonts, etc.)
  {
    WStringView sContentView = sContent;
    while (true)
    {
      const WStringView src = SanitizePath(FindNextSrcValue(sContentView));
      if (src.IsEmpty())
        break;

      if (WFileSystem::ExistsFile(src))
      {
        ref_packageDeps.Insert(src);
      }
      else
      {
        sTemp.SetPath(sFileDir, src);
        if (WFileSystem::ExistsFile(sTemp))
          ref_packageDeps.Insert(sTemp);
      }
    }
  }
}

void WRmlUiAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const WRmlUiAssetProperties* pProp = GetProperties();

  // rcss/rml files referenced via href must be re-transformed and re-thumbnailed if changed, and packaged for runtime
  WDependencyFile deps;
  FindDependencies(deps, pProp->m_sRmlFile).IgnoreResult();

  for (const auto& file : deps.GetFileDependencies())
  {
    pInfo->m_TransformDependencies.Insert(file);
    pInfo->m_ThumbnailDependencies.Insert(file);
    pInfo->m_PackageDependencies.Insert(file);
  }

  // source files (images, fonts) referenced via src must be packaged for runtime and affect the thumbnail,
  // but do not affect the transform output (which only stores file paths)
  WSet<WString> sourceDeps;
  WSet<WString> visited;
  FindPackageDependencies(sourceDeps, pProp->m_sRmlFile, visited);

  for (const auto& file : sourceDeps)
  {
    pInfo->m_ThumbnailDependencies.Insert(file);
    pInfo->m_PackageDependencies.Insert(file);
  }
}
