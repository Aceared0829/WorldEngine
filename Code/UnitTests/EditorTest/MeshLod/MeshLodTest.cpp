#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginAssets/Util/MeshLodCreator.h>
#include <EditorPluginScene/Utils/MeshPrefabCreator.h>
#include <EditorTest/MeshLod/MeshLodTest.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/Declarations.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static WEditorMeshLodTest s_EditorMeshLodTest;

const char* WEditorMeshLodTest::GetTestName() const
{
  return "Mesh LOD Tests";
}

void WEditorMeshLodTest::SetupSubTests()
{
  AddSubTest("Simplification Ladder", SubTests::ST_SimplificationLadder);
  AddSubTest("Create LODs", SubTests::ST_CreateLods);
  AddSubTest("Continues From Simplified Base", SubTests::ST_ContinuesFromSimplifiedBase);
  AddSubTest("Transferred Settings", SubTests::ST_TransferredSettings);
  AddSubTest("Existing LODs", SubTests::ST_ExistingLods);
  AddSubTest("Prefab Picks Them Up", SubTests::ST_PrefabPicksThemUp);
  AddSubTest("Primitive Mesh", SubTests::ST_PrimitiveMesh);
  AddSubTest("Multiple Meshes", SubTests::ST_MultipleMeshes);
  AddSubTest("Sub Mesh Variants Do Not Share", SubTests::ST_SubMeshVariantsDoNotShare);
}

WResult WEditorMeshLodTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return W_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return W_FAILURE;

  // the project has to be settled first, or the curator keeps rehashing while the sub-tests
  // create their assets
  if (WStatus res = WAssetCurator::GetSingleton()->TransformAllAssets(WTransformFlags::TriggeredManually); res.Failed())
  {
    WLog::Error("Asset transform failed: {}", res.GetMessageString());
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WEditorMeshLodTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WTestAppRun WEditorMeshLodTest::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_SimplificationLadder:
      SimplificationLadder();
      break;
    case SubTests::ST_CreateLods:
      CreateLods();
      break;
    case SubTests::ST_ContinuesFromSimplifiedBase:
      ContinuesFromSimplifiedBase();
      break;
    case SubTests::ST_TransferredSettings:
      TransferredSettings();
      break;
    case SubTests::ST_ExistingLods:
      ExistingLods();
      break;
    case SubTests::ST_PrefabPicksThemUp:
      PrefabPicksThemUp();
      break;
    case SubTests::ST_PrimitiveMesh:
      PrimitiveMesh();
      break;
    case SubTests::ST_SubMeshVariantsDoNotShare:
      SubMeshVariantsDoNotShare();
      break;

    case SubTests::ST_MultipleMeshes:
      MultipleMeshes();
      break;
  }

  return WTestAppRun::Quit;
}

WString WEditorMeshLodTest::MakePrivateSourceMesh(const char* szName)
{
  WStringBuilder sSrc = m_sProjectPath;
  sSrc.AppendPath("Meshes/Cube.obj");

  WStringBuilder sRelative;
  sRelative.SetFormat("Meshes/{}.obj", szName);

  WStringBuilder sDst = m_sProjectPath;
  sDst.AppendPath(sRelative);

  if (WOSFile::CopyFile(sSrc, sDst).Failed())
    return {};

  WFileSystemModel::GetSingleton()->NotifyOfChange(sDst);
  return sRelative;
}

WUuid WEditorMeshLodTest::CreateMeshAsset(const char* szRelativePath, const char* szSourceFile, const WVariantDictionary* pExtraProperties)
{
  WStringBuilder sPath = m_sProjectPath;
  sPath.AppendPath(szRelativePath);

  WDocument* pDoc = m_pApplication->m_pEditorApp->CreateDocument(sPath, WDocumentFlags::None);
  if (pDoc == nullptr)
    return {};

  {
    WDocumentObject* pProps = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    WObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();

    pAcc->StartTransaction("Init");
    pAcc->SetValueByName(pProps, "MeshFile", szSourceFile).AssertSuccess();

    if (pExtraProperties != nullptr)
    {
      for (auto it : *pExtraProperties)
      {
        pAcc->SetValueByName(pProps, it.Key(), it.Value()).AssertSuccess();
      }
    }

    pAcc->FinishTransaction();
  }

  pDoc->SaveDocument(true).AssertSuccess();

  const WUuid guid = pDoc->GetGuid();
  pDoc->GetDocumentManager()->CloseDocument(pDoc);

  ProcessEvents(10);
  WAssetCurator::GetSingleton()->MainThreadTick(true);

  return guid;
}

