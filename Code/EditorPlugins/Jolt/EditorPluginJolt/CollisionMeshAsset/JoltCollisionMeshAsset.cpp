#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAsset.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/AssetInfoFile.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltCollisionMeshAssetDocument, 11, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static WStringView WJoltMeshTypeToString(WJoltMeshDesc::Type type)
{
  switch (type)
  {
    case WJoltMeshDesc::Type::Triangle:
      return "Triangle"_wsv;
    case WJoltMeshDesc::Type::ConvexHull:
      return "ConvexHull"_wsv;
    case WJoltMeshDesc::Type::ConvexDecomposition:
      return "ConvexDecomposition"_wsv;
    case WJoltMeshDesc::Type::ConvexHullGroup:
      return "ConvexHullGroup"_wsv;
  }

  return "Unknown"_wsv;
}

static WMat3 CalculateTransformationMatrix(const WJoltCollisionMeshAssetProperties* pProp)
{
  const float us = WMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  auto rightDir = WMeshImportTransform::GetRightDir(pProp->m_ImportTransform, pProp->m_RightDir);
  auto upDir = WMeshImportTransform::GetUpDir(pProp->m_ImportTransform, pProp->m_UpDir);
  auto flipFwd = WMeshImportTransform::GetFlipForward(pProp->m_ImportTransform, pProp->m_bFlipForwardDir);

  const WBasisAxis::Enum forwardDir = WBasisAxis::GetOrthogonalAxis(rightDir, upDir, !flipFwd);

  return WBasisAxis::CalculateTransformationMatrix(forwardDir, rightDir, upDir, us);
}

WJoltCollisionMeshAssetDocument::WJoltCollisionMeshAssetDocument(WStringView sDocumentPath, bool bConvexMesh)
  : WSimpleAssetDocument<WJoltCollisionMeshAssetProperties>(sDocumentPath, WAssetDocEngineConnection::Simple)
{
  m_bIsConvexMesh = bConvexMesh;
}

void WJoltCollisionMeshAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  // this logic is for backwards compatibility, to sync the convex state with existing data
  if (m_bIsConvexMesh)
  {
    GetPropertyObject()->GetTypeAccessor().SetValue("IsConvexMesh", m_bIsConvexMesh);
  }
  else
  {
    m_bIsConvexMesh = GetPropertyObject()->GetTypeAccessor().GetValue("IsConvexMesh").ConvertTo<bool>();
  }

  // the GetProperties object seems distinct from the GetPropertyObject, so keep them in sync
  GetProperties()->m_bIsConvexMesh = m_bIsConvexMesh;
}


//////////////////////////////////////////////////////////////////////////


