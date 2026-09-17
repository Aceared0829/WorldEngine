#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/Dialogs/MeshImportDlg.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshAssetDocumentGenerator, 1, WRTTIDefaultAllocator<WMeshAssetDocumentGenerator>)
W_END_DYNAMIC_REFLECTED_TYPE;

bool WMeshAssetDocumentGenerator::s_bCreateMaterials = true;
bool WMeshAssetDocumentGenerator::s_bUseSharedMaterials = false;
bool WMeshAssetDocumentGenerator::s_bReuseSkeleton = false;
bool WMeshAssetDocumentGenerator::s_bImportAllClips = false;
bool WMeshAssetDocumentGenerator::s_bAddLODs = false;
WUInt8 WMeshAssetDocumentGenerator::s_uiNumLODs = 1;
WUuid WMeshAssetDocumentGenerator::s_SharedSkeleton;

WMeshAssetDocumentGenerator::WMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
  AddSupportedFileType("vox");
}

WMeshAssetDocumentGenerator::WMeshAssetDocumentGenerator(bool bAnimMesh)
{
  m_bAnimatedMesh = bAnimMesh;
}

WMeshAssetDocumentGenerator::~WMeshAssetDocumentGenerator() = default;

void WMeshAssetDocumentGenerator::GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = WAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "MeshImport";
    info.m_sIcon = ":/AssetIcons/Mesh.svg";
  }
}

WStatus WMeshAssetDocumentGenerator::Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  const WStringBuilder sOutFile = GetImportTargetPath(sInputFileAbs);

  auto pApp = WQtEditorApp::GetSingleton();

  WStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  WProjectPreferencesUser* pPref = WPreferences::QueryPreferences<WProjectPreferencesUser>();

  WStringBuilder sSharedMaterialsFolderAbs = pPref->m_sSharedMaterialFolder;

  if (sSharedMaterialsFolderAbs.IsEmpty())
  {
    WStringBuilder tmp = WToolsProject::GetSingleton()->GetProjectDirectory();
    tmp.AppendPath("Materials");

    sSharedMaterialsFolderAbs = tmp;
  }

  // Without a user there is nobody to close this dialog, so the defaults below are used as they are.
  if (m_bShowImportDlg && !pApp->IsInUnattendedMode())
  {
    WMeshImportDlg dlg(nullptr);
    dlg.m_bShowAnimMeshOptions = m_bAnimatedMesh;
    dlg.m_sTitle = sInputFileRel;
    dlg.m_bCreateMaterials = s_bCreateMaterials;
    dlg.m_sSharedMaterialsFolderAbs = sSharedMaterialsFolderAbs;
    dlg.m_bUseSharedMaterials = s_bUseSharedMaterials;
    dlg.m_bReuseExistingSkeleton = s_bReuseSkeleton;
    dlg.m_SharedSkeleton = s_SharedSkeleton;
    dlg.m_bImportAnimationClips = s_bImportAllClips;
    dlg.m_bAddLODs = s_bAddLODs;
    dlg.m_uiNumLODs = s_uiNumLODs;
    dlg.m_sMeshLodPrefix = pPref->m_sMeshLodPrefix.IsEmpty() ? WString("$LOD") : pPref->m_sMeshLodPrefix;

    if (dlg.exec() != QDialog::Accepted)
    {
      return WStatus("User aborted asset import.");
    }

    s_bCreateMaterials = dlg.m_bCreateMaterials;

    if (s_bCreateMaterials)
    {
      s_bUseSharedMaterials = dlg.m_bUseSharedMaterials;

      if (s_bUseSharedMaterials)
      {
        sSharedMaterialsFolderAbs = dlg.m_sSharedMaterialsFolderAbs;
        pPref->m_sSharedMaterialFolder = dlg.m_sSharedMaterialsFolderAbs;
      }
    }

    s_bAddLODs = dlg.m_bAddLODs;
    if (s_bAddLODs)
    {
      s_uiNumLODs = WMath::Clamp<WUInt8>(dlg.m_uiNumLODs, 0, 4);
      pPref->m_sMeshLodPrefix = dlg.m_sMeshLodPrefix;
    }

    if (m_bAnimatedMesh)
    {
      s_bReuseSkeleton = dlg.m_bReuseExistingSkeleton;
      s_SharedSkeleton = dlg.m_SharedSkeleton;
      s_bImportAllClips = dlg.m_bImportAnimationClips;
    }

    if (dlg.m_bApplyToAll)
    {
      m_bShowImportDlg = false;
    }
  }

  WStringBuilder sMaterialFolder = sInputFileAbs;

  if (s_bUseSharedMaterials)
  {
    sMaterialFolder = sSharedMaterialsFolderAbs;
  }

  WTempHybridArray<WMaterialResourceSlot, 8> materials;
  WUniquePtr<WModelImporter2::Importer> pImporter;

  if (s_bCreateMaterials || (m_bAnimatedMesh && s_bImportAllClips))
  {
    pImporter = WModelImporter2::RequestImporterForFileType(sInputFileAbs);
    if (pImporter == nullptr)
      return WStatus("No known importer for this file type.");

    WMeshResourceDescriptor desc;

    WModelImporter2::ImportOptions opt;
    opt.m_sSourceFile = sInputFileAbs;
    opt.m_pMeshOutput = &desc;

    if (pImporter->Import(opt).Failed())
      return WStatus("Model importer was unable to read this asset.");

    WMeshImportUtils::SetMeshAssetMaterialSlots(materials, pImporter.Borrow());
    WMeshImportUtils::ImportMeshAssetMaterials(materials, sMaterialFolder, pImporter.Borrow());
  }

  return ConfigureMeshDocument(sInputFileRel, sOutFile, pImporter.Borrow(), materials, out_generatedDocuments);
}

