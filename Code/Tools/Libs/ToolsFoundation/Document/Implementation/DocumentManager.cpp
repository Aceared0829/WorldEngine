#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Strings/PathUtils.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/DocumentUtils.h>
#include <ToolsFoundation/Project/ToolsProject.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDocumentManager, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, DocumentManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WPlugin::Events().AddEventHandler(WDocumentManager::OnPluginEvent);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WPlugin::Events().RemoveEventHandler(WDocumentManager::OnPluginEvent);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WSet<const WRTTI*> WDocumentManager::s_KnownManagers;
WHybridArray<WDocumentManager*, 16> WDocumentManager::s_AllDocumentManagers;
WMap<WString, const WDocumentTypeDescriptor*> WDocumentManager::s_AllDocumentDescriptors; // maps from "sDocumentTypeName" to descriptor
WCopyOnBroadcastEvent<const WDocumentManager::Event&> WDocumentManager::s_Events;
WEvent<WDocumentManager::Request&> WDocumentManager::s_Requests;
WMap<WString, WDocumentManager::CustomAction> WDocumentManager::s_CustomActions;

void WDocumentManager::OnPluginEvent(const WPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case WPluginEvent::BeforeUnloading:
      UpdateBeforeUnloadingPlugins(e);
      break;
    case WPluginEvent::AfterPluginChanges:
      UpdatedAfterLoadingPlugins();
      break;

    default:
      break;
  }
}

void WDocumentManager::UpdateBeforeUnloadingPlugins(const WPluginEvent& e)
{
  bool bChanges = false;

  // triggers a reevaluation next time
  s_AllDocumentDescriptors.Clear();

  // remove all document managers that belong to this plugin
  for (WUInt32 i = 0; i < s_AllDocumentManagers.GetCount();)
  {
    const WRTTI* pRtti = s_AllDocumentManagers[i]->GetDynamicRTTI();

    if (pRtti->GetPluginName() == e.m_sPluginBinary)
    {
      s_KnownManagers.Remove(pRtti);

      pRtti->GetAllocator()->Deallocate(s_AllDocumentManagers[i]);
      s_AllDocumentManagers.RemoveAtAndSwap(i);

      bChanges = true;
    }
    else
      ++i;
  }

  if (bChanges)
  {
    Event e2;
    e2.m_Type = Event::Type::DocumentTypesRemoved;
    s_Events.Broadcast(e2);
  }
}

void WDocumentManager::UpdatedAfterLoadingPlugins()
{
  bool bChanges = false;

  WRTTI::ForEachDerivedType<WDocumentManager>(
    [&](const WRTTI* pRtti)
    {
      // add the ones that we don't know yet
      if (!s_KnownManagers.Find(pRtti).IsValid())
      {
        // add it as 'known' even if we cannot allocate it
        s_KnownManagers.Insert(pRtti);

        if (pRtti->GetAllocator()->CanAllocate())
        {
          // create one instance of each manager type
          WDocumentManager* pManager = pRtti->GetAllocator()->Allocate<WDocumentManager>();
          s_AllDocumentManagers.PushBack(pManager);

          bChanges = true;
        }
      }
    });

  // triggers a reevaluation next time
  s_AllDocumentDescriptors.Clear();
  GetAllDocumentDescriptors();

  if (bChanges)
  {
    Event e;
    e.m_Type = Event::Type::DocumentTypesAdded;
    s_Events.Broadcast(e);
  }
}

void WDocumentManager::GetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_documentTypes) const
{
  InternalGetSupportedDocumentTypes(inout_documentTypes);

  for (auto& dt : inout_documentTypes)
  {
    W_ASSERT_DEBUG(dt->m_bCanCreate == false || dt->m_pDocumentType != nullptr, "No document type is set");
    W_ASSERT_DEBUG(!dt->m_sFileExtension.IsEmpty(), "File extension must be valid");
    W_ASSERT_DEBUG(dt->m_pManager != nullptr, "Document manager must be set");
  }
}

WStatus WDocumentManager::CanOpenDocument(WStringView sFilePath) const
{
  WTempHybridArray<const WDocumentTypeDescriptor*, 4> DocumentTypes;
  GetSupportedDocumentTypes(DocumentTypes);

  WStringBuilder sPath = sFilePath;
  WStringBuilder sExt = sPath.GetFileExtension();

  // check whether the file extension is in the list of possible extensions
  // if not, we can definitely not open this file
  for (WUInt32 i = 0; i < DocumentTypes.GetCount(); ++i)
  {
    if (DocumentTypes[i]->m_sFileExtension.IsEqual_NoCase(sExt))
    {
      return WStatus(W_SUCCESS);
    }
  }

  return WStatus("File extension is not handled by any registered type");
}