WTransformStatus WJoltCollisionMeshAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  WProgressRange range("Transforming Asset", 2, false);

  WJoltCollisionMeshAssetProperties* pProp = GetProperties();

  WJoltMeshDesc meshDesc;

  if (pProp->m_bIsConvexMesh)
  {
    if (pProp->m_ConvexMeshType == WJoltConvexCollisionMeshType::ConvexHullGroup)
    {
      meshDesc.m_Type = WJoltMeshDesc::Type::ConvexHullGroup;
    }
    else if (pProp->m_ConvexMeshType == WJoltConvexCollisionMeshType::ConvexDecomposition)
    {
      meshDesc.m_Type = WJoltMeshDesc::Type::ConvexDecomposition;
      meshDesc.m_uiMaxConvexPieces = pProp->m_uiMaxConvexPieces;
    }
    else
    {
      W_ASSERT_DEV(pProp->m_ConvexMeshType == WJoltConvexCollisionMeshType::ConvexHull || pProp->m_ConvexMeshType == WJoltConvexCollisionMeshType::Cylinder, "Unknown convex mesh type");
      meshDesc.m_Type = WJoltMeshDesc::Type::ConvexHull;
    }
  }

  {
    range.BeginNextStep("Preparing Mesh");

    if (pProp->m_ConvexMeshType == WJoltConvexCollisionMeshType::Cylinder)
    {
      const WMat3 mTransformation = CalculateTransformationMatrix(pProp);

      WGeometry geom;
      WGeometry::GeoOptions opt;
      opt.m_Transform = WMat4(mTransformation, pProp->m_vPositionOffset);

      meshDesc.m_bFlipNormals = WGraphicsUtils::IsTriangleFlipRequired(mTransformation);

      geom.AddCylinderOnePiece(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f, WMath::Clamp<WUInt16>(pProp->m_uiDetail, 3, 32), opt);

      W_SUCCEED_OR_RETURN(CreateMeshFromGeom(geom, meshDesc));
    }
    else
    {
      W_SUCCEED_OR_RETURN(CreateMeshFromFile(meshDesc));
    }

    pProp = GetProperties(); // retrieve again in case they got re-created during mesh creation

    for (const auto& slot : pProp->m_Slots)
    {
      meshDesc.m_Surfaces.PushBack(slot.m_sResource);
    }

    // For triangle meshes: merge sub-meshes that share the same surface to reduce the material count,
    // since Jolt supports at most 32 different materials per triangle mesh.
    if (meshDesc.m_Type == WJoltMeshDesc::Type::Triangle)
    {
      WDynamicArray<WUInt16> oldToNewIndex;
      oldToNewIndex.SetCountUninitialized(meshDesc.m_Surfaces.GetCount());

      WMap<WString, WUInt16> surfaceToNewIndex;
      WDynamicArray<WString> dedupSurfaces;

      for (WUInt32 i = 0; i < meshDesc.m_Surfaces.GetCount(); ++i)
      {
        const WString& sSurface = meshDesc.m_Surfaces[i];

        auto it = surfaceToNewIndex.Find(sSurface);
        if (it.IsValid())
        {
          oldToNewIndex[i] = it.Value();
        }
        else
        {
          const WUInt16 uiNewIdx = static_cast<WUInt16>(dedupSurfaces.GetCount());
          surfaceToNewIndex[sSurface] = uiNewIdx;
          oldToNewIndex[i] = uiNewIdx;
          dedupSurfaces.PushBack(sSurface);
        }
      }

      if (dedupSurfaces.GetCount() < meshDesc.m_Surfaces.GetCount())
      {
        for (WUInt16& surfaceID : meshDesc.m_TriangleSurfaceID)
        {
          if (surfaceID != 0xFFFF)
          {
            surfaceID = oldToNewIndex[surfaceID];
          }
        }

        meshDesc.m_Surfaces = std::move(dedupSurfaces);
      }

      if (meshDesc.m_Surfaces.GetCount() > 32)
      {
        return WTransformStatus(WFmt("Collision mesh uses {} different surfaces. Jolt supports at most 32 per triangle mesh.", meshDesc.m_Surfaces.GetCount()));
      }
    }
  }

  // Surfaces and bounds are properties of the input mesh and do not change during cooking. The vertex
  // and triangle counts are taken from the cooking statistics below instead. \see WJoltCookedMeshStats
  {
    WAssetInfoFile& info = GetTransformInfo();
    info.SetValue(WAssetInfoFile::Keys::NumSurfaces, meshDesc.m_Surfaces.GetCount());
    info.SetValue(WAssetInfoFile::Keys::CollisionMeshType, WJoltMeshTypeToString(meshDesc.m_Type));

    if (!meshDesc.m_Vertices.IsEmpty())
    {
      const WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeFromPoints(meshDesc.m_Vertices.GetData(), meshDesc.m_Vertices.GetCount());

      if (bounds.IsValid())
      {
        info.SetValue(WAssetInfoFile::Keys::BoundsCenter, bounds.m_vCenter);
        info.SetValue(WAssetInfoFile::Keys::BoundsHalfExtents, bounds.m_vBoxHalfExtents);
        info.SetValue(WAssetInfoFile::Keys::BoundsRadius, bounds.m_fSphereRadius);
      }
    }
  }

  // Please check that the code here is in sync with WJoltMeshResourceWriter::WriteMeshResource()
  W_ASSERT_DEV(AssetHeader.GetFileVersion() == 11, "Version change");

  range.BeginNextStep("Writing Result");

  const bool bWriteAssetHeader = false; // already written outside of InternalTransformAsset

  WJoltCookedMeshStats stats;
  W_SUCCEED_OR_RETURN(WJoltMeshResourceWriter::WriteMeshResource(std::move(meshDesc), stream, bWriteAssetHeader, 0, &stats));

  {
    WAssetInfoFile& info = GetTransformInfo();
    info.SetValue(WAssetInfoFile::Keys::NumVertices, stats.m_uiNumVertices);
    info.SetValue(WAssetInfoFile::Keys::NumTriangles, stats.m_uiNumTriangles);

    // Only interesting when the mesh was split into several hulls.
    if (stats.m_uiNumParts > 1)
    {
      info.SetValue(WAssetInfoFile::Keys::NumConvexParts, stats.m_uiNumParts);
    }
  }

  return WStatus(W_SUCCESS);
}

