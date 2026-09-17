#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginJolt/Utils/MeshColliderCreator.h>
#include <EditorTest/MeshCollider/MeshColliderTest.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/Declarations.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static WEditorMeshColliderTest s_EditorMeshColliderTest;

const char* WEditorMeshColliderTest::GetTestName() const
{
  return "Mesh Collider Tests";
}

void WEditorMeshColliderTest::SetupSubTests()
{
  AddSubTest("Triangle Mesh", SubTests::ST_TriangleMesh);
  AddSubTest("Convex Mesh", SubTests::ST_ConvexMesh);
  AddSubTest("Transferred Settings", SubTests::ST_TransferredSettings);
  AddSubTest("Existing Collider", SubTests::ST_ExistingCollider);
  AddSubTest("Primitive Mesh", SubTests::ST_PrimitiveMesh);
  AddSubTest("Keeps Open Documents", SubTests::ST_KeepsOpenDocuments);
  AddSubTest("Simplification Settings", SubTests::ST_SimplificationSettings);
  AddSubTest("Data Dir Relative Path", SubTests::ST_DataDirRelativePath);
  AddSubTest("Multiple Meshes", SubTests::ST_MultipleMeshes);
  AddSubTest("Shared Source File", SubTests::ST_SharedSourceFile);
  AddSubTest("Surface", SubTests::ST_Surface);
}

WResult WEditorMeshColliderTest::InitializeTest()
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

WResult WEditorMeshColliderTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WTestAppRun WEditorMeshColliderTest::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_TriangleMesh:
      TriangleMesh();
      break;
    case SubTests::ST_ConvexMesh:
      ConvexMesh();
      break;
    case SubTests::ST_TransferredSettings:
      TransferredSettings();
      break;
    case SubTests::ST_ExistingCollider:
      ExistingCollider();
      break;
    case SubTests::ST_PrimitiveMesh:
      PrimitiveMesh();
      break;
    case SubTests::ST_KeepsOpenDocuments:
      KeepsOpenDocuments();
      break;
    case SubTests::ST_SimplificationSettings:
      SimplificationSettings();
      break;
    case SubTests::ST_DataDirRelativePath:
      DataDirRelativePath();
      break;
    case SubTests::ST_MultipleMeshes:
      MultipleMeshes();
      break;
    case SubTests::ST_Surface:
      Surface();
      break;
    case SubTests::ST_SharedSourceFile:
      SharedSourceFile();
      break;
  }

  return WTestAppRun::Quit;
}

WString WEditorMeshColliderTest::MakePrivateSourceMesh(const char* szName)
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

WUuid WEditorMeshColliderTest::CreateMeshAsset(const char* szRelativePath, const char* szSourceFile, const WVariantDictionary* pExtraProperties)
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

WVariant WEditorMeshColliderTest::ReadColliderProperty(WStringView sAbsPath, WStringView sProperty)
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

void WEditorMeshColliderTest::TriangleMesh()
{
  // Colliders are matched to a mesh by source file, so every sub-test needs its own copy of the
  // source, or it finds the colliders that the other sub-tests generated.
  const WString sSource = MakePrivateSourceMesh("ColliderTriangle");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const WUuid meshGuid = CreateMeshAsset("MeshCollider/Triangle.WMeshAsset", sSource);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshColliderSource source;
  if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  W_TEST_BOOL(!source.m_bAnimated);
  W_TEST_BOOL(!source.m_bIsPrimitive);
  W_TEST_STRING(source.m_sMeshFile, sSource);
  W_TEST_BOOL(!source.GetExisting(WMeshColliderKind::TriangleMesh).IsValid());

  const WString sSuggested = WMeshColliderCreator::SuggestColliderPath(source, WMeshColliderKind::TriangleMesh);
  W_TEST_STRING(WPathUtils::GetFileExtension(sSuggested), "WJoltCollisionMeshAsset");

  WMeshColliderOptions options;
  options.m_sColliderPath = sSuggested;
  options.m_Kind = WMeshColliderKind::TriangleMesh;
  options.m_bOpenAfterCreate = false;

  if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
    return;

  if (!W_TEST_BOOL(WOSFile::ExistsFile(sSuggested)))
    return;

  // the source file is what makes the collider describe the same geometry as the mesh
  W_TEST_STRING(ReadColliderProperty(sSuggested, "MeshFile").ConvertTo<WString>(), sSource);

  // a triangle mesh document must not be flagged as convex, or it builds the wrong shape
  W_TEST_BOOL(ReadColliderProperty(sSuggested, "IsConvexMesh").ConvertTo<bool>() == false);
}

