#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/Util/MeshColliderUtils.h>
#include <EditorPluginAssets/Util/MeshLodCreator.h>
#include <EditorPluginScene/Utils/MeshPrefabCreator.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/AssetInfoFile.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

namespace
{
  /// Equivalent of WSimpleAssetDocument::GetPropertyObject(), which can't be called without knowing
  /// the concrete asset type.
  const WDocumentObject* GetTopLevelObject(const WDocument* pDoc)
  {
    const WDocumentObject* pRoot = pDoc->GetObjectManager()->GetRootObject();
    if (pRoot == nullptr || pRoot->GetChildren().GetCount() != 1)
      return nullptr;

    return pRoot->GetChildren()[0];
  }

  /// Reads a property from an asset document without knowing its C++ type.
  ///
  /// Only closes the document again if it had to be opened here and nothing has claimed a window for
  /// it, so that a document someone else is working with is left alone.
  WVariant ReadAssetProperty(WStringView sAbsDocumentPath, WStringView sProperty)
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
      return {};

    WVariant res;
    if (const WDocumentObject* pPropObj = GetTopLevelObject(pDoc))
    {
      res = pPropObj->GetTypeAccessor().GetValue(sProperty);
    }

    if (!bWasOpen && !pDoc->HasWindowBeenRequested())
    {
      pDoc->GetDocumentManager()->CloseDocument(pDoc);
    }

    return res;
  }

  /// An asset reference is the guid in braces, which is how WUuid already formats itself.
  WString FormatResourceRef(const WUuid& guid)
  {
    WStringBuilder s;
    s.SetFormat("{}", guid);
    return s;
  }

  /// Returns an invalid uuid if the type is unknown, i.e. its plugin is not loaded.
  WUuid AddComponent(WCommandHistory* pHistory, const WUuid& parentObject, WStringView sType)
  {
    if (WRTTI::FindTypeByName(sType) == nullptr)
      return {};

    WAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType(sType);
    cmd.m_Parent = parentObject;
    cmd.m_sParentProperty = "Components";

    if (pHistory->AddCommand(cmd).Failed())
      return {};

    return cmd.m_NewObjectGuid;
  }

  WStatus SetProperty(WCommandHistory* pHistory, const WUuid& object, WStringView sProperty, const WVariant& value)
  {
    WSetObjectPropertyCommand cmd;
    cmd.m_Object = object;
    cmd.m_sProperty = sProperty;
    cmd.m_NewValue = value;
    return pHistory->AddCommand(cmd);
  }
} // namespace

WStringView WMeshPrefabSource::GetDefaultRenderComponentType() const
{
  if (m_bAnimated)
    return m_LodGuids.IsEmpty() ? "WAnimatedMeshComponent"_wsv : "WLodAnimatedMeshComponent"_wsv;

  return m_LodGuids.IsEmpty() ? "WMeshComponent"_wsv : "WLodMeshComponent"_wsv;
}

bool WMeshPrefabCreator::IsPhysicsAvailable()
{
  return WRTTI::FindTypeByName("WJoltStaticActorComponent") != nullptr;
}

bool WMeshPrefabCreator::IsMeshAsset(const WUuid& assetGuid)
{
  return WMeshColliderUtils::IsMeshAsset(assetGuid);
}

namespace
{
  /// Collects LOD-1..N from one folder. Stops at the first gap, as LODs form a contiguous run.
  void CollectLodsFromFolder(WStringView sFolder, WDynamicArray<WUuid>& out_lodGuids)
  {
    for (WUInt32 uiLod = 1; uiLod <= WMeshLodCreator::s_uiMaxLods; ++uiLod)
    {
      WStringBuilder sLodName;
      sLodName.SetFormat("LOD-{}.WMeshAsset", uiLod);

      WStringBuilder sLodPath = sFolder;
      sLodPath.AppendPath(sLodName);

      auto pLod = WAssetCurator::GetSingleton()->FindSubAsset(sLodPath);
      if (!pLod.isValid())
        return;

      out_lodGuids.PushBack(pLod->m_Data.m_Guid);
    }
  }

