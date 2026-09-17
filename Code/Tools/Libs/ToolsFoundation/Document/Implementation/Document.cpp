#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/DocumentTasks.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDocumentObjectMetaData, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    //W_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
    W_MEMBER_PROPERTY("MetaFromPrefab", m_CreateFromPrefab),
    W_MEMBER_PROPERTY("MetaPrefabSeed", m_PrefabSeedGuid),
    W_MEMBER_PROPERTY("MetaBasePrefab", m_sBasePrefab),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDocumentInfo, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("DocumentID", m_DocumentID),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WDocumentInfo::WDocumentInfo()
{
  m_DocumentID = WUuid::MakeUuid();
}


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WEvent<const WDocumentEvent&> WDocument::s_EventsAny;

WDocument::WDocument(WStringView sPath, WDocumentObjectManager* pDocumentObjectManagerImpl)
{
  using ObjectMetaData = WObjectMetaData<WUuid, WDocumentObjectMetaData>;
  m_DocumentObjectMetaData = W_DEFAULT_NEW(ObjectMetaData);
  SetDocumentPath(sPath);
  m_pObjectManager = WUniquePtr<WDocumentObjectManager>(pDocumentObjectManagerImpl, WFoundation::GetDefaultAllocator());
  m_pObjectManager->SetDocument(this);
  m_pCommandHistory = W_DEFAULT_NEW(WCommandHistory, this);
  m_pSelectionManager = W_DEFAULT_NEW(WSelectionManager, m_pObjectManager.Borrow());

  if (m_pObjectAccessor == nullptr)
  {
    m_pObjectAccessor = W_DEFAULT_NEW(WObjectCommandAccessor, m_pCommandHistory.Borrow());
  }

  m_pHostDocument = this;
  m_pActiveSubDocument = this;
}

WDocument::~WDocument()
{
  m_pSelectionManager = nullptr;

  m_pObjectManager->DestroyAllObjects();

  m_pCommandHistory->ClearRedoHistory();
  m_pCommandHistory->ClearUndoHistory();

  W_DEFAULT_DELETE(m_pDocumentInfo);
}

void WDocument::SetupDocumentInfo(const WDocumentTypeDescriptor* pTypeDescriptor)
{
  m_pTypeDescriptor = pTypeDescriptor;
  m_pDocumentInfo = CreateDocumentInfo();

  W_ASSERT_DEV(m_pDocumentInfo != nullptr, "invalid document info");
}

