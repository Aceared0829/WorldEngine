#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/Util/MeshColliderUtils.h>
#include <EditorPluginAssets/Util/MeshLodCreator.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

namespace
{
  /// The mesh asset properties that a LOD asset has to share with the mesh it is a LOD of, so that
  /// it describes the same geometry in the same place, only with fewer triangles.
  constexpr WStringView s_ImportProperties[] = {
    "MeshFile"_wsv,
    "MeshIncludeTags"_wsv,
    "MeshExcludeTags"_wsv,
    "ImportTransform"_wsv,
    "RightDir"_wsv,
    "UpDir"_wsv,
    "FlipForwardDir"_wsv,
    "PositionOffset"_wsv,
    "UniformScaling"_wsv,
    "RecalculateNormals"_wsv,
    "RecalculateTangents"_wsv,
    "HighPrecision"_wsv,
    "VertexColorConversion"_wsv,
    "ImportMaterials"_wsv,
    "NormalWeight"_wsv,
    "AggressiveSimplification"_wsv,
  };

  /// Read alongside the import properties, but handled separately: they decide whether LODs can be
  /// made at all and where the ladder starts.
  constexpr WStringView s_sPrimitiveType = "PrimitiveType"_wsv;
  constexpr WStringView s_sSimplifyMesh = "SimplifyMesh"_wsv;
  constexpr WStringView s_sMeshSimplification = "MeshSimplification"_wsv;

  /// Equivalent of WSimpleAssetDocument::GetPropertyObject(), which can't be called without knowing
  /// the concrete asset type.
  const WDocumentObject* GetTopLevelObject(const WDocument* pDoc)
  {
    const WDocumentObject* pRoot = pDoc->GetObjectManager()->GetRootObject();
    if (pRoot == nullptr || pRoot->GetChildren().GetCount() != 1)
      return nullptr;

    return pRoot->GetChildren()[0];
  }

  /// Reads the mesh asset's import settings and material slots without knowing its C++ type.
  ///
  /// Only closes the document again if it had to be opened here and nothing has claimed a window for
  /// it, so that a document someone else is working with is left alone.
  WResult ReadMeshAsset(WStringView sAbsDocumentPath, WMeshLodSource& ref_source)
  {
    bool bWasOpen = false;
    WDocument* pDoc = nullptr;

    const WDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (WDocumentManager::FindDocumentTypeFromPath(sAbsDocumentPath, false, pTypeDesc).Succeeded())
    {
      pDoc = pTypeDesc->m_pManager->GetDocumentByPath(sAbsDocumentPath);
      bWasOpen = (pDoc != nullptr);
    }

    if (pDoc == nullptr)
      pDoc = WQtEditorApp::GetSingleton()->OpenDocument(sAbsDocumentPath, WDocumentFlags::None);

    if (pDoc == nullptr)
      return W_FAILURE;

    WResult res = W_FAILURE;

    if (const WDocumentObject* pPropObj = GetTopLevelObject(pDoc))
    {
      const WIReflectedTypeAccessor& accessor = pPropObj->GetTypeAccessor();

      for (WStringView sProperty : s_ImportProperties)
      {
        const WVariant value = accessor.GetValue(sProperty);
        if (value.IsValid())
        {
          ref_source.m_ImportProperties.Insert(sProperty, value);
        }
      }

      // a primitive is generated procedurally, there is no geometry to simplify
      const WVariant primitiveType = accessor.GetValue(s_sPrimitiveType);
      ref_source.m_bIsPrimitive = primitiveType.IsValid() && primitiveType.ConvertTo<WInt64>() != 0; // 0 is WMeshPrimitive::File

      const WVariant meshFile = accessor.GetValue("MeshFile"_wsv);
      if (!ref_source.m_bIsPrimitive && meshFile.IsA<WString>())
      {
        ref_source.m_sMeshFile = meshFile.Get<WString>();
      }

      // decides whether the LODs may share the folder named after the model file
      const WVariant includeTags = accessor.GetValue("MeshIncludeTags"_wsv);
      if (includeTags.IsA<WString>())
      {
        ref_source.m_sMeshIncludeTags = includeTags.Get<WString>();
      }

      // Where the LOD ladder starts. A mesh that does not simplify at all starts from the full model,
      // no matter what value the (then unused) property happens to hold.
      const WVariant bSimplify = accessor.GetValue(s_sSimplifyMesh);
      const WVariant uiSimplification = accessor.GetValue(s_sMeshSimplification);

      if (bSimplify.IsValid() && bSimplify.ConvertTo<bool>() && uiSimplification.IsValid())
      {
        ref_source.m_uiBaseSimplification = (WUInt8)WMath::Clamp<WInt64>(uiSimplification.ConvertTo<WInt64>(), 0, 99);
      }

      // the LODs render with the same materials as the mesh, so the slots are copied rather than re-imported
      const WInt32 iSlots = accessor.GetCount("Materials"_wsv);
      for (WInt32 i = 0; i < iSlots; ++i)
      {
        const WVariant slotGuid = accessor.GetValue("Materials"_wsv, i);
        if (!slotGuid.IsA<WUuid>())
          continue;

        const WDocumentObject* pSlot = pDoc->GetObjectManager()->GetObject(slotGuid.Get<WUuid>());
        if (pSlot == nullptr)
          continue;

        WVariantDictionary& slot = ref_source.m_MaterialSlots.ExpandAndGetRef();
        slot.Insert("Label", pSlot->GetTypeAccessor().GetValue("Label"_wsv));
        slot.Insert("Resource", pSlot->GetTypeAccessor().GetValue("Resource"_wsv));
      }

      res = W_SUCCESS;
    }

    if (!bWasOpen && !pDoc->HasWindowBeenRequested())
    {
      pDoc->GetDocumentManager()->CloseDocument(pDoc);
    }

    return res;
  }