WVariant WEditorMeshLodTest::ReadAssetProperty(WStringView sAbsPath, WStringView sProperty)
{
  WDocument* pDoc = m_pApplication->m_pEditorApp->OpenDocument(sAbsPath, WDocumentFlags::None);
  if (pDoc == nullptr)
    return {};

  W_SCOPE_EXIT(pDoc->GetDocumentManager()->CloseDocument(pDoc));

  const auto& children = pDoc->GetObjectManager()->GetRootObject()->GetChildren();
  if (children.GetCount() != 1)
    return {};

  return children[0]->GetTypeAccessor().GetValue(sProperty);
}

void WEditorMeshLodTest::SimplificationLadder()
{
  // Each level takes the midpoint of what is left between the level before it and 100, so from an
  // unsimplified mesh the ladder is 50, 75, 88, 94.
  W_TEST_INT(WMeshLodCreator::GetLodSimplification(0, 1), 50);
  W_TEST_INT(WMeshLodCreator::GetLodSimplification(0, 2), 75);
  W_TEST_INT(WMeshLodCreator::GetLodSimplification(0, 3), 88);
  W_TEST_INT(WMeshLodCreator::GetLodSimplification(0, 4), 94);

  // A mesh that is already simplified continues from where it stands: LOD-1 of a mesh at 50% has to
  // be sparser than the mesh, not equal to it.
  W_TEST_INT(WMeshLodCreator::GetLodSimplification(50, 1), 75);
  W_TEST_INT(WMeshLodCreator::GetLodSimplification(50, 2), 88);
  W_TEST_INT(WMeshLodCreator::GetLodSimplification(75, 1), 88);

  // every level has to be strictly sparser than the one before, from every starting point
  for (WUInt8 uiBase = 0; uiBase < 99; ++uiBase)
  {
    WUInt8 uiPrev = uiBase;

    for (WUInt32 uiLod = 1; uiLod <= WMeshLodCreator::s_uiMaxLods; ++uiLod)
    {
      const WUInt8 uiThis = WMeshLodCreator::GetLodSimplification(uiBase, uiLod);

      // At the very top the steps round down to nothing, which is expected - there is no room left.
      if (uiPrev >= 97)
        break;

      W_TEST_BOOL_MSG(uiThis > uiPrev, "Every LOD has to be sparser than the one before it.");
      uiPrev = uiThis;
    }
  }

  // 100 would remove the entire mesh, so the ladder must never reach it.
  for (WUInt8 uiBase = 0; uiBase <= 99; ++uiBase)
  {
    for (WUInt32 uiLod = 1; uiLod <= WMeshLodCreator::s_uiMaxLods; ++uiLod)
    {
      W_TEST_BOOL(WMeshLodCreator::GetLodSimplification(uiBase, uiLod) < 100);
      W_TEST_BOOL(WMeshLodCreator::GetLodSimplification(uiBase, uiLod) >= 1);
    }
  }

  // The tolerated error grows with the level, because a distant mesh can afford a coarser silhouette.
  W_TEST_INT(WMeshLodCreator::GetLodSimplificationError(1), 5);
  W_TEST_BOOL(WMeshLodCreator::GetLodSimplificationError(3) > WMeshLodCreator::GetLodSimplificationError(1));

  // past the table it must still answer rather than reading out of bounds
  W_TEST_BOOL(WMeshLodCreator::GetLodSimplificationError(WMeshLodCreator::s_uiMaxLods) > 0);
}