WStatus WJoltCollisionMeshAssetDocument::CreateMeshFromFile(WJoltMeshDesc& outMesh)
{
  WJoltCollisionMeshAssetProperties* pProp = GetProperties();

  WStringBuilder sAbsFilename = pProp->m_sMeshFile;
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return WStatus(WFmt("Couldn't make path absolute: '{0};", sAbsFilename));
  }

  WUniquePtr<WModelImporter2::Importer> pImporter = WModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return WStatus("No known importer for this file type.");

  WMeshResourceDescriptor meshDesc;

  WModelImporter2::ImportOptions opt;
  opt.m_sSourceFile = sAbsFilename;
  opt.m_pMeshOutput = &meshDesc;
  opt.m_RootTransform = CalculateTransformationMatrix(pProp);
  opt.m_vRootPosition = pProp->m_vPositionOffset;

  // include tags
  {
    WTempHybridArray<WStringView, 8> tags;
    pProp->m_sMeshIncludeTags.Split(false, tags, ";");
    for (WStringView tag : tags)
    {
      tag.Trim();
      opt.m_MeshIncludeTags.PushBack(tag);
    }
  }

  // exclude tags
  {
    WTempHybridArray<WStringView, 8> tags;
    pProp->m_sMeshExcludeTags.Split(false, tags, ";");
    for (WStringView tag : tags)
    {
      tag.Trim();
      opt.m_MeshExcludeTags.PushBack(tag);
    }
  }

  if (pProp->m_bSimplifyMesh)
  {
    opt.m_uiMeshSimplification = pProp->m_uiMeshSimplification;
    opt.m_uiMaxSimplificationError = pProp->m_uiMaxSimplificationError;
    opt.m_fNormalWeight = pProp->m_fNormalWeight;
    opt.m_bAggressiveSimplification = pProp->m_bAggressiveSimplification;
  }

  if (pImporter->Import(opt).Failed())
    return WStatus("Model importer was unable to read this asset.");

  WMeshImportUtils::RecordAvailableMeshes(GetTransformInfo(), pImporter.Borrow());

  const auto& meshBuffer = meshDesc.MeshBufferDesc();

  const WUInt32 uiNumTriangles = meshBuffer.GetPrimitiveCount();
  const WUInt32 uiNumVertices = meshBuffer.GetVertexCount();

  if (uiNumTriangles < 3 || uiNumVertices < 3)
    return WStatus("Invalid collision mesh.");

  outMesh.m_TriangleSurfaceID.SetCountUninitialized(uiNumTriangles);

  for (WUInt32 uiTriangle = 0; uiTriangle < uiNumTriangles; ++uiTriangle)
  {
    outMesh.m_TriangleSurfaceID[uiTriangle] = 0; // default value, will be updated below when extracting materials.
  }

  // Extract vertices
  {
    const WVec3* pVertexData = meshBuffer.GetPositionData().GetPtr();

    outMesh.m_Vertices.SetCountUninitialized(uiNumVertices);
    for (WUInt32 v = 0; v < uiNumVertices; ++v)
    {
      outMesh.m_Vertices[v] = pVertexData[v];
    }
  }

  // Extract indices
  {
    if (meshBuffer.Uses32BitIndices())
    {
      WArrayPtr<const WUInt32> indices = WMakeArrayPtr(reinterpret_cast<const WUInt32*>(meshBuffer.GetIndexBufferData().GetPtr()), uiNumTriangles * 3);
      outMesh.m_TriangleIndices = indices;
    }
    else
    {
      outMesh.m_TriangleIndices.SetCountUninitialized(uiNumTriangles * 3);
      const WUInt16* pIndices = reinterpret_cast<const WUInt16*>(meshBuffer.GetIndexBufferData().GetPtr());

      for (WUInt32 tri = 0; tri < uiNumTriangles * 3; ++tri)
      {
        outMesh.m_TriangleIndices[tri] = pIndices[tri];
      }
    }
  }

  const bool bUseSingleMaterial = m_bIsConvexMesh && (pProp->m_ConvexMeshType != WJoltConvexCollisionMeshType::ConvexHullGroup);

  // Extract Material Information
  if (bUseSingleMaterial)
  {
    meshDesc.CollapseSubMeshes();
    pProp->m_Slots.SetCount(1);
    pProp->m_Slots[0].m_sLabel = "Convex";
    pProp->m_Slots[0].m_sResource = pProp->m_sConvexMeshSurface;

    const auto subMeshInfo = meshDesc.GetSubMeshes()[0];

    for (WUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
    {
      outMesh.m_TriangleSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = 0;
    }
  }
  else
  {
    pProp->m_Slots.SetCount(meshDesc.GetSubMeshes().GetCount());

    for (WUInt32 matIdx = 0; matIdx < pImporter->m_OutputMaterials.GetCount(); ++matIdx)
    {
      const WInt32 subMeshIdx = pImporter->m_OutputMaterials[matIdx].m_iReferencedByMesh;
      if (subMeshIdx < 0)
        continue;

      pProp->m_Slots[subMeshIdx].m_sLabel = pImporter->m_OutputMaterials[matIdx].m_sName;

      const auto subMeshInfo = meshDesc.GetSubMeshes()[subMeshIdx];

      if (pProp->m_Slots[subMeshIdx].m_bExclude)
      {
        // update the triangle material information
        for (WUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
        {
          outMesh.m_TriangleSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = 0xFFFF;
        }
      }
      else
      {
        // update the triangle material information
        for (WUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
        {
          outMesh.m_TriangleSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = subMeshIdx;
        }
      }
    }

    ApplyNativePropertyChangesToObjectManager();
  }

  return WStatus(W_SUCCESS);
}

WStatus WJoltCollisionMeshAssetDocument::CreateMeshFromGeom(WGeometry& geom, WJoltMeshDesc& outMesh)
{
  WJoltCollisionMeshAssetProperties* pProp = GetProperties();

  // Material setup.
  {
    // Ensure there is just one slot.
    if (pProp->m_Slots.GetCount() != 1)
    {
      GetObjectAccessor()->StartTransaction("Update Mesh Material Info");

      pProp->m_Slots.SetCount(1);
      pProp->m_Slots[0].m_sLabel = "Default";

      ApplyNativePropertyChangesToObjectManager();
      GetObjectAccessor()->FinishTransaction();

      // Need to reacquire pProp pointer since it might be reallocated.
      pProp = GetProperties();
      W_IGNORE_UNUSED(pProp);
    }
  }

  geom.TriangulatePolygons();

  // copy vertex positions
  {
    outMesh.m_Vertices.SetCountUninitialized(geom.GetVertices().GetCount());
    for (WUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
    {
      outMesh.m_Vertices[v] = geom.GetVertices()[v].m_vPosition;
    }
  }

  // Copy Polygon Data
  {
    outMesh.m_TriangleSurfaceID.SetCountUninitialized(geom.GetPolygons().GetCount());
    outMesh.m_TriangleIndices.Reserve(geom.GetPolygons().GetCount() * 3);

    for (WUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      const auto& poly = geom.GetPolygons()[p];
      W_ASSERT_DEBUG(poly.m_Vertices.GetCount() == 3, "Expected triangulated polygons.");
      outMesh.m_TriangleSurfaceID[p] = 0;

      for (WUInt32 posIdx : poly.m_Vertices)
      {
        outMesh.m_TriangleIndices.PushBack(posIdx);
      }
    }
  }

  return WStatus(W_SUCCESS);
}

WTransformStatus WJoltCollisionMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  WStatus status = WAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

void WJoltCollisionMeshAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (GetProperties()->m_ConvexMeshType != WJoltConvexCollisionMeshType::ConvexHull)
  {
    // remove the mesh file dependency, if it is not actually used
    const auto& sMeshFile = GetProperties()->m_sMeshFile;
    pInfo->m_TransformDependencies.Remove(sMeshFile);
  }
  else
  {
    // For glTF files, add any referenced external buffer files as dependencies
    WMeshImportUtils::AddGltfBufferDependencies(GetProperties()->m_sMeshFile, pInfo->m_TransformDependencies);
  }
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltCollisionMeshAssetDocumentGenerator, 1, WRTTIDefaultAllocator<WJoltCollisionMeshAssetDocumentGenerator>)
W_END_DYNAMIC_REFLECTED_TYPE;

WJoltCollisionMeshAssetDocumentGenerator::WJoltCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

WJoltCollisionMeshAssetDocumentGenerator::~WJoltCollisionMeshAssetDocumentGenerator() = default;

void WJoltCollisionMeshAssetDocumentGenerator::GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "Jolt_Colmesh_Triangle";
    info.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh.svg";
  }
}