void WEditorMeshColliderTest::ConvexMesh()
{
  const WString sSource = MakePrivateSourceMesh("ColliderConvex");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const WUuid meshGuid = CreateMeshAsset("MeshCollider/Convex.WMeshAsset", sSource);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshColliderSource source;
  if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  const WString sSuggested = WMeshColliderCreator::SuggestColliderPath(source, WMeshColliderKind::ConvexHull);
  W_TEST_STRING(WPathUtils::GetFileExtension(sSuggested), "WJoltConvexCollisionMeshAsset");

  WMeshColliderOptions options;
  options.m_sColliderPath = sSuggested;
  options.m_Kind = WMeshColliderKind::ConvexHull;
  options.m_bOpenAfterCreate = false;

  if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
    return;

  if (!W_TEST_BOOL(WOSFile::ExistsFile(sSuggested)))
    return;

  W_TEST_STRING(ReadColliderProperty(sSuggested, "MeshFile").ConvertTo<WString>(), sSource);

  // the convex flag comes from the document type, not from the transferred properties
  W_TEST_BOOL(ReadColliderProperty(sSuggested, "IsConvexMesh").ConvertTo<bool>() == true);
}

void WEditorMeshColliderTest::TransferredSettings()
{
  const WString sSource = MakePrivateSourceMesh("ColliderSettings");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  // every value differs from the collision mesh asset's default, so a transferred value cannot be
  // mistaken for a coincidental match
  const WVec3 vOffset(1.0f, -2.0f, 3.5f);

  WVariantDictionary extras;
  extras.Insert("MeshIncludeTags", "Collide");
  extras.Insert("MeshExcludeTags", "NoCollide;UCX_");
  extras.Insert("ImportTransform", (WInt64)WMeshImportTransform::Custom);
  extras.Insert("RightDir", (WInt64)WBasisAxis::PositiveZ);
  extras.Insert("UpDir", (WInt64)WBasisAxis::NegativeY);
  extras.Insert("FlipForwardDir", true);
  extras.Insert("PositionOffset", vOffset);
  extras.Insert("UniformScaling", 5.0f);

  const WUuid meshGuid = CreateMeshAsset("MeshCollider/Settings.WMeshAsset", sSource, &extras);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshColliderSource source;
  if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  const WString sSuggested = WMeshColliderCreator::SuggestColliderPath(source, WMeshColliderKind::TriangleMesh);

  WMeshColliderOptions options;
  options.m_sColliderPath = sSuggested;
  options.m_Kind = WMeshColliderKind::TriangleMesh;
  options.m_bOpenAfterCreate = false;

  if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
    return;

  if (!W_TEST_BOOL(WOSFile::ExistsFile(sSuggested)))
    return;

  // the collider has to describe the same geometry, in the same place, at the same size, as the mesh
  W_TEST_STRING(ReadColliderProperty(sSuggested, "MeshFile").ConvertTo<WString>(), sSource);
  W_TEST_STRING(ReadColliderProperty(sSuggested, "MeshIncludeTags").ConvertTo<WString>(), "Collide");
  W_TEST_STRING(ReadColliderProperty(sSuggested, "MeshExcludeTags").ConvertTo<WString>(), "NoCollide;UCX_");
  W_TEST_INT(ReadColliderProperty(sSuggested, "ImportTransform").ConvertTo<WInt64>(), (WInt64)WMeshImportTransform::Custom);
  W_TEST_INT(ReadColliderProperty(sSuggested, "RightDir").ConvertTo<WInt64>(), (WInt64)WBasisAxis::PositiveZ);
  W_TEST_INT(ReadColliderProperty(sSuggested, "UpDir").ConvertTo<WInt64>(), (WInt64)WBasisAxis::NegativeY);
  W_TEST_BOOL(ReadColliderProperty(sSuggested, "FlipForwardDir").ConvertTo<bool>() == true);
  W_TEST_VEC3(ReadColliderProperty(sSuggested, "PositionOffset").ConvertTo<WVec3>(), vOffset, 0.0001f);
  W_TEST_FLOAT(ReadColliderProperty(sSuggested, "UniformScaling").ConvertTo<float>(), 5.0f, 0.0001f);
}

