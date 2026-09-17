#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshAssetDocument, 16, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static WMat3 CalculateTransformationMatrix(const WMeshAssetProperties* pProp)
{
  const float us = WMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  auto rightDir = WMeshImportTransform::GetRightDir(pProp->m_ImportTransform, pProp->m_RightDir);
  auto upDir = WMeshImportTransform::GetUpDir(pProp->m_ImportTransform, pProp->m_UpDir);
  auto flipFwd = WMeshImportTransform::GetFlipForward(pProp->m_ImportTransform, pProp->m_bFlipForwardDir);

  const WBasisAxis::Enum forwardDir = WBasisAxis::GetOrthogonalAxis(rightDir, upDir, !flipFwd);

  return WBasisAxis::CalculateTransformationMatrix(forwardDir, rightDir, upDir, us);
}

WMeshAssetDocument::WMeshAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WMeshAssetProperties>(sDocumentPath, WAssetDocEngineConnection::Simple, true)
{
}

WTransformStatus WMeshAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  WProgressRange range("Transforming Asset", 2, false);

  WMeshAssetProperties* pProp = GetProperties();

  WMeshResourceDescriptor desc;

  range.SetStepWeighting(0, 0.9f);
  range.BeginNextStep("Importing Mesh");

  if (pProp->m_PrimitiveType == WMeshPrimitive::File)
  {
    W_SUCCEED_OR_RETURN(CreateMeshFromFile(pProp, desc, !transformFlags.IsSet(WTransformFlags::BackgroundProcessing)));
  }
  else
  {
    CreateMeshFromGeom(pProp, desc);
  }

  // if there is no material set for a slot, use the "Pattern" material as a fallback
  for (WUInt32 matIdx = 0; matIdx < desc.GetMaterials().GetCount(); ++matIdx)
  {
    if (desc.GetMaterials()[matIdx].m_sPath.IsEmpty())
    {
      // Data/Base/Materials/Common/Pattern.WMaterialAsset
      desc.SetMaterial(matIdx, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");
    }
  }

  range.BeginNextStep("Writing Result");
  desc.Save(stream);

  WMeshImportUtils::RecordMeshTransformInfo(GetTransformInfo(), desc);

  return WStatus(W_SUCCESS);
}

void WMeshAssetDocument::CreateMeshFromGeom(WMeshAssetProperties* pProp, WMeshResourceDescriptor& desc)
{
  const WMat3 mTransformation = CalculateTransformationMatrix(pProp);

  WGeometry geom;
  // const WMat4 mTrans(mTransformation, WVec3::MakeZero());

  WGeometry::GeoOptions opt;
  opt.m_Transform = WMat4(mTransformation, pProp->m_vPositionOffset);

  auto detail1 = pProp->m_uiDetail;
  auto detail2 = pProp->m_uiDetail2;

  if (pProp->m_PrimitiveType == WMeshPrimitive::Box)
  {
    geom.AddBox(WVec3(1.0f), true, opt);
  }
  else if (pProp->m_PrimitiveType == WMeshPrimitive::Capsule)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 16;

    geom.AddCapsule(pProp->m_fRadius, WMath::Max(0.0f, pProp->m_fHeight), WMath::Max<WUInt16>(3, detail1), WMath::Max<WUInt16>(1, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == WMeshPrimitive::Cone)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;

    geom.AddCone(pProp->m_fRadius, pProp->m_fHeight, pProp->m_bCap, WMath::Max<WUInt16>(3, detail1), opt);
  }
  else if (pProp->m_PrimitiveType == WMeshPrimitive::Cylinder)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;

    geom.AddCylinder(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f, pProp->m_bCap, pProp->m_bCap2, WMath::Max<WUInt16>(3, detail1), opt, WMath::Clamp(pProp->m_Angle, WAngle::MakeFromDegree(0.0f), WAngle::MakeFromDegree(360.0f)));
  }
  else if (pProp->m_PrimitiveType == WMeshPrimitive::GeodesicSphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 2;

    geom.AddGeodesicSphere(pProp->m_fRadius, WMath::Clamp<WUInt16>(detail1, 0, 6), opt);
  }
  else if (pProp->m_PrimitiveType == WMeshPrimitive::HalfSphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 16;

    geom.AddHalfSphere(pProp->m_fRadius, WMath::Max<WUInt16>(3, detail1), WMath::Max<WUInt16>(1, detail2), pProp->m_bCap, opt);
  }
  else if (pProp->m_PrimitiveType == WMeshPrimitive::Pyramid)
  {
    geom.AddPyramid(1.0f, 1.0f, pProp->m_bCap, opt);
  }
  else if (pProp->m_PrimitiveType == WMeshPrimitive::Rect)
  {
    opt.m_Transform.Element(2, 0) = -opt.m_Transform.Element(2, 0);
    opt.m_Transform.Element(2, 1) = -opt.m_Transform.Element(2, 1);
    opt.m_Transform.Element(2, 2) = -opt.m_Transform.Element(2, 2);

    geom.AddRect(WVec2(1.0f), WMath::Max<WUInt16>(1, detail1), WMath::Max<WUInt16>(1, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == WMeshPrimitive::Sphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 32;

    geom.AddStackedSphere(pProp->m_fRadius, WMath::Max<WUInt16>(3, detail1), WMath::Max<WUInt16>(2, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == WMeshPrimitive::Torus)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 32;

    float r1 = pProp->m_fRadius;
    float r2 = pProp->m_fRadius2;

    if (r1 == r2)
      r1 = r2 * 0.5f;

    geom.AddTorus(r1, WMath::Max(r1 + 0.01f, r2), WMath::Max<WUInt16>(3, detail1), WMath::Max<WUInt16>(3, detail2), true, opt);
  }

  geom.TriangulatePolygons(4);
  geom.ComputeTangents();

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
    }

    // Set material for mesh.
    if (!pProp->m_Slots.IsEmpty())
      desc.SetMaterial(0, pProp->m_Slots[0].m_sResource);
    else
      desc.SetMaterial(0, "");
  }

  // the the procedurally generated geometry we can always use fixed, low precision data, because we know that the geometry isn't detailed enough to run into problems
  // and then we can unclutter the UI a little by not showing those options at all
  auto& mbd = desc.MeshBufferDesc();
  mbd.AddCommonStreams();

  mbd.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);
  desc.AddSubMesh(mbd.GetPrimitiveCount(), 0, 0);
}

