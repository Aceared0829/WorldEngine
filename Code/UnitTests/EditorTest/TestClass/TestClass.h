#pragma once

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <QApplication>
#include <QSettings>
#include <QtNetwork/QHostInfo>
#include <TestFramework/Framework/TestBaseClass.h>
#include <Texture/Image/Image.h>
#include <memory>

class WSceneDocument;
class QMimeData;
class WScene2Document;

class WEditorTestApplication : public WApplication
{
public:
  using SUPER = WApplication;

  WEditorTestApplication(WStringView sTestName);
  virtual WResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsShutdown() override;
  virtual void Run() override;


  virtual void AfterCoreSystemsStartup() override;


  virtual void BeforeHighLevelSystemsShutdown() override;

public:
  WQtEditorApp* m_pEditorApp = nullptr;
  WString m_sTestName;
};

class WEditorTest : public WTestBaseClass
{
  using SUPER = WTestBaseClass;

public:
  WEditorTest();
  ~WEditorTest();

  virtual WEditorTestApplication* CreateApplication();
  virtual WResult GetImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber) override;

protected:
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;

  WResult CreateAndLoadProject(const char* name);
  /// Opens a project by copying it to a temp location and opening that one.
  /// This ensures that the tests always work on a clean state.
  WResult OpenProject(const char* path);
  WDocument* OpenDocument(const char* subpath);
  void ExecuteDocumentAction(const char* szActionName, WDocument* pDocument, const WVariant& argument = WVariant());
  WResult CaptureImage(WQtDocumentWindow* pWindow, const char* szImageName);

  /// Path of the WEditorProcessor executable next to the test executable.
  WString GetEditorProcessorPath() const;
  /// Runs WEditorProcessor with the given arguments and waits for it. A non-zero exit code is a failure.
  WStatus RunEditorProcessor(const WDynamicArray<WString>& arguments);

  void CloseCurrentProject();
  void SafeProfilingData();
  void ProcessEvents(WUInt32 uiIterations = 1);
  void WaitFrames(WUInt32 uiFrames = 1);
  void UIServicesTickEventHandler(const WQtUiServices::TickEvent& e);

  std::unique_ptr<QMimeData> AssetsToDragMimeData(WArrayPtr<WUuid> assetGuids);
  std::unique_ptr<QMimeData> ObjectsDragMimeData(const WDeque<const WDocumentObject*>& objects);
  void MoveObjectsToLayer(WScene2Document* pDoc, const WDeque<const WDocumentObject*>& objects, const WUuid& layer, WDeque<const WDocumentObject*>& new_objects);
  const WDocumentObject* DropAsset(WScene2Document* pDoc, const char* szAssetGuidOrPath, bool bShift = false, bool bCtrl = false);
  const WDocumentObject* CreateGameObject(WScene2Document* pDoc, const WDocumentObject* pParent = nullptr, WStringView sName = {});


  WEditorTestApplication* m_pApplication = nullptr;
  WString m_sProjectPath;
  WImage m_CapturedImage;
  WDynamicArray<WString> m_CommandLineArguments;
  WDynamicArray<const char*> m_CommandLineArgumentPointers;
  WEventSubscriptionID m_UIServicesTickEventHandlerID = {};
  WUInt32 m_uiRenderedFrames = 0;
};