void WEditorMeshLodTest::CreateLods()
{
  const WString sSource = MakePrivateSourceMesh("LodBasic");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const WUuid meshGuid = CreateMeshAsset("MeshLod/Basic.WMeshAsset", sSource);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshLodSource source;
  if (!W_TEST_BOOL(WMeshLodCreator::GatherMeshLodSource(meshGuid, source).Succeeded()))
    return;

  W_TEST_BOOL(!source.m_bIsPrimitive);
  W_TEST_STRING(source.m_sMeshFile, sSource);
  W_TEST_INT_MSG(source.m_uiBaseSimplification, 0, "This mesh does not simplify, so the ladder starts from the full model.");

  // The folder name is what the prefab tool looks for, so it is not a free choice.
  W_TEST_BOOL_MSG(source.m_sLodFolder.EndsWith("Basic_data"), "The LODs belong in <MeshName>_data.");

  WMeshLodOptions options;
  options.m_uiLodCount = 2;

  WUInt32 uiCreated = 0;
  WUInt32 uiSkipped = 0;
  if (!W_TEST_BOOL(WMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded()))
    return;

  W_TEST_INT(uiCreated, 2);
  W_TEST_INT(uiSkipped, 0);

  const WString sLod1 = WMeshLodCreator::GetLodPath(source, 1);
  const WString sLod2 = WMeshLodCreator::GetLodPath(source, 2);

  if (!W_TEST_BOOL(WOSFile::ExistsFile(sLod1) && WOSFile::ExistsFile(sLod2)))
    return;

  // The exact names matter: LOD-1 and LOD-2, so that the prefab tool finds them.
  W_TEST_STRING(WPathUtils::GetFileName(sLod1), "LOD-1");
  W_TEST_STRING(WPathUtils::GetFileName(sLod2), "LOD-2");

  // each LOD is a simplified version of the same model, not a copy of it
  W_TEST_STRING(ReadAssetProperty(sLod1, "MeshFile").ConvertTo<WString>(), sSource);
  W_TEST_BOOL(ReadAssetProperty(sLod1, "SimplifyMesh").ConvertTo<bool>() == true);
  W_TEST_INT(ReadAssetProperty(sLod1, "MeshSimplification").ConvertTo<WInt64>(), 50);
  W_TEST_INT(ReadAssetProperty(sLod2, "MeshSimplification").ConvertTo<WInt64>(), 75);

  // and it must not re-import the materials, which would produce a second set for the same model
  W_TEST_BOOL(ReadAssetProperty(sLod1, "ImportMaterials").ConvertTo<bool>() == false);
}

void WEditorMeshLodTest::ContinuesFromSimplifiedBase()
{
  const WString sSource = MakePrivateSourceMesh("LodContinue");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  // The mesh is already simplified, so its LODs have to go further rather than starting at 50 again.
  WVariantDictionary extras;
  extras.Insert("SimplifyMesh", true);
  extras.Insert("MeshSimplification", (WInt64)50);

  const WUuid meshGuid = CreateMeshAsset("MeshLod/Continue.WMeshAsset", sSource, &extras);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshLodSource source;
  if (!W_TEST_BOOL(WMeshLodCreator::GatherMeshLodSource(meshGuid, source).Succeeded()))
    return;

  W_TEST_INT(source.m_uiBaseSimplification, 50);

  WMeshLodOptions options;
  options.m_uiLodCount = 2;

  WUInt32 uiCreated = 0;
  WUInt32 uiSkipped = 0;
  if (!W_TEST_BOOL(WMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded()))
    return;

  const WString sLod1 = WMeshLodCreator::GetLodPath(source, 1);
  const WString sLod2 = WMeshLodCreator::GetLodPath(source, 2);

  // a LOD of a mesh that is already at 50% has to be sparser than that mesh, not equal to it
  W_TEST_INT(ReadAssetProperty(sLod1, "MeshSimplification").ConvertTo<WInt64>(), 75);
  W_TEST_INT(ReadAssetProperty(sLod2, "MeshSimplification").ConvertTo<WInt64>(), 88);
}

