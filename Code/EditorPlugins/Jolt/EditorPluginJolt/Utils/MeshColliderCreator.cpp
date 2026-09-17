#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/Util/MeshColliderUtils.h>
#include <EditorPluginJolt/Utils/MeshColliderCreator.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

namespace
{
  constexpr WStringView s_sTriangleExtension = "WJoltCollisionMeshAsset"_wsv;
  constexpr WStringView s_sConvexExtension = "WJoltConvexCollisionMeshAsset"_wsv;

  /// Read alongside the import properties, but not transferred: the collision mesh asset has no
  /// equivalent, it only tells us whether there is a source file at all.
  constexpr WStringView s_sPrimitiveType = "PrimitiveType"_wsv;

} // namespace

WUuid WMeshColliderSource::GetExisting(WEnum<WMeshColliderKind> kind) const
{
  return (kind == WMeshColliderKind::ConvexHull) ? m_ExistingConvexColMesh : m_ExistingTriangleColMesh;
}

bool WMeshColliderCreator::IsMeshAsset(const WUuid& assetGuid)
{
  return WMeshColliderUtils::IsMeshAsset(assetGuid);
}

WResult WMeshColliderCreator::GatherMeshColliderSource(const WUuid& meshAssetGuid, WMeshColliderSource& out_source)
{
  out_source = WMeshColliderSource();
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

  WHybridArray<WStringView, 24> toRead;
  toRead = WMeshColliderUtils::GetImportPropertyNames();
  toRead.PushBackRange(WMeshColliderUtils::GetSimplificationPropertyNames());
  toRead.PushBack(s_sPrimitiveType);

  // failing to read the properties leaves the source without a mesh file, which the caller reports
  WMeshColliderUtils::ReadMeshProperties(sMeshAssetPath, toRead, out_source.m_ImportProperties).IgnoreResult();

  // a primitive mesh is generated procedurally, there is no model file for the collision mesh asset
  WVariant primitiveType;
  if (out_source.m_ImportProperties.TryGetValue(s_sPrimitiveType, primitiveType))
  {
    out_source.m_bIsPrimitive = primitiveType.ConvertTo<WInt64>() != 0; // 0 is WMeshPrimitive::File
    out_source.m_ImportProperties.Remove(s_sPrimitiveType);
  }

  WVariant meshFile;
  if (!out_source.m_bIsPrimitive && out_source.m_ImportProperties.TryGetValue("MeshFile"_wsv, meshFile) && meshFile.IsA<WString>())
  {
    out_source.m_sMeshFile = meshFile.Get<WString>();
  }

  out_source.m_ExistingTriangleColMesh = WMeshColliderUtils::FindExisting(WCollisionMeshKind::TriangleMesh, out_source.m_sMeshFile, out_source.m_ImportProperties, out_source.m_sMeshAssetPath);
  out_source.m_ExistingConvexColMesh = WMeshColliderUtils::FindExisting(WCollisionMeshKind::ConvexHull, out_source.m_sMeshFile, out_source.m_ImportProperties, out_source.m_sMeshAssetPath);

  return W_SUCCESS;
}

WString WMeshColliderCreator::SuggestColliderPath(const WMeshColliderSource& source, WEnum<WMeshColliderKind> kind, bool bAllowExisting)
{
  const WStringView sExtension = (kind == WMeshColliderKind::ConvexHull) ? s_sConvexExtension : s_sTriangleExtension;

  WStringBuilder sPath = source.m_sMeshAssetPath;
  sPath.ChangeFileExtension(sExtension);

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

WString WMeshColliderCreator::MakeDisplayPath(WStringView sAbsolutePath)
{
  WStringBuilder sPath = sAbsolutePath;
  WQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath);
  return sPath;
}

WResult WMeshColliderCreator::ResolveDisplayPath(WStringView sPath, WStringBuilder& out_sAbsolutePath)
{
  out_sAbsolutePath = sPath;

  if (out_sAbsolutePath.IsEmpty())
    return W_FAILURE;

  // the file is about to be created, so it does not exist yet
  return WQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(out_sAbsolutePath, false) ? W_SUCCESS : W_FAILURE;
}

