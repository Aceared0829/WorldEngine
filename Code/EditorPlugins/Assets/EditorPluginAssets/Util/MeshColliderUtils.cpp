#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/Util/MeshColliderUtils.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/PathUtils.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

namespace
{
  constexpr WStringView s_sMeshIncludeTags = "MeshIncludeTags"_wsv;
  constexpr WStringView s_sMeshExcludeTags = "MeshExcludeTags"_wsv;

  constexpr WStringView s_ColliderSubMeshProperties[] = {s_sMeshIncludeTags, s_sMeshExcludeTags};

  constexpr WStringView s_ColliderImportProperties[] = {
    "MeshFile"_wsv,
    s_sMeshIncludeTags,
    s_sMeshExcludeTags,
    "ImportTransform"_wsv,
    "RightDir"_wsv,
    "UpDir"_wsv,
    "FlipForwardDir"_wsv,
    "PositionOffset"_wsv,
    "UniformScaling"_wsv,
  };

  constexpr WStringView s_ColliderSimplificationProperties[] = {
    "SimplifyMesh"_wsv,
    "MeshSimplification"_wsv,
    "MaxSimplificationError"_wsv,
    "NormalWeight"_wsv,
    "AggressiveSimplification"_wsv,
  };

  /// An unwritten property and one set to an empty string mean the same thing here.
  WString GetTagValue(const WVariantDictionary& properties, WStringView sProperty)
  {
    WVariant value;
    if (!properties.TryGetValue(sProperty, value) || !value.IsA<WString>())
      return {};

    return value.Get<WString>();
  }

  /// Equivalent of WSimpleAssetDocument::GetPropertyObject(), which can't be called without knowing
  /// the concrete asset type.
  const WDocumentObject* GetColliderTopLevelObject(const WDocument* pDoc)
  {
    const WDocumentObject* pRoot = pDoc->GetObjectManager()->GetRootObject();
    if (pRoot == nullptr || pRoot->GetChildren().GetCount() != 1)
      return nullptr;

    return pRoot->GetChildren()[0];
  }
} // namespace

bool WMeshColliderUtils::IsMeshAsset(const WUuid& assetGuid)
{
  // The actions that call this refresh their state whenever a menu is built, which also happens
  // in the headless WEditorProcessor, where no asset curator exists.
  if (WAssetCurator::GetSingleton() == nullptr)
    return false;

  auto pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (!pSubAsset.isValid() || pSubAsset->m_pAssetInfo == nullptr || pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor == nullptr)
    return false;

  const WStringView sType = pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName;
  return sType == s_sMeshDocType || sType == s_sAnimatedMeshDocType;
}

WArrayPtr<const WStringView> WMeshColliderUtils::GetImportPropertyNames()
{
  return WMakeArrayPtr(s_ColliderImportProperties);
}

WArrayPtr<const WStringView> WMeshColliderUtils::GetSimplificationPropertyNames()
{
  return WMakeArrayPtr(s_ColliderSimplificationProperties);
}

WArrayPtr<const WStringView> WMeshColliderUtils::GetSubMeshPropertyNames()
{
  return WMakeArrayPtr(s_ColliderSubMeshProperties);
}

WStringView WMeshColliderUtils::GetDocumentType(WEnum<WCollisionMeshKind> kind)
{
  return (kind == WCollisionMeshKind::ConvexHull) ? "Jolt_Colmesh_Convex"_wsv : "Jolt_Colmesh_Triangle"_wsv;
}

WStringView WMeshColliderUtils::GetExtension(WEnum<WCollisionMeshKind> kind)
{
  return (kind == WCollisionMeshKind::ConvexHull) ? "WJoltConvexCollisionMeshAsset"_wsv : "WJoltCollisionMeshAsset"_wsv;
}