void WEditorMeshLodTest::SubMeshVariantsDoNotShare()
{
  // One model file that several mesh assets import a different part of, which is how a set of plant
  // or rock variants is usually authored.
  const WString sSource = MakePrivateSourceMesh("LodVariants");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  WVariantDictionary extrasA;
  extrasA.Insert("MeshIncludeTags", "Variant_A");

  WVariantDictionary extrasB;
  extrasB.Insert("MeshIncludeTags", "Variant_B");

  const WUuid guidA = CreateMeshAsset("MeshLod/VariantA.WMeshAsset", sSource, &extrasA);
  const WUuid guidB = CreateMeshAsset("MeshLod/VariantB.WMeshAsset", sSource, &extrasB);

  if (!W_TEST_BOOL(guidA.IsValid() && guidB.IsValid()))
    return;

  WMeshLodSource sourceA, sourceB;
  if (!W_TEST_BOOL(WMeshLodCreator::GatherMeshLodSource(guidA, sourceA).Succeeded()))
    return;
  if (!W_TEST_BOOL(WMeshLodCreator::GatherMeshLodSource(guidB, sourceB).Succeeded()))
    return;

  W_TEST_STRING(sourceA.m_sMeshIncludeTags, "Variant_A");
  W_TEST_STRING(sourceB.m_sMeshIncludeTags, "Variant_B");

  // Each variant is named after its own mesh asset, not after the model file they share. Sharing it
  // would make one variant render the other's geometry at distance.
  W_TEST_BOOL_MSG(sourceA.m_sLodFolder.EndsWith("VariantA_data"), "A sub-mesh variant gets its own LOD folder.");
  W_TEST_BOOL_MSG(sourceB.m_sLodFolder.EndsWith("VariantB_data"), "A sub-mesh variant gets its own LOD folder.");
  W_TEST_BOOL(sourceA.m_sLodFolder != sourceB.m_sLodFolder);

  WMeshLodOptions options;
  options.m_uiLodCount = 1;

  WUInt32 uiCreated = 0;
  WUInt32 uiSkipped = 0;
  if (!W_TEST_BOOL(WMeshLodCreator::CreateMeshLods(sourceA, options, uiCreated, uiSkipped).Succeeded()))
    return;

  W_TEST_INT(uiCreated, 1);

  // The second variant must still create its own, rather than finding the first one's and skipping.
  uiCreated = 0;
  uiSkipped = 0;
  if (!W_TEST_BOOL(WMeshLodCreator::CreateMeshLods(sourceB, options, uiCreated, uiSkipped).Succeeded()))
    return;

  W_TEST_INT_MSG(uiCreated, 1, "The variants must not claim each other's LOD assets.");
  W_TEST_INT(uiSkipped, 0);

  // and each LOD simplifies the part its own mesh asset uses
  W_TEST_STRING(ReadAssetProperty(WMeshLodCreator::GetLodPath(sourceA, 1), "MeshIncludeTags").ConvertTo<WString>(), "Variant_A");
  W_TEST_STRING(ReadAssetProperty(WMeshLodCreator::GetLodPath(sourceB, 1), "MeshIncludeTags").ConvertTo<WString>(), "Variant_B");
}