  /// Looks in the same folders that WMeshLodCreator writes to, so that LODs it created are picked up.
  ///
  /// sMeshIncludeTags has to be passed for the same reason the creator needs it: without it a mesh
  /// that is one sub-object of a shared model file would find the LODs of a sibling sub-object.
  void FindLodSiblings(WStringView sMeshAssetPath, WStringView sMeshFile, WStringView sMeshIncludeTags, WDynamicArray<WUuid>& out_lodGuids)
  {
    WHybridArray<WString, 2> folders;
    WMeshLodCreator::GetLodFolderCandidates(sMeshAssetPath, sMeshFile, sMeshIncludeTags, folders);

    for (const WString& sFolder : folders)
    {
      CollectLodsFromFolder(sFolder, out_lodGuids);

      if (!out_lodGuids.IsEmpty())
        return;
    }
  }
} // namespace

WResult WMeshPrefabCreator::GatherMeshPrefabSource(const WUuid& meshAssetGuid, WMeshPrefabSource& out_source)
{
  out_source = WMeshPrefabSource();
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

    // bounds are only available once the asset has been transformed at least once
    if (const WAssetInfoFile* pInfo = pAssetInfo->GetTransformInfo())
    {
      const WVariant center = pInfo->GetValue(WAssetInfoFile::Keys::BoundsCenter);
      const WVariant extents = pInfo->GetValue(WAssetInfoFile::Keys::BoundsHalfExtents);
      const WVariant radius = pInfo->GetValue(WAssetInfoFile::Keys::BoundsRadius);

      if (center.IsA<WVec3>() && extents.IsA<WVec3>())
      {
        out_source.m_vBoundsCenter = center.Get<WVec3>();
        out_source.m_vBoundsHalfExtents = extents.Get<WVec3>();
        out_source.m_fBoundsRadius = radius.IsValid() ? radius.ConvertTo<float>() : out_source.m_vBoundsHalfExtents.GetLength();
        out_source.m_bHasBounds = true;
      }
    }
  }

  const WVariant meshFile = ReadAssetProperty(sMeshAssetPath, "MeshFile");
  if (meshFile.IsA<WString>())
  {
    out_source.m_sMeshFile = meshFile.Get<WString>();
  }

  WVariantDictionary subMeshProperties;
  WMeshColliderUtils::ReadMeshProperties(sMeshAssetPath, WMeshColliderUtils::GetSubMeshPropertyNames(), subMeshProperties).IgnoreResult();

  WVariant includeTags;
  subMeshProperties.TryGetValue("MeshIncludeTags", includeTags);

  FindLodSiblings(sMeshAssetPath, out_source.m_sMeshFile, includeTags.IsA<WString>() ? includeTags.Get<WString>().GetView() : WStringView(), out_source.m_LodGuids);

  out_source.m_ExistingTriangleColMesh = WMeshColliderUtils::FindExisting(WCollisionMeshKind::TriangleMesh, out_source.m_sMeshFile, subMeshProperties, out_source.m_sMeshAssetPath);
  out_source.m_ExistingConvexColMesh = WMeshColliderUtils::FindExisting(WCollisionMeshKind::ConvexHull, out_source.m_sMeshFile, subMeshProperties, out_source.m_sMeshAssetPath);

  return W_SUCCESS;
}

WEnum<WMeshPrefabPhysics> WMeshPrefabSource::GetDefaultPhysics() const
{
  // a triangle mesh is only usable for static bodies, so its existence is the more specific signal
  if (m_ExistingTriangleColMesh.IsValid())
    return WMeshPrefabPhysics::StaticTriangleMesh;

  // a convex hull works for both; dynamic additionally needs mass and material set up sensibly
  if (m_ExistingConvexColMesh.IsValid())
    return WMeshPrefabPhysics::StaticConvexHull;

  return WMeshPrefabPhysics::None;
}

