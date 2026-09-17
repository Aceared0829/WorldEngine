#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QClipboard>
#include <QFileDialog>
#include <QMimeData>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Project/ToolsProject.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDocumentAction, 1, WRTTINoAllocator)
  ;
W_END_DYNAMIC_REFLECTED_TYPE;

////////////////////////////////////////////////////////////////////////
// WDocumentActions
////////////////////////////////////////////////////////////////////////

WActionDescriptorHandle WDocumentActions::s_hSaveCategory;
WActionDescriptorHandle WDocumentActions::s_hSave;
WActionDescriptorHandle WDocumentActions::s_hSaveAs;
WActionDescriptorHandle WDocumentActions::s_hSaveAll;
WActionDescriptorHandle WDocumentActions::s_hClose;
WActionDescriptorHandle WDocumentActions::s_hCloseAll;
WActionDescriptorHandle WDocumentActions::s_hCloseAllButThis;
WActionDescriptorHandle WDocumentActions::s_hOpenContainingFolder;
WActionDescriptorHandle WDocumentActions::s_hCopyDocumentPath;
WActionDescriptorHandle WDocumentActions::s_hUpdatePrefabs;

void WDocumentActions::RegisterActions()
{
  s_hSaveCategory = W_REGISTER_CATEGORY("SaveCategory");
  s_hSave = W_REGISTER_ACTION_1("Document.Save", WActionScope::Document, "Document", "Ctrl+S", WDocumentAction, WDocumentAction::ButtonType::Save);
  s_hSaveAll = W_REGISTER_ACTION_1("Document.SaveAll", WActionScope::Document, "Document", "Ctrl+Shift+S", WDocumentAction, WDocumentAction::ButtonType::SaveAll);
  s_hSaveAs = W_REGISTER_ACTION_1("Document.SaveAs", WActionScope::Document, "Document", "", WDocumentAction, WDocumentAction::ButtonType::SaveAs);
  s_hClose = W_REGISTER_ACTION_1("Document.Close", WActionScope::Document, "Document", "Ctrl+W", WDocumentAction, WDocumentAction::ButtonType::Close);
  s_hCloseAll = W_REGISTER_ACTION_1("Document.CloseAll", WActionScope::Document, "Document", "Ctrl+Shift+W", WDocumentAction, WDocumentAction::ButtonType::CloseAll);
  s_hCloseAllButThis = W_REGISTER_ACTION_1("Document.CloseAllButThis", WActionScope::Document, "Document", "Shift+Alt+W", WDocumentAction, WDocumentAction::ButtonType::CloseAllButThis);
  s_hOpenContainingFolder = W_REGISTER_ACTION_1("Document.OpenContainingFolder", WActionScope::Document, "Document", "", WDocumentAction, WDocumentAction::ButtonType::OpenContainingFolder);
  s_hCopyDocumentPath = W_REGISTER_ACTION_1("Document.CopyDocumentPath", WActionScope::Document, "Document", "", WDocumentAction, WDocumentAction::ButtonType::CopyDocumentPath);
  s_hUpdatePrefabs = W_REGISTER_ACTION_1("Prefabs.UpdateAll", WActionScope::Document, "Scene", "Ctrl+Shift+P", WDocumentAction, WDocumentAction::ButtonType::UpdatePrefabs);
}

void WDocumentActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hSaveCategory);
  WActionManager::UnregisterAction(s_hSave);
  WActionManager::UnregisterAction(s_hSaveAs);
  WActionManager::UnregisterAction(s_hSaveAll);
  WActionManager::UnregisterAction(s_hClose);
  WActionManager::UnregisterAction(s_hCloseAll);
  WActionManager::UnregisterAction(s_hCloseAllButThis);
  WActionManager::UnregisterAction(s_hOpenContainingFolder);
  WActionManager::UnregisterAction(s_hCopyDocumentPath);
  WActionManager::UnregisterAction(s_hUpdatePrefabs);
}