WUuid WMeshColliderUtils::FindExisting(WEnum<WCollisionMeshKind> kind, WStringView sMeshFile, const WVariantDictionary& meshImportProperties, WStringView sMeshAssetPath)
{
  if (sMeshFile.IsEmpty())
    return {};

  const WStringView sDocType = GetDocumentType(kind);

  struct Candidate
  {
    WUuid m_Guid;
    WString m_sPath;
  };

  WHybridArray<Candidate, 8> candidates;

  auto pAssets = WAssetCurator::GetSingleton()->GetKnownSubAssets();
  for (auto it : *pAssets)
  {
    const WSubAsset& subAsset = it.Value();
    if (!subAsset.m_bMainAsset || subAsset.m_pAssetInfo == nullptr || subAsset.m_pAssetInfo->m_pDocumentTypeDescriptor == nullptr)
      continue;

    if (subAsset.m_pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName != sDocType)
      continue;

    const WAssetDocumentInfo* pInfo = subAsset.m_pAssetInfo->m_Info.Borrow();
    if (pInfo != nullptr && pInfo->m_TransformDependencies.Contains(sMeshFile))
    {
      candidates.PushBack({subAsset.m_Data.m_Guid, subAsset.m_pAssetInfo->m_Path.GetAbsolutePath()});
    }
  }

  if (candidates.IsEmpty())
    return {};

  // Which sub-mesh a candidate selects is only stored inside its document, so every candidate has to
  // be read, even a single one: it may belong to a different mesh asset importing a different
  // sub-mesh out of the same model file.
  const WString sMeshInclude = GetTagValue(meshImportProperties, s_sMeshIncludeTags);
  const WString sMeshExclude = GetTagValue(meshImportProperties, s_sMeshExcludeTags);

  const WStringBuilder sMeshStem = WPathUtils::GetFileName(sMeshAssetPath);

  WUuid nameMatch;
  WUuid subMeshMatch;

  for (const Candidate& candidate : candidates)
  {
    WVariantDictionary colliderProperties;
    if (ReadMeshProperties(candidate.m_sPath, GetSubMeshPropertyNames(), colliderProperties).Failed())
      continue;

    // a collider built from a different sub-mesh is the wrong geometry, not a worse match
    if (GetTagValue(colliderProperties, s_sMeshIncludeTags) != sMeshInclude || GetTagValue(colliderProperties, s_sMeshExcludeTags) != sMeshExclude)
      continue;

    if (!subMeshMatch.IsValid())
      subMeshMatch = candidate.m_Guid;

    // Several colliders can share a sub-mesh, e.g. one simplified and one not. The one named after
    // the mesh asset is then the one generated for it.
    if (!sMeshStem.IsEmpty() && !nameMatch.IsValid() && sMeshStem.IsEqual_NoCase(WPathUtils::GetFileName(candidate.m_sPath)))
      nameMatch = candidate.m_Guid;
  }

  if (nameMatch.IsValid())
    return nameMatch;

  return subMeshMatch;
}

WResult WMeshColliderUtils::ReadMeshProperties(WStringView sAbsMeshAssetPath, WArrayPtr<const WStringView> properties, WVariantDictionary& out_values)
{
  bool bWasOpen = false;
  WDocument* pDoc = nullptr;

  const WDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (WDocumentManager::FindDocumentTypeFromPath(sAbsMeshAssetPath, false, pTypeDesc).Succeeded())
  {
    pDoc = pTypeDesc->m_pManager->GetDocumentByPath(sAbsMeshAssetPath);
    bWasOpen = (pDoc != nullptr);
  }

  if (pDoc == nullptr)
    pDoc = WQtEditorApp::GetSingleton()->OpenDocument(sAbsMeshAssetPath, WDocumentFlags::None);

  if (pDoc == nullptr)
    return W_FAILURE;

  WResult res = W_FAILURE;

  if (const WDocumentObject* pPropObj = GetColliderTopLevelObject(pDoc))
  {
    const WIReflectedTypeAccessor& accessor = pPropObj->GetTypeAccessor();

    for (WStringView sProperty : properties)
    {
      const WVariant value = accessor.GetValue(sProperty);
      if (value.IsValid())
      {
        out_values.Insert(sProperty, value);
      }
    }

    res = W_SUCCESS;
  }

  if (!bWasOpen && !pDoc->HasWindowBeenRequested())
  {
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
  }

  return res;
}

