#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAssetDocumentGenerator, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WAssetDocumentGenerator::WAssetDocumentGenerator() = default;

WAssetDocumentGenerator::~WAssetDocumentGenerator() = default;

void WAssetDocumentGenerator::AddSupportedFileType(WStringView sExtension)
{
  WStringBuilder tmp = sExtension;
  tmp.ToLower();

  m_SupportedFileTypes.PushBack(tmp);
}

bool WAssetDocumentGenerator::SupportsFileType(WStringView sFile) const
{
  WStringBuilder tmp = WPathUtils::GetFileExtension(sFile);

  if (tmp.IsEmpty())
    tmp = sFile;

  tmp.ToLower();

  return m_SupportedFileTypes.Contains(tmp);
}

void WAssetDocumentGenerator::BuildFileDialogFilterString(WStringBuilder& out_sFilter) const
{
  bool semicolon = false;
  out_sFilter.SetFormat("{0} (", GetDocumentExtension());
  AppendFileFilterStrings(out_sFilter, semicolon);
  out_sFilter.Append(")");
}

void WAssetDocumentGenerator::AppendFileFilterStrings(WStringBuilder& out_sFilter, bool& ref_bSemicolon) const
{
  for (const WString& ext : m_SupportedFileTypes)
  {
    WStringBuilder extWithStarDot;
    extWithStarDot.AppendFormat("*.{0}", ext);

    if (const char* pos = out_sFilter.FindSubString(extWithStarDot.GetData()))
    {
      const char afterExt = *(pos + extWithStarDot.GetElementCount());

      if (afterExt == '\0' || afterExt == ';')
        continue;
    }

    if (ref_bSemicolon)
    {
      out_sFilter.AppendFormat("; {0}", extWithStarDot.GetView());
    }
    else
    {
      out_sFilter.Append(extWithStarDot.GetView());
      ref_bSemicolon = true;
    }
  }
}

void WAssetDocumentGenerator::CreateGenerators(WDynamicArray<WAssetDocumentGenerator*>& out_generators)
{
  WRTTI::ForEachDerivedType<WAssetDocumentGenerator>(
    [&](const WRTTI* pRtti)
    {
      out_generators.PushBack(pRtti->GetAllocator()->Allocate<WAssetDocumentGenerator>());
    },
    WRTTI::ForEachOptions::ExcludeNonAllocatable);

  // sort by name
  out_generators.Sort([](WAssetDocumentGenerator* lhs, WAssetDocumentGenerator* rhs) -> bool
    { return lhs->GetDocumentExtension().Compare_NoCase(rhs->GetDocumentExtension()) < 0; });
}

void WAssetDocumentGenerator::DestroyGenerators(const WDynamicArray<WAssetDocumentGenerator*>& generators)
{
  for (WAssetDocumentGenerator* pGen : generators)
  {
    pGen->GetDynamicRTTI()->GetAllocator()->Deallocate(pGen);
  }
}

void WAssetDocumentGenerator::ImportAssets(const WDynamicArray<WString>& filesToImport)
{
  WAssetProcessor::GetSingleton()->m_iPauseProcessing.Increment();
  W_SCOPE_EXIT(WAssetProcessor::GetSingleton()->m_iPauseProcessing.Decrement());

  WTempHybridArray<WAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  WDynamicArray<WAssetDocumentGenerator::ImportGroupOptions> allImports;
  allImports.Reserve(filesToImport.GetCount());

  CreateImportOptionList(filesToImport, allImports, generators);

  SortAndSelectBestImportOption(allImports);

  if (!allImports.IsEmpty())
  {
    WQtAssetImportDlg dlg(QApplication::activeWindow(), allImports);
    dlg.exec();
  }

  DestroyGenerators(generators);
}

void WAssetDocumentGenerator::GetSupportsFileTypes(WSet<WString>& out_extensions)
{
  out_extensions.Clear();

  WTempHybridArray<WAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);
  for (auto pGen : generators)
  {
    for (const WString& ext : pGen->m_SupportedFileTypes)
    {
      out_extensions.Insert(ext);
    }
  }
  DestroyGenerators(generators);
}