static void FindLODs(WArrayMap<WUInt32, WString>& out_foundLods, WModelImporter2::Importer* pImporter)
{
  out_foundLods.Clear();

  WProjectPreferencesUser* pPref = WPreferences::QueryPreferences<WProjectPreferencesUser>();

  if (!pPref->m_sMeshLodPrefix.IsEmpty())
  {
    WStringBuilder lod;

    for (WUInt32 i = 0; i < 4; ++i)
    {
      lod = pPref->m_sMeshLodPrefix;
      lod.AppendFormat("{}", i);

      for (const auto& meshName : pImporter->m_OutputMeshNames)
      {
        if (meshName.StartsWith_NoCase(lod) || meshName.EndsWith_NoCase(lod))
        {
          out_foundLods[i] = lod;
        }
      }
    }
  }

  out_foundLods.Sort();
}

static void SetMeshLod(WUInt32 uiLod, WArrayMap<WUInt32, WString>& ref_foundLods, WDocumentObject* pPropObj, WObjectCommandAccessor& ref_accessor)
{
  if (uiLod > 0)
  {
    if (ref_foundLods.IsEmpty())
    {
      ref_accessor.SetValueByName(pPropObj, "SimplifyMesh", true).AssertSuccess();

      const WInt32 uiSimp[5] = {0, 50, 75, 90, 95};
      const WInt32 uiErro[5] = {0, 5, 5, 10, 15};
      ref_accessor.SetValueByName(pPropObj, "MeshSimplification", uiSimp[uiLod]).AssertSuccess();
      ref_accessor.SetValueByName(pPropObj, "MaxSimplificationError", uiErro[uiLod]).AssertSuccess();
    }
    else
    {
      // always use the smallest next LOD that was found
      const WString& sLodToUse = ref_foundLods.GetValue(0);

      ref_accessor.SetValueByName(pPropObj, "MeshIncludeTags", sLodToUse).AssertSuccess();

      ref_foundLods.RemoveAtAndCopy(0);
    }
  }
}

