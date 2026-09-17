#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Command/TreeCommands.h>


void WSceneDocument::UnlinkPrefabs(WArrayPtr<const WDocumentObject*> selection)
{
  SUPER::UnlinkPrefabs(selection);

  // Clear cached names.
  for (auto pObject : selection)
  {
    auto pMetaScene = m_GameObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMetaScene->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(WGameObjectMetaData::CachedName);
  }
}


bool WSceneDocument::IsObjectEditorPrefab(const WUuid& object, WUuid* out_pPrefabAssetGuid) const
{
  auto pMeta = m_DocumentObjectMetaData->BeginReadMetaData(object);
  const bool bIsPrefab = pMeta->m_CreateFromPrefab.IsValid();

  if (out_pPrefabAssetGuid)
  {
    *out_pPrefabAssetGuid = pMeta->m_CreateFromPrefab;
  }

  m_DocumentObjectMetaData->EndReadMetaData();

  return bIsPrefab;
}


bool WSceneDocument::IsObjectEnginePrefab(const WUuid& object, WUuid* out_pPrefabAssetGuid) const
{
  const WDocumentObject* pObject = GetObjectManager()->GetObject(object);

  WTempHybridArray<WVariant, 16> values;
  pObject->GetTypeAccessor().GetValues("Components", values);

  for (WVariant& value : values)
  {
    auto pChild = GetObjectManager()->GetObject(value.Get<WUuid>());

    // search for prefab components
    if (pChild->GetTypeAccessor().GetType()->IsDerivedFrom<WPrefabReferenceComponent>())
    {
      WVariant varPrefab = pChild->GetTypeAccessor().GetValue("Prefab");

      if (varPrefab.IsA<WString>())
      {
        if (out_pPrefabAssetGuid)
        {
          const WString sAsset = varPrefab.Get<WString>();

          const auto info = WAssetCurator::GetSingleton()->FindSubAsset(sAsset);

          if (info.isValid())
          {
            *out_pPrefabAssetGuid = info->m_Data.m_Guid;
          }
        }

        return true;
      }
    }
  }

  return false;
}

void WSceneDocument::UpdatePrefabs()
{
  W_LOCK(m_GameObjectMetaData->GetMutex());
  SUPER::UpdatePrefabs();
}


WUuid WSceneDocument::ReplaceByPrefab(const WDocumentObject* pRootObject, WStringView sPrefabFile, const WUuid& prefabAsset, const WUuid& prefabSeed, bool bEnginePrefab)
{
  WUuid newGuid = SUPER::ReplaceByPrefab(pRootObject, sPrefabFile, prefabAsset, prefabSeed, bEnginePrefab);
  if (newGuid.IsValid())
  {
    auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(newGuid);
    pMeta->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(WGameObjectMetaData::CachedName);
  }
  return newGuid;
}

WUuid WSceneDocument::RevertPrefab(const WDocumentObject* pObject)
{
  auto pHistory = GetCommandHistory();
  const WVec3 vLocalPos = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<WVec3>();
  const WQuat vLocalRot = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<WQuat>();
  const WVec3 vLocalScale = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<WVec3>();
  const float fLocalUniformScale = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  WUuid newGuid = SUPER::RevertPrefab(pObject);

  if (newGuid.IsValid())
  {
    WSetObjectPropertyCommand setCmd;
    setCmd.m_Object = newGuid;

    setCmd.m_sProperty = "LocalPosition";
    setCmd.m_NewValue = vLocalPos;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalRotation";
    setCmd.m_NewValue = vLocalRot;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalScaling";
    setCmd.m_NewValue = vLocalScale;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalUniformScaling";
    setCmd.m_NewValue = fLocalUniformScale;
    pHistory->AddCommand(setCmd).AssertSuccess();
  }
  return newGuid;
}

void WSceneDocument::UpdatePrefabObject(WDocumentObject* pObject, const WUuid& PrefabAsset, const WUuid& PrefabSeed, WStringView sBasePrefab)
{
  auto pHistory = GetCommandHistory();
  const WVec3 vLocalPos = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<WVec3>();
  const WQuat vLocalRot = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<WQuat>();
  const WVec3 vLocalScale = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<WVec3>();
  const float fLocalUniformScale = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  SUPER::UpdatePrefabObject(pObject, PrefabAsset, PrefabSeed, sBasePrefab);

  // the root object has the same GUID as the PrefabSeed
  if (PrefabSeed.IsValid())
  {
    WSetObjectPropertyCommand setCmd;
    setCmd.m_Object = PrefabSeed;

    setCmd.m_sProperty = "LocalPosition";
    setCmd.m_NewValue = vLocalPos;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalRotation";
    setCmd.m_NewValue = vLocalRot;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalScaling";
    setCmd.m_NewValue = vLocalScale;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalUniformScaling";
    setCmd.m_NewValue = fLocalUniformScale;
    pHistory->AddCommand(setCmd).AssertSuccess();
  }
}