namespace
{
  /// Creates a collision mesh asset next to the mesh asset, or reuses a matching existing one.
  /// \see WMeshColliderUtils
  WStatus GetOrCreateCollisionMesh(const WMeshPrefabSource& source, bool bConvex, WUuid& out_guid)
  {
    const WEnum<WCollisionMeshKind> kind = bConvex ? WCollisionMeshKind::ConvexHull : WCollisionMeshKind::TriangleMesh;

    if (source.m_sMeshFile.IsEmpty())
      return WStatus("The mesh asset has no source file, so no collision mesh can be generated from it.");

    WHybridArray<WStringView, 24> toRead;
    toRead = WMeshColliderUtils::GetImportPropertyNames();
    toRead.PushBackRange(WMeshColliderUtils::GetSimplificationPropertyNames());

    WVariantDictionary importProperties;
    WMeshColliderUtils::ReadMeshProperties(source.m_sMeshAssetPath, toRead, importProperties).IgnoreResult();

    // A matching collider counts wherever it sits, so this finds more than the path check below.
    out_guid = WMeshColliderUtils::FindExisting(kind, source.m_sMeshFile, importProperties, source.m_sMeshAssetPath);
    if (out_guid.IsValid())
      return WStatus(W_SUCCESS);

    WStringBuilder sColMeshPath = source.m_sMeshAssetPath;
    sColMeshPath.ChangeFileExtension(WMeshColliderUtils::GetExtension(kind));

    if (WOSFile::ExistsFile(sColMeshPath))
    {
      auto pExisting = WAssetCurator::GetSingleton()->FindSubAsset(sColMeshPath);
      if (pExisting.isValid())
      {
        out_guid = pExisting->m_Data.m_Guid;
        return WStatus(W_SUCCESS);
      }

      return WStatus(WFmt("'{}' already exists but is not a collision mesh asset.", sColMeshPath));
    }

    // the mesh file is what the collider is built from
    if (!importProperties.Contains("MeshFile"_wsv))
    {
      importProperties.Insert("MeshFile"_wsv, WVariant(source.m_sMeshFile));
    }

    // no surface here: a prefab's collider gets whatever the shape component specifies
    return WMeshColliderUtils::CreateCollisionMesh(sColMeshPath, kind, importProperties, {}, false, out_guid);
  }
} // namespace

WString WMeshPrefabCreator::SuggestPrefabPath(const WMeshPrefabSource& source, bool bAllowExisting)
{
  WStringBuilder sPath = source.m_sMeshAssetPath;
  sPath.ChangeFileExtension("WPrefab");

  if (bAllowExisting || !WOSFile::ExistsFile(sPath))
    return sPath;

  const WString sBaseName = WPathUtils::GetFileName(sPath);

  for (WUInt32 i = 2; i < 100; ++i)
  {
    WStringBuilder sCandidateName;
    sCandidateName.SetFormat("{}{}", sBaseName, i);

    WStringBuilder sCandidate = sPath;
    sCandidate.ChangeFileName(sCandidateName);

    if (!WOSFile::ExistsFile(sCandidate))
      return sCandidate;
  }

  return sPath;
}

WString WMeshPrefabCreator::MakeDisplayPath(WStringView sAbsolutePath)
{
  WStringBuilder sPath = sAbsolutePath;
  WQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath);
  return sPath;
}

WResult WMeshPrefabCreator::ResolveDisplayPath(WStringView sPath, WStringBuilder& out_sAbsolutePath)
{
  out_sAbsolutePath = sPath;

  if (out_sAbsolutePath.IsEmpty())
    return W_FAILURE;

  // the file is about to be created, so it does not exist yet
  return WQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(out_sAbsolutePath, false) ? W_SUCCESS : W_FAILURE;
}

