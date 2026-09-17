#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

// class WMaterialAssetDocument;

class WMaterialDocumentTest : public WEditorTest
{
public:
  using SUPER = WEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_CreateNewMaterialFromShader,
    ST_CreateNewMaterialFromBase,
    ST_CreateNewMaterialFromVSE,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  WResult CreateMaterial(const char* szSceneName);
  void CloseMaterial();
  const WDocumentObject* GetShaderProperties(const WDocumentObject* pMaterialProperties);
  void CaptureMaterialImage();

  void CreateMaterialFromShader();
  void CreateMaterialFromBase();
  void CreateMaterialFromVSE();

private:
  WAssetDocument* m_pDoc = nullptr;
  WUuid m_MaterialGuid;
};