void WDocument::SetModified(bool b)
{
  if (m_bModified == b)
    return;

  m_bModified = b;
  m_ModifiedTime = b ? WTime::Now() : WTime();

  WDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = WDocumentEvent::Type::ModifiedChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

void WDocument::SetReadOnly(bool b)
{
  if (m_bReadOnly == b)
    return;

  m_bReadOnly = b;

  WDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = WDocumentEvent::Type::ReadOnlyChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

WStatus WDocument::SaveDocument(bool bForce)
{
  if (!IsModified() && !bForce)
    return WStatus(W_SUCCESS);

  if (m_pCommandHistory->IsInTransaction())
    return WStatus("Can't save document while a transaction is in progress");
  // In the unlikely event that we manage to edit a doc and call save again while
  // an async save is already in progress we block on the first save to ensure
  // the correct chronological state on disk after both save ops are done.
  if (m_ActiveSaveTask.IsValid())
  {
    WTaskSystem::WaitForGroup(m_ActiveSaveTask);
    m_ActiveSaveTask.Invalidate();
  }
  WStatus result(W_SUCCESS);

  m_ActiveSaveTask = InternalSaveDocument([&result](WDocument* pDoc, WStatus res)
    { result = res; });

  WTaskSystem::WaitForGroup(m_ActiveSaveTask);
  m_ActiveSaveTask.Invalidate();

  return result;
}


WTaskGroupID WDocument::SaveDocumentAsync(AfterSaveCallback callback, bool bForce)
{
  if (!IsModified() && !bForce)
    return WTaskGroupID();

  m_ActiveSaveTask = InternalSaveDocument(callback);
  return m_ActiveSaveTask;
}

void WDocument::SetDocumentPath(WStringView sPath)
{
  WStringBuilder sTmp = sPath;
  sTmp.MakeCleanPath();

  // on Windows the same file can be referenced with an upper or lower case drive letter,
  // normalizing it here makes sure that all string comparisons against document paths work
  WPathUtils::NormalizeWindowsDriveLetter(sTmp);

  m_sDocumentPath = sTmp;
}

void WDocument::DocumentRenamed(WStringView sNewDocumentPath)
{
  SetDocumentPath(sNewDocumentPath);

  WDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = WDocumentEvent::Type::DocumentRenamed;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

void WDocument::EnsureVisible()
{
  WDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = WDocumentEvent::Type::EnsureVisible;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

WTaskGroupID WDocument::InternalSaveDocument(AfterSaveCallback callback)
{
  W_PROFILE_SCOPE("InternalSaveDocument");
  WTaskGroupID saveID = WTaskSystem::CreateTaskGroup(WTaskPriority::LongRunningHighPriority);
  auto saveTask = W_DEFAULT_NEW(WSaveDocumentTask);

  {
    saveTask->m_document = this;
    saveTask->file.SetOutput(m_sDocumentPath);
    WTaskSystem::AddTaskToGroup(saveID, saveTask);

    {
      WRttiConverterContext context;
      WRttiConverterWriter rttiConverter(&saveTask->headerGraph, &context, true, true);
      context.RegisterObject(GetGuid(), m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);
      rttiConverter.AddObjectToGraph(m_pDocumentInfo, "Header");
    }
    {
      // Do not serialize any temporary properties into the document.
      auto filter = [](const WDocumentObject*, const WAbstractProperty* pProp) -> bool
      {
        if (pProp->GetAttributeByType<WTemporaryAttribute>() != nullptr)
          return false;
        return true;
      };
      WDocumentObjectConverterWriter objectConverter(&saveTask->objectGraph, GetObjectManager(), filter);
      objectConverter.AddObjectToGraph(GetObjectManager()->GetRootObject(), "ObjectTree");

      AttachMetaDataBeforeSaving(saveTask->objectGraph);
    }
    {
      WSet<const WRTTI*> types;
      WToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types);
      WToolsSerializationUtils::SerializeTypes(types, saveTask->typesGraph);
    }
  }

  WTaskGroupID afterSaveID = WTaskSystem::CreateTaskGroup(WTaskPriority::SomeFrameMainThread);
  {
    auto afterSaveTask = W_DEFAULT_NEW(WAfterSaveDocumentTask);
    afterSaveTask->m_document = this;
    afterSaveTask->m_callback = callback;
    WTaskSystem::AddTaskToGroup(afterSaveID, afterSaveTask);
  }
  WTaskSystem::AddTaskGroupDependency(afterSaveID, saveID);
  if (!WTaskSystem::IsTaskGroupFinished(m_ActiveSaveTask))
  {
    WTaskSystem::AddTaskGroupDependency(saveID, m_ActiveSaveTask);
  }

  WTaskSystem::StartTaskGroup(saveID);
  WTaskSystem::StartTaskGroup(afterSaveID);
  return afterSaveID;
}

WStatus WDocument::ReadDocument(WStringView sDocumentPath, WUniquePtr<WAbstractObjectGraph>& ref_pHeader, WUniquePtr<WAbstractObjectGraph>& ref_pObjects,
  WUniquePtr<WAbstractObjectGraph>& ref_pTypes)
{
  WDefaultMemoryStreamStorage storage;
  WMemoryStreamReader memreader(&storage);

  {
    W_PROFILE_SCOPE("Read File");
    WFileReader file;
    if (file.Open(sDocumentPath) == W_FAILURE)
    {
      return WStatus("Unable to open file for reading!");
    }

    // range.BeginNextStep("Reading File");
    storage.ReadAll(file);

    // range.BeginNextStep("Parsing Graph");
    {
      W_PROFILE_SCOPE("parse DDL graph");
      WStopwatch sw;
      if (WAbstractGraphDdlSerializer::ReadDocument(memreader, ref_pHeader, ref_pObjects, ref_pTypes, true).Failed())
        return WStatus("Failed to parse DDL graph");

      WTime t = sw.GetRunningTotal();
      WLog::Debug("DDL parsing time: {0} msec", WArgF(t.GetMilliseconds(), 1));
    }
  }
  return WStatus(W_SUCCESS);
}

WStatus WDocument::ReadAndRegisterTypes(const WAbstractObjectGraph& types)
{
  W_PROFILE_SCOPE("Deserializing Types");
  // range.BeginNextStep("Deserializing Types");

  // Deserialize and register serialized phantom types.
  WString sDescTypeName = WGetStaticRTTI<WReflectedTypeDescriptor>()->GetTypeName();
  WDynamicArray<WReflectedTypeDescriptor*> descriptors;
  auto& nodes = types.GetAllNodes();
  descriptors.Reserve(nodes.GetCount()); // Overkill but doesn't matter much as it's just temporary.
  WRttiConverterContext context;
  WRttiConverterReader rttiConverter(&types, &context);

  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetType() == sDescTypeName)
    {
      WReflectedTypeDescriptor* pDesc = rttiConverter.CreateObjectFromNode(it.Value()).Cast<WReflectedTypeDescriptor>();
      if (pDesc->m_Flags.IsSet(WTypeFlags::Minimal))
      {
        WGetStaticRTTI<WReflectedTypeDescriptor>()->GetAllocator()->Deallocate(pDesc);
      }
      else
      {
        descriptors.PushBack(pDesc);
      }
    }
  }
  WToolsReflectionUtils::DependencySortTypeDescriptorArray(descriptors);
  for (WReflectedTypeDescriptor* desc : descriptors)
  {
    if (!WRTTI::FindTypeByName(desc->m_sTypeName))
    {
      WPhantomRttiManager::RegisterType(*desc);
    }
    WGetStaticRTTI<WReflectedTypeDescriptor>()->GetAllocator()->Deallocate(desc);
  }
  return WStatus(W_SUCCESS);
}

WStatus WDocument::InternalLoadDocument()
{
  W_PROFILE_SCOPE("InternalLoadDocument");
  // this would currently crash in Qt, due to the processEvents in the QtProgressBar
  // WProgressRange range("Loading Document", 5, false);

  WUniquePtr<WAbstractObjectGraph> header;
  WUniquePtr<WAbstractObjectGraph> objects;
  WUniquePtr<WAbstractObjectGraph> types;

  WStatus res = ReadDocument(m_sDocumentPath, header, objects, types);
  if (res.Failed())
    return res;

  res = ReadAndRegisterTypes(*types.Borrow());
  if (res.Failed())
    return res;

  {
    W_PROFILE_SCOPE("Restoring Header");
    WRttiConverterContext context;
    WRttiConverterReader rttiConverter(header.Borrow(), &context);
    auto* pHeaderNode = header->GetNodeByName("Header");
    rttiConverter.ApplyPropertiesToObject(pHeaderNode, m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);
  }

  {
    W_PROFILE_SCOPE("Restoring Objects");
    WDocumentObjectConverterReader objectConverter(
      objects.Borrow(), GetObjectManager(), WDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
    // range.BeginNextStep("Restoring Objects");
    auto* pRootNode = objects->GetNodeByName("ObjectTree");
    objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());

    SetUnknownObjectTypes(objectConverter.GetUnknownObjectTypes(), objectConverter.GetNumUnknownObjectCreations());
  }

  {
    W_PROFILE_SCOPE("Restoring Meta-Data");
    // range.BeginNextStep("Restoring Meta-Data");
    RestoreMetaDataAfterLoading(*objects.Borrow(), false);
  }

  SetModified(false);
  return WStatus(W_SUCCESS);
}

void WDocument::AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const
{
  m_DocumentObjectMetaData->AttachMetaDataToAbstractGraph(graph);
}

void WDocument::RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable)
{
  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(graph);
}