WStatus WMeshColliderCreator::CreateMeshCollider(const WMeshColliderSource& source, const WMeshColliderOptions& options)
{
  // An empty path means "wherever this collider belongs", which is what creating several at once uses.
  WStringBuilder sColliderPath;
  if (options.m_sColliderPath.IsEmpty())
  {
    sColliderPath = SuggestColliderPath(source, options.m_Kind);
  }
  else if (ResolveDisplayPath(options.m_sColliderPath, sColliderPath).Failed())
  {
    return WStatus(WFmt("'{}' does not name a known data directory.", options.m_sColliderPath));
  }

  if (sColliderPath.IsEmpty())
    return WStatus("No path for the collision mesh asset was given.");

  if (source.m_bIsPrimitive)
    return WStatus("This mesh asset uses a procedural primitive, not a model file, so no collision mesh can be generated from it.");

  if (source.m_sMeshFile.IsEmpty())
    return WStatus("The source file of this mesh asset could not be read, so no collision mesh can be generated from it.");

  const WEnum<WCollisionMeshKind> kind = (options.m_Kind == WMeshColliderKind::ConvexHull) ? WCollisionMeshKind::ConvexHull : WCollisionMeshKind::TriangleMesh;

  WUuid colliderGuid;
  W_SUCCEED_OR_RETURN(WMeshColliderUtils::CreateCollisionMesh(sColliderPath, kind, source.m_ImportProperties, options.m_sSurface, options.m_bOverwriteExisting, colliderGuid));

  if (options.m_bOpenAfterCreate)
  {
    WQtEditorApp::GetSingleton()->OpenDocumentQueued(sColliderPath);
  }

  return WStatus(W_SUCCESS);
}

WStatus WMeshColliderCreator::CreateMeshColliders(WArrayPtr<const WUuid> meshAssetGuids, const WMeshColliderOptions& options, WUInt32& out_uiCreated, WUInt32& out_uiSkipped)
{
  out_uiCreated = 0;
  out_uiSkipped = 0;

  // The path is decided per mesh below, so a path meant for a single collider must not leak in.
  WMeshColliderOptions perMesh = options;
  perMesh.m_sColliderPath.Clear();

  for (const WUuid& meshGuid : meshAssetGuids)
  {
    WMeshColliderSource source;
    if (GatherMeshColliderSource(meshGuid, source).Failed())
    {
      // not a mesh asset - with a mixed selection this is the normal case, not a problem
      ++out_uiSkipped;
      continue;
    }

    // a collider built from the same model file counts wherever it sits, unlike the path check below
    if (!options.m_bOverwriteExisting && source.GetExisting(options.m_Kind).IsValid())
    {
      WLog::Info("Skipping '{}': a collision mesh built from the same model file already exists.", MakeDisplayPath(source.m_sMeshAssetPath));
      ++out_uiSkipped;
      continue;
    }

    if (source.m_bIsPrimitive || source.m_sMeshFile.IsEmpty())
    {
      WLog::Info("Skipping '{}': it has no model file to build a collision mesh from.", MakeDisplayPath(source.m_sMeshAssetPath));
      ++out_uiSkipped;
      continue;
    }

    // SuggestColliderPath() dodges an existing file by appending a number, which is wrong here: a
    // mesh that already has a collider is done, it should not get a second, numbered one.
    WStringBuilder sPath = source.m_sMeshAssetPath;
    sPath.ChangeFileExtension((options.m_Kind == WMeshColliderKind::ConvexHull) ? s_sConvexExtension : s_sTriangleExtension);

    if (!options.m_bOverwriteExisting && WOSFile::ExistsFile(sPath))
    {
      WLog::Info("Skipping '{}': '{}' already exists.", MakeDisplayPath(source.m_sMeshAssetPath), MakeDisplayPath(sPath));
      ++out_uiSkipped;
      continue;
    }

    perMesh.m_sColliderPath = sPath;

    // "nothing to do here" was handled above, so what is left is a real failure and stops the run
    W_SUCCEED_OR_RETURN(CreateMeshCollider(source, perMesh));

    WLog::Success("Created '{}'.", MakeDisplayPath(sPath));
    ++out_uiCreated;
  }

  return WStatus(W_SUCCESS);
}