void WEditorMeshLodTest::TransferredSettings()
{
  const WString sSource = MakePrivateSourceMesh("LodSettings");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  // every value differs from the mesh asset's default, so a transferred value cannot be mistaken
  // for a coincidental match
  const WVec3 vOffset(1.0f, -2.0f, 3.5f);

  WVariantDictionary extras;
  extras.Insert("MeshIncludeTags", "Render");
  extras.Insert("MeshExcludeTags", "NoRender;UCX_");
  extras.Insert("ImportTransform", (WInt64)WMeshImportTransform::Custom);
  extras.Insert("RightDir", (WInt64)WBasisAxis::PositiveZ);
  extras.Insert("UpDir", (WInt64)WBasisAxis::NegativeY);
  extras.Insert("FlipForwardDir", true);
  extras.Insert("PositionOffset", vOffset);
  extras.Insert("UniformScaling", 5.0f);

  const WUuid meshGuid = CreateMeshAsset("MeshLod/Settings.WMeshAsset", sSource, &extras);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshLodSource source;
  if (!W_TEST_BOOL(WMeshLodCreator::GatherMeshLodSource(meshGuid, source).Succeeded()))
    return;

  WMeshLodOptions options;
  options.m_uiLodCount = 1;

  WUInt32 uiCreated = 0;
  WUInt32 uiSkipped = 0;
  if (!W_TEST_BOOL(WMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded()))
    return;

  const WString sLod1 = WMeshLodCreator::GetLodPath(source, 1);

  // A LOD has to sit in the same place, at the same size, built from the same part of the model as
  // the mesh it stands in for - otherwise the object visibly jumps when the LOD switches.
  W_TEST_STRING(ReadAssetProperty(sLod1, "MeshFile").ConvertTo<WString>(), sSource);
  W_TEST_STRING(ReadAssetProperty(sLod1, "MeshIncludeTags").ConvertTo<WString>(), "Render");
  W_TEST_STRING(ReadAssetProperty(sLod1, "MeshExcludeTags").ConvertTo<WString>(), "NoRender;UCX_");
  W_TEST_INT(ReadAssetProperty(sLod1, "ImportTransform").ConvertTo<WInt64>(), (WInt64)WMeshImportTransform::Custom);
  W_TEST_INT(ReadAssetProperty(sLod1, "RightDir").ConvertTo<WInt64>(), (WInt64)WBasisAxis::PositiveZ);
  W_TEST_INT(ReadAssetProperty(sLod1, "UpDir").ConvertTo<WInt64>(), (WInt64)WBasisAxis::NegativeY);
  W_TEST_BOOL(ReadAssetProperty(sLod1, "FlipForwardDir").ConvertTo<bool>() == true);
  W_TEST_VEC3(ReadAssetProperty(sLod1, "PositionOffset").ConvertTo<WVec3>(), vOffset, 0.0001f);
  W_TEST_FLOAT(ReadAssetProperty(sLod1, "UniformScaling").ConvertTo<float>(), 5.0f, 0.0001f);
}

void WEditorMeshLodTest::ExistingLods()
{
  const WString sSource = MakePrivateSourceMesh("LodExisting");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const WUuid meshGuid = CreateMeshAsset("MeshLod/Existing.WMeshAsset", sSource);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WString sLod1;

  {
    WMeshLodSource source;
    WMeshLodCreator::GatherMeshLodSource(meshGuid, source).AssertSuccess();
    W_TEST_BOOL_MSG(!source.HasLod(1), "Nothing has been created yet.");

    sLod1 = WMeshLodCreator::GetLodPath(source, 1);

    WMeshLodOptions options;
    options.m_uiLodCount = 1;

    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    W_TEST_BOOL(WMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded());
    W_TEST_INT(uiCreated, 1);
  }

  ProcessEvents(10);
  WAssetCurator::GetSingleton()->MainThreadTick(true);

  // a LOD may have been tuned by hand, so a second run must not throw that away
  {
    WDocument* pLod = m_pApplication->m_pEditorApp->OpenDocument(sLod1, WDocumentFlags::None);
    if (W_TEST_BOOL(pLod != nullptr))
    {
      WDocumentObject* pProps = pLod->GetObjectManager()->GetRootObject()->GetChildren()[0];
      WObjectAccessorBase* pAcc = pLod->GetObjectAccessor();

      pAcc->StartTransaction("Tune");
      pAcc->SetValueByName(pProps, "MeshSimplification", (WInt64)62).AssertSuccess();
      pAcc->FinishTransaction();

      pLod->SaveDocument(true).AssertSuccess();
      pLod->GetDocumentManager()->CloseDocument(pLod);
    }
  }

  {
    WMeshLodSource source;
    WMeshLodCreator::GatherMeshLodSource(meshGuid, source).AssertSuccess();
    W_TEST_BOOL_MSG(source.HasLod(1), "Gathering has to report the LOD that now exists.");

    WMeshLodOptions options;
    options.m_uiLodCount = 1;
    options.m_bOverwriteExisting = false;

    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    W_TEST_BOOL(WMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded());
    W_TEST_INT(uiCreated, 0);
    W_TEST_INT(uiSkipped, 1);

    W_TEST_INT_MSG(ReadAssetProperty(sLod1, "MeshSimplification").ConvertTo<WInt64>(), 62, "The hand-tuned value had to survive.");
  }

  // asking for it explicitly does replace it
  {
    WMeshLodSource source;
    WMeshLodCreator::GatherMeshLodSource(meshGuid, source).AssertSuccess();

    WMeshLodOptions options;
    options.m_uiLodCount = 1;
    options.m_bOverwriteExisting = true;

    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    W_TEST_BOOL(WMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded());
    W_TEST_INT(uiCreated, 1);
    W_TEST_INT(uiSkipped, 0);

    W_TEST_INT_MSG(ReadAssetProperty(sLod1, "MeshSimplification").ConvertTo<WInt64>(), 50, "Replacing has to restore the generated value.");
  }

  // Raising the count adds only what is missing, rather than refusing because something is there.
  {
    WMeshLodSource source;
    WMeshLodCreator::GatherMeshLodSource(meshGuid, source).AssertSuccess();

    WMeshLodOptions options;
    options.m_uiLodCount = 3;
    options.m_bOverwriteExisting = false;

    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    W_TEST_BOOL(WMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded());
    W_TEST_INT_MSG(uiCreated, 2, "LOD-2 and LOD-3 were missing.");
    W_TEST_INT_MSG(uiSkipped, 1, "LOD-1 was already there.");
  }
}