WStatus WMeshAssetDocumentGenerator::ConfigureMeshDocument(WStringView sInputFile, WStringView sOutFile, WModelImporter2::Importer* pImporter, WArrayPtr<WMaterialResourceSlot> materials, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  auto pApp = WQtEditorApp::GetSingleton();

  WUInt32 uiNumLODs = 1;

  WArrayMap<WUInt32, WString> foundLods;
  if (s_bAddLODs)
  {
    FindLODs(foundLods, pImporter);
    uiNumLODs += (!foundLods.IsEmpty()) ? foundLods.GetCount() : s_uiNumLODs;
  }

  WStringBuilder sFinalName, sFinalPath;

  for (WUInt32 uiLod = 0; uiLod < uiNumLODs; ++uiLod)
  {
    sFinalPath = sOutFile;

    if (uiLod > 0)
    {
      // sFinalName.SetFormat("{}_lod{}", sOutFile.GetFileName(), uiLod);
      sFinalName.SetFormat("{}_data/LOD-{}", sOutFile.GetFileName(), uiLod);
      sFinalPath.ChangeFileName(sFinalName);
    }

    WDocument* pDoc = pApp->CreateDocument(sFinalPath, WDocumentFlags::None);
    if (pDoc == nullptr)
      return WStatus(WFmt("Could not create document '{}'", sFinalPath));

    out_generatedDocuments.PushBack(pDoc);

    WMeshAssetDocument* pAssetDoc = WDynamicCast<WMeshAssetDocument*>(pDoc);

    auto pPropObj = pAssetDoc->GetPropertyObject();

    WObjectCommandAccessor ca(pAssetDoc->GetCommandHistory());
    ca.StartTransaction("Init Values");
    ca.SetValueByName(pPropObj, "MeshFile", sInputFile).AssertSuccess();
    ca.SetValueByName(pPropObj, "ImportMaterials", false).AssertSuccess();

    for (WUInt32 i = 0; i < materials.GetCount(); ++i)
    {
      WUuid guid = WUuid::MakeUuid();
      ca.AddObjectByName(pPropObj, "Materials", i, WGetStaticRTTI<WMaterialResourceSlot>(), guid).AssertSuccess();

      auto* pChildMatObj = ca.GetObject(guid);
      ca.SetValueByName(pChildMatObj, "Label", materials[i].m_sLabel).AssertSuccess();
      ca.SetValueByName(pChildMatObj, "Resource", materials[i].m_sResource).AssertSuccess();
    }

    SetMeshLod(uiLod, foundLods, pPropObj, ca);

    ca.FinishTransaction();

    WLog::Success("Imported mesh: '{}'", sFinalPath);
  }

  return WStatus(W_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimatedMeshAssetDocumentGenerator, 1, WRTTIDefaultAllocator<WAnimatedMeshAssetDocumentGenerator>)
W_END_DYNAMIC_REFLECTED_TYPE;

WAnimatedMeshAssetDocumentGenerator::WAnimatedMeshAssetDocumentGenerator()
  : WMeshAssetDocumentGenerator(true)
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

WAnimatedMeshAssetDocumentGenerator::~WAnimatedMeshAssetDocumentGenerator() = default;

void WAnimatedMeshAssetDocumentGenerator::GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = WAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "AnimatedMeshImport";
    info.m_sIcon = ":/AssetIcons/Animated_Mesh.svg";
  }
}