void WSceneDocument::ConvertToEditorPrefab(WArrayPtr<const WDocumentObject*> selection)
{
  WDeque<const WDocumentObject*> newSelection;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Convert to Editor Prefab");

  for (const WDocumentObject* pObject : selection)
  {
    WUuid assetGuid;
    if (!IsObjectEnginePrefab(pObject->GetGuid(), &assetGuid))
      continue;

    auto pAsset = WAssetCurator::GetSingleton()->GetSubAsset(assetGuid);

    if (!pAsset.isValid())
      continue;

    const WTransform transform = GetGlobalTransform(pObject);

    WUuid newGuid = WUuid::MakeUuid();
    WUuid newObject = ReplaceByPrefab(pObject, pAsset->m_pAssetInfo->m_Path.GetAbsolutePath(), assetGuid, newGuid, false);

    if (newObject.IsValid())
    {
      const WDocumentObject* pNewObject = GetObjectManager()->GetObject(newObject);
      SetGlobalTransform(pNewObject, transform, TransformationChanges::All);

      newSelection.PushBack(pNewObject);
    }
  }

  pHistory->FinishTransaction();

  GetSelectionManager()->SetSelection(newSelection);
}

void WSceneDocument::ConvertToEnginePrefab(WArrayPtr<const WDocumentObject*> selection)
{
  WDeque<const WDocumentObject*> newSelection;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Convert to Engine Prefab");

  WStringBuilder tmp;

  for (const WDocumentObject* pObject : selection)
  {
    WUuid assetGuid;
    if (!IsObjectEditorPrefab(pObject->GetGuid(), &assetGuid))
      continue;

    auto pAsset = WAssetCurator::GetSingleton()->GetSubAsset(assetGuid);

    if (!pAsset.isValid())
      continue;

    const WTransform transform = ComputeGlobalTransform(pObject);

    const WDocumentObject* pNewObject = nullptr;

    // create an object with the reference prefab component
    {
      WUuid ObjectGuid, CmpGuid;
      ObjectGuid = WUuid::MakeUuid();
      CmpGuid = WUuid::MakeUuid();

      WAddObjectCommand cmd;
      cmd.m_Parent = (pObject->GetParent() == GetObjectManager()->GetRootObject()) ? WUuid() : pObject->GetParent()->GetGuid();
      cmd.m_Index = pObject->GetPropertyIndex();
      cmd.SetType("WGameObject");
      cmd.m_NewObjectGuid = ObjectGuid;
      cmd.m_sParentProperty = "Children";

      W_VERIFY(pHistory->AddCommand(cmd).Succeeded(), "AddCommand failed");

      cmd.SetType("WPrefabReferenceComponent");
      cmd.m_sParentProperty = "Components";
      cmd.m_Index = -1;
      cmd.m_NewObjectGuid = CmpGuid;
      cmd.m_Parent = ObjectGuid;
      W_VERIFY(pHistory->AddCommand(cmd).Succeeded(), "AddCommand failed");

      WSetObjectPropertyCommand cmd2;
      cmd2.m_Object = CmpGuid;
      cmd2.m_sProperty = "Prefab";
      cmd2.m_NewValue = WConversionUtils::ToString(assetGuid, tmp).GetData();
      W_VERIFY(pHistory->AddCommand(cmd2).Succeeded(), "AddCommand failed");


      pNewObject = GetObjectManager()->GetObject(ObjectGuid);
    }

    // set same position
    SetGlobalTransform(pNewObject, transform, TransformationChanges::All);

    newSelection.PushBack(pNewObject);

    // delete old object
    {
      WRemoveObjectCommand rem;
      rem.m_Object = pObject->GetGuid();

      W_VERIFY(pHistory->AddCommand(rem).Succeeded(), "AddCommand failed");
    }
  }

  pHistory->FinishTransaction();

  GetSelectionManager()->SetSelection(newSelection);
}