void WDocumentActions::MapMenuActions(WStringView sMapping, WStringView sTargetMenu)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hSave, sTargetMenu, 5.0f);
  pMap->MapAction(s_hSaveAs, sTargetMenu, 6.0f);
  pMap->MapAction(s_hSaveAll, sTargetMenu, 7.0f);
  pMap->MapAction(s_hClose, sTargetMenu, 8.0f);
  pMap->MapAction(s_hCloseAll, sTargetMenu, 9.0f);
  pMap->MapAction(s_hCloseAllButThis, sTargetMenu, 10.0f);
  pMap->MapAction(s_hOpenContainingFolder, sTargetMenu, 11.0f);
  pMap->MapAction(s_hCopyDocumentPath, sTargetMenu, 12.0f);
}

void WDocumentActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hSaveCategory, "", 1.0f);
  WStringView sSubPath = "SaveCategory";

  pMap->MapAction(s_hSave, sSubPath, 1.0f);
  pMap->MapAction(s_hSaveAll, sSubPath, 3.0f);
}


void WDocumentActions::MapToolsActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hUpdatePrefabs, "G.Tools.Document", 1.0f);
}

////////////////////////////////////////////////////////////////////////
// WDocumentAction
////////////////////////////////////////////////////////////////////////

WDocumentAction::WDocumentAction(const WActionContext& context, const char* szName, ButtonType button)
  : WButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case WDocumentAction::ButtonType::Save:
      SetIconPath(":/GuiFoundation/Icons/Save.svg");
      break;
    case WDocumentAction::ButtonType::SaveAs:
      SetIconPath("");
      break;
    case WDocumentAction::ButtonType::SaveAll:
      SetIconPath(":/GuiFoundation/Icons/SaveAll.svg");
      break;
    case WDocumentAction::ButtonType::Close:
      SetIconPath("");
      break;
    case WDocumentAction::ButtonType::CloseAll:
      SetIconPath("");
      break;
    case WDocumentAction::ButtonType::CloseAllButThis:
      SetIconPath("");
      break;
    case WDocumentAction::ButtonType::OpenContainingFolder:
      SetIconPath(":/GuiFoundation/Icons/OpenFolder.svg");
      break;
    case WDocumentAction::ButtonType::CopyDocumentPath:
      SetIconPath("");
      break;
    case WDocumentAction::ButtonType::UpdatePrefabs:
      SetIconPath(":/EditorPluginScene/Icons/PrefabUpdate.svg");
      break;
  }

  if (context.m_pDocument == nullptr)
  {
    if (button == ButtonType::Save || button == ButtonType::SaveAs)
    {
      // for actions that require a document, hide them
      SetVisible(false);
    }
  }
  else
  {
    m_Context.m_pDocument->m_EventsOne.AddEventHandler(WMakeDelegate(&WDocumentAction::DocumentEventHandler, this));

    if (m_ButtonType == ButtonType::Save)
    {
      SetVisible(!m_Context.m_pDocument->IsReadOnly());
      SetEnabled(m_Context.m_pDocument->IsModified());
    }
  }
}

WDocumentAction::~WDocumentAction()
{
  if (m_Context.m_pDocument)
  {
    m_Context.m_pDocument->m_EventsOne.RemoveEventHandler(WMakeDelegate(&WDocumentAction::DocumentEventHandler, this));
  }
}

void WDocumentAction::DocumentEventHandler(const WDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case WDocumentEvent::Type::DocumentSaved:
    case WDocumentEvent::Type::ModifiedChanged:
    {
      if (m_ButtonType == ButtonType::Save)
      {
        SetEnabled(m_Context.m_pDocument->IsModified());
      }
    }
    break;

    default:
      break;
  }
}

