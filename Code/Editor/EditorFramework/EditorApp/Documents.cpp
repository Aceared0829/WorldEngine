#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Document/DocumentUtils.h>

void WQtEditorApp::OpenDocumentQueued(WStringView sDocument, const WDocumentObject* pOpenContext /*= nullptr*/)
{
  QMetaObject::invokeMethod(this, "SlotQueuedOpenDocument", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, WMakeQString(sDocument)), Q_ARG(void*, (void*)pOpenContext));
}

WDocument* WQtEditorApp::OpenDocument(WStringView sDocument, WBitflags<WDocumentFlags> flags, const WDocumentObject* pOpenContext)
{
  W_PROFILE_SCOPE("OpenDocument");

  if (IsInHeadlessMode())
    flags.Remove(WDocumentFlags::RequestWindow);

  const WDocumentTypeDescriptor* pTypeDesc = nullptr;

  if (WDocumentManager::FindDocumentTypeFromPath(sDocument, false, pTypeDesc).Failed())
  {
    WStringBuilder sTemp;
    sTemp.SetFormat("The selected file extension '{0}' is not registered with any known type.\nCannot open file '{1}'", WPathUtils::GetFileExtension(sDocument), sDocument);
    WQtUiServices::MessageBoxWarning(sTemp);
    return nullptr;
  }

  // does the same document already exist and is open ?
  WDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(sDocument);
  if (!pDocument)
  {
    WStatus res = pTypeDesc->m_pManager->CanOpenDocument(sDocument);
    if (res.Succeeded())
    {
      res = pTypeDesc->m_pManager->OpenDocument(pTypeDesc->m_sDocumentTypeName, sDocument, pDocument, flags, pOpenContext);
    }

    if (res.Failed())
    {
      WStringBuilder s;
      s.SetFormat("Failed to open document: \n'{0}'", sDocument);
      WQtUiServices::MessageBoxStatus(res, s);
      return nullptr;
    }

    W_ASSERT_DEV(pDocument != nullptr, "Opening of document type '{0}' succeeded, but returned pointer is nullptr", pTypeDesc->m_sDocumentTypeName);

    if (pDocument->GetUnknownObjectTypeInstances() > 0)
    {
      WStringBuilder s;
      s.SetFormat("The document '{}' contained {} objects of an unknown type. Necessary plugins may be missing.\n\n\
If you save this document, all data for these objects is lost permanently!\n\n\
The following types are missing:\n",
        sDocument, pDocument->GetUnknownObjectTypeInstances());

      for (auto it = pDocument->GetUnknownObjectTypes().GetIterator(); it.IsValid(); ++it)
      {
        s.AppendFormat(" '{0}' ", (*it));
      }
      WQtUiServices::MessageBoxWarning(s);
    }

    if (!pDocument->GetLoadingErrors().IsEmpty())
    {
      WStringBuilder s;
      s.SetFormat("The document '{}' had errors during loading:\n\n", sDocument);
      for (const WString& err : pDocument->GetLoadingErrors())
      {
        s.Append(err, "\n");
      }
      WQtUiServices::MessageBoxWarning(s);
    }
  }

  if (flags.IsSet(WDocumentFlags::RequestWindow))
  {
    WQtContainerWindow::EnsureVisibleAnyContainer(pDocument).IgnoreResult();
  }

  return pDocument;
}