WStatus WMeshPrefabCreator::CreateMeshPrefab(const WMeshPrefabSource& source, const WMeshPrefabOptions& options)
{
  // An empty path means "wherever this prefab belongs", which is what creating several at once uses.
  WStringBuilder sPrefabPath;
  if (options.m_sPrefabPath.IsEmpty())
  {
    sPrefabPath = SuggestPrefabPath(source);
  }
  else if (ResolveDisplayPath(options.m_sPrefabPath, sPrefabPath).Failed())
  {
    return WStatus(WFmt("'{}' does not name a known data directory.", options.m_sPrefabPath));
  }

  if (sPrefabPath.IsEmpty())
    return WStatus("No prefab path was given.");

  const bool bExists = WOSFile::ExistsFile(sPrefabPath);

  // CreateDocument reports an already open document through a modal message box, which would hang an
  // automated caller. Refuse here instead.
  if (bExists && !options.m_bOverwriteExisting)
  {
    return WStatus(WFmt("'{}' already exists. Delete it first, or choose a different name.", sPrefabPath));
  }

  const bool bWantsPhysics = options.m_Physics != WMeshPrefabPhysics::None;
  const bool bConvex = options.m_Physics == WMeshPrefabPhysics::StaticConvexHull || options.m_Physics == WMeshPrefabPhysics::DynamicConvexHull;
  const bool bDynamic = options.m_Physics == WMeshPrefabPhysics::DynamicConvexHull || options.m_Physics == WMeshPrefabPhysics::DynamicBox;
  const bool bBoxShape = options.m_Physics == WMeshPrefabPhysics::StaticBox || options.m_Physics == WMeshPrefabPhysics::DynamicBox;

  if (bWantsPhysics && !IsPhysicsAvailable())
    return WStatus("Physics components are not available. Enable the Jolt plugin in the project settings.");

  if (bBoxShape && !source.m_bHasBounds)
    return WStatus("The mesh bounds are unknown. Transform the mesh asset first, or use a collision mesh instead of a box.");

  // first, so that a failure here doesn't leave a half-built prefab behind
  WUuid colMeshGuid;
  if (bWantsPhysics && !bBoxShape)
  {
    W_SUCCEED_OR_RETURN(GetOrCreateCollisionMesh(source, bConvex, colMeshGuid));
  }

  // An existing prefab is rewritten in place rather than deleted and created again, so that it keeps
  // its guid and anything referencing it keeps working.
  WDocument* pDoc = bExists ? WQtEditorApp::GetSingleton()->OpenDocument(sPrefabPath, WDocumentFlags::None)
                             : WQtEditorApp::GetSingleton()->CreateDocument(sPrefabPath, WDocumentFlags::None);

  if (pDoc == nullptr)
    return WStatus(WFmt("Failed to {} prefab document '{}'.", bExists ? "open" : "create", sPrefabPath));

  WStatus result = WStatus(W_SUCCESS);

  {
    auto pHistory = pDoc->GetCommandHistory();
    pHistory->StartTransaction("Create Prefab from Mesh");

    // in a lambda, so that every failure path below cancels the transaction
    auto BuildPrefab = [&]() -> WStatus
    {
      // A new prefab is not empty: the document manager clones a template that provides the root
      // object. A second one would make the prefab invalid.
      WUuid rootObject;
      for (const WDocumentObject* pChild : pDoc->GetObjectManager()->GetRootObject()->GetChildren())
      {
        if (pChild->GetParentProperty() == "Children"_wsv)
        {
          rootObject = pChild->GetGuid();
          break;
        }
      }

      if (!rootObject.IsValid())
      {
        WAddObjectCommand cmd;
        cmd.m_Index = -1;
        cmd.SetType("WGameObject");
        cmd.m_sParentProperty = "Children";
        W_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));
        rootObject = cmd.m_NewObjectGuid;
      }

      // this exact name is what marks the object as the prefab root
      W_SUCCEED_OR_RETURN(SetProperty(pHistory, rootObject, "Name", "<Prefab-Root>"));

      // a prefab that is being rewritten still holds the components and children it had
      if (const WDocumentObject* pRoot = pDoc->GetObjectManager()->GetObject(rootObject))
      {
        WHybridArray<WUuid, 16> toRemove;
        for (const WDocumentObject* pChild : pRoot->GetChildren())
        {
          toRemove.PushBack(pChild->GetGuid());
        }

        for (const WUuid& guid : toRemove)
        {
          WRemoveObjectCommand remove;
          remove.m_Object = guid;
          W_SUCCEED_OR_RETURN(pHistory->AddCommand(remove));
        }
      }

      {
        const WString sRenderType = options.m_sRenderComponentType.IsEmpty() ? WString(source.GetDefaultRenderComponentType()) : options.m_sRenderComponentType;

        const WUuid renderComponent = AddComponent(pHistory, rootObject, sRenderType);
        if (!renderComponent.IsValid())
          return WStatus(WFmt("Failed to add component '{}'.", sRenderType));

        // The two LOD components are separate types with identical properties, one skinned and one not.
        const bool bLodComponent = (sRenderType == "WLodMeshComponent") || (sRenderType == "WLodAnimatedMeshComponent");

        if (bLodComponent)
        {
          // LOD 0 is the mesh asset itself, the gathered guids continue from LOD 1
          WHybridArray<WUuid, 8> allLods;
          allLods.PushBack(source.m_MeshAssetGuid);
          allLods.PushBackRange(source.m_LodGuids);

          // Screen coverage fractions, not distances: the coverage below which the component switches
          // away from that LOD. Even a nearby model covers little of the screen, hence the small
          // values. The last LOD gets 0, so that it is used out to the horizon.
          const float fThresholds[] = {0.2f, 0.1f, 0.05f, 0.02f};

          for (WUInt32 i = 0; i < allLods.GetCount(); ++i)
          {
            WAddObjectCommand cmd;
            cmd.m_Index = (WInt32)i;
            cmd.SetType((sRenderType == "WLodAnimatedMeshComponent") ? "WLodAnimatedMeshLod" : "WLodMeshLod");
            cmd.m_Parent = renderComponent;
            cmd.m_sParentProperty = "Meshes";
            W_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));

            // halving past the end of the table keeps the values strictly decreasing, which the
            // component needs to switch between LODs
            const bool bLastLod = (i + 1 == allLods.GetCount());

            float fThreshold = 0.0f;
            if (!bLastLod)
            {
              fThreshold = fThresholds[W_ARRAY_SIZE(fThresholds) - 1];

              for (WUInt32 uiExtra = W_ARRAY_SIZE(fThresholds); uiExtra <= i; ++uiExtra)
              {
                fThreshold *= 0.5f;
              }

              if (i < W_ARRAY_SIZE(fThresholds))
              {
                fThreshold = fThresholds[i];
              }
            }

            W_SUCCEED_OR_RETURN(SetProperty(pHistory, cmd.m_NewObjectGuid, "Mesh", FormatResourceRef(allLods[i])));
            W_SUCCEED_OR_RETURN(SetProperty(pHistory, cmd.m_NewObjectGuid, "Threshold", fThreshold));
          }

          // the component culls by these rather than deriving them from the meshes
          if (source.m_bHasBounds)
          {
            W_SUCCEED_OR_RETURN(SetProperty(pHistory, renderComponent, "BoundsOffset", source.m_vBoundsCenter));
            W_SUCCEED_OR_RETURN(SetProperty(pHistory, renderComponent, "BoundsRadius", WMath::Clamp(source.m_fBoundsRadius, 0.01f, 100.0f)));
          }
        }
        else
        {
          W_SUCCEED_OR_RETURN(SetProperty(pHistory, renderComponent, "Mesh", FormatResourceRef(source.m_MeshAssetGuid)));
        }
      }

      if (bWantsPhysics)
      {
        const WStringView sActorType = bDynamic ? "WJoltDynamicActorComponent"_wsv : "WJoltStaticActorComponent"_wsv;

        const WUuid actorComponent = AddComponent(pHistory, rootObject, sActorType);
        if (!actorComponent.IsValid())
          return WStatus(WFmt("Failed to add component '{}'.", sActorType));

        W_SUCCEED_OR_RETURN(SetProperty(pHistory, actorComponent, "CollisionLayer", options.m_uiCollisionLayer));

        if (!options.m_sSurfaceAsset.IsEmpty())
        {
          W_SUCCEED_OR_RETURN(SetProperty(pHistory, actorComponent, "Surface", options.m_sSurfaceAsset));
        }

        if (bBoxShape)
        {
          // a child object, so that the box can be offset to the mesh bounds centre
          WUuid shapeObject;
          {
            WAddObjectCommand cmd;
            cmd.m_Index = -1;
            cmd.SetType("WGameObject");
            cmd.m_Parent = rootObject;
            cmd.m_sParentProperty = "Children";
            W_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));
            shapeObject = cmd.m_NewObjectGuid;
          }

          W_SUCCEED_OR_RETURN(SetProperty(pHistory, shapeObject, "Name", "Collider"));
          W_SUCCEED_OR_RETURN(SetProperty(pHistory, shapeObject, "LocalPosition", source.m_vBoundsCenter));

          const WUuid shapeComponent = AddComponent(pHistory, shapeObject, "WJoltShapeBoxComponent");
          if (!shapeComponent.IsValid())
            return WStatus("Failed to add component 'WJoltShapeBoxComponent'.");

          W_SUCCEED_OR_RETURN(SetProperty(pHistory, shapeComponent, "HalfExtents", source.m_vBoundsHalfExtents));
        }
        else if (bConvex)
        {
          const WUuid shapeComponent = AddComponent(pHistory, rootObject, "WJoltShapeConvexHullComponent");
          if (!shapeComponent.IsValid())
            return WStatus("Failed to add component 'WJoltShapeConvexHullComponent'.");

          W_SUCCEED_OR_RETURN(SetProperty(pHistory, shapeComponent, "CollisionMesh", FormatResourceRef(colMeshGuid)));
        }
        else
        {
          // a triangle mesh is referenced by the actor directly, no shape component involved
          W_SUCCEED_OR_RETURN(SetProperty(pHistory, actorComponent, "CollisionMesh", FormatResourceRef(colMeshGuid)));
        }
      }

      return WStatus(W_SUCCESS);
    };

    result = BuildPrefab();

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
    return WStatus(WFmt("Failed to save prefab '{}'.", sPrefabPath));
  }

  const WString sPath = pDoc->GetDocumentPath();
  pDoc->GetDocumentManager()->CloseDocument(pDoc);

  WFileSystemModel::GetSingleton()->NotifyOfChange(sPath);

  if (options.m_bOpenAfterCreate)
  {
    WQtEditorApp::GetSingleton()->OpenDocumentQueued(sPath);
  }

  return WStatus(W_SUCCESS);
}