void WDocumentManager::EnsureWindowRequested(WDocument* pDocument, const WDocumentObject* pOpenContext /*= nullptr*/)
{
  if (pDocument->m_bWindowRequested)
    return;

  W_PROFILE_SCOPE("EnsureWindowRequested");
  pDocument->m_bWindowRequested = true;

  Event e;
  e.m_pDocument = pDocument;
  e.m_Type = Event::Type::DocumentWindowRequested;
  e.m_pOpenContext = pOpenContext;
  s_Events.Broadcast(e);

  e.m_pDocument = pDocument;
  e.m_Type = Event::Type::AfterDocumentWindowRequested;
  e.m_pOpenContext = pOpenContext;
  s_Events.Broadcast(e);
}

WStatus WDocumentManager::CreateOrOpenDocument(bool bCreate, WStringView sDocumentTypeName, WStringView sPath2, WDocument*& out_pDocument,
  WBitflags<WDocumentFlags> flags, const WDocumentObject* pOpenContext /*= nullptr*/)
{
#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  WFileStats fs;
  WStringBuilder sPath = sPath2;
  sPath.MakeCleanPath();
  WPathUtils::NormalizeWindowsDriveLetter(sPath);
  if (!bCreate && WOSFile::GetFileStats(sPath, fs).Failed())
  {
    return WStatus("The file does not exist.");
  }

  Request r;
  r.m_Type = Request::Type::DocumentAllowedToOpen;
  r.m_sDocumentType = sDocumentTypeName;
  r.m_sDocumentPath = sPath;
  s_Requests.Broadcast(r);

  // if for example no project is open, or not the correct one, then a document cannot be opened
  if (r.m_RequestStatus.Failed())
    return r.m_RequestStatus;

  out_pDocument = nullptr;

  WStatus status(W_SUCCESS);

  WTempHybridArray<const WDocumentTypeDescriptor*, 4> DocumentTypes;
  GetSupportedDocumentTypes(DocumentTypes);

  for (WUInt32 i = 0; i < DocumentTypes.GetCount(); ++i)
  {
    if (DocumentTypes[i]->m_sDocumentTypeName == sDocumentTypeName)
    {
      // See if there is a default asset document registered for the type, if so clone
      // it and use that as the new document instead of creating one from scratch.
      if (bCreate && !flags.IsSet(WDocumentFlags::EmptyDocument))
      {
        WStringBuilder sTemplateDoc = "Editor/DocumentTemplates/Default";
        sTemplateDoc.ChangeFileExtension(sPath.GetFileExtension());

        if (WFileSystem::ExistsFile(sTemplateDoc))
        {
          WUuid CloneUuid;
          if (CloneDocument(sTemplateDoc, sPath, CloneUuid).Succeeded())
          {
            if (OpenDocument(sDocumentTypeName, sPath, out_pDocument, flags, pOpenContext).Succeeded())
            {
              return W_SUCCESS;
            }
          }

          WLog::Warning("Failed to create document from template '{}'", sTemplateDoc);
        }
      }

      W_ASSERT_DEV(DocumentTypes[i]->m_bCanCreate, "This document manager cannot create the document type '{0}'", sDocumentTypeName);

      {
        W_PROFILE_SCOPE(sDocumentTypeName);
        status = W_SUCCESS;
        InternalCreateDocument(sDocumentTypeName, sPath, bCreate, out_pDocument, pOpenContext);
      }
      out_pDocument->SetAddToResetFilesList(flags.IsSet(WDocumentFlags::AddToRecentFilesList));

      if (status.Succeeded())
      {
        out_pDocument->SetupDocumentInfo(DocumentTypes[i]);

        out_pDocument->m_pDocumentManager = this;
        m_AllOpenDocuments.PushBack(out_pDocument);

        if (!bCreate)
        {
          status = out_pDocument->LoadDocument();
        }

        {
          W_PROFILE_SCOPE("InitializeAfterLoading");
          out_pDocument->InitializeAfterLoading(bCreate);
        }

        if (bCreate)
        {
          out_pDocument->SetModified(true);
          if (flags.IsSet(WDocumentFlags::AsyncSave))
          {
            out_pDocument->SaveDocumentAsync({});
            status = WStatus(W_SUCCESS);
          }
          else
          {
            status = out_pDocument->SaveDocument();
          }
        }

        {
          W_PROFILE_SCOPE("InitializeAfterLoadingAndSaving");
          out_pDocument->InitializeAfterLoadingAndSaving();
        }

        Event e;
        e.m_pDocument = out_pDocument;
        e.m_Type = Event::Type::DocumentOpened;

        s_Events.Broadcast(e);

        if (flags.IsSet(WDocumentFlags::RequestWindow))
          EnsureWindowRequested(out_pDocument, pOpenContext);
      }

      return status;
    }
  }

  W_REPORT_FAILURE("This document manager does not support the document type '{0}'", sDocumentTypeName);
  return status;
#else
  W_ASSERT_NOT_IMPLEMENTED;
  return WStatus("Not implemented");
#endif
}