WStatus WJoltCollisionMeshAssetDocumentGenerator::Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  const WStringBuilder sOutFile = GetImportTargetPath(sInputFileAbs);

  auto pApp = WQtEditorApp::GetSingleton();

  WStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  WDocument* pDoc = pApp->CreateDocument(sOutFile, WDocumentFlags::None);
  if (pDoc == nullptr)
    return WStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  WJoltCollisionMeshAssetDocument* pAssetDoc = WDynamicCast<WJoltCollisionMeshAssetDocument*>(pDoc);
  if (pAssetDoc == nullptr)
    return WStatus("Target document is not a valid WJoltCollisionMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sInputFileRel.GetView());

  WLog::Success("Imported collision mesh: '{}'", sOutFile);

  return WStatus(W_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltConvexCollisionMeshAssetDocumentGenerator, 1, WRTTIDefaultAllocator<WJoltConvexCollisionMeshAssetDocumentGenerator>)
W_END_DYNAMIC_REFLECTED_TYPE;

WJoltConvexCollisionMeshAssetDocumentGenerator::WJoltConvexCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

WJoltConvexCollisionMeshAssetDocumentGenerator::~WJoltConvexCollisionMeshAssetDocumentGenerator() = default;

void WJoltConvexCollisionMeshAssetDocumentGenerator::GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "Jolt_Colmesh_Convex";
    info.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh_Convex.svg";
  }
}

WStatus WJoltConvexCollisionMeshAssetDocumentGenerator::Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  const WStringBuilder sOutFile = GetImportTargetPath(sInputFileAbs);

  auto pApp = WQtEditorApp::GetSingleton();

  WStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  WDocument* pDoc = pApp->CreateDocument(sOutFile, WDocumentFlags::None);
  if (pDoc == nullptr)
    return WStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  WJoltCollisionMeshAssetDocument* pAssetDoc = WDynamicCast<WJoltCollisionMeshAssetDocument*>(pDoc);
  if (pAssetDoc == nullptr)
    return WStatus("Target document is not a valid WJoltCollisionMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sInputFileRel.GetView());

  WLog::Success("Imported convex collision mesh: '{}'", sOutFile);

  return WStatus(W_SUCCESS);
}