void WAssetDocumentGenerator::ImportAssets()
{
  // The file picker below is a native window, so it is not covered by WQtDialog and would block
  // indefinitely. Use the overload taking the file list when there is no user to pick them.
  if (WQtUiServices::SuppressModalWindow("Import Assets (file picker)"))
    return;

  WTempHybridArray<WAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  WStringBuilder singleFilter, fullFilter, allExtensions;
  bool semicolon = false;

  for (auto pGen : generators)
  {
    pGen->AppendFileFilterStrings(allExtensions, semicolon);
    pGen->BuildFileDialogFilterString(singleFilter);
    fullFilter.Append(singleFilter, "\n");
  }

  fullFilter.Append("All files (*.*)");
  fullFilter.Prepend("All asset files (", allExtensions, ")\n");

  static WStringBuilder s_StartDir;
  if (s_StartDir.IsEmpty())
  {
    s_StartDir = WToolsProject::GetSingleton()->GetProjectDirectory();
  }

  QStringList filenames = QFileDialog::getOpenFileNames(QApplication::activeWindow(), "Import Assets", s_StartDir.GetData(),
    QString::fromUtf8(fullFilter.GetData()), nullptr, QFileDialog::Option::DontResolveSymlinks);

  DestroyGenerators(generators);

  if (filenames.empty())
    return;

  s_StartDir = filenames[0].toUtf8().data();
  s_StartDir.PathParentDirectory();

  WTempHybridArray<WString, 16> filesToImport;
  for (QString s : filenames)
  {
    filesToImport.PushBack(s.toUtf8().data());
  }

  ImportAssets(filesToImport);
}

void WAssetDocumentGenerator::CreateImportOptionList(const WDynamicArray<WString>& filesToImport, WDynamicArray<WAssetDocumentGenerator::ImportGroupOptions>& allImports, const WDynamicArray<WAssetDocumentGenerator*>& generators)
{
  WQtEditorApp* pApp = WQtEditorApp::GetSingleton();
  WStringBuilder sInputRelative, sGroup;

  for (const WString& sInputAbsolute : filesToImport)
  {
    sInputRelative = sInputAbsolute;

    if (!pApp->MakePathDataDirectoryRelative(sInputRelative))
    {
      // error, file is not in data directory -> skip
      continue;
    }

    for (WAssetDocumentGenerator* pGen : generators)
    {
      if (pGen->SupportsFileType(sInputRelative))
      {
        sGroup = pGen->GetGeneratorGroup();

        ImportGroupOptions* pData = nullptr;
        for (auto& importer : allImports)
        {
          if (importer.m_sGroup == sGroup && importer.m_sInputFileAbsolute == sInputAbsolute)
          {
            pData = &importer;
          }
        }

        if (pData == nullptr)
        {
          pData = &allImports.ExpandAndGetRef();
          pData->m_sGroup = sGroup;
          pData->m_sInputFileAbsolute = sInputAbsolute;
          pData->m_sInputFileRelative = sInputRelative;
        }

        WTempHybridArray<WAssetDocumentGenerator::ImportMode, 4> options;
        pGen->GetImportModes(sInputAbsolute, options);

        for (auto& option : options)
        {
          option.m_pGenerator = pGen;
        }

        pData->m_ImportOptions.PushBackRange(options);
      }
    }
  }
}