WStatus WMeshPrefabCreator::CreateMeshPrefabs(WArrayPtr<const WUuid> meshAssetGuids, const WMeshPrefabOptions& options, WUInt32& out_uiCreated, WUInt32& out_uiSkipped)
{
  out_uiCreated = 0;
  out_uiSkipped = 0;

  const bool bWantsPhysics = options.m_Physics != WMeshPrefabPhysics::None;
  const bool bBoxShape = options.m_Physics == WMeshPrefabPhysics::StaticBox || options.m_Physics == WMeshPrefabPhysics::DynamicBox;

  if (bWantsPhysics && !IsPhysicsAvailable())
    return WStatus("Physics components are not available. Enable the Jolt plugin in the project settings.");

  for (const WUuid& meshGuid : meshAssetGuids)
  {
    WMeshPrefabSource source;
    if (GatherMeshPrefabSource(meshGuid, source).Failed())
    {
      // not a mesh asset - with a mixed selection this is the normal case, not a problem
      ++out_uiSkipped;
      continue;
    }

    // SuggestPrefabPath() dodges an existing file by appending a number, which is wrong here: a mesh
    // that already has a prefab is done, it should not get a second, numbered one.
    WStringBuilder sPath = source.m_sMeshAssetPath;
    sPath.ChangeFileExtension("WPrefab");

    if (!options.m_bOverwriteExisting && WOSFile::ExistsFile(sPath))
    {
      WLog::Info("Skipping '{}': '{}' already exists.", MakeDisplayPath(source.m_sMeshAssetPath), MakeDisplayPath(sPath));
      ++out_uiSkipped;
      continue;
    }

    if (bBoxShape && !source.m_bHasBounds)
    {
      WLog::Info("Skipping '{}': its bounds are unknown, so no box collider can be sized. Transform the mesh asset first.", MakeDisplayPath(source.m_sMeshAssetPath));
      ++out_uiSkipped;
      continue;
    }

    if (bWantsPhysics && !bBoxShape && source.m_sMeshFile.IsEmpty())
    {
      WLog::Info("Skipping '{}': it has no model file to build a collision mesh from.", MakeDisplayPath(source.m_sMeshAssetPath));
      ++out_uiSkipped;
      continue;
    }

    WMeshPrefabOptions perMesh = options;

    // the path was just checked to be free, so it is passed on explicitly
    perMesh.m_sPrefabPath = sPath;

    // a selection can mix animated and static meshes, so the component type is decided per mesh
    perMesh.m_sRenderComponentType.Clear();

    // "nothing to do here" was handled above, so what is left is a real failure and stops the run
    W_SUCCEED_OR_RETURN(CreateMeshPrefab(source, perMesh));

    WLog::Success("Created '{}'.", MakeDisplayPath(sPath));
    ++out_uiCreated;
  }

  return WStatus(W_SUCCESS);
}