  /// An existing folder is preferred over inventing a second one next to it, so that a mesh that was
  /// renamed after its import keeps writing into the folder its LODs are already in.
  WString DetermineLodFolder(WStringView sMeshAssetPath, WStringView sMeshFile, WStringView sMeshIncludeTags)
  {
    WHybridArray<WString, 2> candidates;
    WMeshLodCreator::GetLodFolderCandidates(sMeshAssetPath, sMeshFile, sMeshIncludeTags, candidates);

    for (const WString& sPath : candidates)
    {
      if (WOSFile::ExistsDirectory(sPath))
        return sPath;
    }

    // none exists yet, so the mesh asset's own name decides - which is what a fresh import would use
    return candidates.IsEmpty() ? WString() : candidates[0];
  }
} // namespace

void WMeshLodCreator::GetLodFolderCandidates(WStringView sMeshAssetPath, WStringView sMeshFile, WStringView sMeshIncludeTags, WDynamicArray<WString>& out_folders)
{
  out_folders.Clear();

  WStringBuilder sDir = sMeshAssetPath;
  sDir.PathParentDirectory();

  WHybridArray<WStringView, 2> names;
  names.PushBack(WPathUtils::GetFileName(sMeshAssetPath));

  // A mesh that imports only one sub-object shares its model file with the other sub-objects, which
  // are separate mesh assets next to it. They would all resolve to the folder named after that file
  // and take over whichever LODs got there first, so only this asset's own name is allowed.
  if (!sMeshFile.IsEmpty() && sMeshIncludeTags.IsEmpty())
  {
    const WStringView sSourceName = WPathUtils::GetFileName(sMeshFile);
    if (sSourceName != names[0])
    {
      names.PushBack(sSourceName);
    }
  }

  for (WStringView sName : names)
  {
    WStringBuilder sFolderName;
    sFolderName.SetFormat("{}_data", sName);

    WStringBuilder sPath = sDir;
    sPath.AppendPath(sFolderName);

    out_folders.PushBack(sPath);
  }
}

bool WMeshLodSource::HasLod(WUInt32 uiLod) const
{
  if (uiLod == 0 || uiLod > m_ExistingLods.GetCount())
    return false;

  return m_ExistingLods[uiLod - 1].IsValid();
}

