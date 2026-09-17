#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

void WQtEditorApp::GuiCreateOrOpenDocument(bool bCreate)
{
  // The file picker below is a native window, so it is not covered by WQtDialog. Automated callers
  // have to name the document, i.e. go through CreateDocument()/OpenDocument() directly.
  if (WQtUiServices::SuppressModalWindow(bCreate ? "Create Document (file picker)" : "Open Document (file picker)"))
    return;

  const WString sAllFilters = BuildDocumentTypeFileFilter(bCreate);

  if (sAllFilters.IsEmpty())
  {
    WQtUiServices::MessageBoxInformation("No file types are currently known. Load plugins to add file types.");
    return;
  }

  static QString sSelectedExt;
  const QString sDir = QString::fromUtf8(m_sLastDocumentFolder.GetData());

  WString sFile;

  if (bCreate)
    sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Document"), sDir,
      QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt, QFileDialog::Option::DontResolveSymlinks)
              .toUtf8()
              .data();
  else
    sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open Document"), sDir, QString::fromUtf8(sAllFilters.GetData()),
      &sSelectedExt, QFileDialog::Option::DontResolveSymlinks)
              .toUtf8()
              .data();

  if (sFile.IsEmpty())
    return;

  m_sLastDocumentFolder = WPathUtils::GetFileDirectory(sFile);

  const WDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (WDocumentManager::FindDocumentTypeFromPath(sFile, bCreate, pTypeDesc).Succeeded())
  {
    sSelectedExt = pTypeDesc->m_sDocumentTypeName;
  }

  if (bCreate)
    CreateDocument(sFile, WDocumentFlags::AddToRecentFilesList | WDocumentFlags::RequestWindow);
  else
    OpenDocument(sFile, WDocumentFlags::AddToRecentFilesList | WDocumentFlags::RequestWindow);
}

void WQtEditorApp::GuiCreateDocument()
{
  GuiCreateOrOpenDocument(true);
}

void WQtEditorApp::GuiOpenDocument()
{
  WQtAssetBrowserDlg dlg(QApplication::activeWindow(), WUuid(), "", "");
  if (dlg.exec() == 0)
    return;

  WQtEditorApp::GetSingleton()->OpenDocument(dlg.GetSelectedAssetPathAbsolute(), WDocumentFlags::RequestWindow | WDocumentFlags::AddToRecentFilesList);
}


WString WQtEditorApp::BuildDocumentTypeFileFilter(bool bForCreation)
{
  WStringBuilder sAllFilters;
  const char* sepsep = "";

  if (!bForCreation)
  {
    sAllFilters = "All Files (*.*)";
    sepsep = ";;";
  }

  const auto& assetTypes = WDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  WMap<WString, const WDocumentTypeDescriptor*> allDesc;
  for (auto it : assetTypes)
  {
    allDesc[WTranslate(it.Key())] = it.Value();
  }

  for (auto it : allDesc)
  {
    auto desc = it.Value();

    if (bForCreation && !desc->m_bCanCreate)
      continue;

    if (desc->m_sFileExtension.IsEmpty())
      continue;

    sAllFilters.Append(sepsep, WTranslate(desc->m_sDocumentTypeName), " (*.", desc->m_sFileExtension, ")");
    sepsep = ";;";
  }

  return sAllFilters;
}
