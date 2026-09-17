#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <Foundation/Strings/PathUtils.h>
#include <ToolsFoundation/Command/TreeCommands.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneDocumentManager, 1, WRTTIDefaultAllocator<WSceneDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;


WSceneDocumentManager::WSceneDocumentManager()
{
  // Document type descriptor for a standard W scene
  {
    auto& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();
    docTypeDesc.m_sDocumentTypeName = "Scene";
    docTypeDesc.m_sFileExtension = "WScene";
    docTypeDesc.m_sIcon = ":/AssetIcons/Scene.svg";
    docTypeDesc.m_sAssetCategory = "Construction";
    docTypeDesc.m_pDocumentType = WGetStaticRTTI<WScene2Document>();
    docTypeDesc.m_pManager = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Scene");

    docTypeDesc.m_sResourceFileExtension = "WBinScene";
    docTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::OnlyTransformManually | WAssetDocumentFlags::SupportsThumbnail;
  }

  // Document type descriptor for a prefab
  {
    auto& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();

    docTypeDesc.m_sDocumentTypeName = "Prefab";
    docTypeDesc.m_sFileExtension = "WPrefab";
    docTypeDesc.m_sIcon = ":/AssetIcons/Prefab.svg";
    docTypeDesc.m_sAssetCategory = "Construction";
    docTypeDesc.m_pDocumentType = WGetStaticRTTI<WSceneDocument>();
    docTypeDesc.m_pManager = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Prefab");

    docTypeDesc.m_sResourceFileExtension = "WBinPrefab";
    docTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave | WAssetDocumentFlags::SupportsThumbnail;
  }

  // Document type descriptor for a layer (similar to a normal scene) as it holds a scene object graph
  {
    auto& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();
    docTypeDesc.m_sDocumentTypeName = "Layer";
    docTypeDesc.m_sFileExtension = "WSceneLayer";
    docTypeDesc.m_sIcon = ":/AssetIcons/Layer.svg";
    docTypeDesc.m_sAssetCategory = "Construction";
    docTypeDesc.m_pDocumentType = WGetStaticRTTI<WLayerDocument>();
    docTypeDesc.m_pManager = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Scene_Layer");

    docTypeDesc.m_sResourceFileExtension = "";
    // A layer can not be transformed individually (at least at the moment)
    // all layers for a scene are gathered and put into one cohesive runtime scene
    docTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::DisableTransform; // TODO: Disable creation in "New Document"?
  }
}

WResult WSceneDocumentManager::OpenPickedDocument(const WDocumentObject* pPickedComponent, WUInt32 uiPartIndex)
{
  if (!pPickedComponent->GetTypeAccessor().GetType()->IsDerivedFrom<WPrefabReferenceComponent>())
    return W_FAILURE;

  // access the prefab asset
  const WVariant varPrefabGuid = pPickedComponent->GetTypeAccessor().GetValue("Prefab");

  if (varPrefabGuid.IsA<WString>())
  {
    if (TryOpenAssetDocument(varPrefabGuid.Get<WString>()).Succeeded())
      return W_SUCCESS;
  }

  return W_FAILURE;
}

void WSceneDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  if (sDocumentTypeName.IsEqual("Scene"))
  {
    out_pDocument = new WScene2Document(sPath);

    if (bCreateNewDocument)
    {
      SetupDefaultScene(out_pDocument);
    }
  }
  else if (sDocumentTypeName.IsEqual("Prefab"))
  {
    out_pDocument = new WSceneDocument(sPath, WSceneDocument::DocumentType::Prefab);
  }
  else if (sDocumentTypeName.IsEqual("Layer"))
  {
    if (pOpenContext == nullptr)
    {
      // Opened individually
      out_pDocument = new WSceneDocument(sPath, WSceneDocument::DocumentType::Layer);
    }
    else
    {
      // Opened via a parent scene document
      WScene2Document* pDoc = const_cast<WScene2Document*>(WDynamicCast<const WScene2Document*>(pOpenContext->GetDocumentObjectManager()->GetDocument()));
      out_pDocument = new WLayerDocument(sPath, pDoc);
    }
  }
}

void WSceneDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  for (auto& docTypeDesc : m_DocTypeDescs)
  {
    inout_DocumentTypes.PushBack(&docTypeDesc);
  }
}