bool WMeshLodCreator::IsMeshAsset(const WUuid& assetGuid)
{
  return WMeshColliderUtils::IsMeshAsset(assetGuid);
}

WUInt8 WMeshLodCreator::GetLodSimplification(WUInt8 uiBaseSimplification, WUInt32 uiLod)
{
  // the value is the percentage of triangles removed, so halving what is left each time is the
  // midpoint between the previous level and 100
  float fSimplification = WMath::Clamp<float>(uiBaseSimplification, 0.0f, 99.0f);

  for (WUInt32 i = 0; i < uiLod; ++i)
  {
    fSimplification += (100.0f - fSimplification) * 0.5f;
  }

  // 100 would remove the whole mesh, and the importer clamps to 99 anyway
  return (WUInt8)WMath::Clamp<WInt32>((WInt32)(fSimplification + 0.5f), 1, 99);
}

WUInt8 WMeshLodCreator::GetLodSimplificationError(WUInt32 uiLod)
{
  // What the mesh import uses for its own LODs. A more distant mesh can afford a coarser silhouette,
  // and the last entry repeats for levels past the table.
  constexpr WUInt8 uiErrors[] = {5, 5, 10, 15};

  if (uiLod == 0)
    return uiErrors[0];

  return uiErrors[WMath::Min<WUInt32>(uiLod - 1, W_ARRAY_SIZE(uiErrors) - 1)];
}

WString WMeshLodCreator::GetLodPath(const WMeshLodSource& source, WUInt32 uiLod)
{
  WStringBuilder sName;
  sName.SetFormat("LOD-{}.WMeshAsset", uiLod);

  WStringBuilder sPath = source.m_sLodFolder;
  sPath.AppendPath(sName);
  return sPath;
}

WResult WMeshLodCreator::GatherMeshLodSource(const WUuid& meshAssetGuid, WMeshLodSource& out_source)
{
  out_source = WMeshLodSource();
  out_source.m_MeshAssetGuid = meshAssetGuid;

  WStringBuilder sMeshAssetPath;

  // the curator lock must not be held while documents are opened further below
  {
    auto pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(meshAssetGuid);
    if (!pSubAsset.isValid() || pSubAsset->m_pAssetInfo == nullptr)
      return W_FAILURE;

    const WAssetInfo* pAssetInfo = pSubAsset->m_pAssetInfo;
    if (pAssetInfo->m_pDocumentTypeDescriptor == nullptr)
      return W_FAILURE;

    const WStringView sDocType = pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName;
    if (sDocType != WMeshColliderUtils::s_sMeshDocType && sDocType != WMeshColliderUtils::s_sAnimatedMeshDocType)
      return W_FAILURE;

    out_source.m_bAnimated = (sDocType == WMeshColliderUtils::s_sAnimatedMeshDocType);
    sMeshAssetPath = pAssetInfo->m_Path.GetAbsolutePath();
    out_source.m_sMeshAssetPath = sMeshAssetPath;
  }

  // a mesh asset that cannot be read leaves the source without a mesh file, which the caller reports
  ReadMeshAsset(sMeshAssetPath, out_source).IgnoreResult();

  out_source.m_sLodFolder = DetermineLodFolder(sMeshAssetPath, out_source.m_sMeshFile, out_source.m_sMeshIncludeTags);

  for (WUInt32 uiLod = 1; uiLod <= s_uiMaxLods; ++uiLod)
  {
    const WString sPath = GetLodPath(out_source, uiLod);

    auto pLod = WAssetCurator::GetSingleton()->FindSubAsset(sPath);
    out_source.m_ExistingLods.PushBack(pLod.isValid() ? pLod->m_Data.m_Guid : WUuid());
  }

  // trailing gaps say nothing, only the ones between existing LODs matter
  while (!out_source.m_ExistingLods.IsEmpty() && !out_source.m_ExistingLods.PeekBack().IsValid())
  {
    out_source.m_ExistingLods.PopBack();
  }

  return W_SUCCESS;
}