void WDocument::BeforeClosing()
{
  // This can't be done in the dtor as the task uses virtual functions on this object.
  if (m_ActiveSaveTask.IsValid())
  {
    WTaskSystem::WaitForGroup(m_ActiveSaveTask);
    m_ActiveSaveTask.Invalidate();
  }
}

void WDocument::SetUnknownObjectTypes(const WSet<WString>& Types, WUInt32 uiInstances)
{
  m_UnknownObjectTypes = Types;
  m_uiUnknownObjectTypeInstances = uiInstances;
}

void WDocument::AddLoadingError(WStringView sError)
{
  m_LoadingErrors.PushBack(sError);
}


void WDocument::BroadcastInterDocumentMessage(WReflectedClass* pMessage, WDocument* pSender)
{
  for (auto& man : WDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : man->GetAllOpenDocuments())
    {
      if (pDoc == pSender)
        continue;

      pDoc->OnInterDocumentMessage(pMessage, pSender);
    }
  }
}

void WDocument::DeleteSelectedObjects() const
{
  WTempHybridArray<WSelectionEntry, 64> objects;
  GetSelectionManager()->GetTopLevelSelection(objects);

  // make sure the whole selection is cleared, otherwise each delete command would reduce the selection one by one
  GetSelectionManager()->Clear();

  auto history = GetCommandHistory();
  history->StartTransaction("Delete Object");

  WRemoveObjectCommand cmd;

  for (const WSelectionEntry& entry : objects)
  {
    cmd.m_Object = entry.m_pObject->GetGuid();

    if (history->AddCommand(cmd).Failed())
    {
      history->CancelTransaction();
      return;
    }
  }

  history->FinishTransaction();
}

void WDocument::ShowDocumentStatus(const WFormatString& msg) const
{
  WStringBuilder tmp;

  WDocumentEvent e;
  e.m_pDocument = this;
  e.m_sStatusMsg = msg.GetText(tmp);
  e.m_Type = WDocumentEvent::Type::DocumentStatusMsg;

  m_EventsOne.Broadcast(e);
}


WResult WDocument::ComputeObjectTransformation(const WDocumentObject* pObject, WTransform& out_result) const
{
  out_result.SetIdentity();
  return W_FAILURE;
}

WObjectAccessorBase* WDocument::GetObjectAccessor() const
{
  return m_pObjectAccessor.Borrow();
}