void WSceneDocumentManager::InternalCloneDocument(WStringView sPath, WStringView sClonePath, const WUuid& documentId, const WUuid& seedGuid, const WUuid& cloneGuid, WAbstractObjectGraph* pHeader, WAbstractObjectGraph* pObjects, WAbstractObjectGraph* pTypes)
{
  WAssetDocumentManager::InternalCloneDocument(sPath, sClonePath, documentId, seedGuid, cloneGuid, pHeader, pObjects, pTypes);


  auto pRoot = pObjects->GetNodeByName("ObjectTree");
  WUuid settingsGuid = pRoot->FindProperty("Settings")->m_Value.Get<WUuid>();
  auto pSettings = pObjects->GetNode(settingsGuid);
  if (WRTTI::FindTypeByName(pSettings->GetType()) != WGetStaticRTTI<WSceneDocumentSettings>())
    return;

  // Fix up scene layers during cloning
  pObjects->ModifyNodeViaNativeCounterpart(pSettings, [&](void* pNativeObject, const WRTTI* pType)
    {
    WSceneDocumentSettings* pObject = static_cast<WSceneDocumentSettings*>(pNativeObject);

    for (WSceneLayerBase* pLayerBase : pObject->m_Layers)
    {
      if (auto pLayer = WDynamicCast<WSceneLayer*>(pLayerBase))
      {
        if (pLayer->m_Layer == documentId)
        {
          // Fix up main layer reference in layer list
          pLayer->m_Layer = cloneGuid;
        }
        else
        {
          // Clone layer.
          WStringBuilder sLayerPath;
          {
            auto assetInfo = WAssetCurator::GetSingleton()->GetSubAsset(pLayer->m_Layer);
            if (assetInfo.isValid())
            {
              sLayerPath = assetInfo->m_pAssetInfo->m_Path;
            }
            else
            {
              WLog::Error("Failed to resolve layer: {}. Cloned Layer will be invalid.");
              pLayer->m_Layer = WUuid::MakeInvalid();
            }
          }
          if (!sLayerPath.IsEmpty())
          {
            WUuid newLayerGuid = pLayer->m_Layer;
            newLayerGuid.CombineWithSeed(seedGuid);

            WStringBuilder sLayerClonePath = sClonePath;
            sLayerClonePath.RemoveFileExtension();
            sLayerClonePath.Append("_data");
            WStringBuilder sCloneFleName = WPathUtils::GetFileNameAndExtension(sLayerPath.GetData());
            sLayerClonePath.AppendPath(sCloneFleName);
            // We assume that all layers are handled by the same document manager, i.e. this.
            CloneDocument(sLayerPath, sLayerClonePath, newLayerGuid).LogFailure();
            pLayer->m_Layer = newLayerGuid;
          }
        }
      }
    } });
}

void WSceneDocumentManager::SetupDefaultScene(WDocument* pDocument)
{
  auto history = pDocument->GetCommandHistory();
  history->StartTransaction("Initial Scene Setup");

  const WUuid skyObjectGuid = WUuid::MakeUuid();
  const WUuid lightObjectGuid = WUuid::MakeUuid();
  const WUuid meshObjectGuid = WUuid::MakeUuid();

  // Thumbnail Camera
  {
    const WUuid objectGuid = WUuid::MakeUuid();

    WAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("WGameObject");
    cmd.m_NewObjectGuid = objectGuid;
    cmd.m_sParentProperty = "Children";
    W_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    // object name
    {
      WSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Name";
      propCmd.m_NewValue = "Scene Thumbnail Camera";
      W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    // camera position
    {
      WSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = WVec3(0, 0, 0);
      W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    // camera component
    {
      WAddObjectCommand cmd;
      cmd.m_Index = -1;
      cmd.SetType("WCameraComponent");
      cmd.m_Parent = objectGuid;
      cmd.m_sParentProperty = "Components";
      W_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

      // camera shortcut
      {
        WSetObjectPropertyCommand propCmd;
        propCmd.m_Object = cmd.m_NewObjectGuid;
        propCmd.m_sProperty = "EditorShortcut";
        propCmd.m_NewValue = 1;
        W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
      }

      // camera usage hint
      {
        WSetObjectPropertyCommand propCmd;
        propCmd.m_Object = cmd.m_NewObjectGuid;
        propCmd.m_sProperty = "UsageHint";
        propCmd.m_NewValue = (int)WCameraUsageHint::Thumbnail;
        W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
      }
    }
  }

  {
    WAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("WGameObject");
    cmd.m_NewObjectGuid = meshObjectGuid;
    cmd.m_sParentProperty = "Children";
    W_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    {
      WSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = WVec3(3, 0, 0);
      W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }
  }

  {
    WAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("WGameObject");
    cmd.m_NewObjectGuid = skyObjectGuid;
    cmd.m_sParentProperty = "Children";
    W_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    {
      WSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = WVec3(0, 0, 1);
      W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    {
      WRemoveObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Tags";
      propCmd.m_Index = 0; // There is only one value in the set, CastShadow.
      W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    {
      WInsertObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Tags";
      propCmd.m_Index = 0;
      propCmd.m_NewValue = "SkyLight";
      W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }
  }

  {
    WAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("WGameObject");
    cmd.m_NewObjectGuid = lightObjectGuid;
    cmd.m_sParentProperty = "Children";
    W_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    {
      WSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = WVec3(0, 0, 2);
      W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    {
      WQuat qRot = WQuat::MakeFromEulerAngles(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(55), WAngle::MakeFromDegree(90));

      WSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalRotation";
      propCmd.m_NewValue = qRot;
      W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }
  }

  {
    WAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("WSkyBoxComponent");
    cmd.m_Parent = skyObjectGuid;
    cmd.m_sParentProperty = "Components";
    W_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    {
      WSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "CubeMap";
      propCmd.m_NewValue = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
      W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    {
      WSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "ExposureBias";
      propCmd.m_NewValue = 1.0f;
      W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }
  }

  {
    WAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("WSkyLightComponent");
    cmd.m_Parent = lightObjectGuid;
    cmd.m_sParentProperty = "Components";
    W_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");
  }

  {
    WAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("WDirectionalLightComponent");
    cmd.m_Parent = lightObjectGuid;
    cmd.m_sParentProperty = "Components";
    W_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");
  }

  {
    WAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("WMeshComponent");
    cmd.m_Parent = meshObjectGuid;
    cmd.m_sParentProperty = "Components";
    W_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    {
      WSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Mesh";
      propCmd.m_NewValue = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }"; // Base/Meshes/Sphere.WMeshAsset
      W_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }
  }

  history->FinishTransaction();
}
