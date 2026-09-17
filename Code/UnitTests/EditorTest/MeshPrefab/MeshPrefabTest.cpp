#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Utils/MeshPrefabCreator.h>
#include <EditorTest/MeshPrefab/MeshPrefabTest.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static WEditorMeshPrefabTest s_EditorMeshPrefabTest;

namespace
{
  void GetComponentTypes(const WDocumentObject* pObject, WDynamicArray<WString>& out_types)
  {
    out_types.Clear();

    for (const WDocumentObject* pChild : pObject->GetChildren())
    {
      if (pChild->GetParentProperty() == "Components"_wsv)
      {
        out_types.PushBack(pChild->GetType()->GetTypeName());
      }
    }
  }

  const WDocumentObject* FindComponent(const WDocumentObject* pObject, WStringView sType)
  {
    for (const WDocumentObject* pChild : pObject->GetChildren())
    {
      if (pChild->GetParentProperty() == "Components"_wsv && pChild->GetType()->GetTypeName() == sType)
        return pChild;
    }

    return nullptr;
  }

  WDynamicArray<const WDocumentObject*> GetChildObjects(const WDocumentObject* pObject)
  {
    WDynamicArray<const WDocumentObject*> res;

    for (const WDocumentObject* pChild : pObject->GetChildren())
    {
      if (pChild->GetParentProperty() == "Children"_wsv)
        res.PushBack(pChild);
    }

    return res;
  }
} // namespace

const char* WEditorMeshPrefabTest::GetTestName() const
{
  return "Mesh Prefab Tests";
}

void WEditorMeshPrefabTest::SetupSubTests()
{
  AddSubTest("Simple Mesh", SubTests::ST_SimpleMesh);
  AddSubTest("LOD Mesh", SubTests::ST_LodMesh);
  AddSubTest("Box Collider", SubTests::ST_BoxCollider);
  AddSubTest("Collision Mesh", SubTests::ST_CollisionMesh);
  AddSubTest("Convex And Defaults", SubTests::ST_ConvexAndDefaults);
  AddSubTest("Existing Prefab", SubTests::ST_ExistingPrefab);
  AddSubTest("Keeps Open Documents", SubTests::ST_KeepsOpenDocuments);
  AddSubTest("Data Dir Relative Path", SubTests::ST_DataDirRelativePath);
  AddSubTest("Multiple Prefabs", SubTests::ST_MultiplePrefabs);
  AddSubTest("Animated LOD Component", SubTests::ST_AnimatedLodComponent);
}

WResult WEditorMeshPrefabTest::InitializeTest()
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

WResult WEditorMeshPrefabTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WTestAppRun WEditorMeshPrefabTest::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_SimpleMesh:
      SimpleMesh();
      break;
    case SubTests::ST_LodMesh:
      LodMesh();
      break;
    case SubTests::ST_BoxCollider:
      BoxCollider();
      break;
    case SubTests::ST_CollisionMesh:
      CollisionMesh();
      break;
    case SubTests::ST_ConvexAndDefaults:
      ConvexAndDefaults();
      break;
    case SubTests::ST_ExistingPrefab:
      ExistingPrefab();
      break;
    case SubTests::ST_KeepsOpenDocuments:
      KeepsOpenDocuments();
      break;
    case SubTests::ST_DataDirRelativePath:
      PrefabDataDirRelativePath();
      break;
    case SubTests::ST_MultiplePrefabs:
      MultiplePrefabs();
      break;
    case SubTests::ST_AnimatedLodComponent:
      AnimatedLodComponent();
      break;
  }

  return WTestAppRun::Quit;
}

WString WEditorMeshPrefabTest::MakePrivateSourceMesh(const char* szName)
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

