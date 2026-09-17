#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

class WEditorSceneDocumentTest : public WEditorTest
{
public:
  using SUPER = WEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_LayerOperations,
    ST_PrefabOperations,
    ST_ComponentOperations,
    ST_ObjectPropertyPath,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  WResult CreateSimpleScene(const char* szSceneName);
  void CloseSimpleScene();
  void LayerOperations();
  void PrefabOperations();
  void ComponentOperations();
  void ObjectPropertyPath();

  static void CheckHierarchy(WObjectAccessorBase* pAccessor, const WDocumentObject* pRoot, WDelegate<void(const WDocumentObject* pChild)> functor);

private:
  WScene2Document* m_pDoc = nullptr;
  WLayerDocument* m_pLayer = nullptr;
  WUuid m_SceneGuid;
  WUuid m_LayerGuid;
};