void WEditorMeshColliderTest::ExistingCollider()
{
  const WString sSource = MakePrivateSourceMesh("ColliderExisting");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const WUuid meshGuid = CreateMeshAsset("MeshCollider/Existing.WMeshAsset", sSource);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WString sColliderPath;

  {
    WMeshColliderSource source;
    if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
      return;

    W_TEST_BOOL(!source.GetExisting(WMeshColliderKind::TriangleMesh).IsValid());

    sColliderPath = WMeshColliderCreator::SuggestColliderPath(source, WMeshColliderKind::TriangleMesh);

    WMeshColliderOptions options;
    options.m_sColliderPath = sColliderPath;
    options.m_Kind = WMeshColliderKind::TriangleMesh;
    options.m_bOpenAfterCreate = false;

    if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    const WStatus second = WMeshColliderCreator::CreateMeshCollider(source, options);
    W_TEST_BOOL_MSG(second.Failed(), "Creating a collider over an existing file should fail, not overwrite it.");
    W_TEST_BOOL(!second.GetMessageString().IsEmpty());
  }

  ProcessEvents(10);
  WAssetCurator::GetSingleton()->MainThreadTick(true);

  // gathering again has to report the collider that now exists, so the dialog can point it out
  {
    WMeshColliderSource source;
    if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
      return;

    W_TEST_BOOL(source.GetExisting(WMeshColliderKind::TriangleMesh).IsValid());
    W_TEST_BOOL_MSG(!source.GetExisting(WMeshColliderKind::ConvexHull).IsValid(), "Only a triangle mesh was created.");
  }

  // and the suggested path must move on, rather than proposing a name that cannot be used
  {
    WMeshColliderSource source;
    WMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).AssertSuccess();

    const WString sNext = WMeshColliderCreator::SuggestColliderPath(source, WMeshColliderKind::TriangleMesh);
    W_TEST_BOOL(sNext != sColliderPath);
    W_TEST_BOOL(!WOSFile::ExistsFile(sNext));
  }
}

void WEditorMeshColliderTest::PrimitiveMesh()
{
  // A procedural primitive has no model file, so there is nothing to build a collision mesh from.
  // WMeshPrimitive lives in EditorPluginAssets, which this test does not link. 6 is Sphere.
  WVariantDictionary extras;
  extras.Insert("PrimitiveType", (WInt64)6);

  const WUuid meshGuid = CreateMeshAsset("MeshCollider/Primitive.WMeshAsset", "", &extras);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshColliderSource source;
  if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  W_TEST_BOOL_MSG(source.m_bIsPrimitive, "A non-File primitive type has to be recognized.");
  W_TEST_BOOL(source.m_sMeshFile.IsEmpty());

  WStringBuilder sColliderPath = m_sProjectPath;
  sColliderPath.AppendPath("MeshCollider/Primitive.WJoltCollisionMeshAsset");

  WMeshColliderOptions options;
  options.m_sColliderPath = sColliderPath;
  options.m_Kind = WMeshColliderKind::TriangleMesh;
  options.m_bOpenAfterCreate = false;

  const WStatus res = WMeshColliderCreator::CreateMeshCollider(source, options);
  W_TEST_BOOL_MSG(res.Failed(), "A primitive mesh has no source file, so this must fail.");
  W_TEST_BOOL_MSG(!WOSFile::ExistsFile(sColliderPath), "A failed creation must not leave a document behind.");
}