WDocument* WQtEditorApp::CreateDocument(WStringView sDocument, WBitflags<WDocumentFlags> flags, const WDocumentObject* pOpenContext)
{
  W_PROFILE_SCOPE("CreateDocument");

  if (IsInHeadlessMode())
    flags.Remove(WDocumentFlags::RequestWindow);

  const WDocumentTypeDescriptor* pTypeDesc = nullptr;

  {
    WStatus res = WDocumentUtils::IsValidSaveLocationForDocument(sDocument, &pTypeDesc);
    if (res.Failed())
    {
      WStringBuilder s;
      s.SetFormat("Failed to create document: \n'{0}'", sDocument);
      WQtUiServices::MessageBoxStatus(res, s);
      return nullptr;
    }
  }

  WDocument* pDocument = nullptr;
  {
    WStatus result = pTypeDesc->m_pManager->CreateDocument(pTypeDesc->m_sDocumentTypeName, sDocument, pDocument, flags, pOpenContext);
    if (result.Failed())
    {
      WStringBuilder s;
      s.SetFormat("Failed to create document: \n'{0}'", sDocument);
      WQtUiServices::MessageBoxStatus(result, s);
      return nullptr;
    }

    W_ASSERT_DEV(pDocument != nullptr, "Creation of document type '{0}' succeeded, but returned pointer is nullptr", pTypeDesc->m_sDocumentTypeName);
    W_ASSERT_DEV(pDocument->GetUnknownObjectTypeInstances() == 0, "Newly created documents should not contain unknown types.");
  }


  if (flags.IsSet(WDocumentFlags::RequestWindow))
  {
    WQtContainerWindow::EnsureVisibleAnyContainer(pDocument).IgnoreResult();
  }

  return pDocument;
}

void WQtEditorApp::SlotQueuedOpenDocument(QString sProject, void* pOpenContext)
{
  OpenDocument(sProject.toUtf8().data(), WDocumentFlags::RequestWindow | WDocumentFlags::AddToRecentFilesList, static_cast<const WDocumentObject*>(pOpenContext));
}

void WQtEditorApp::DocumentEventHandler(const WDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case WDocumentEvent::Type::DocumentSaved:
    {
      WPreferences::SaveDocumentPreferences(e.m_pDocument);
    }
    break;

    default:
      break;
  }
}


void WQtEditorApp::DocumentManagerEventHandler(const WDocumentManager::Event& r)
{
  switch (r.m_Type)
  {
    case WDocumentManager::Event::Type::AfterDocumentWindowRequested:
    {
      if (r.m_pDocument->GetAddToRecentFilesList())
      {
        m_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath(), 0);
        if (!m_bLoadingProjectInProgress)
        {
          SaveOpenDocumentsList();
        }
      }
    }
    break;

    case WDocumentManager::Event::Type::DocumentClosing2:
    {
      WPreferences::SaveDocumentPreferences(r.m_pDocument);
      WPreferences::ClearDocumentPreferences(r.m_pDocument);
    }
    break;

    case WDocumentManager::Event::Type::DocumentClosing:
    {
      if (r.m_pDocument->GetAddToRecentFilesList())
      {
        // again, insert it into the recent documents list, such that the LAST CLOSED document is the LAST USED
        m_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath(), 0);
      }
    }
    break;

    default:
      break;
  }
}



void WQtEditorApp::DocumentManagerRequestHandler(WDocumentManager::Request& r)
{
  switch (r.m_Type)
  {
    case WDocumentManager::Request::Type::DocumentAllowedToOpen:
    {
      // if someone else already said no, don't bother to check further
      if (r.m_RequestStatus.Failed())
        return;

      if (!WToolsProject::IsProjectOpen())
      {
        // if no project is open yet, try to open the corresponding one

        WStringBuilder sProjectPath = WToolsProject::FindProjectDirectoryForDocument(r.m_sDocumentPath);

        // if no project could be located, just reject the request
        if (sProjectPath.IsEmpty())
        {
          r.m_RequestStatus = WStatus("No project could be opened");
          return;
        }
        else
        {
          // append the project file
          sProjectPath.AppendPath("WProject");

          // if a project could be found, try to open it
          WStatus res = WToolsProject::OpenProject(sProjectPath);

          // if project opening failed, relay that error message
          if (res.Failed())
          {
            r.m_RequestStatus = res;
            return;
          }
        }
      }
      else
      {
        if (!WToolsProject::GetSingleton()->IsDocumentInAllowedRoot(r.m_sDocumentPath))
        {
          r.m_RequestStatus = WStatus("The document is not part of the currently open project");
          return;
        }
      }
    }
      return;
  }
}