WStatus WMeshLodCreator::CreateMeshLods(const WMeshLodSource& source, const WMeshLodOptions& options, WUInt32& out_uiCreated, WUInt32& out_uiSkipped)
{
  out_uiCreated = 0;
  out_uiSkipped = 0;

  if (source.m_bIsPrimitive)
    return WStatus("This mesh asset uses a procedural primitive, not a model file, so no LODs can be generated from it.");

  if (source.m_sMeshFile.IsEmpty())
    return WStatus("The source file of this mesh asset could not be read, so no LODs can be generated from it.");

  if (source.m_sLodFolder.IsEmpty())
    return WStatus("The folder for the LOD assets could not be determined.");

  const WUInt32 uiLodCount = WMath::Min(options.m_uiLodCount, s_uiMaxLods);
  if (uiLodCount == 0)
    return WStatus("No LODs were requested.");

  // The LOD sub-folder usually does not exist yet. Creating a document below a missing folder fails
  // through a modal message box, which would hang an automated caller.
  if (WOSFile::CreateDirectoryStructure(source.m_sLodFolder).Failed())
    return WStatus(WFmt("Failed to create the folder '{}'.", source.m_sLodFolder));

  WHybridArray<WString, 4> created;

  for (WUInt32 uiLod = 1; uiLod <= uiLodCount; ++uiLod)
  {
    const WString sPath = GetLodPath(source, uiLod);

    const bool bExists = WOSFile::ExistsFile(sPath);

    // An existing LOD may have been tuned by hand, so replacing it has to be asked for.
    if (bExists && !options.m_bOverwriteExisting)
    {
      WLog::Info("Skipping '{}': it already exists.", sPath);
      ++out_uiSkipped;
      continue;
    }

    // An existing LOD is rewritten in place rather than deleted and created again, so that it keeps
    // its guid and anything referencing it keeps working.
    WDocument* pDoc = bExists ? WQtEditorApp::GetSingleton()->OpenDocument(sPath, WDocumentFlags::None)
                               : WQtEditorApp::GetSingleton()->CreateDocument(sPath, WDocumentFlags::None);

    if (pDoc == nullptr)
      return WStatus(WFmt("Failed to {} LOD asset '{}'.", bExists ? "open" : "create", sPath));

    WStatus result = WStatus(W_SUCCESS);

    {
      auto pHistory = pDoc->GetCommandHistory();
      pHistory->StartTransaction("Create LOD from Mesh");

      // in a lambda, so that every failure path below cancels the transaction
      auto ApplyProperties = [&]() -> WStatus
      {
        const WDocumentObject* pPropObj = GetTopLevelObject(pDoc);
        if (pPropObj == nullptr)
          return WStatus("The mesh asset has an unexpected structure.");

        const WRTTI* pType = pPropObj->GetTypeAccessor().GetType();

        auto SetProperty = [&](WStringView sProperty, const WVariant& value) -> WStatus
        {
          WSetObjectPropertyCommand cmd;
          cmd.m_Object = pPropObj->GetGuid();
          cmd.m_sProperty = sProperty;
          cmd.m_NewValue = value;
          return pHistory->AddCommand(cmd);
        };

        for (WStringView sProperty : s_ImportProperties)
        {
          WVariant value;
          if (!source.m_ImportProperties.TryGetValue(sProperty, value))
            continue;

          // a mesh and an animated mesh asset do not have exactly the same properties
          if (pType->FindPropertyByName(sProperty) == nullptr)
            continue;

          W_SUCCEED_OR_RETURN(SetProperty(sProperty, value));
        }

        // what makes this a LOD rather than a copy of the mesh
        W_SUCCEED_OR_RETURN(SetProperty(s_sSimplifyMesh, true));
        W_SUCCEED_OR_RETURN(SetProperty(s_sMeshSimplification, GetLodSimplification(source.m_uiBaseSimplification, uiLod)));
        W_SUCCEED_OR_RETURN(SetProperty("MaxSimplificationError"_wsv, GetLodSimplificationError(uiLod)));

        // The LODs share the mesh's materials rather than importing their own, which would create a
        // second set of material assets for the same model.
        W_SUCCEED_OR_RETURN(SetProperty("ImportMaterials"_wsv, false));

        // a LOD that is being rewritten still holds the slots it had
        for (WInt32 i = pPropObj->GetTypeAccessor().GetCount("Materials"_wsv) - 1; i >= 0; --i)
        {
          const WVariant slotGuid = pPropObj->GetTypeAccessor().GetValue("Materials"_wsv, i);
          if (!slotGuid.IsA<WUuid>())
            continue;

          WRemoveObjectCommand remove;
          remove.m_Object = slotGuid.Get<WUuid>();
          W_SUCCEED_OR_RETURN(pHistory->AddCommand(remove));
        }

        for (WUInt32 i = 0; i < source.m_MaterialSlots.GetCount(); ++i)
        {
          WAddObjectCommand add;
          add.m_Index = (WInt32)i;
          add.m_pType = WGetStaticRTTI<WMaterialResourceSlot>();
          add.m_Parent = pPropObj->GetGuid();
          add.m_sParentProperty = "Materials";
          W_SUCCEED_OR_RETURN(pHistory->AddCommand(add));

          for (auto it : source.m_MaterialSlots[i])
          {
            WSetObjectPropertyCommand cmd;
            cmd.m_Object = add.m_NewObjectGuid;
            cmd.m_sProperty = it.Key();
            cmd.m_NewValue = it.Value();
            W_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));
          }
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
      return WStatus(WFmt("Failed to save LOD asset '{}'.", sPath));
    }

    const WString sSavedPath = pDoc->GetDocumentPath();
    pDoc->GetDocumentManager()->CloseDocument(pDoc);

    // no '%' sign: a literal percent in an WLog format string is consumed as a format spec
    WLog::Success("Created '{}' at {} percent simplification.", sSavedPath, (WUInt32)GetLodSimplification(source.m_uiBaseSimplification, uiLod));
    created.PushBack(sSavedPath);
    ++out_uiCreated;
  }

  // Only once every document is written and closed: notifying the curator makes it look at the
  // folder, which would re-enter this code between two LODs of the same mesh.
  for (const WString& sPath : created)
  {
    WFileSystemModel::GetSingleton()->NotifyOfChange(sPath);
  }

  if (options.m_bOpenAfterCreate)
  {
    for (const WString& sPath : created)
    {
      WQtEditorApp::GetSingleton()->OpenDocumentQueued(sPath);
    }
  }

  return WStatus(W_SUCCESS);
}