void WEditorMeshColliderTest::KeepsOpenDocuments()
{
  const WString sSource = MakePrivateSourceMesh("ColliderOpen");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const WUuid meshGuid = CreateMeshAsset("MeshColliderOpen/Open.WMeshAsset", sSource);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WStringBuilder sMeshPath = m_sProjectPath;
  sMeshPath.AppendPath("MeshColliderOpen/Open.WMeshAsset");

  // Opened without a window, which is what the curator does while transforming. Reading properties
  // out of it must not close it.
  WDocument* pOpened = m_pApplication->m_pEditorApp->OpenDocument(sMeshPath, WDocumentFlags::None);
  if (!W_TEST_BOOL(pOpened != nullptr))
    return;

  WMeshColliderSource source;
  W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded());
  W_TEST_STRING(source.m_sMeshFile, sSource);

  const WDocument* pStillOpen = WDocumentManager::GetDocumentByGuid(meshGuid);
  W_TEST_BOOL_MSG(pStillOpen == pOpened, "Gathering must not close a document that was already open.");

  if (pStillOpen == pOpened)
  {
    pOpened->GetDocumentManager()->CloseDocument(pOpened);
  }
}

void WEditorMeshColliderTest::SimplificationSettings()
{
  const WString sSource = MakePrivateSourceMesh("ColliderSimplify");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  // Again every value differs from the collision mesh asset's default. MaxSimplificationError
  // defaults to 5 on the mesh asset but to 20 on the collision mesh asset.
  WVariantDictionary extras;
  extras.Insert("SimplifyMesh", true);
  extras.Insert("MeshSimplification", (WInt64)33);
  extras.Insert("MaxSimplificationError", (WInt64)7);
  extras.Insert("NormalWeight", 0.75f);
  extras.Insert("AggressiveSimplification", true);

  const WUuid meshGuid = CreateMeshAsset("MeshCollider/Simplify.WMeshAsset", sSource, &extras);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshColliderSource source;
  if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  // A triangle mesh keeps the source geometry, so simplifying it changes the collision shape.
  {
    const WString sPath = WMeshColliderCreator::SuggestColliderPath(source, WMeshColliderKind::TriangleMesh);

    WMeshColliderOptions options;
    options.m_sColliderPath = sPath;
    options.m_Kind = WMeshColliderKind::TriangleMesh;
    options.m_bOpenAfterCreate = false;

    if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    W_TEST_BOOL(ReadColliderProperty(sPath, "SimplifyMesh").ConvertTo<bool>() == true);
    W_TEST_INT(ReadColliderProperty(sPath, "MeshSimplification").ConvertTo<WInt64>(), 33);
    W_TEST_INT(ReadColliderProperty(sPath, "MaxSimplificationError").ConvertTo<WInt64>(), 7);
    W_TEST_FLOAT(ReadColliderProperty(sPath, "NormalWeight").ConvertTo<float>(), 0.75f, 0.0001f);
    W_TEST_BOOL(ReadColliderProperty(sPath, "AggressiveSimplification").ConvertTo<bool>() == true);
  }

  // A convex hull is built from the hull of the vertices, so simplifying the source first changes
  // nothing about it. The collision mesh asset hides these options for a convex mesh.
  {
    const WString sPath = WMeshColliderCreator::SuggestColliderPath(source, WMeshColliderKind::ConvexHull);

    WMeshColliderOptions options;
    options.m_sColliderPath = sPath;
    options.m_Kind = WMeshColliderKind::ConvexHull;
    options.m_bOpenAfterCreate = false;

    if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    W_TEST_BOOL_MSG(ReadColliderProperty(sPath, "SimplifyMesh").ConvertTo<bool>() == false, "A convex hull must not take over the simplification settings.");
    W_TEST_INT(ReadColliderProperty(sPath, "MeshSimplification").ConvertTo<WInt64>(), 50);

    // but the geometry settings still have to be there, this is not a general opt out
    W_TEST_STRING(ReadColliderProperty(sPath, "MeshFile").ConvertTo<WString>(), sSource);
  }
}

