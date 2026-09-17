#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorTest/AssetDocument/AssetDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static WEditorAssetDocumentTest s_EditorAssetDocumentTest;

const char* WEditorAssetDocumentTest::GetTestName() const
{
  return "Asset Document Tests";
}

void WEditorAssetDocumentTest::SetupSubTests()
{
  AddSubTest("Async Save", SubTests::ST_AsyncSave);
  AddSubTest("Save on Transform", SubTests::ST_SaveOnTransform);
  AddSubTest("File Operations", SubTests::ST_FileOperations);
}

WResult WEditorAssetDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return W_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WResult WEditorAssetDocumentTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WTestAppRun WEditorAssetDocumentTest::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_AsyncSave:
      AsyncSave();
      break;
    case SubTests::ST_SaveOnTransform:
      SaveOnTransform();
      break;
    case SubTests::ST_FileOperations:
      FileOperations();
      break;
  }
  return WTestAppRun::Quit;
}

void WEditorAssetDocumentTest::AsyncSave()
{
  WAssetDocument* pDoc = nullptr;
  WStringBuilder sName;
  W_TEST_BLOCK(WTestBlock::Enabled, "Create Document")
  {
    sName = m_sProjectPath;
    sName.AppendPath("mesh.WMeshAsset");
    pDoc = static_cast<WAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, WDocumentFlags::RequestWindow));
    if (!W_TEST_BOOL(pDoc != nullptr))
      return;
    ProcessEvents();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Save Document")
  {
    // Save doc twice in a row without processing messages and then close it.
    WDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    WObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
    WInt32 iOrder = 0;
    WTaskGroupID id = pDoc->SaveDocumentAsync(
      [&iOrder](WDocument* pDoc, WStatus res)
      {
        W_TEST_INT(iOrder, 0);
        iOrder = 1;
      },
      true);

    pAcc->StartTransaction("Edit Mesh");
    W_TEST_BOOL(pAcc->SetValueByName(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();

    // Saving while another save is in progress should block. This ensures the correct state on disk.
    WString sFile = pAcc->GetByName<WString>(pMeshAsset, "MeshFile");
    WTaskGroupID id2 = pDoc->SaveDocumentAsync([&iOrder](WDocument* pDoc, WStatus res)
      {
      W_TEST_INT(iOrder, 1);
      iOrder = 2; });

    // Closing the document should wait for the async save to finish.
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    W_TEST_INT(iOrder, 2);
    W_TEST_BOOL(WTaskSystem::IsTaskGroupFinished(id));
    W_TEST_BOOL(WTaskSystem::IsTaskGroupFinished(id2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Verify State of Disk")
  {
    pDoc = static_cast<WAssetDocument*>(m_pApplication->m_pEditorApp->OpenDocument(sName, WDocumentFlags::None));
    WDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    WObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
    WString sFile = pAcc->GetByName<WString>(pMeshAsset, "MeshFile");
    W_TEST_STRING(sFile, "Meshes/Cube.obj");
  }
  pDoc->GetDocumentManager()->CloseDocument(pDoc);
}

void WEditorAssetDocumentTest::SaveOnTransform()
{
  WAssetDocument* pDoc = nullptr;
  W_TEST_BLOCK(WTestBlock::Enabled, "Create Document")
  {
    WStringBuilder sName = m_sProjectPath;
    sName.AppendPath("mesh2.WMeshAsset");
    pDoc = static_cast<WAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, WDocumentFlags::RequestWindow));
    if (!W_TEST_BOOL(pDoc != nullptr))
      return;
    ProcessEvents();
  }

  WObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
  const WDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
  W_TEST_BLOCK(WTestBlock::Enabled, "Transform")
  {
    pAcc->StartTransaction("Edit Mesh");
    W_TEST_BOOL(pAcc->SetValueByName(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();

    WTransformStatus res = pDoc->SaveDocument();
    W_TEST_BOOL(res.Succeeded());

    // Transforming an asset in the background should fail and return NeedsImport as the asset needs to be modified which the background is not allowed to do, e.g. materials need to be created.
    res = WAssetCurator::GetSingleton()->TransformAsset(pDoc->GetGuid(), WTransformFlags::ForceTransform | WTransformFlags::BackgroundProcessing);
    W_TEST_BOOL(res.m_Result == WTransformResult::NeedsImport);

    // Transforming a mesh asset with a mesh reference will trigger the material import and update
    // the materials table which requires a save during transform.
    res = WAssetCurator::GetSingleton()->TransformAsset(pDoc->GetGuid(), WTransformFlags::ForceTransform | WTransformFlags::TriggeredManually);
    W_TEST_BOOL(res.Succeeded());
    ProcessEvents();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Verify Transform")
  {
    // Transforming should have update the mesh asset with new material slots.
    WInt32 iCount = 0;
    W_TEST_BOOL(pAcc->GetCountByName(pMeshAsset, "Materials", iCount).Succeeded());
    W_TEST_INT(iCount, 1);

    WUuid subObject = pAcc->GetByName<WUuid>(pMeshAsset, "Materials", (WInt64)0);
    W_TEST_BOOL(subObject.IsValid());
    const WDocumentObject* pSubObject = pAcc->GetObject(subObject);

    WString sLabel = pAcc->GetByName<WString>(pSubObject, "Label");
    W_TEST_STRING(sLabel, "initialShadingGroup");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Verify Failed Transform State")
  {
    WTestLogInterface log;
    WTestLogSystemScope logSystemScope(&log, true);
    // Once while resolving the path for the transform, once while writing the asset's dependencies.
    log.ExpectMessage("Failed to make path absolute 'Meshes/Missing.obj'", WLogMsgType::ErrorMsg, 2);

    // Point the asset at a mesh that does not exist, so that transforming it is bound to fail.
    pAcc->StartTransaction("Set Invalid Mesh");
    W_TEST_BOOL(pAcc->SetValueByName(pMeshAsset, "MeshFile", "Meshes/Missing.obj").Succeeded());
    pAcc->FinishTransaction();
    W_TEST_BOOL(pDoc->SaveDocument().Succeeded());

    const WTransformStatus res = WAssetCurator::GetSingleton()->TransformAsset(pDoc->GetGuid(), WTransformFlags::ForceTransform | WTransformFlags::TriggeredManually);
    W_TEST_BOOL(res.Failed());
    ProcessEvents();

    // A failed manual transform has to leave the asset in the error state, or the asset curator panel
    // does not list it, and it has to keep the message around, or the panel cannot say what went wrong.
    const WAssetCurator::WLockedSubAsset subAsset = WAssetCurator::GetSingleton()->GetSubAsset(pDoc->GetGuid());
    if (W_TEST_BOOL(subAsset.isValid()))
    {
      W_TEST_BOOL(subAsset->m_pAssetInfo->m_TransformState == WAssetInfo::TransformState::TransformError);
      W_TEST_BOOL(!subAsset->m_pAssetInfo->m_LogEntries.IsEmpty());
    }
  }
  pDoc->GetDocumentManager()->CloseDocument(pDoc);
}


void WEditorAssetDocumentTest::FileOperations()
{
  struct AssetEvent
  {
    AssetEvent() = default;
    AssetEvent(WStringView sAbsPath, WUuid assetGuid, WAssetCuratorEvent::Type type)
      : m_sAbsPath(sAbsPath)
      , m_AssetGuid(assetGuid)
      , m_Type(type)
    {
    }

    WString m_sAbsPath;
    WUuid m_AssetGuid;
    WAssetCuratorEvent::Type m_Type;
  };

  class EventHandler : public WRefCounted
  {
  public:
    void Init(const WSharedPtr<EventHandler>& pStrongThis)
    {
      // Multi-threaded event subscription can outlive the target lifetime if we don't capture a string reference to it.
      m_FileID = WFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler([pStrongThis](const WFileChangedEvent& e)
        { pStrongThis->OnAssetFilesEvent(e); });
      m_AssetID = WAssetCurator::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&EventHandler::OnAssetEvent, this));
    }

    void DeInit()
    {
      WFileSystemModel::GetSingleton()->m_FileChangedEvents.RemoveEventHandler(m_FileID);
      WAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(m_AssetID);
    }

    void OnAssetFilesEvent(const WFileChangedEvent& e)
    {
      if (e.m_Path.GetAbsolutePath().GetFileExtension().IsEqual_NoCase("WAidlt"_wsv))
        return;

      if (m_FileID == 0)
        return;

      W_LOCK(m_EventMutex);
      m_FileEvents.PushBack(e);
    }

    void OnAssetEvent(const WAssetCuratorEvent& e)
    {
      m_AssetEvents.PushBack({e.m_pInfo->m_pAssetInfo->m_Path, e.m_AssetGuid, e.m_Type});
    }



    WMutex m_EventMutex;
    WTempHybridArray<WFileChangedEvent, 4> m_FileEvents;
    WTempHybridArray<AssetEvent, 4> m_AssetEvents;

  private:
    WEventSubscriptionID m_FileID = 0;
    WEventSubscriptionID m_AssetID = 0;
  };

  WSharedPtr<EventHandler> events = W_DEFAULT_NEW(EventHandler);
  events->Init(events);
  W_SCOPE_EXIT(events->DeInit());

  auto CompareFiles = [&](WArrayPtr<WFileChangedEvent> expectedFiles, WArrayPtr<AssetEvent> expectedAssets)
  {
    W_LOCK(events->m_EventMutex);
    if (W_TEST_INT(expectedFiles.GetCount(), events->m_FileEvents.GetCount()))
    {
      for (WUInt32 i = 0; i < expectedFiles.GetCount(); i++)
      {
        W_TEST_INT((int)expectedFiles[i].m_Type, (int)events->m_FileEvents[i].m_Type);
        W_TEST_STRING(expectedFiles[i].m_Path, events->m_FileEvents[i].m_Path);
        // Ignore stats
      }
    }

    if (W_TEST_INT(expectedAssets.GetCount(), events->m_AssetEvents.GetCount()))
    {
      for (WUInt32 i = 0; i < expectedAssets.GetCount(); i++)
      {
        W_TEST_INT((int)expectedAssets[i].m_Type, (int)events->m_AssetEvents[i].m_Type);
        W_TEST_STRING(expectedAssets[i].m_sAbsPath, events->m_AssetEvents[i].m_sAbsPath);
        W_TEST_BOOL(expectedAssets[i].m_AssetGuid == events->m_AssetEvents[i].m_AssetGuid);
        // Ignore stats
      }
    }
    events->m_FileEvents.Clear();
    events->m_AssetEvents.Clear();
  };

  auto WaitForFileEvents = [&](WUInt32 uiFileEventCount, WUInt32 uiAssetEventCount)
  {
    constexpr WUInt32 WAIT_LOOPS = 500;
    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ProcessEvents();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(16));

      W_LOCK(events->m_EventMutex);
      if (events->m_FileEvents.GetCount() == uiFileEventCount && events->m_AssetEvents.GetCount() == uiAssetEventCount)
        break;
    }
  };

  auto FlushEvents = [&]()
  {
    constexpr WUInt32 WAIT_LOOPS = 60;
    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ProcessEvents();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(16));

      W_LOCK(events->m_EventMutex);
      if (!events->m_FileEvents.IsEmpty() || !events->m_AssetEvents.IsEmpty())
      {
        i = 0;
        events->m_FileEvents.Clear();
        events->m_AssetEvents.Clear();
      }
    }
  };

  auto CheckSubAsset = [](const WAssetCurator::WLockedSubAsset& subAsset, WUuid documentGuid, WString sAbsAssetPath)
  {
    if (W_TEST_BOOL(subAsset))
    {
      W_TEST_BOOL(subAsset->m_ExistanceState == WAssetExistanceState::FileUnchanged);
      W_TEST_BOOL(subAsset->m_bMainAsset);
      W_TEST_BOOL(subAsset->m_Data.m_Guid == documentGuid);
      W_TEST_STRING(subAsset->m_Data.m_sSubAssetsDocumentTypeName, "Mesh");
      W_TEST_STRING(subAsset->m_Data.m_sName, "");
      W_TEST_BOOL(subAsset->m_pAssetInfo->m_ExistanceState == WAssetExistanceState::FileUnchanged);
      W_TEST_BOOL(subAsset->m_pAssetInfo->m_TransformState == WAssetInfo::NeedsTransform);
      W_TEST_BOOL(subAsset->m_pAssetInfo->m_Info->m_DocumentID == documentGuid);
      W_TEST_STRING(subAsset->m_pAssetInfo->m_Path, sAbsAssetPath);
    }
  };

  WTempHybridArray<WString, 4> rootFolders(WFileSystemModel::GetSingleton()->GetDataDirectoryRoots());
  auto MakePath = [&](WStringView sPath)
  {
    return WDataDirPath(sPath, rootFolders);
  };

  // Wait for the asset curator to process all file events from opening the project.
  FlushEvents();

  WAssetDocument* pDoc = nullptr;
  WString sAbsAssetPath;
  WUuid documentGuid;
  W_TEST_BLOCK(WTestBlock::Enabled, "Create Document")
  {
    WStringBuilder sName = m_sProjectPath;
    sName.AppendPath("mesh3.WMeshAsset");
    pDoc = static_cast<WAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, WDocumentFlags::RequestWindow));
    W_TEST_BOOL(pDoc != nullptr);
    sAbsAssetPath = pDoc->GetDocumentPath();
    documentGuid = pDoc->GetGuid();
    ProcessEvents();

    WaitForFileEvents(2, 1);
    WFileStatus stat;
    stat.m_DocumentID = documentGuid;
    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sAbsAssetPath), {}, WFileChangedEvent::Type::FileAdded),
      WFileChangedEvent(MakePath(sAbsAssetPath), stat, WFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {AssetEvent(sAbsAssetPath, documentGuid, WAssetCuratorEvent::Type::AssetAdded)};
    CompareFiles(WMakeArrayPtr(expected), WMakeArrayPtr(expected2));

    CheckSubAsset(WAssetCurator::GetSingleton()->GetSubAsset(documentGuid), documentGuid, sAbsAssetPath);
  }

  WStringBuilder sAbsAssetCopyPath = sAbsAssetPath;
  sAbsAssetCopyPath.ChangeFileName("meshCopy");
  WUuid copyGuid;

  // Tests that copy gets a unique ID to resolve conflict.
  W_TEST_BLOCK(WTestBlock::Enabled, "Copy Asset")
  {
    const WUuid mod = WUuid::MakeStableUuidFromString(sAbsAssetCopyPath);
    copyGuid = documentGuid;
    copyGuid.CombineWithSeed(mod);

    WTestLogInterface log;
    WTestLogSystemScope logSystemScope(&log, true);
    log.ExpectMessage("Two assets have identical GUIDs:", WLogMsgType::ErrorMsg, 1);

    WOSFile::CopyFile(sAbsAssetPath, sAbsAssetCopyPath).AssertSuccess("Failed to copy file");

    WaitForFileEvents(3, 1);
    WFileStatus stat;
    stat.m_DocumentID = copyGuid;
    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sAbsAssetCopyPath), {}, WFileChangedEvent::Type::FileAdded),
      WFileChangedEvent(MakePath(sAbsAssetCopyPath), {}, WFileChangedEvent::Type::FileChanged),
      WFileChangedEvent(MakePath(sAbsAssetCopyPath), stat, WFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {AssetEvent(sAbsAssetCopyPath, copyGuid, WAssetCuratorEvent::Type::AssetAdded)};
    CompareFiles(WMakeArrayPtr(expected), WMakeArrayPtr(expected2));

    CheckSubAsset(WAssetCurator::GetSingleton()->GetSubAsset(documentGuid), documentGuid, sAbsAssetPath);
    CheckSubAsset(WAssetCurator::GetSingleton()->GetSubAsset(copyGuid), copyGuid, sAbsAssetCopyPath);
  }

  WStringBuilder sAbsAssetCopyRenamedPath = sAbsAssetCopyPath;
  sAbsAssetCopyRenamedPath.ChangeFileName("meshCopyRenamed");

  // Simple rename of not opened document
  W_TEST_BLOCK(WTestBlock::Enabled, "Rename Copied Asset")
  {
    WOSFile::MoveFileOrDirectory(sAbsAssetCopyPath, sAbsAssetCopyRenamedPath).AssertSuccess("Failed to rename file");

    WaitForFileEvents(4, 2);
    WFileStatus stat;
    stat.m_DocumentID = copyGuid;
    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sAbsAssetCopyRenamedPath), {}, WFileChangedEvent::Type::FileAdded),
      WFileChangedEvent(MakePath(sAbsAssetCopyPath), stat, WFileChangedEvent::Type::DocumentUnlinked),
      WFileChangedEvent(MakePath(sAbsAssetCopyPath), {}, WFileChangedEvent::Type::FileRemoved),
      WFileChangedEvent(MakePath(sAbsAssetCopyRenamedPath), stat, WFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {
      AssetEvent(sAbsAssetCopyRenamedPath, copyGuid, WAssetCuratorEvent::Type::AssetMoved),    // Moved
      AssetEvent(sAbsAssetCopyRenamedPath, copyGuid, WAssetCuratorEvent::Type::AssetUpdated)}; // Asset transform state updated
    CompareFiles(WMakeArrayPtr(expected), WMakeArrayPtr(expected2));

    W_TEST_BOOL(!WAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetCopyPath));
    CheckSubAsset(WAssetCurator::GetSingleton()->GetSubAsset(copyGuid), copyGuid, sAbsAssetCopyRenamedPath);
  }


  m_pApplication->m_pEditorApp->OpenDocument(sAbsAssetCopyRenamedPath, WDocumentFlags::RequestWindow);
  ProcessEvents();

  // Delete an open document, make sure document gets closed.
  W_TEST_BLOCK(WTestBlock::Enabled, "Delete Copied Asset")
  {
    WOSFile::DeleteFile(sAbsAssetCopyRenamedPath).AssertSuccess("Failed to delete file");

    WaitForFileEvents(1, 1);
    WFileStatus stat;
    stat.m_DocumentID = copyGuid;
    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sAbsAssetCopyRenamedPath), {}, WFileChangedEvent::Type::FileRemoved)};
    AssetEvent expected2[] = {AssetEvent(sAbsAssetCopyRenamedPath, copyGuid, WAssetCuratorEvent::Type::AssetRemoved)};
    CompareFiles(WMakeArrayPtr(expected), WMakeArrayPtr(expected2));
    W_TEST_BOOL(!WAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetCopyPath));
    W_TEST_BOOL(!WAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetCopyRenamedPath));
    W_TEST_BOOL(WDocumentManager::GetDocumentByGuid(copyGuid) == nullptr);
  }

  WStringBuilder sAbsAssetRenamedPath = sAbsAssetPath;
  sAbsAssetRenamedPath.ChangeFileName("meshRenamed");
  // Test that a renamed asset renames the open document as well.
  W_TEST_BLOCK(WTestBlock::Enabled, "Rename Asset")
  {
    WOSFile::MoveFileOrDirectory(sAbsAssetPath, sAbsAssetRenamedPath).AssertSuccess("Failed to rename file");

    WaitForFileEvents(4, 2);
    WFileStatus stat;
    stat.m_DocumentID = documentGuid;
    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sAbsAssetRenamedPath), {}, WFileChangedEvent::Type::FileAdded),
      WFileChangedEvent(MakePath(sAbsAssetPath), stat, WFileChangedEvent::Type::DocumentUnlinked),
      WFileChangedEvent(MakePath(sAbsAssetPath), {}, WFileChangedEvent::Type::FileRemoved),
      WFileChangedEvent(MakePath(sAbsAssetRenamedPath), stat, WFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {
      AssetEvent(sAbsAssetRenamedPath, documentGuid, WAssetCuratorEvent::Type::AssetMoved),    // Moved
      AssetEvent(sAbsAssetRenamedPath, documentGuid, WAssetCuratorEvent::Type::AssetUpdated)}; // Asset transform state updated
    CompareFiles(WMakeArrayPtr(expected), WMakeArrayPtr(expected2));
    W_TEST_BOOL(!WAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetPath));
    CheckSubAsset(WAssetCurator::GetSingleton()->GetSubAsset(documentGuid), documentGuid, sAbsAssetRenamedPath);

    W_TEST_STRING(pDoc->GetDocumentPath(), sAbsAssetRenamedPath);
  }

  WStringBuilder sAbsAssetRenamedPath2 = sAbsAssetPath;
  sAbsAssetRenamedPath2.ChangeFileName("MeshRenameD");
  W_TEST_BLOCK(WTestBlock::Enabled, "Rename Asset Casing Only")
  {
    WOSFile::MoveFileOrDirectory(sAbsAssetRenamedPath, sAbsAssetRenamedPath2).AssertSuccess("Failed to rename file");

    WaitForFileEvents(4, 2);
    WFileStatus stat;
    stat.m_DocumentID = documentGuid;
    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sAbsAssetRenamedPath2), {}, WFileChangedEvent::Type::FileAdded),
      WFileChangedEvent(MakePath(sAbsAssetRenamedPath), stat, WFileChangedEvent::Type::DocumentUnlinked),
      WFileChangedEvent(MakePath(sAbsAssetRenamedPath), {}, WFileChangedEvent::Type::FileRemoved),
      WFileChangedEvent(MakePath(sAbsAssetRenamedPath2), stat, WFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {
      AssetEvent(sAbsAssetRenamedPath2, documentGuid, WAssetCuratorEvent::Type::AssetMoved),    // Moved
      AssetEvent(sAbsAssetRenamedPath2, documentGuid, WAssetCuratorEvent::Type::AssetUpdated)}; // Asset transform state updated
    CompareFiles(WMakeArrayPtr(expected), WMakeArrayPtr(expected2));
    W_TEST_BOOL(!WAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetPath));
    CheckSubAsset(WAssetCurator::GetSingleton()->GetSubAsset(documentGuid), documentGuid, sAbsAssetRenamedPath2);

    W_TEST_STRING(pDoc->GetDocumentPath(), sAbsAssetRenamedPath2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Overwrite asset with a different asset")
  {
    const WDocumentTypeDescriptor* pTypeDesc = pDoc->GetAssetDocumentTypeDescriptor();

    WUuid overwriteGuid = documentGuid;
    overwriteGuid.CombineWithSeed(WUuid::MakeStableUuidFromString("Overwritten"));

    WStringBuilder sTemp;
    WStringBuilder sTempTarget = WOSFile::GetTempDataFolder();
    sTempTarget.AppendPath(WPathUtils::GetFileNameAndExtension(sAbsAssetRenamedPath2));
    sTempTarget.ChangeFileName(WConversionUtils::ToString(overwriteGuid, sTemp));

    W_TEST_RESULT(pTypeDesc->m_pManager->CloneDocument(sAbsAssetRenamedPath2, sTempTarget, overwriteGuid).GetResult());
    W_TEST_RESULT(WOSFile::CopyFile(sTempTarget, sAbsAssetRenamedPath2));
    WOSFile::DeleteFile(sTempTarget).IgnoreResult();

    WaitForFileEvents(3, 2);

    WFileStatus stat;
    stat.m_DocumentID = documentGuid;
    WFileStatus newStat;
    stat.m_DocumentID = overwriteGuid;
    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sAbsAssetRenamedPath2), stat, WFileChangedEvent::Type::FileChanged),
      WFileChangedEvent(MakePath(sAbsAssetRenamedPath2), stat, WFileChangedEvent::Type::DocumentUnlinked),
      WFileChangedEvent(MakePath(sAbsAssetRenamedPath2), newStat, WFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {
      AssetEvent(sAbsAssetRenamedPath2, documentGuid, WAssetCuratorEvent::Type::AssetRemoved),
      AssetEvent(sAbsAssetRenamedPath2, overwriteGuid, WAssetCuratorEvent::Type::AssetAdded)};
    CompareFiles(WMakeArrayPtr(expected), WMakeArrayPtr(expected2));
    W_TEST_BOOL(WAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetRenamedPath2));
    W_TEST_BOOL(!WAssetCurator::GetSingleton()->GetSubAsset(documentGuid));
    CheckSubAsset(WAssetCurator::GetSingleton()->GetSubAsset(overwriteGuid), overwriteGuid, sAbsAssetRenamedPath2);
  }
}