WTransformStatus WMeshAssetDocument::CreateMeshFromFile(WMeshAssetProperties* pProp, WMeshResourceDescriptor& desc, bool bAllowMaterialImport)
{
  WProgressRange range("Mesh Import", 5, false);

  range.SetStepWeighting(0, 0.7f);
  range.BeginNextStep("Importing Mesh Data");

  WStringBuilder sAbsFilename = pProp->m_sMeshFile;
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return WStatus(WFmt("Couldn't make path absolute: '{0};", sAbsFilename));
  }

  WUniquePtr<WModelImporter2::Importer> pImporter = WModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return WStatus("No known importer for this file type.");

  WModelImporter2::ImportOptions opt;
  opt.m_sSourceFile = sAbsFilename;
  opt.m_bRecomputeNormals = pProp->m_bRecalculateNormals;
  opt.m_bRecomputeTangents = pProp->m_bRecalculateTangents;
  opt.m_bHighPrecision = pProp->m_bHighPrecision;
  opt.m_MeshVertexColorConversion = pProp->m_VertexColorConversion;
  opt.m_RootTransform = CalculateTransformationMatrix(pProp);
  opt.m_vRootPosition = pProp->m_vPositionOffset;
  opt.m_pMeshOutput = &desc;

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

  if (desc.GetSubMeshes().IsEmpty() || !desc.GetBounds().IsValid())
    return WStatus("Imported mesh is empty.");

  for (auto& sm : desc.GetSubMeshes())
  {
    if (sm.m_uiPrimitiveCount == 0)
    {
      return WStatus("Imported mesh is empty.");
    }
  }

  range.BeginNextStep("Importing Materials");

  // correct the number of material slots
  bool bSlotCountMissmatch = pProp->m_Slots.GetCount() != desc.GetSubMeshes().GetCount();
  if (pProp->m_bImportMaterials || bSlotCountMissmatch)
  {
    if (!bAllowMaterialImport && bSlotCountMissmatch)
    {
      return WTransformStatus(WTransformResult::NeedsImport);
    }

    GetObjectAccessor()->StartTransaction("Update Mesh Materials");

    WMeshImportUtils::SetMeshAssetMaterialSlots(pProp->m_Slots, pImporter.Borrow());

    if (pProp->m_bImportMaterials)
    {
      WMeshImportUtils::ImportMeshAssetMaterials(pProp->m_Slots, GetDocumentPath(), pImporter.Borrow());
    }

    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }

  WMeshImportUtils::CopyMeshAssetMaterialSlotToResource(desc, pProp->m_Slots);

  return WStatus(W_SUCCESS);
}

WTransformStatus WMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  WStatus status = WAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

void WMeshAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (GetProperties()->m_PrimitiveType != WMeshPrimitive::File)
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
