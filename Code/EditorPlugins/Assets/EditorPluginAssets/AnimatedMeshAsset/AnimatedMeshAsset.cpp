#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimatedMeshAssetDocument, 11, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAnimatedMeshAssetDocument::WAnimatedMeshAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WAnimatedMeshAssetProperties>(sDocumentPath, WAssetDocEngineConnection::Simple, true)
{
}

WTransformStatus WAnimatedMeshAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  WProgressRange range("Transforming Asset", 2, false);

  WAnimatedMeshAssetProperties* pProp = GetProperties();

  if (pProp->m_sDefaultSkeleton.IsEmpty())
  {
    return WStatus("Animated mesh doesn't have a default skeleton assigned.");
  }

  WMeshResourceDescriptor desc;

  range.SetStepWeighting(0, 0.9f);
  range.BeginNextStep("Importing Mesh");

  W_SUCCEED_OR_RETURN(CreateMeshFromFile(pProp, desc));

  // the properties object can get invalidated by the CreateMeshFromFile() call
  pProp = GetProperties();

  range.BeginNextStep("Writing Result");

  if (!pProp->m_sDefaultSkeleton.IsEmpty())
  {
    desc.m_hDefaultSkeleton = WResourceManager::LoadResource<WSkeletonResource>(pProp->m_sDefaultSkeleton);
  }

  desc.Save(stream);

  WMeshImportUtils::RecordMeshTransformInfo(GetTransformInfo(), desc);

  return WStatus(W_SUCCESS);
}

WStatus WAnimatedMeshAssetDocument::CreateMeshFromFile(WAnimatedMeshAssetProperties* pProp, WMeshResourceDescriptor& desc)
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
  opt.m_bImportSkinningData = true;
  opt.m_bRecomputeNormals = pProp->m_bRecalculateNormals;
  opt.m_bRecomputeTangents = pProp->m_bRecalculateTangents;
  opt.m_bHighPrecision = pProp->m_bHighPrecision;
  opt.m_MeshVertexColorConversion = pProp->m_VertexColorConversion;
  opt.m_bNormalizeWeights = pProp->m_bNormalizeWeights;
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
  if (pProp->m_bImportMaterials || pProp->m_Slots.GetCount() != desc.GetSubMeshes().GetCount())
  {
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

WTransformStatus WAnimatedMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  WStatus status = WAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

void WAnimatedMeshAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // For glTF files, add any referenced external buffer files as dependencies
  WMeshImportUtils::AddGltfBufferDependencies(GetProperties()->m_sMeshFile, pInfo->m_TransformDependencies);
}