void WEditorMeshColliderTest::DataDirRelativePath()
{
  const WString sSource = MakePrivateSourceMesh("ColliderRelPath");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const WUuid meshGuid = CreateMeshAsset("MeshCollider/RelPath.WMeshAsset", sSource);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshColliderSource source;
  if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  const WString sAbsolute = WMeshColliderCreator::SuggestColliderPath(source, WMeshColliderKind::TriangleMesh);
  const WString sDisplay = WMeshColliderCreator::MakeDisplayPath(sAbsolute);

  // What the dialog shows has to be the short form, not the full path off the drive root.
  W_TEST_BOOL_MSG(!WPathUtils::IsAbsolutePath(sDisplay), "The display path must be relative.");
  W_TEST_BOOL_MSG(sDisplay.FindSubString("MeshCollider/RelPath") != nullptr, "The display path must still name the file.");

  // and it has to round trip, or the dialog cannot hand it back to the creator
  {
    WStringBuilder sResolved;
    W_TEST_BOOL(WMeshColliderCreator::ResolveDisplayPath(sDisplay, sResolved).Succeeded());
    W_TEST_STRING(sResolved, sAbsolute);
  }

  // an absolute path stays valid input, which is what the file browse button produces
  {
    WStringBuilder sResolved;
    W_TEST_BOOL(WMeshColliderCreator::ResolveDisplayPath(sAbsolute, sResolved).Succeeded());
    W_TEST_STRING(sResolved, sAbsolute);
  }

  // a path that names no data directory has to be refused rather than written somewhere unexpected
  {
    WStringBuilder sResolved;
    W_TEST_BOOL(WMeshColliderCreator::ResolveDisplayPath("NoSuchDataDir/Thing.WJoltCollisionMeshAsset", sResolved).Failed());
    W_TEST_BOOL(WMeshColliderCreator::ResolveDisplayPath("", sResolved).Failed());
  }

  // and creating from the relative form has to put the file exactly where the absolute form would
  {
    WMeshColliderOptions options;
    options.m_sColliderPath = sDisplay;
    options.m_Kind = WMeshColliderKind::TriangleMesh;
    options.m_bOpenAfterCreate = false;

    if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    W_TEST_BOOL(WOSFile::ExistsFile(sAbsolute));
  }

  // an unresolvable path must fail instead of creating something
  {
    WMeshColliderOptions options;
    options.m_sColliderPath = "NoSuchDataDir/Thing.WJoltCollisionMeshAsset";
    options.m_Kind = WMeshColliderKind::TriangleMesh;
    options.m_bOpenAfterCreate = false;

    W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(source, options).Failed());
  }
}