WUuid WEditorMeshPrefabTest::CreateMeshAsset(const char* szRelativePath, WUInt8 uiSimplification, const char* szSourceFile)
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

    if (uiSimplification > 0)
    {
      pAcc->SetValueByName(pProps, "SimplifyMesh", true).AssertSuccess();
      pAcc->SetValueByName(pProps, "MeshSimplification", uiSimplification).AssertSuccess();
    }

    pAcc->FinishTransaction();
  }

  pDoc->SaveDocument(true).AssertSuccess();

  const WUuid guid = pDoc->GetGuid();
  pDoc->GetDocumentManager()->CloseDocument(pDoc);

  ProcessEvents(10);
  WAssetCurator::GetSingleton()->MainThreadTick(true);

  // Bounds are recorded by a transform, under the asset hash current at that moment. The curator
  // keeps rehashing a newly created asset while indexing it, so a single transform tends to write
  // them under a hash that is stale by the time they are read. Retry until they can be read back.
  // Only this asset is transformed, transforming the generated prefabs would log unrelated errors.
  for (WUInt32 i = 0; i < 10; ++i)
  {
    WTransformStatus transformRes = WAssetCurator::GetSingleton()->TransformAsset(guid, WTransformFlags::TriggeredManually);
    W_IGNORE_UNUSED(transformRes);
    ProcessEvents(5);
    WAssetCurator::GetSingleton()->MainThreadTick(true);

    auto pCheck = WAssetCurator::GetSingleton()->GetSubAsset(guid);
    if (pCheck.isValid() && pCheck->m_pAssetInfo != nullptr && pCheck->m_pAssetInfo->GetTransformInfo() != nullptr)
      break;
  }

  return guid;
}