void WEditorMeshLodTest::PrefabPicksThemUp()
{
  const WString sSource = MakePrivateSourceMesh("LodForPrefab");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const WUuid meshGuid = CreateMeshAsset("MeshLodPrefab/ForPrefab.WMeshAsset", sSource);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  // Before the LODs exist, the mesh is just a mesh.
  {
    WMeshPrefabSource prefabSource;
    if (!W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, prefabSource).Succeeded()))
      return;

    W_TEST_BOOL(prefabSource.m_LodGuids.IsEmpty());
    W_TEST_BOOL(prefabSource.GetDefaultRenderComponentType() == "WMeshComponent"_wsv);
  }

  WMeshLodSource source;
  if (!W_TEST_BOOL(WMeshLodCreator::GatherMeshLodSource(meshGuid, source).Succeeded()))
    return;

  WMeshLodOptions options;
  options.m_uiLodCount = 2;

  WUInt32 uiCreated = 0;
  WUInt32 uiSkipped = 0;
  if (!W_TEST_BOOL(WMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded()))
    return;

  ProcessEvents(10);
  WAssetCurator::GetSingleton()->MainThreadTick(true);

  // creating a prefab afterwards has to find the LODs by itself and build a LOD component instead
  // of a plain mesh component, which is why the names and the folder are not a free choice
  {
    WMeshPrefabSource prefabSource;
    if (!W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, prefabSource).Succeeded()))
      return;

    W_TEST_INT_MSG(prefabSource.m_LodGuids.GetCount(), 2, "The prefab tool has to find the LODs that were just created.");
    W_TEST_BOOL(prefabSource.GetDefaultRenderComponentType() == "WLodMeshComponent"_wsv);
  }
}

void WEditorMeshLodTest::PrimitiveMesh()
{
  // A procedural primitive has no model file, so there is no geometry to simplify.
  // WMeshPrimitive lives in EditorPluginAssets, which this test does not link. 6 is Sphere.
  WVariantDictionary extras;
  extras.Insert("PrimitiveType", (WInt64)6);

  const WUuid meshGuid = CreateMeshAsset("MeshLod/Primitive.WMeshAsset", "", &extras);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshLodSource source;
  if (!W_TEST_BOOL(WMeshLodCreator::GatherMeshLodSource(meshGuid, source).Succeeded()))
    return;

  W_TEST_BOOL_MSG(source.m_bIsPrimitive, "A non-File primitive type has to be recognized.");
  W_TEST_BOOL(source.m_sMeshFile.IsEmpty());

  WMeshLodOptions options;
  options.m_uiLodCount = 2;

  WUInt32 uiCreated = 0;
  WUInt32 uiSkipped = 0;
  const WStatus res = WMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped);

  W_TEST_BOOL_MSG(res.Failed(), "A primitive mesh has no source file, so this must fail.");
  W_TEST_INT(uiCreated, 0);
  W_TEST_BOOL_MSG(!WOSFile::ExistsFile(WMeshLodCreator::GetLodPath(source, 1)), "A failed run must not leave a document behind.");
}