void WEditorMeshColliderTest::MultipleMeshes()
{
  WHybridArray<WUuid, 4> meshes;
  WHybridArray<WString, 4> expectedColliders;

  for (WUInt32 i = 0; i < 3; ++i)
  {
    WStringBuilder sName;
    sName.SetFormat("ColliderBatch{}", i);

    const WString sSource = MakePrivateSourceMesh(sName);
    if (!W_TEST_BOOL(!sSource.IsEmpty()))
      return;

    WStringBuilder sAssetPath;
    sAssetPath.SetFormat("MeshCollider/Batch{}.WMeshAsset", i);

    const WUuid guid = CreateMeshAsset(sAssetPath, sSource);
    if (!W_TEST_BOOL(guid.IsValid()))
      return;

    meshes.PushBack(guid);

    WStringBuilder sCollider = m_sProjectPath;
    sCollider.AppendPath(sAssetPath);
    sCollider.ChangeFileExtension("WJoltCollisionMeshAsset");
    expectedColliders.PushBack(sCollider);
  }

  WMeshColliderOptions options;
  options.m_Kind = WMeshColliderKind::TriangleMesh;
  options.m_bOpenAfterCreate = false;

  // no path: each collider has to end up next to its own mesh
  {
    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshColliders(meshes, options, uiCreated, uiSkipped).Succeeded()))
      return;

    W_TEST_INT(uiCreated, 3);
    W_TEST_INT(uiSkipped, 0);

    for (const WString& sCollider : expectedColliders)
    {
      W_TEST_BOOL_MSG(WOSFile::ExistsFile(sCollider), "Every mesh has to get a collider at its own default path.");
    }
  }

  ProcessEvents(10);
  WAssetCurator::GetSingleton()->MainThreadTick(true);

  // running it again must not pile up numbered duplicates, a mesh that already has a collider is done
  {
    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshColliders(meshes, options, uiCreated, uiSkipped).Succeeded()))
      return;

    W_TEST_INT(uiCreated, 0);
    W_TEST_INT(uiSkipped, 3);

    for (const WString& sCollider : expectedColliders)
    {
      WStringBuilder sSecondName(WPathUtils::GetFileName(sCollider), "2");

      WStringBuilder sSecond = sCollider;
      sSecond.ChangeFileName(sSecondName);

      W_TEST_BOOL_MSG(!WOSFile::ExistsFile(sSecond), "A second run must not create a numbered duplicate.");
    }
  }

  // A convex collider is a different kind, so none of them exists yet and all three are created.
  {
    WMeshColliderOptions convex = options;
    convex.m_Kind = WMeshColliderKind::ConvexHull;

    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshColliders(meshes, convex, uiCreated, uiSkipped).Succeeded()))
      return;

    W_TEST_INT(uiCreated, 3);
    W_TEST_INT(uiSkipped, 0);
  }

  // A guid that is not a mesh asset is a skip, not a failure - a selection can hold anything.
  {
    WHybridArray<WUuid, 2> mixed;
    mixed.PushBack(WUuid::MakeUuid());

    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    W_TEST_BOOL(WMeshColliderCreator::CreateMeshColliders(mixed, options, uiCreated, uiSkipped).Succeeded());
    W_TEST_INT(uiCreated, 0);
    W_TEST_INT(uiSkipped, 1);
  }

  // A path in the options is meant for a single collider and must not make every mesh write to it.
  {
    WMeshColliderOptions withPath = options;
    withPath.m_sColliderPath = expectedColliders[0];

    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    W_TEST_BOOL(WMeshColliderCreator::CreateMeshColliders(meshes, withPath, uiCreated, uiSkipped).Succeeded());
    W_TEST_INT_MSG(uiCreated, 0, "Everything already exists, so nothing may be created.");
    W_TEST_INT(uiSkipped, 3);
  }
}