WStatus WAnimatedMeshAssetDocumentGenerator::ConfigureMeshDocument(WStringView sInputFile, WStringView sOutFile, WModelImporter2::Importer* pImporter, WArrayPtr<WMaterialResourceSlot> materials, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  auto pApp = WQtEditorApp::GetSingleton();

  WDocument* pMainDoc = pApp->CreateDocument(sOutFile, WDocumentFlags::None);
  if (pMainDoc == nullptr)
    return WStatus(WFmt("Could not create document '{}'", sOutFile));

  out_generatedDocuments.PushBack(pMainDoc);

  WUuid skeletonGuid = s_SharedSkeleton;

  // create skeleton asset
  if (!s_bReuseSkeleton)
  {
    WStringBuilder sOutFile2;

    sOutFile2 = sOutFile;
    sOutFile2.ChangeFileExtension("WSkeletonAsset");

    if (WOSFile::ExistsFile(sOutFile2))
    {
      WLog::Info("Skipping skeleton import, file has been imported before: '{}'", sOutFile2);

      auto pSkeletonDoc = WAssetCurator::GetSingleton()->FindSubAsset(sOutFile2);
      skeletonGuid = pSkeletonDoc->m_Data.m_Guid;
    }
    else
    {
      WDocument* pSkelDoc = pApp->CreateDocument(sOutFile2, WDocumentFlags::None);
      if (pSkelDoc == nullptr)
        return WStatus("Could not create skeleton document");

      WStringBuilder sAnimMeshGuid;
      WConversionUtils::ToString(pMainDoc->GetGuid(), sAnimMeshGuid);

      out_generatedDocuments.PushBack(pSkelDoc);

      WSkeletonAssetDocument* pSkeletonDoc = WDynamicCast<WSkeletonAssetDocument*>(pSkelDoc);

      auto pSkeletonPropObj = pSkeletonDoc->GetPropertyObject();

      WObjectCommandAccessor ca(pSkeletonDoc->GetCommandHistory());
      ca.StartTransaction("Init Values");
      ca.SetValueByName(pSkeletonPropObj, "File", sInputFile).AssertSuccess();
      ca.SetValueByName(pSkeletonPropObj, "PreviewMesh", sAnimMeshGuid.GetView()).AssertSuccess();
      ca.FinishTransaction();

      skeletonGuid = pSkeletonDoc->GetGuid();

      WLog::Success("Imported skeleton: '{}'", sOutFile2);
    }
  }

  // configure animated mesh asset
  {
    WStringBuilder sFinalName, sFinalPath;

    WUInt32 uiNumLODs = 1;

    WArrayMap<WUInt32, WString> foundLods;
    if (s_bAddLODs)
    {
      FindLODs(foundLods, pImporter);
      uiNumLODs += (!foundLods.IsEmpty()) ? foundLods.GetCount() : s_uiNumLODs;
    }

    for (WUInt32 uiLod = 0; uiLod < uiNumLODs; ++uiLod)
    {
      sFinalPath = sOutFile;

      WAnimatedMeshAssetDocument* pAnimMeshDoc;

      if (uiLod == 0)
      {
        pAnimMeshDoc = WDynamicCast<WAnimatedMeshAssetDocument*>(pMainDoc);
      }
      else
      {
        if (uiLod > 0)
        {
          // sFinalName.SetFormat("{}_lod{}", sOutFile.GetFileName(), uiLod);
          sFinalName.SetFormat("{}_data/LOD-{}", sOutFile.GetFileName(), uiLod);
          sFinalPath.ChangeFileName(sFinalName);
        }

        pAnimMeshDoc = WDynamicCast<WAnimatedMeshAssetDocument*>(pApp->CreateDocument(sFinalPath, WDocumentFlags::None));
        if (pAnimMeshDoc == nullptr)
          return WStatus(WFmt("Could not create document '{}'", sFinalPath));

        out_generatedDocuments.PushBack(pAnimMeshDoc);
      }

      auto pPropObj = pAnimMeshDoc->GetPropertyObject();

      WStringBuilder sSkeletonGuid;
      WConversionUtils::ToString(skeletonGuid, sSkeletonGuid);

      WObjectCommandAccessor ca(pAnimMeshDoc->GetCommandHistory());
      ca.StartTransaction("Init Values");
      ca.SetValueByName(pPropObj, "MeshFile", sInputFile).AssertSuccess();
      ca.SetValueByName(pPropObj, "ImportMaterials", false).AssertSuccess();
      ca.SetValueByName(pPropObj, "DefaultSkeleton", sSkeletonGuid.GetView()).AssertSuccess();

      for (WUInt32 i = 0; i < materials.GetCount(); ++i)
      {
        WUuid guid = WUuid::MakeUuid();
        ca.AddObjectByName(pPropObj, "Materials", i, WGetStaticRTTI<WMaterialResourceSlot>(), guid).AssertSuccess();

        auto* pChildMatObj = ca.GetObject(guid);
        ca.SetValueByName(pChildMatObj, "Label", materials[i].m_sLabel).AssertSuccess();
        ca.SetValueByName(pChildMatObj, "Resource", materials[i].m_sResource).AssertSuccess();
      }

      SetMeshLod(uiLod, foundLods, pPropObj, ca);

      ca.FinishTransaction();

      WLog::Success("Imported animated mesh: '{}'", sFinalPath);
    }
  }

  // create animation clip assets
  if (s_bImportAllClips)
  {
    WStringBuilder sFilename;
    WStringBuilder sOutFile2;

    WStringBuilder sPreviewMesh;
    WConversionUtils::ToString(pMainDoc->GetGuid(), sPreviewMesh);

    for (const auto& clip : pImporter->m_OutputAnimationNames)
    {
      WPathUtils::MakeValidFilename(clip, '-', sFilename);
      sFilename.ReplaceAll(" ", "-");
      sFilename.Prepend(sOutFile.GetFileName(), "_");

      sOutFile2 = sOutFile;
      sOutFile2.ChangeFileName(sFilename);
      sOutFile2.ChangeFileExtension("WAnimationClipAsset");

      if (WOSFile::ExistsFile(sOutFile2))
      {
        WLog::Info("Skipping animation clip import, file has been imported before: '{}'", sOutFile2);
        continue;
      }

      WDocument* pAnimDoc = pApp->CreateDocument(sOutFile2, WDocumentFlags::None);
      if (pAnimDoc == nullptr)
        return WStatus("Could not create animation clip document");

      out_generatedDocuments.PushBack(pAnimDoc);

      WAnimationClipAssetDocument* pAnimClipDoc = WDynamicCast<WAnimationClipAssetDocument*>(pAnimDoc);

      auto pAnimPropObj = pAnimClipDoc->GetPropertyObject();

      WObjectCommandAccessor ca(pAnimClipDoc->GetCommandHistory());
      ca.StartTransaction("Init Values");

      ca.SetValueByName(pAnimPropObj, "File", sInputFile).AssertSuccess();
      ca.SetValueByName(pAnimPropObj, "UseAnimationClip", clip).AssertSuccess();
      ca.SetValueByName(pAnimPropObj, "PreviewMesh", sPreviewMesh.GetView()).AssertSuccess();

      ca.FinishTransaction();

      WLog::Success("Imported animation clip: '{}'", sOutFile2);
    }
  }

  return WStatus(W_SUCCESS);
}