WStatus WMeshColliderUtils::CreateCollisionMesh(WStringView sAbsColliderPath, WEnum<WCollisionMeshKind> kind, const WVariantDictionary& importProperties, WStringView sSurface, bool bOverwriteExisting, WUuid& out_guid)
{
  out_guid = WUuid();

  if (sAbsColliderPath.IsEmpty())
    return WStatus("No path for the collision mesh asset was given.");

  const bool bExists = WOSFile::ExistsFile(sAbsColliderPath);

  // CreateDocument reports an already open document through a modal message box, which would hang an
  // automated caller. Refuse here instead.
  if (bExists && !bOverwriteExisting)
    return WStatus(WFmt("'{}' already exists. Delete it first, or choose a different name.", sAbsColliderPath));

  // An existing collider is rewritten in place rather than deleted and created again, so that it
  // keeps its guid and anything referencing it keeps working.
  WDocument* pDoc = bExists ? WQtEditorApp::GetSingleton()->OpenDocument(sAbsColliderPath, WDocumentFlags::None)
                             : WQtEditorApp::GetSingleton()->CreateDocument(sAbsColliderPath, WDocumentFlags::None);

  if (pDoc == nullptr)
    return WStatus(WFmt("Failed to {} collision mesh asset '{}'. Is the Jolt plugin enabled?", bExists ? "open" : "create", sAbsColliderPath));

  WStatus result = WStatus(W_SUCCESS);

  {
    auto pHistory = pDoc->GetCommandHistory();
    pHistory->StartTransaction("Create Collision Mesh from Mesh");

    // in a lambda, so that every failure path below cancels the transaction
    auto ApplyProperties = [&]() -> WStatus
    {
      const WDocumentObject* pPropObj = GetColliderTopLevelObject(pDoc);
      if (pPropObj == nullptr)
        return WStatus("The collision mesh asset has an unexpected structure.");

      const WRTTI* pType = pPropObj->GetTypeAccessor().GetType();

      WHybridArray<WStringView, 24> toWrite;
      toWrite = WMakeArrayPtr(s_ColliderImportProperties);

      if (kind == WCollisionMeshKind::TriangleMesh)
      {
        toWrite.PushBackRange(WMakeArrayPtr(s_ColliderSimplificationProperties));
      }

      for (WStringView sProperty : toWrite)
      {
        WVariant value;
        if (!importProperties.TryGetValue(sProperty, value))
          continue;

        // the two types are matched by name, so one of them may not have this property
        if (pType->FindPropertyByName(sProperty) == nullptr)
          continue;

        WSetObjectPropertyCommand cmd;
        cmd.m_Object = pPropObj->GetGuid();
        cmd.m_sProperty = sProperty;
        cmd.m_NewValue = value;
        W_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));
      }

      // "Surface" is the convex mesh's single surface. A triangle mesh has the "Surfaces" array
      // instead, which is filled from the model's material slots at transform time.
      if (!sSurface.IsEmpty() && kind == WCollisionMeshKind::ConvexHull && pType->FindPropertyByName("Surface") != nullptr)
      {
        WSetObjectPropertyCommand cmd;
        cmd.m_Object = pPropObj->GetGuid();
        cmd.m_sProperty = "Surface";
        cmd.m_NewValue = WString(sSurface);
        W_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));
      }

      return WStatus(W_SUCCESS);
    };

    result = ApplyProperties();

    if (result.Succeeded())
    {
      pHistory->FinishTransaction();
    }
    else
    {
      pHistory->CancelTransaction();
    }
  }

  if (result.Failed())
  {
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    return result;
  }

  if (pDoc->SaveDocument(true).Failed())
  {
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    return WStatus(WFmt("Failed to save collision mesh asset '{}'.", sAbsColliderPath));
  }

  out_guid = pDoc->GetGuid();

  const WString sPath = pDoc->GetDocumentPath();
  pDoc->GetDocumentManager()->CloseDocument(pDoc);

  // without this the asset is only picked up by the next file system scan
  WFileSystemModel::GetSingleton()->NotifyOfChange(sPath);

  return WStatus(W_SUCCESS);
}