WStatus WMeshLodCreator::CreateMeshLodsForAll(WArrayPtr<const WUuid> meshAssetGuids, const WMeshLodOptions& options, WUInt32& out_uiCreated, WUInt32& out_uiSkipped)
{
  out_uiCreated = 0;
  out_uiSkipped = 0;

  // opening the created documents is left to the caller, which may not want that many tabs
  WMeshLodOptions perMesh = options;

  for (const WUuid& meshGuid : meshAssetGuids)
  {
    WMeshLodSource source;
    if (GatherMeshLodSource(meshGuid, source).Failed())
    {
      // not a mesh asset - with a mixed selection this is the normal case, not a problem
      ++out_uiSkipped;
      continue;
    }

    // A mesh that is already a LOD must not get LODs of its own, or the folders nest without end.
    if (WPathUtils::GetFileName(source.m_sMeshAssetPath).StartsWith_NoCase("LOD-"))
    {
      WLog::Info("Skipping '{}': it is itself a LOD.", source.m_sMeshAssetPath);
      ++out_uiSkipped;
      continue;
    }

    if (source.m_bIsPrimitive || source.m_sMeshFile.IsEmpty())
    {
      WLog::Info("Skipping '{}': it has no model file to build LODs from.", source.m_sMeshAssetPath);
      ++out_uiSkipped;
      continue;
    }

    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;

    // "nothing to do here" was handled above, so what is left is a real failure and stops the run
    W_SUCCEED_OR_RETURN(CreateMeshLods(source, perMesh, uiCreated, uiSkipped));

    out_uiCreated += uiCreated;
    out_uiSkipped += uiSkipped;
  }

  return WStatus(W_SUCCESS);
}