WStatus WDocumentManager::CreateDocument(WStringView sDocumentTypeName, WStringView sPath, WDocument*& out_pDocument, WBitflags<WDocumentFlags> flags, const WDocumentObject* pOpenContext)
{
  return CreateOrOpenDocument(true, sDocumentTypeName, sPath, out_pDocument, flags, pOpenContext);
}

WStatus WDocumentManager::OpenDocument(WStringView sDocumentTypeName, WStringView sPath, WDocument*& out_pDocument,
  WBitflags<WDocumentFlags> flags, const WDocumentObject* pOpenContext)
{
  return CreateOrOpenDocument(false, sDocumentTypeName, sPath, out_pDocument, flags, pOpenContext);
}


WStatus WDocumentManager::CloneDocument(WStringView sPath, WStringView sClonePath, WUuid& inout_cloneGuid)
{
  const WDocumentTypeDescriptor* pTypeDesc = nullptr;
  WStatus res = WDocumentUtils::IsValidSaveLocationForDocument(sClonePath, &pTypeDesc);
  if (res.Failed())
    return res;

  WUniquePtr<WAbstractObjectGraph> header;
  WUniquePtr<WAbstractObjectGraph> objects;
  WUniquePtr<WAbstractObjectGraph> types;

  res = WDocument::ReadDocument(sPath, header, objects, types);
  if (res.Failed())
    return res;

  WUuid documentId;
  WAbstractObjectNode::Property* documentIdProp = nullptr;
  {
    auto* pHeaderNode = header->GetNodeByName("Header");
    W_ASSERT_DEV(pHeaderNode, "No header found, document '{0}' is corrupted.", sPath);
    documentIdProp = pHeaderNode->FindProperty("DocumentID");
    W_ASSERT_DEV(documentIdProp, "No document ID property found in header, document document '{0}' is corrupted.", sPath);
    documentId = documentIdProp->m_Value.Get<WUuid>();
  }

  WUuid seedGuid;
  if (inout_cloneGuid.IsValid())
  {
    seedGuid = inout_cloneGuid;
    seedGuid.RevertCombinationWithSeed(documentId);

    WUuid test = documentId;
    test.CombineWithSeed(seedGuid);
    W_ASSERT_DEV(test == inout_cloneGuid, "");
  }
  else
  {
    seedGuid = WUuid::MakeUuid();
    inout_cloneGuid = documentId;
    inout_cloneGuid.CombineWithSeed(seedGuid);
  }

  InternalCloneDocument(sPath, sClonePath, documentId, seedGuid, inout_cloneGuid, header.Borrow(), objects.Borrow(), types.Borrow());

  {
    WDeferredFileWriter file;
    file.SetOutput(sClonePath);
    WAbstractGraphDdlSerializer::WriteDocument(file, header.Borrow(), objects.Borrow(), types.Borrow(), false);
    if (file.Close() == W_FAILURE)
    {
      return WStatus(WFmt("Unable to open file '{0}' for writing!", sClonePath));
    }
  }
  return WStatus(W_SUCCESS);
}

void WDocumentManager::InternalCloneDocument(WStringView sPath, WStringView sClonePath, const WUuid& documentId, const WUuid& seedGuid, const WUuid& cloneGuid, WAbstractObjectGraph* header, WAbstractObjectGraph* objects, WAbstractObjectGraph* types)
{
  // Remap
  header->ReMapNodeGuids(seedGuid);
  objects->ReMapNodeGuids(seedGuid);

  auto* pHeaderNode = header->GetNodeByName("Header");
  auto* documentIdProp = pHeaderNode->FindProperty("DocumentID");
  documentIdProp->m_Value = cloneGuid;

  // Fix cloning of docs containing prefabs.
  // TODO: generalize this for other doc features?
  auto& AllNodes = objects->GetAllNodes();
  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    WAbstractObjectNode::Property* pProp = pNode->FindProperty("MetaPrefabSeed");
    if (pProp && pProp->m_Value.IsA<WUuid>())
    {
      WUuid prefabSeed = pProp->m_Value.Get<WUuid>();
      prefabSeed.CombineWithSeed(seedGuid);
      pProp->m_Value = prefabSeed;
    }
  }
}