void WEditorMeshPrefabTest::SimpleMesh()
{
  const WUuid meshGuid = CreateMeshAsset("MeshPrefab/Simple.WMeshAsset");
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshPrefabSource source;
  if (!W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  W_TEST_BOOL(!source.m_bAnimated);
  W_TEST_BOOL(source.m_LodGuids.IsEmpty());
  W_TEST_BOOL_MSG(source.m_bHasBounds, "The mesh was transformed, so its bounds should be known.");
  W_TEST_BOOL(source.GetDefaultRenderComponentType() == "WMeshComponent"_wsv);

  WStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefab/Simple.WPrefab");

  WMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = source.GetDefaultRenderComponentType();
  options.m_bOpenAfterCreate = false;

  if (!W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  W_TEST_BOOL(WOSFile::ExistsFile(sPrefabPath));

  WDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, WDocumentFlags::None);
  if (!W_TEST_BOOL(pPrefab != nullptr))
    return;

  W_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

  const WDocumentObject* pRoot = pPrefab->GetObjectManager()->GetRootObject();
  auto topLevel = GetChildObjects(pRoot);
  if (!W_TEST_INT(topLevel.GetCount(), 1))
    return;

  const WDocumentObject* pPrefabRoot = topLevel[0];
  W_TEST_STRING(pPrefabRoot->GetTypeAccessor().GetValue("Name").ConvertTo<WString>(), "<Prefab-Root>");

  const WDocumentObject* pMeshComp = FindComponent(pPrefabRoot, "WMeshComponent");
  if (!W_TEST_BOOL(pMeshComp != nullptr))
    return;

  WStringBuilder sExpectedRef;
  sExpectedRef.SetFormat("{}", meshGuid);
  W_TEST_STRING(pMeshComp->GetTypeAccessor().GetValue("Mesh").ConvertTo<WString>(), sExpectedRef);
}

void WEditorMeshPrefabTest::LodMesh()
{
  // the layout the mesh import produces: the main asset plus LOD-N assets in a sibling _data folder
  const WUuid meshGuid = CreateMeshAsset("MeshPrefab/Lod.WMeshAsset");
  const WUuid lod1Guid = CreateMeshAsset("MeshPrefab/Lod_data/LOD-1.WMeshAsset", 50);
  const WUuid lod2Guid = CreateMeshAsset("MeshPrefab/Lod_data/LOD-2.WMeshAsset", 75);

  if (!W_TEST_BOOL(meshGuid.IsValid() && lod1Guid.IsValid() && lod2Guid.IsValid()))
    return;

  WMeshPrefabSource source;
  if (!W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  if (!W_TEST_INT(source.m_LodGuids.GetCount(), 2))
    return;

  W_TEST_BOOL(source.m_LodGuids[0] == lod1Guid);
  W_TEST_BOOL(source.m_LodGuids[1] == lod2Guid);
  W_TEST_BOOL(source.GetDefaultRenderComponentType() == "WLodMeshComponent"_wsv);

  // A mesh asset renamed after import keeps its original _data folder, named after the source model
  // file. The LODs still have to be found.
  {
    WStringBuilder sRenamedDir = m_sProjectPath;
    sRenamedDir.AppendPath("MeshPrefabRenamed");
    WOSFile::CreateDirectoryStructure(sRenamedDir).AssertSuccess();

    const WUuid renamedGuid = CreateMeshAsset("MeshPrefabRenamed/Renamed.WMeshAsset");
    const WUuid renamedLod1 = CreateMeshAsset("MeshPrefabRenamed/Cube_data/LOD-1.WMeshAsset", 50);

    if (W_TEST_BOOL(renamedGuid.IsValid() && renamedLod1.IsValid()))
    {
      WMeshPrefabSource renamedSource;
      if (W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(renamedGuid, renamedSource).Succeeded()))
      {
        if (W_TEST_INT(renamedSource.m_LodGuids.GetCount(), 1))
        {
          W_TEST_BOOL(renamedSource.m_LodGuids[0] == renamedLod1);
        }
      }
    }
  }

  WStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefab/Lod.WPrefab");

  WMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = source.GetDefaultRenderComponentType();
  options.m_bOpenAfterCreate = false;

  if (!W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  WDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, WDocumentFlags::None);
  if (!W_TEST_BOOL(pPrefab != nullptr))
    return;

  W_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

  const WDocumentObject* pPrefabRoot = GetChildObjects(pPrefab->GetObjectManager()->GetRootObject())[0];
  const WDocumentObject* pLodComp = FindComponent(pPrefabRoot, "WLodMeshComponent");
  if (!W_TEST_BOOL(pLodComp != nullptr))
    return;

  // LOD 0 is the mesh asset itself, so there is one entry more than there are LOD assets
  WHybridArray<const WDocumentObject*, 4> lods;
  for (const WDocumentObject* pChild : pLodComp->GetChildren())
  {
    if (pChild->GetParentProperty() == "Meshes"_wsv)
      lods.PushBack(pChild);
  }

  if (!W_TEST_INT(lods.GetCount(), 3))
    return;

  const WUuid expected[] = {meshGuid, lod1Guid, lod2Guid};
  float fPrevThreshold = 2.0f;

  for (WUInt32 i = 0; i < 3; ++i)
  {
    WStringBuilder sExpectedRef;
    sExpectedRef.SetFormat("{}", expected[i]);
    W_TEST_STRING(lods[i]->GetTypeAccessor().GetValue("Mesh").ConvertTo<WString>(), sExpectedRef);

    // thresholds have to decrease, otherwise the component never switches LOD
    const float fThreshold = lods[i]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>();
    W_TEST_BOOL(fThreshold < fPrevThreshold);
    fPrevThreshold = fThreshold;
  }

  W_TEST_FLOAT(lods[0]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>(), 0.2f, 0.001f);
  W_TEST_FLOAT(lods[1]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>(), 0.1f, 0.001f);

  // the last LOD has to reach all the way out, or the mesh disappears in the distance
  W_TEST_FLOAT(lods[2]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>(), 0.0f, 0.001f);

  const float fRadius = pLodComp->GetTypeAccessor().GetValue("BoundsRadius").ConvertTo<float>();
  W_TEST_FLOAT(fRadius, source.m_fBoundsRadius, 0.001f);
  W_TEST_BOOL_MSG(fRadius != 1.0f, "The bounds radius should come from the mesh, not stay at the default.");
}

void WEditorMeshPrefabTest::BoxCollider()
{
  if (!WMeshPrefabCreator::IsPhysicsAvailable())
  {
    WLog::Info("Jolt is not available, skipping the collider test.");
    return;
  }

  const WUuid meshGuid = CreateMeshAsset("MeshPrefab/Box.WMeshAsset");
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshPrefabSource source;
  if (!W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  if (!W_TEST_BOOL_MSG(source.m_bHasBounds, "A box collider cannot be sized without bounds."))
    return;

  WStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefab/Box.WPrefab");

  WMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = "WMeshComponent";
  options.m_Physics = WMeshPrefabPhysics::StaticBox;
  options.m_uiCollisionLayer = 3;
  options.m_bOpenAfterCreate = false;

  if (!W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  WDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, WDocumentFlags::None);
  if (!W_TEST_BOOL(pPrefab != nullptr))
    return;

  W_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

  const WDocumentObject* pPrefabRoot = GetChildObjects(pPrefab->GetObjectManager()->GetRootObject())[0];

  const WDocumentObject* pActor = FindComponent(pPrefabRoot, "WJoltStaticActorComponent");
  if (!W_TEST_BOOL(pActor != nullptr))
    return;

  W_TEST_INT(pActor->GetTypeAccessor().GetValue("CollisionLayer").ConvertTo<WUInt32>(), 3);

  auto children = GetChildObjects(pPrefabRoot);
  if (!W_TEST_INT(children.GetCount(), 1))
    return;

  const WDocumentObject* pShape = FindComponent(children[0], "WJoltShapeBoxComponent");
  if (!W_TEST_BOOL(pShape != nullptr))
    return;

  const WVec3 vHalfExtents = pShape->GetTypeAccessor().GetValue("HalfExtents").ConvertTo<WVec3>();
  W_TEST_VEC3(vHalfExtents, source.m_vBoundsHalfExtents, 0.001f);

  const WVec3 vPos = children[0]->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<WVec3>();
  W_TEST_VEC3(vPos, source.m_vBoundsCenter, 0.001f);
}

void WEditorMeshPrefabTest::CollisionMesh()
{
  if (!WMeshPrefabCreator::IsPhysicsAvailable())
  {
    WLog::Info("Jolt is not available, skipping the collision mesh test.");
    return;
  }

  const WUuid meshGuid = CreateMeshAsset("MeshPrefab/ColMesh.WMeshAsset");
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshPrefabSource source;
  if (!W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  W_TEST_STRING(source.m_sMeshFile, "Meshes/Cube.obj");

  WStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefab/ColMesh.WPrefab");

  WMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = "WMeshComponent";
  options.m_Physics = WMeshPrefabPhysics::StaticTriangleMesh;
  options.m_bOpenAfterCreate = false;

  if (!W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  WStringBuilder sColMeshPath = m_sProjectPath;
  sColMeshPath.AppendPath("MeshPrefab/ColMesh.WJoltCollisionMeshAsset");
  W_TEST_BOOL(WOSFile::ExistsFile(sColMeshPath));

  WDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, WDocumentFlags::None);
  if (!W_TEST_BOOL(pPrefab != nullptr))
    return;

  W_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

  const WDocumentObject* pPrefabRoot = GetChildObjects(pPrefab->GetObjectManager()->GetRootObject())[0];
  const WDocumentObject* pActor = FindComponent(pPrefabRoot, "WJoltStaticActorComponent");
  if (!W_TEST_BOOL(pActor != nullptr))
    return;

  // a triangle mesh collider needs no separate shape component, the actor references the mesh itself
  const WString sColMeshRef = pActor->GetTypeAccessor().GetValue("CollisionMesh").ConvertTo<WString>();
  W_TEST_BOOL_MSG(!sColMeshRef.IsEmpty(), "The actor should reference the generated collision mesh.");
  W_TEST_INT(GetChildObjects(pPrefabRoot).GetCount(), 0);

  // running it again has to reuse that asset rather than create a second one
  WStringBuilder sPrefabPath2 = m_sProjectPath;
  sPrefabPath2.AppendPath("MeshPrefab/ColMesh2.WPrefab");

  WMeshPrefabOptions options2 = options;
  options2.m_sPrefabPath = sPrefabPath2;

  if (!W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefab(source, options2).Succeeded()))
    return;

  WDocument* pPrefab2 = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath2, WDocumentFlags::None);
  if (!W_TEST_BOOL(pPrefab2 != nullptr))
    return;

  W_SCOPE_EXIT(pPrefab2->GetDocumentManager()->CloseDocument(pPrefab2));

  const WDocumentObject* pPrefabRoot2 = GetChildObjects(pPrefab2->GetObjectManager()->GetRootObject())[0];
  const WDocumentObject* pActor2 = FindComponent(pPrefabRoot2, "WJoltStaticActorComponent");
  if (!W_TEST_BOOL(pActor2 != nullptr))
    return;

  W_TEST_STRING(pActor2->GetTypeAccessor().GetValue("CollisionMesh").ConvertTo<WString>(), sColMeshRef);
}

void WEditorMeshPrefabTest::ConvexAndDefaults()
{
  if (!WMeshPrefabCreator::IsPhysicsAvailable())
  {
    WLog::Info("Jolt is not available, skipping the convex hull test.");
    return;
  }

  // Collision meshes are matched by source file, and every other mesh here is built from Cube.obj.
  // Without a private source this would find the colliders the other sub-tests generated.
  const WString sSource = MakePrivateSourceMesh("ConvexCube");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const WUuid meshGuid = CreateMeshAsset("MeshPrefabConvex/Convex.WMeshAsset", 0, sSource);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  {
    WMeshPrefabSource source;
    if (!W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
      return;

    W_TEST_BOOL(source.GetDefaultPhysics() == WMeshPrefabPhysics::None);
    W_TEST_BOOL(!source.m_ExistingConvexColMesh.IsValid());

    WStringBuilder sPrefabPath = m_sProjectPath;
    sPrefabPath.AppendPath("MeshPrefabConvex/Convex.WPrefab");

    WMeshPrefabOptions options;
    options.m_sPrefabPath = sPrefabPath;
    options.m_sRenderComponentType = "WMeshComponent";
    options.m_Physics = WMeshPrefabPhysics::StaticConvexHull;
    options.m_bOpenAfterCreate = false;

    if (!W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
      return;

    // a convex hull is a static actor plus a shape component, unlike a triangle mesh
    WDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, WDocumentFlags::None);
    if (!W_TEST_BOOL(pPrefab != nullptr))
      return;

    W_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

    const WDocumentObject* pPrefabRoot = GetChildObjects(pPrefab->GetObjectManager()->GetRootObject())[0];
    W_TEST_BOOL(FindComponent(pPrefabRoot, "WJoltStaticActorComponent") != nullptr);

    const WDocumentObject* pShape = FindComponent(pPrefabRoot, "WJoltShapeConvexHullComponent");
    if (!W_TEST_BOOL(pShape != nullptr))
      return;

    W_TEST_BOOL(!pShape->GetTypeAccessor().GetValue("CollisionMesh").ConvertTo<WString>().IsEmpty());
  }

  // the convex collision mesh now exists, so it should drive the default on a second run
  {
    WMeshPrefabSource source;
    if (!W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
      return;

    W_TEST_BOOL(source.m_ExistingConvexColMesh.IsValid());
    W_TEST_BOOL(source.GetDefaultPhysics() == WMeshPrefabPhysics::StaticConvexHull);
  }
}

void WEditorMeshPrefabTest::ExistingPrefab()
{
  const WUuid meshGuid = CreateMeshAsset("MeshPrefabExisting/Existing.WMeshAsset");
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshPrefabSource source;
  if (!W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  WStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefabExisting/Existing.WPrefab");

  WMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = "WMeshComponent";
  options.m_bOpenAfterCreate = false;

  if (!W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  const WStatus second = WMeshPrefabCreator::CreateMeshPrefab(source, options);
  W_TEST_BOOL_MSG(second.Failed(), "Creating a prefab over an existing one should fail, not overwrite it.");
  W_TEST_BOOL(!second.GetMessageString().IsEmpty());
}

void WEditorMeshPrefabTest::KeepsOpenDocuments()
{
  const WUuid meshGuid = CreateMeshAsset("MeshPrefabOpen/Open.WMeshAsset");
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WStringBuilder sMeshPath = m_sProjectPath;
  sMeshPath.AppendPath("MeshPrefabOpen/Open.WMeshAsset");

  // Opened without a window, which is what the curator does while transforming. Reading a property
  // out of it must not close it.
  WDocument* pOpened = m_pApplication->m_pEditorApp->OpenDocument(sMeshPath, WDocumentFlags::None);
  if (!W_TEST_BOOL(pOpened != nullptr))
    return;

  WMeshPrefabSource source;
  W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded());
  W_TEST_STRING(source.m_sMeshFile, "Meshes/Cube.obj");

  const WDocument* pStillOpen = WDocumentManager::GetDocumentByGuid(meshGuid);
  W_TEST_BOOL_MSG(pStillOpen == pOpened, "Gathering must not close a document that was already open.");

  if (pStillOpen == pOpened)
  {
    pOpened->GetDocumentManager()->CloseDocument(pOpened);
  }
}

void WEditorMeshPrefabTest::PrefabDataDirRelativePath()
{
  const WUuid meshGuid = CreateMeshAsset("MeshPrefab/RelPath.WMeshAsset");
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshPrefabSource source;
  if (!W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  const WString sAbsolute = WMeshPrefabCreator::SuggestPrefabPath(source);
  const WString sDisplay = WMeshPrefabCreator::MakeDisplayPath(sAbsolute);

  // What the dialog shows has to be the short form, not the full path off the drive root.
  W_TEST_BOOL_MSG(!WPathUtils::IsAbsolutePath(sDisplay), "The display path must be relative.");
  W_TEST_BOOL_MSG(sDisplay.FindSubString("MeshPrefab/RelPath") != nullptr, "The display path must still name the file.");

  // and it has to round trip, or the dialog cannot hand it back to the creator
  {
    WStringBuilder sResolved;
    W_TEST_BOOL(WMeshPrefabCreator::ResolveDisplayPath(sDisplay, sResolved).Succeeded());
    W_TEST_STRING(sResolved, sAbsolute);
  }

  // an absolute path stays valid input, which is what the file browse button produces
  {
    WStringBuilder sResolved;
    W_TEST_BOOL(WMeshPrefabCreator::ResolveDisplayPath(sAbsolute, sResolved).Succeeded());
    W_TEST_STRING(sResolved, sAbsolute);
  }

  // a path that names no data directory has to be refused rather than written somewhere unexpected
  {
    WStringBuilder sResolved;
    W_TEST_BOOL(WMeshPrefabCreator::ResolveDisplayPath("NoSuchDataDir/Thing.WPrefab", sResolved).Failed());
    W_TEST_BOOL(WMeshPrefabCreator::ResolveDisplayPath("", sResolved).Failed());
  }

  // creating from the relative form has to put the file exactly where the absolute form would
  {
    WMeshPrefabOptions options;
    options.m_sPrefabPath = sDisplay;
    options.m_bOpenAfterCreate = false;

    if (!W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
      return;

    W_TEST_BOOL(WOSFile::ExistsFile(sAbsolute));
  }

  // an unresolvable path must fail instead of creating something
  {
    WMeshPrefabOptions options;
    options.m_sPrefabPath = "NoSuchDataDir/Thing.WPrefab";
    options.m_bOpenAfterCreate = false;

    W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefab(source, options).Failed());
  }
}

void WEditorMeshPrefabTest::MultiplePrefabs()
{
  WHybridArray<WUuid, 4> meshes;
  WHybridArray<WString, 4> expectedPrefabs;

  for (WUInt32 i = 0; i < 3; ++i)
  {
    WStringBuilder sAssetPath;
    sAssetPath.SetFormat("MeshPrefabBatch/Batch{}.WMeshAsset", i);

    const WUuid guid = CreateMeshAsset(sAssetPath);
    if (!W_TEST_BOOL(guid.IsValid()))
      return;

    meshes.PushBack(guid);

    WStringBuilder sPrefab = m_sProjectPath;
    sPrefab.AppendPath(sAssetPath);
    sPrefab.ChangeFileExtension("WPrefab");
    expectedPrefabs.PushBack(sPrefab);
  }

  WMeshPrefabOptions options;
  options.m_bOpenAfterCreate = false;

  // no path: each prefab has to end up next to its own mesh
  {
    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    if (!W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefabs(meshes, options, uiCreated, uiSkipped).Succeeded()))
      return;

    W_TEST_INT(uiCreated, 3);
    W_TEST_INT(uiSkipped, 0);

    for (const WString& sPrefab : expectedPrefabs)
    {
      W_TEST_BOOL_MSG(WOSFile::ExistsFile(sPrefab), "Every mesh has to get a prefab at its own default path.");
    }
  }

  ProcessEvents(10);
  WAssetCurator::GetSingleton()->MainThreadTick(true);

  // Running it again must not pile up numbered duplicates. A mesh that already has a prefab is done,
  // which is why such a mesh is skipped rather than failing the run.
  {
    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    if (!W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefabs(meshes, options, uiCreated, uiSkipped).Succeeded()))
      return;

    W_TEST_INT(uiCreated, 0);
    W_TEST_INT(uiSkipped, 3);

    for (const WString& sPrefab : expectedPrefabs)
    {
      WStringBuilder sSecondName(WPathUtils::GetFileName(sPrefab), "2");

      WStringBuilder sSecond = sPrefab;
      sSecond.ChangeFileName(sSecondName);

      W_TEST_BOOL_MSG(!WOSFile::ExistsFile(sSecond), "A second run must not create a numbered duplicate.");
    }
  }

  // A guid that is not a mesh asset is a skip, not a failure - a selection can hold anything.
  {
    WHybridArray<WUuid, 2> mixed;
    mixed.PushBack(WUuid::MakeUuid());

    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefabs(mixed, options, uiCreated, uiSkipped).Succeeded());
    W_TEST_INT(uiCreated, 0);
    W_TEST_INT(uiSkipped, 1);
  }
}

void WEditorMeshPrefabTest::AnimatedLodComponent()
{
  // The LOD folder is found by the mesh asset name, so this mesh needs a name of its own.
  const WUuid meshGuid = CreateMeshAsset("MeshPrefabAnimLod/AnimLod.WMeshAsset");
  const WUuid lod1Guid = CreateMeshAsset("MeshPrefabAnimLod/AnimLod_data/LOD-1.WMeshAsset", 50);

  if (!W_TEST_BOOL(meshGuid.IsValid() && lod1Guid.IsValid()))
    return;

  WMeshPrefabSource source;
  if (!W_TEST_BOOL(WMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  if (!W_TEST_INT(source.m_LodGuids.GetCount(), 1))
    return;

  // A static mesh with LODs still defaults to the static LOD component.
  W_TEST_BOOL(source.GetDefaultRenderComponentType() == "WLodMeshComponent"_wsv);

  // The animated LOD component is only offered for animated meshes. What matters here is that it is
  // built correctly when asked for, including the LOD element type, which differs from the static one.
  WStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefabAnimLod/AnimLod.WPrefab");

  WMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = "WLodAnimatedMeshComponent";
  options.m_bOpenAfterCreate = false;

  if (!W_TEST_BOOL(WMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  WDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, WDocumentFlags::None);
  if (!W_TEST_BOOL(pPrefab != nullptr))
    return;

  W_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

  auto topLevel = GetChildObjects(pPrefab->GetObjectManager()->GetRootObject());
  if (!W_TEST_INT(topLevel.GetCount(), 1))
    return;

  const WDocumentObject* pComp = FindComponent(topLevel[0], "WLodAnimatedMeshComponent");
  if (!W_TEST_BOOL(pComp != nullptr))
    return;

  // LOD 0 is the mesh itself, LOD 1 is the sibling that was found
  WHybridArray<const WDocumentObject*, 4> lods;
  for (const WDocumentObject* pChild : pComp->GetChildren())
  {
    if (pChild->GetParentProperty() == "Meshes"_wsv)
      lods.PushBack(pChild);
  }

  if (!W_TEST_INT(lods.GetCount(), 2))
    return;

  // The two LOD components have separate element types with identical property names. Using the
  // static one here would build a document the animated component cannot read.
  W_TEST_STRING(lods[0]->GetTypeAccessor().GetType()->GetTypeName(), "WLodAnimatedMeshLod");

  WStringBuilder sExpectedRef;
  sExpectedRef.SetFormat("{}", meshGuid);
  W_TEST_STRING(lods[0]->GetTypeAccessor().GetValue("Mesh").ConvertTo<WString>(), sExpectedRef);

  sExpectedRef.SetFormat("{}", lod1Guid);
  W_TEST_STRING(lods[1]->GetTypeAccessor().GetValue("Mesh").ConvertTo<WString>(), sExpectedRef);

  // the last LOD has to reach out to the horizon, or the object disappears at a distance
  W_TEST_FLOAT(lods[1]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>(), 0.0f, 0.0001f);
  W_TEST_BOOL(lods[0]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>() > 0.0f);
}