void WEditorMeshLodTest::MultipleMeshes()
{
  WHybridArray<WUuid, 4> meshes;
  WHybridArray<WMeshLodSource, 4> sources;

  for (WUInt32 i = 0; i < 3; ++i)
  {
    WStringBuilder sName;
    sName.SetFormat("LodBatch{}", i);

    const WString sSource = MakePrivateSourceMesh(sName);
    if (!W_TEST_BOOL(!sSource.IsEmpty()))
      return;

    WStringBuilder sAssetPath;
    sAssetPath.SetFormat("MeshLodBatch/Batch{}.WMeshAsset", i);

    const WUuid guid = CreateMeshAsset(sAssetPath, sSource);
    if (!W_TEST_BOOL(guid.IsValid()))
      return;

    meshes.PushBack(guid);

    WMeshLodSource& source = sources.ExpandAndGetRef();
    WMeshLodCreator::GatherMeshLodSource(guid, source).AssertSuccess();
  }

  WMeshLodOptions options;
  options.m_uiLodCount = 2;

  {
    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    if (!W_TEST_BOOL(WMeshLodCreator::CreateMeshLodsForAll(meshes, options, uiCreated, uiSkipped).Succeeded()))
      return;

    W_TEST_INT_MSG(uiCreated, 6, "Three meshes, two LODs each.");
    W_TEST_INT(uiSkipped, 0);

    for (const WMeshLodSource& source : sources)
    {
      W_TEST_BOOL(WOSFile::ExistsFile(WMeshLodCreator::GetLodPath(source, 1)));
      W_TEST_BOOL(WOSFile::ExistsFile(WMeshLodCreator::GetLodPath(source, 2)));
    }
  }

  ProcessEvents(10);
  WAssetCurator::GetSingleton()->MainThreadTick(true);

  // Running it again leaves everything alone rather than replacing what is there.
  {
    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    W_TEST_BOOL(WMeshLodCreator::CreateMeshLodsForAll(meshes, options, uiCreated, uiSkipped).Succeeded());
    W_TEST_INT(uiCreated, 0);
    W_TEST_INT(uiSkipped, 6);
  }

  // A guid that is not a mesh asset is a skip, not a failure - a selection can hold anything.
  {
    WHybridArray<WUuid, 2> mixed;
    mixed.PushBack(WUuid::MakeUuid());

    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    W_TEST_BOOL(WMeshLodCreator::CreateMeshLodsForAll(mixed, options, uiCreated, uiSkipped).Succeeded());
    W_TEST_INT(uiCreated, 0);
    W_TEST_INT(uiSkipped, 1);
  }

  // A LOD asset must never get LODs of its own, or the _data folders nest without end.
  {
    auto pLod = WAssetCurator::GetSingleton()->FindSubAsset(WMeshLodCreator::GetLodPath(sources[0], 1));
    if (W_TEST_BOOL(pLod.isValid()))
    {
      WHybridArray<WUuid, 2> lodOnly;
      lodOnly.PushBack(pLod->m_Data.m_Guid);

      WUInt32 uiCreated = 0;
      WUInt32 uiSkipped = 0;
      W_TEST_BOOL(WMeshLodCreator::CreateMeshLodsForAll(lodOnly, options, uiCreated, uiSkipped).Succeeded());
      W_TEST_INT_MSG(uiCreated, 0, "A LOD must not get LODs of its own.");
      W_TEST_INT(uiSkipped, 1);
    }
  }
}