void WDocumentManager::CloseDocument(WDocument* pDocument)
{
  W_ASSERT_DEV(pDocument != nullptr, "Invalid document pointer");

  if (!m_AllOpenDocuments.RemoveAndCopy(pDocument))
    return;

  Event e;
  e.m_pDocument = pDocument;

  e.m_Type = Event::Type::DocumentClosing;
  s_Events.Broadcast(e);

  e.m_Type = Event::Type::DocumentClosing2;
  s_Events.Broadcast(e);

  pDocument->BeforeClosing();
  delete pDocument; // the pointer in e.m_pDocument won't be valid anymore at broadcast time, it is only sent for comparison purposes, not to be dereferenced

  e.m_Type = Event::Type::DocumentClosed;
  s_Events.Broadcast(e);
}

void WDocumentManager::CloseAllDocumentsOfManager()
{
  while (!m_AllOpenDocuments.IsEmpty())
  {
    CloseDocument(m_AllOpenDocuments[0]);
  }
}

void WDocumentManager::CloseAllDocuments()
{
  for (WDocumentManager* pMan : s_AllDocumentManagers)
  {
    pMan->CloseAllDocumentsOfManager();
  }
}

WDocument* WDocumentManager::GetDocumentByPath(WStringView sPath) const
{
  WStringBuilder sPath2 = sPath;
  sPath2.MakeCleanPath();

  for (WDocument* pDoc : m_AllOpenDocuments)
  {
    if (sPath2.IsEqual_NoCase(pDoc->GetDocumentPath()))
      return pDoc;
  }

  return nullptr;
}


WDocument* WDocumentManager::GetDocumentByGuid(const WUuid& guid)
{
  for (auto man : s_AllDocumentManagers)
  {
    for (auto doc : man->m_AllOpenDocuments)
    {
      if (doc->GetGuid() == guid)
        return doc;
    }
  }

  return nullptr;
}


bool WDocumentManager::EnsureDocumentIsClosedInAllManagers(WStringView sPath)
{
  bool bClosedAny = false;
  for (auto man : s_AllDocumentManagers)
  {
    if (man->EnsureDocumentIsClosed(sPath))
      bClosedAny = true;
  }

  return bClosedAny;
}

bool WDocumentManager::EnsureDocumentIsClosed(WStringView sPath)
{
  auto pDoc = GetDocumentByPath(sPath);

  if (pDoc == nullptr)
    return false;

  CloseDocument(pDoc);

  return true;
}

WResult WDocumentManager::FindDocumentTypeFromPath(WStringView sPath, bool bForCreation, const WDocumentTypeDescriptor*& out_pTypeDesc)
{
  const WString sFileExt = WPathUtils::GetFileExtension(sPath);

  const auto& allDesc = GetAllDocumentDescriptors();

  for (auto it : allDesc)
  {
    const auto* desc = it.Value();

    if (bForCreation && !desc->m_bCanCreate)
      continue;

    if (desc->m_sFileExtension.IsEqual_NoCase(sFileExt))
    {
      out_pTypeDesc = desc;
      return W_SUCCESS;
    }
  }

  return W_FAILURE;
}

const WMap<WString, const WDocumentTypeDescriptor*>& WDocumentManager::GetAllDocumentDescriptors()
{
  if (s_AllDocumentDescriptors.IsEmpty())
  {
    for (WDocumentManager* pMan : WDocumentManager::GetAllDocumentManagers())
    {
      WTempHybridArray<const WDocumentTypeDescriptor*, 4> descriptors;
      pMan->GetSupportedDocumentTypes(descriptors);

      for (auto pDesc : descriptors)
      {
        s_AllDocumentDescriptors[pDesc->m_sDocumentTypeName] = pDesc;
      }
    }
  }

  return s_AllDocumentDescriptors;
}

const WDocumentTypeDescriptor* WDocumentManager::GetDescriptorForDocumentType(WStringView sDocumentType)
{
  return GetAllDocumentDescriptors().GetValueOrDefault(sDocumentType, nullptr);
}

/// \todo on close doc: remove from m_AllDocuments