void WAssetDocumentGenerator::SortAndSelectBestImportOption(WDynamicArray<WAssetDocumentGenerator::ImportGroupOptions>& allImports)
{
  allImports.Sort([](const WAssetDocumentGenerator::ImportGroupOptions& lhs, const WAssetDocumentGenerator::ImportGroupOptions& rhs) -> bool
    { return lhs.m_sInputFileRelative < rhs.m_sInputFileRelative; });

  for (auto& singleImport : allImports)
  {
    singleImport.m_ImportOptions.Sort([](const WAssetDocumentGenerator::ImportMode& lhs, const WAssetDocumentGenerator::ImportMode& rhs) -> bool
      { return WTranslate(lhs.m_sName).Compare_NoCase(WTranslate(rhs.m_sName)) < 0; });

    WUInt32 uiNumPrios[(WUInt32)WAssetDocGeneratorPriority::ENUM_COUNT] = {0};
    WUInt32 uiBestPrio[(WUInt32)WAssetDocGeneratorPriority::ENUM_COUNT] = {0};

    for (WUInt32 i = 0; i < singleImport.m_ImportOptions.GetCount(); ++i)
    {
      uiNumPrios[(WUInt32)singleImport.m_ImportOptions[i].m_Priority]++;
      uiBestPrio[(WUInt32)singleImport.m_ImportOptions[i].m_Priority] = i;
    }

    singleImport.m_iSelectedOption = -1;
    for (WUInt32 prio = (WUInt32)WAssetDocGeneratorPriority::HighPriority; prio > (WUInt32)WAssetDocGeneratorPriority::Undecided; --prio)
    {
      if (uiNumPrios[prio] == 1)
      {
        singleImport.m_iSelectedOption = uiBestPrio[prio];
        break;
      }

      if (uiNumPrios[prio] > 1)
        break;
    }
  }
}

WStringBuilder WAssetDocumentGenerator::GetImportTargetPath(WStringView sInputFileAbs) const
{
  WStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  return sOutFile;
}

bool WAssetDocumentGenerator::NeedsImport(WStringView sInputFileAbs, WStringView sMode) const
{
  return !WOSFile::ExistsFile(GetImportTargetPath(sInputFileAbs));
}

WStatus WAssetDocumentGenerator::Import(WStringView sInputFileAbs, WStringView sMode, bool bOpenDocument, ImportResult* out_pResult, WStringBuilder* out_pDocumentPath)
{
  WStringBuilder ext = sInputFileAbs.GetFileExtension();
  ext.ToLower();

  if (!m_SupportedFileTypes.Contains(ext))
    return WStatus(WFmt("Files of type '{}' cannot be imported as '{}' documents.", ext, GetDocumentExtension()));

  const WStringBuilder sTargetPath = GetImportTargetPath(sInputFileAbs);

  if (out_pDocumentPath != nullptr)
    *out_pDocumentPath = sTargetPath;

  if (!NeedsImport(sInputFileAbs, sMode))
  {
    WLog::Info("Skipping import, file has been imported before: '{}'", sTargetPath);

    if (out_pResult != nullptr)
      *out_pResult = ImportResult::AlreadyExists;

    return WStatus(W_SUCCESS);
  }

  WTempHybridArray<WDocument*, 16> pGeneratedDocs;
  W_SUCCEED_OR_RETURN(Generate(sInputFileAbs, sMode, pGeneratedDocs));

  if (out_pResult != nullptr)
  {
    // Generating nothing without failing means everything that would have been created was already
    // there. This is how modes that create several documents report that all of them existed.
    *out_pResult = pGeneratedDocs.IsEmpty() ? ImportResult::AlreadyExists : ImportResult::Imported;
  }

  // For modes that create several documents the nominal target path may never be created at all,
  // so prefer what was actually generated.
  if (out_pDocumentPath != nullptr && !pGeneratedDocs.IsEmpty())
    *out_pDocumentPath = pGeneratedDocs[0]->GetDocumentPath();

  for (WDocument* pDoc : pGeneratedDocs)
  {
    const WString sDocPath = pDoc->GetDocumentPath();

    pDoc->SaveDocument(true).LogFailure();
    pDoc->GetDocumentManager()->CloseDocument(pDoc);

    if (bOpenDocument)
    {
      WQtEditorApp::GetSingleton()->OpenDocumentQueued(sDocPath);
    }
  }

  return WStatus(W_SUCCESS);
}
