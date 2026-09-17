#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

class WEditorMeshPrefabTest : public WEditorTest
{
public:
  using SUPER = WEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_SimpleMesh,
    ST_LodMesh,
    ST_BoxCollider,
    ST_CollisionMesh,
    ST_ConvexAndDefaults,
    ST_ExistingPrefab,
    ST_KeepsOpenDocuments,
    ST_DataDirRelativePath,
    ST_MultiplePrefabs,
    ST_AnimatedLodComponent,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  /// Creates a mesh asset at the given project relative path and transforms it, so that its bounds
  /// are recorded.
  WUuid CreateMeshAsset(const char* szRelativePath, WUInt8 uiSimplification = 0, const char* szSourceFile = "Meshes/Cube.obj");

  /// Copies Cube.obj to a new name, so that a test can use a source file nothing else shares.
  /// Returns the project relative path to use as a mesh asset's MeshFile.
  WString MakePrivateSourceMesh(const char* szName);

  void SimpleMesh();
  void LodMesh();
  void BoxCollider();
  void CollisionMesh();
  void ConvexAndDefaults();
  void ExistingPrefab();
  void KeepsOpenDocuments();
  void PrefabDataDirRelativePath();
  void MultiplePrefabs();
  void AnimatedLodComponent();
};