void WDocumentAction::Execute(const WVariant& value)
{
  switch (m_ButtonType)
  {
    case WDocumentAction::ButtonType::Save:
    {
      WQtDocumentWindow* pWnd = WQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);
      pWnd->SaveDocument().LogFailure();
    }
    break;

    case WDocumentAction::ButtonType::SaveAs:
    {
      WQtDocumentWindow* pWnd = WQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);
      if (pWnd->SaveDocument().Succeeded())
      {
        auto* desc = m_Context.m_pDocument->GetDocumentTypeDescriptor();
        WStringBuilder sAllFilters;
        sAllFilters.Append(desc->m_sDocumentTypeName, " (*.", desc->m_sFileExtension, ")");
        QString sSelectedExt;
        WString sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Document"),
          WMakeQString(m_Context.m_pDocument->GetDocumentPath()), QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt, QFileDialog::Option::DontResolveSymlinks)
                           .toUtf8()
                           .data();

        if (!sFile.IsEmpty())
        {
          WUuid newDoc = WUuid::MakeUuid();
          WStatus res = m_Context.m_pDocument->GetDocumentManager()->CloneDocument(m_Context.m_pDocument->GetDocumentPath(), sFile, newDoc);

          if (res.Failed())
          {
            WStringBuilder s;
            s.SetFormat("Failed to save document: \n'{0}'", sFile);
            WQtUiServices::MessageBoxStatus(res, s);
          }
          else
          {
            const WDocumentTypeDescriptor* pTypeDesc = nullptr;
            if (WDocumentManager::FindDocumentTypeFromPath(sFile, false, pTypeDesc).Succeeded())
            {
              WDocument* pDocument = nullptr;
              m_Context.m_pDocument->GetDocumentManager()->OpenDocument(pTypeDesc->m_sDocumentTypeName, sFile, pDocument).LogFailure();
            }
          }
        }
      }
    }
    break;

    case WDocumentAction::ButtonType::SaveAll:
    {
      WToolsProject::GetSingleton()->BroadcastSaveAll();
    }
    break;

    case WDocumentAction::ButtonType::Close:
    {
      WQtDocumentWindow* pWindow = WQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);

      if (!pWindow->CanCloseWindow())
        return;

      pWindow->CloseDocumentWindow();
    }
    break;

    case WDocumentAction::ButtonType::CloseAll:
    {
      auto& documentWindows = WQtDocumentWindow::GetAllDocumentWindows();
      for (WQtDocumentWindow* pWindow : documentWindows)
      {
        if (!pWindow->CanCloseWindow())
          continue;

        // Prevent closing the document root window.
        if (WStringUtils::Compare(pWindow->GetUniqueName(), "Settings") == 0)
          continue;

        pWindow->CloseDocumentWindow();
      }
    }
    break;

    case WDocumentAction::ButtonType::CloseAllButThis:
    {
      WQtDocumentWindow* pThisWindow = WQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);

      auto& documentWindows = WQtDocumentWindow::GetAllDocumentWindows();
      for (WQtDocumentWindow* pWindow : documentWindows)
      {
        if (!pWindow->CanCloseWindow() || pWindow == pThisWindow)
          continue;

        // Prevent closing the document root window.
        if (WStringUtils::Compare(pWindow->GetUniqueName(), "Settings") == 0)
          continue;

        pWindow->CloseDocumentWindow();
      }
    }
    break;

    case WDocumentAction::ButtonType::OpenContainingFolder:
    {
      WString sPath;

      if (!m_Context.m_pDocument)
      {
        if (WToolsProject::IsProjectOpen())
          sPath = WToolsProject::GetSingleton()->GetProjectFile();
        else
          sPath = WOSFile::GetApplicationDirectory();
      }
      else
        sPath = m_Context.m_pDocument->GetDocumentPath();

      WQtUiServices::OpenInExplorer(sPath, true);
    }
    break;

    case WDocumentAction::ButtonType::CopyDocumentPath:
    {
      if (m_Context.m_pDocument == nullptr)
        return;

      WStringBuilder sPath = m_Context.m_pDocument->GetDocumentPath();
      sPath.MakePathSeparatorsNative();

      QMimeData* pMimeData = new QMimeData();
      pMimeData->setText(WMakeQString(sPath));
      QApplication::clipboard()->setMimeData(pMimeData);

      WQtUiServices::ShowAllDocumentsTemporaryStatusBarMessage(WFmt("Copied path: {}", sPath), WTime::MakeFromSeconds(5));
    }
    break;

    case WDocumentAction::ButtonType::UpdatePrefabs:
      // TODO const cast
      const_cast<WDocument*>(m_Context.m_pDocument)->UpdatePrefabs();
      return;
  }
}