void WEditorMeshColliderTest::SharedSourceFile()
{
  // Two mesh assets importing different sub-meshes out of one model file, as is common for asset
  // packs. Their colliders both depend on that file, so the file alone cannot tell them apart.
  const WString sSource = MakePrivateSourceMesh("ColliderShared");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  WVariantDictionary propsA;
  propsA.Insert("MeshIncludeTags", WVariant(WString("part_a")));

  WVariantDictionary propsB;
  propsB.Insert("MeshIncludeTags", WVariant(WString("part_b")));

  const WUuid meshA = CreateMeshAsset("MeshCollider/SharedA.WMeshAsset", sSource, &propsA);
  const WUuid meshB = CreateMeshAsset("MeshCollider/SharedB.WMeshAsset", sSource, &propsB);
  if (!W_TEST_BOOL(meshA.IsValid() && meshB.IsValid()))
    return;

  // give A a collider, B none
  WString sColliderA;
  {
    WMeshColliderSource source;
    if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshA, source).Succeeded()))
      return;

    sColliderA = WMeshColliderCreator::SuggestColliderPath(source, WMeshColliderKind::TriangleMesh);

    WMeshColliderOptions options;
    options.m_sColliderPath = sColliderA;
    options.m_Kind = WMeshColliderKind::TriangleMesh;
    options.m_bOpenAfterCreate = false;

    if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;
  }

  ProcessEvents(10);
  WAssetCurator::GetSingleton()->MainThreadTick(true);

  {
    WMeshColliderSource source;
    if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshA, source).Succeeded()))
      return;

    W_TEST_BOOL_MSG(source.GetExisting(WMeshColliderKind::TriangleMesh).IsValid(), "The mesh the collider was built from has to find it.");
  }

  {
    WMeshColliderSource source;
    if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshB, source).Succeeded()))
      return;

    W_TEST_BOOL_MSG(!source.GetExisting(WMeshColliderKind::TriangleMesh).IsValid(), "A collider for a different sub-mesh must not be picked up.");
  }
}

void WEditorMeshColliderTest::Surface()
{
  const WString sSource = MakePrivateSourceMesh("ColliderSurface");
  if (!W_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const WUuid meshGuid = CreateMeshAsset("MeshCollider/Surface.WMeshAsset", sSource);
  if (!W_TEST_BOOL(meshGuid.IsValid()))
    return;

  WMeshColliderSource source;
  if (!W_TEST_BOOL(WMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  // The surface is stored as an asset reference, which is a guid. Any guid does, it just has to
  // arrive in the document unchanged.
  WStringBuilder sSurface;
  WConversionUtils::ToString(WUuid::MakeUuid(), sSurface);

  {
    const WString sPath = WMeshColliderCreator::SuggestColliderPath(source, WMeshColliderKind::ConvexHull);

    WMeshColliderOptions options;
    options.m_sColliderPath = sPath;
    options.m_Kind = WMeshColliderKind::ConvexHull;
    options.m_sSurface = sSurface;
    options.m_bOpenAfterCreate = false;

    if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    W_TEST_STRING(ReadColliderProperty(sPath, "Surface").ConvertTo<WString>(), sSurface);
  }

  // A triangle mesh gets one surface per material slot of the model when it is transformed, so the
  // single "Surface" property is not its own and must be left alone.
  {
    const WString sPath = WMeshColliderCreator::SuggestColliderPath(source, WMeshColliderKind::TriangleMesh);

    WMeshColliderOptions options;
    options.m_sColliderPath = sPath;
    options.m_Kind = WMeshColliderKind::TriangleMesh;
    options.m_sSurface = sSurface;
    options.m_bOpenAfterCreate = false;

    if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    W_TEST_BOOL_MSG(ReadColliderProperty(sPath, "Surface").ConvertTo<WString>().IsEmpty(), "A triangle mesh has no single surface.");
  }

  // no surface chosen has to leave the property alone rather than writing an empty reference
  {
    WMeshColliderSource fresh;
    WMeshColliderCreator::GatherMeshColliderSource(meshGuid, fresh).AssertSuccess();

    const WString sPath = WMeshColliderCreator::SuggestColliderPath(fresh, WMeshColliderKind::ConvexHull);

    WMeshColliderOptions options;
    options.m_sColliderPath = sPath;
    options.m_Kind = WMeshColliderKind::ConvexHull;
    options.m_bOpenAfterCreate = false;

    if (!W_TEST_BOOL(WMeshColliderCreator::CreateMeshCollider(fresh, options).Succeeded()))
      return;

    W_TEST_BOOL(ReadColliderProperty(sPath, "Surface").ConvertTo<WString>().IsEmpty());
  }
}
