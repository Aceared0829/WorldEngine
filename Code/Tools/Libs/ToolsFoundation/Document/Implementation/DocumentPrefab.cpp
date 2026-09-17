#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

void WDocument::UpdatePrefabs()
{
  GetCommandHistory()->StartTransaction("Update Prefabs");

  UpdatePrefabsRecursive(GetObjectManager()->GetRootObject());

  GetCommandHistory()->FinishTransaction();

  ShowDocumentStatus("Prefabs have been updated");
  SetModified(true);
}

void WDocument::RevertPrefabs(WArrayPtr<const WDocumentObject*> selection)
{
  if (selection.IsEmpty())
    return;

  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Revert Prefab");

  for (auto pItem : selection)
  {
    RevertPrefab(pItem);
  }

  pHistory->FinishTransaction();
}

void WDocument::UnlinkPrefabs(WArrayPtr<const WDocumentObject*> selection)
{
  if (selection.IsEmpty())
    return;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Unlink Prefab");

  for (auto pObject : selection)
  {
    WUnlinkPrefabCommand cmd;
    cmd.m_Object = pObject->GetGuid();

    pHistory->AddCommand(cmd).AssertSuccess();
  }

  pHistory->FinishTransaction();
}

WStatus WDocument::CreatePrefabDocumentFromSelection(WStringView sFile, const WRTTI* pRootType, WDelegate<void(WAbstractObjectNode*)> adjustGraphNodeCB, WDelegate<void(WDocumentObject*)> adjustNewNodesCB, WDelegate<void(WAbstractObjectGraph& graph, WDynamicArray<WAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB)
{
  WTempHybridArray<WSelectionEntry, 64> selection;
  GetSelectionManager()->GetTopLevelSelectionOfType(pRootType, selection);

  if (selection.IsEmpty())
    return WStatus("To create a prefab, the selection must not be empty");

  WTempHybridArray<const WDocumentObject*, 32> nodes;
  nodes.Reserve(selection.GetCount());
  for (const auto& e : selection)
  {
    nodes.PushBack(e.m_pObject);
  }

  WUuid PrefabGuid, SeedGuid;
  SeedGuid = WUuid::MakeUuid();
  WStatus res = CreatePrefabDocument(sFile, nodes, SeedGuid, PrefabGuid, adjustGraphNodeCB, true, finalizeGraphCB);

  if (res.Succeeded())
  {
    GetCommandHistory()->StartTransaction("Replace all by Prefab");

    // this replaces ONE object by the new prefab (we pick the last one in the selection)
    WUuid newObj = ReplaceByPrefab(nodes.PeekBack(), sFile, PrefabGuid, SeedGuid, true);

    // if we had more than one selected objects, remove the others as well
    if (nodes.GetCount() > 1)
    {
      nodes.PopBack();

      for (auto pNode : nodes)
      {
        WRemoveObjectCommand remCmd;
        remCmd.m_Object = pNode->GetGuid();

        GetCommandHistory()->AddCommand(remCmd).AssertSuccess();
      }
    }

    auto pObject = GetObjectManager()->GetObject(newObj);

    if (adjustNewNodesCB.IsValid())
    {
      adjustNewNodesCB(pObject);
    }

    GetCommandHistory()->FinishTransaction();
    GetSelectionManager()->SetSelection(pObject);
  }

  return res;
}

WStatus WDocument::CreatePrefabDocument(WStringView sFile, WArrayPtr<const WDocumentObject*> rootObjects, const WUuid& invPrefabSeed,
  WUuid& out_newDocumentGuid, WDelegate<void(WAbstractObjectNode*)> adjustGraphNodeCB, bool bKeepOpen, WDelegate<void(WAbstractObjectGraph& graph, WDynamicArray<WAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB)
{
  const WDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (WDocumentManager::FindDocumentTypeFromPath(sFile, true, pTypeDesc).Failed())
    return WStatus(WFmt("Document type is unknown: '{0}'", sFile));

  pTypeDesc->m_pManager->EnsureDocumentIsClosed(sFile);

  // prepare the current state as a graph
  WAbstractObjectGraph PrefabGraph;
  WDocumentObjectConverterWriter writer(&PrefabGraph, GetObjectManager());

  WTempHybridArray<WAbstractObjectNode*, 32> graphRootNodes;
  graphRootNodes.Reserve(rootObjects.GetCount() + 1);

  for (WUInt32 i = 0; i < rootObjects.GetCount(); ++i)
  {
    auto pSaveAsPrefab = rootObjects[i];

    W_ASSERT_DEV(pSaveAsPrefab != nullptr, "CreatePrefabDocument: pSaveAsPrefab must be a valid object!");

    auto pPrefabGraphMainNode = writer.AddObjectToGraph(pSaveAsPrefab);
    graphRootNodes.PushBack(pPrefabGraphMainNode);

    // allow external adjustments
    if (adjustGraphNodeCB.IsValid())
    {
      adjustGraphNodeCB(pPrefabGraphMainNode);
    }
  }

  if (finalizeGraphCB.IsValid())
  {
    finalizeGraphCB(PrefabGraph, graphRootNodes);
  }

  PrefabGraph.ReMapNodeGuids(invPrefabSeed, true);

  WDocument* pSceneDocument = nullptr;

  W_SUCCEED_OR_RETURN(pTypeDesc->m_pManager->CreateDocument("Prefab", sFile, pSceneDocument, WDocumentFlags::RequestWindow | WDocumentFlags::AddToRecentFilesList | WDocumentFlags::EmptyDocument));

  out_newDocumentGuid = pSceneDocument->GetGuid();
  auto pPrefabSceneRoot = pSceneDocument->GetObjectManager()->GetRootObject();

  WDocumentObjectConverterReader reader(&PrefabGraph, pSceneDocument->GetObjectManager(), WDocumentObjectConverterReader::Mode::CreateAndAddToDocument);

  for (WUInt32 i = 0; i < graphRootNodes.GetCount(); ++i)
  {
    const WRTTI* pRootType = WRTTI::FindTypeByName(graphRootNodes[i]->GetType());

    WUuid rootGuid = graphRootNodes[i]->GetGuid();
    rootGuid.RevertCombinationWithSeed(invPrefabSeed);

    WDocumentObject* pPrefabSceneMainObject = pSceneDocument->GetObjectManager()->CreateObject(pRootType, rootGuid);
    pSceneDocument->GetObjectManager()->AddObject(pPrefabSceneMainObject, pPrefabSceneRoot, "Children", -1);

    reader.ApplyPropertiesToObject(graphRootNodes[i], pPrefabSceneMainObject);
  }

  pSceneDocument->SetModified(true);
  auto res = pSceneDocument->SaveDocument();

  if (!bKeepOpen)
  {
    pTypeDesc->m_pManager->CloseDocument(pSceneDocument);
  }

  return res;
}


WUuid WDocument::ReplaceByPrefab(const WDocumentObject* pRootObject, WStringView sPrefabFile, const WUuid& prefabAsset, const WUuid& prefabSeed, bool bEnginePrefab)
{
  GetCommandHistory()->StartTransaction("Replace by Prefab");

  WUuid instantiatedRoot;

  if (!bEnginePrefab) // create editor prefab
  {
    WInstantiatePrefabCommand instCmd;
    instCmd.m_Index = pRootObject->GetPropertyIndex().ConvertTo<WInt32>();
    instCmd.m_bAllowPickedPosition = false;
    instCmd.m_CreateFromPrefab = prefabAsset;
    instCmd.m_Parent = pRootObject->GetParent() == GetObjectManager()->GetRootObject() ? WUuid() : pRootObject->GetParent()->GetGuid();
    instCmd.m_sBasePrefabGraph = WPrefabUtils::ReadDocumentAsString(
      sPrefabFile); // since the prefab might have been created just now, going through the cache (via GUID) will most likely fail
    instCmd.m_RemapGuid = prefabSeed;

    GetCommandHistory()->AddCommand(instCmd).AssertSuccess();

    instantiatedRoot = instCmd.m_CreatedRootObject;
  }
  else // create an object with the reference prefab component
  {
    auto pHistory = GetCommandHistory();

    WStringBuilder tmp;
    WUuid CmpGuid = WUuid::MakeUuid();
    instantiatedRoot = WUuid::MakeUuid();

    WAddObjectCommand cmd;
    cmd.m_Parent = (pRootObject->GetParent() == GetObjectManager()->GetRootObject()) ? WUuid() : pRootObject->GetParent()->GetGuid();
    cmd.m_Index = pRootObject->GetPropertyIndex();
    cmd.SetType("WGameObject");
    cmd.m_NewObjectGuid = instantiatedRoot;
    cmd.m_sParentProperty = "Children";

    W_VERIFY(pHistory->AddCommand(cmd).Succeeded(), "AddCommand failed");

    cmd.SetType("WPrefabReferenceComponent");
    cmd.m_sParentProperty = "Components";
    cmd.m_Index = -1;
    cmd.m_NewObjectGuid = CmpGuid;
    cmd.m_Parent = instantiatedRoot;
    W_VERIFY(pHistory->AddCommand(cmd).Succeeded(), "AddCommand failed");

    WSetObjectPropertyCommand cmd2;
    cmd2.m_Object = CmpGuid;
    cmd2.m_sProperty = "Prefab";
    cmd2.m_NewValue = WConversionUtils::ToString(prefabAsset, tmp).GetData();
    W_VERIFY(pHistory->AddCommand(cmd2).Succeeded(), "AddCommand failed");
  }

  {
    WRemoveObjectCommand remCmd;
    remCmd.m_Object = pRootObject->GetGuid();

    GetCommandHistory()->AddCommand(remCmd).AssertSuccess();
  }

  GetCommandHistory()->FinishTransaction();

  return instantiatedRoot;
}

WUuid WDocument::RevertPrefab(const WDocumentObject* pObject)
{
  auto pHistory = GetCommandHistory();
  auto pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

  const WUuid PrefabAsset = pMeta->m_CreateFromPrefab;

  if (!PrefabAsset.IsValid())
  {
    m_DocumentObjectMetaData->EndReadMetaData();
    return WUuid();
  }

  WRemoveObjectCommand remCmd;
  remCmd.m_Object = pObject->GetGuid();

  WInstantiatePrefabCommand instCmd;
  instCmd.m_Index = pObject->GetPropertyIndex().ConvertTo<WInt32>();
  instCmd.m_bAllowPickedPosition = false;
  instCmd.m_CreateFromPrefab = PrefabAsset;
  instCmd.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? WUuid() : pObject->GetParent()->GetGuid();
  instCmd.m_RemapGuid = pMeta->m_PrefabSeedGuid;
  instCmd.m_sBasePrefabGraph = WPrefabCache::GetSingleton()->GetCachedPrefabDocument(pMeta->m_CreateFromPrefab);

  m_DocumentObjectMetaData->EndReadMetaData();

  pHistory->AddCommand(remCmd).AssertSuccess();
  pHistory->AddCommand(instCmd).AssertSuccess();

  return instCmd.m_CreatedRootObject;
}


void WDocument::UpdatePrefabsRecursive(WDocumentObject* pObject)
{
  // Deliberately copy the array as the UpdatePrefabObject function will add / remove elements from the array.
  auto ChildArray = pObject->GetChildren();

  WStringBuilder sPrefabBase;

  for (auto pChild : ChildArray)
  {
    auto pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pChild->GetGuid());
    const WUuid PrefabAsset = pMeta->m_CreateFromPrefab;
    const WUuid PrefabSeed = pMeta->m_PrefabSeedGuid;
    sPrefabBase = pMeta->m_sBasePrefab;

    m_DocumentObjectMetaData->EndReadMetaData();

    // if this is a prefab instance, update it
    if (PrefabAsset.IsValid())
    {
      UpdatePrefabObject(pChild, PrefabAsset, PrefabSeed, sPrefabBase);
    }
    else
    {
      // only recurse if no prefab was found
      // nested prefabs are not allowed
      UpdatePrefabsRecursive(pChild);
    }
  }
}

void WDocument::UpdatePrefabObject(WDocumentObject* pObject, const WUuid& PrefabAsset, const WUuid& PrefabSeed, WStringView sBasePrefab)
{
  const WStringBuilder& sNewBasePrefab = WPrefabCache::GetSingleton()->GetCachedPrefabDocument(PrefabAsset);

  WStringBuilder sNewMergedGraph;
  WPrefabUtils::Merge(sBasePrefab, sNewBasePrefab, pObject, true, PrefabSeed, sNewMergedGraph);

  // remove current object
  WRemoveObjectCommand rm;
  rm.m_Object = pObject->GetGuid();

  // instantiate prefab again
  WInstantiatePrefabCommand inst;
  inst.m_Index = pObject->GetPropertyIndex().ConvertTo<WInt32>();
  inst.m_bAllowPickedPosition = false;
  inst.m_CreateFromPrefab = PrefabAsset;
  inst.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? WUuid() : pObject->GetParent()->GetGuid();
  inst.m_RemapGuid = PrefabSeed;
  inst.m_sBasePrefabGraph = sNewBasePrefab;
  inst.m_sObjectGraph = sNewMergedGraph;

  GetCommandHistory()->AddCommand(rm).AssertSuccess();
  GetCommandHistory()->AddCommand(inst).AssertSuccess();
}
