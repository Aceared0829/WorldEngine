#include <EditorTest/EditorTestPCH.h>

#include "TestClass.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <QMimeData>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <Texture/Image/ImageUtils.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

WEditorTestApplication::WEditorTestApplication(WStringView sTestName)
  : WApplication("WEditor")
{
  EnableMemoryLeakReporting(true);

  m_pEditorApp = new WQtEditorApp;
  m_sTestName = sTestName;
}

WResult WEditorTestApplication::BeforeCoreSystemsStartup()
{
  if (SUPER::BeforeCoreSystemsStartup().Failed())
    return W_FAILURE;

  WStartup::AddApplicationTag("tool");
  WStartup::AddApplicationTag("editor");
  WStartup::AddApplicationTag("editorapp");

  WQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());
  return W_SUCCESS;
}

void WEditorTestApplication::AfterCoreSystemsShutdown()
{
  WQtEditorApp::GetSingleton()->DeInitQt();

  delete m_pEditorApp;
  m_pEditorApp = nullptr;
}

void WEditorTestApplication::Run()
{
  qApp->processEvents();
}

void WEditorTestApplication::AfterCoreSystemsStartup()
{
  W_PROFILE_SCOPE("AfterCoreSystemsStartup");
  // We override the user data dir to not pollute the editor settings.
  WStringBuilder userDataDir = WTestFramework::GetInstance()->GetAbsOutputPath();
  userDataDir.AppendPath(m_sTestName);
  userDataDir.MakeCleanPath();

  WQtEditorApp::GetSingleton()->StartupEditor(WQtEditorApp::StartupFlags::SafeMode | WQtEditorApp::StartupFlags::Dashboard | WQtEditorApp::StartupFlags::UnitTest, userDataDir);
  // Disable msg boxes.
  WQtUiServices::SetHeadless(true);
  WFileSystem::SetSpecialDirectory("testout", WTestFramework::GetInstance()->GetAbsOutputPath());

  WFileSystem::AddDataDirectory(">Wtest/", "ImageComparisonDataDir", "imgout", WDataDirUsage::AllowWrites).IgnoreResult();

  // To read reference images
  WStringBuilder sReadDir(">sdk/", WTestFramework::GetInstance()->GetRelTestDataPath());
  WFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir").IgnoreResult();
}

void WEditorTestApplication::BeforeHighLevelSystemsShutdown()
{
  W_PROFILE_SCOPE("BeforeHighLevelSystemsShutdown");
  WQtEditorApp::GetSingleton()->ShutdownEditor();
}

//////////////////////////////////////////////////////////////////////////

WEditorTest::WEditorTest()
{
  WQtEngineViewWidget::s_FixedResolution = WSizeU32(512, 512);
}

WEditorTest::~WEditorTest() = default;

WEditorTestApplication* WEditorTest::CreateApplication()
{
  WEditorTestApplication* pTestApplication = W_DEFAULT_NEW(WEditorTestApplication, GetTestName());

  m_CommandLineArguments = WCommandLineUtils::GetGlobalInstance()->GetCommandLineArray();
  W_ASSERT_DEV(m_CommandLineArguments.GetCount() > 0, "There should always be at least 1 command line argument (the executable name)");

  m_CommandLineArgumentPointers.Clear();
  for (auto& s : m_CommandLineArguments)
  {
    m_CommandLineArgumentPointers.PushBack(s.GetData());
  }

  pTestApplication->SetCommandLineArguments(m_CommandLineArgumentPointers.GetCount(), m_CommandLineArgumentPointers.GetData());

  return pTestApplication;
}

WResult WEditorTest::GetImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber)
{
  if (!m_CapturedImage.IsValid())
    return W_FAILURE;

  if (m_CapturedImage.GetWidth() == 512 && m_CapturedImage.GetHeight() == 512)
  {
    ref_img.ResetAndMove(std::move(m_CapturedImage));
  }
  else
  {
    // Due to DPI scaling, we can't guarantee that the swapchain is exactly 512x512. The viewport still is though, so as long as we have something bigger the resulting image is correct if we crop off the remaining pixels.
    WImageUtils::CropImage(m_CapturedImage, {0, 0}, {512, 512}, ref_img);
    m_CapturedImage.Clear();
  }
  return W_SUCCESS;
}

WResult WEditorTest::InitializeTest()
{
  m_pApplication = CreateApplication();
  m_sProjectPath.Clear();

  if (m_pApplication == nullptr)
    return W_FAILURE;

  W_SUCCEED_OR_RETURN(WRun_Startup(m_pApplication));

  static bool s_bCheckedAdapter = false;
  static WString s_sAdapterName;

  if (!s_bCheckedAdapter)
  {
    s_bCheckedAdapter = true;

#if W_ENABLED(W_PLATFORM_WINDOWS)
    WUniquePtr<WGALDevice> pDevice;
    WGALDeviceCreationDescription DeviceInit;

    pDevice = WGALDeviceFactory::CreateDevice(WGameApplication::GetActiveRenderer(), WFoundation::GetDefaultAllocator(), DeviceInit);

    W_SUCCEED_OR_RETURN(pDevice->Init());
    s_sAdapterName = pDevice->GetCapabilities().m_sAdapterName;
    W_SUCCEED_OR_RETURN(pDevice->Shutdown());
    pDevice.Clear();
#endif
  }

  WTestFramework::GetInstance()->SetImageReferenceTagsFromEnvironment(W_PLATFORM_NAME, WGameApplication::GetActiveRenderer(), s_sAdapterName);
  m_UIServicesTickEventHandlerID = WQtUiServices::s_TickEvent.AddEventHandler(WMakeDelegate(&WEditorTest::UIServicesTickEventHandler, this));

  return W_SUCCESS;
}

WResult WEditorTest::DeInitializeTest()
{
  WQtUiServices::s_TickEvent.RemoveEventHandler(m_UIServicesTickEventHandlerID);
  CloseCurrentProject();

  if (m_pApplication)
  {
    WRun_Shutdown(m_pApplication);

    W_DEFAULT_DELETE(m_pApplication);
  }


  return W_SUCCESS;
}

WString WEditorTest::GetEditorProcessorPath() const
{
  WStringBuilder path;
  // Get the directory where EditorTest.exe is located
  path = WOSFile::GetApplicationDirectory();
#if W_ENABLED(W_PLATFORM_WINDOWS)
  path.AppendPath("WEditorProcessor.exe");
#else
  path.AppendPath("WEditorProcessor");
#endif
  return path;
}

WStatus WEditorTest::RunEditorProcessor(const WDynamicArray<WString>& arguments)
{
  WProcessOptions processOptions;
  processOptions.m_sProcess = GetEditorProcessorPath();
  processOptions.m_Arguments = arguments;
  processOptions.m_bHideConsoleWindow = true;

  WInt32 exitCode = 0;
  WResult result = WProcess::Execute(processOptions, &exitCode);

  if (result.Failed() || exitCode != 0)
  {
    return WStatus(WFmt("WEditorProcessor failed with exit code: {}", exitCode));
  }

  return WStatus(W_SUCCESS);
}

WResult WEditorTest::CreateAndLoadProject(const char* name)
{
  W_PROFILE_SCOPE("CreateAndLoadProject");
  WStringBuilder relPath;
  relPath = ":APPDATA";
  relPath.AppendPath(name);

  WStringBuilder absPath;
  if (WFileSystem::ResolvePath(relPath, &absPath, nullptr).Failed())
  {
    WLog::Error("Failed to resolve project path '{0}'.", relPath);
    return W_FAILURE;
  }
  if (WOSFile::DeleteFolder(absPath).Failed())
  {
    WLog::Error("Failed to delete old project folder '{0}'.", absPath);
    return W_FAILURE;
  }

  WStringBuilder projectFile = absPath;
  projectFile.AppendPath("WProject");
  if (m_pApplication->m_pEditorApp->CreateOrOpenProject(true, projectFile).Failed())
  {
    WLog::Error("Failed to create project '{0}'.", projectFile);
    return W_FAILURE;
  }

  m_sProjectPath = absPath;
  return W_SUCCESS;
}

WResult WEditorTest::OpenProject(const char* path)
{
  W_PROFILE_SCOPE("OpenProject");
  WStringBuilder relPath;
  relPath = ">sdk";
  relPath.AppendPath(path);

  WStringBuilder absPath;
  if (WFileSystem::ResolveSpecialDirectory(relPath, absPath).Failed())
  {
    WLog::Error("Failed to resolve project path '{0}'.", relPath);
    return W_FAILURE;
  }

  // Copy project to temp folder
  WStringBuilder projectName = WPathUtils::GetFileName(path);
  WStringBuilder relTempPath;
  relTempPath = ":APPDATA";
  relTempPath.AppendPath(projectName);

  WStringBuilder absTempPath;
  if (WFileSystem::ResolvePath(relTempPath, &absTempPath, nullptr).Failed())
  {
    WLog::Error("Failed to resolve project temp path '{0}'.", relPath);
    return W_FAILURE;
  }
  if (WOSFile::DeleteFolder(absTempPath).Failed())
  {
    WLog::Error("Failed to delete old project temp folder '{0}'.", absTempPath);
    return W_FAILURE;
  }
  if (WOSFile::CopyFolder(absPath, absTempPath).Failed())
  {
    WLog::Error("Failed to copy project '{0}' to temp location: '{1}'.", absPath, absTempPath);
    return W_FAILURE;
  }


  WStringBuilder projectFile = absTempPath;
  projectFile.AppendPath("WProject");
  if (m_pApplication->m_pEditorApp->CreateOrOpenProject(false, projectFile).Failed())
  {
    WLog::Error("Failed to open project '{0}'.", projectFile);
    return W_FAILURE;
  }

  m_sProjectPath = absTempPath;
  return W_SUCCESS;
}

WDocument* WEditorTest::OpenDocument(const char* subpath)
{
  WStringBuilder fullpath;
  fullpath = m_sProjectPath;
  fullpath.AppendPath(subpath);

  WDocument* pDoc = m_pApplication->m_pEditorApp->OpenDocument(fullpath, WDocumentFlags::RequestWindow);

  if (pDoc)
  {
    ProcessEvents();
  }

  return pDoc;
}

void WEditorTest::ExecuteDocumentAction(const char* szActionName, WDocument* pDocument, const WVariant& argument /*= WVariant()*/)
{
  W_TEST_BOOL(WActionManager::ExecuteAction(nullptr, szActionName, pDocument, argument).Succeeded());
}

WResult WEditorTest::CaptureImage(WQtDocumentWindow* pWindow, const char* szImageName)
{
  WStringBuilder sImgPath = WOSFile::GetUserDataFolder("EditorTests");
  sImgPath.AppendFormat("/{}.tga", szImageName);

  WOSFile::DeleteFile(sImgPath).IgnoreResult();

  pWindow->CreateImageCapture(sImgPath);

  for (int i = 0; i < 10; ++i)
  {
    ProcessEvents();

    if (WOSFile::ExistsFile(sImgPath))
      break;

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
  }

  if (!WOSFile::ExistsFile(sImgPath))
    return W_FAILURE;

  W_SUCCEED_OR_RETURN(m_CapturedImage.LoadFrom(sImgPath));

  return W_SUCCESS;
}

void WEditorTest::CloseCurrentProject()
{
  W_PROFILE_SCOPE("CloseCurrentProject");
  m_sProjectPath.Clear();
  m_pApplication->m_pEditorApp->CloseProject();
}

void WEditorTest::SafeProfilingData()
{
  WProfilingUtils::SaveProfilingCapture(":appdata/profiling.json").IgnoreResult();
}

void WEditorTest::ProcessEvents(WUInt32 uiIterations)
{
  W_PROFILE_SCOPE("ProcessEvents");
  if (qApp)
  {
    for (WUInt32 i = 0; i < uiIterations; i++)
    {
      qApp->processEvents();
    }
  }
}

void WEditorTest::WaitFrames(WUInt32 uiFrames)
{
  W_PROFILE_SCOPE("WaitFrames");
  // Add one because we could have started waiting between start and end frame.
  uiFrames++;
  WUInt32 uiCurrentFrame = m_uiRenderedFrames;
  if (qApp)
  {
    while ((m_uiRenderedFrames - uiCurrentFrame) < uiFrames)
    {
      qApp->processEvents();
    }
  }
}

void WEditorTest::UIServicesTickEventHandler(const WQtUiServices::TickEvent& e)
{
  if (e.m_Type == WQtUiServices::TickEvent::Type::EndFrame)
  {
    m_uiRenderedFrames++;
  }
}

std::unique_ptr<QMimeData> WEditorTest::AssetsToDragMimeData(WArrayPtr<WUuid> assetGuids)
{
  std::unique_ptr<QMimeData> mimeData(new QMimeData());
  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  QString sGuids;
  QList<QUrl> urls;

  WStringBuilder tmp;

  stream << (int)1;
  for (WUInt32 i = 0; i < assetGuids.GetCount(); ++i)
  {
    QString sGuid(WConversionUtils::ToString(assetGuids[i], tmp).GetData());
    stream << sGuid;
  }

  mimeData->setData("application/WEditor.AssetGuid", encodedData);
  return std::move(mimeData);
}

std::unique_ptr<QMimeData> WEditorTest::ObjectsDragMimeData(const WDeque<const WDocumentObject*>& objects)
{
  WTempHybridArray<const WDocumentObject*, 32> Dragged;
  for (const WDocumentObject* pObject : objects)
  {
    Dragged.PushBack(pObject);
  }

  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << Dragged;

  std::unique_ptr<QMimeData> mimeData(new QMimeData());
  mimeData->setData("application/WEditor.ObjectSelection", encodedData);
  return std::move(mimeData);
}

void WEditorTest::MoveObjectsToLayer(WScene2Document* pDoc, const WDeque<const WDocumentObject*>& objects, const WUuid& layer, WDeque<const WDocumentObject*>& new_objects)
{
  pDoc->GetSelectionManager()->SetSelection(objects);

  WQtLayerAdapter adapter(pDoc);
  auto mimeData = ObjectsDragMimeData(objects);
  WDragDropInfo info;
  info.m_iTargetObjectInsertChildIndex = -1;
  info.m_pMimeData = mimeData.get();
  info.m_sTargetContext = "layertree";
  info.m_TargetDocument = pDoc->GetGuid();
  info.m_TargetObject = pDoc->GetLayerObject(layer)->GetGuid();
  info.m_bCtrlKeyDown = false;
  info.m_bShiftKeyDown = false;
  info.m_pAdapter = &adapter;
  if (!W_TEST_BOOL(WDragDropHandler::DropOnly(&info)))
    return;

  new_objects = pDoc->GetLayerDocument(layer)->GetSelectionManager()->GetSelection();
}

const WDocumentObject* WEditorTest::DropAsset(WScene2Document* pDoc, const char* szAssetGuidOrPath, bool bShift /*= false*/, bool bCtrl /*= false*/)
{
  const WAssetCurator::WLockedSubAsset asset = WAssetCurator::GetSingleton()->FindSubAsset(szAssetGuidOrPath);
  if (W_TEST_BOOL(asset.isValid()))
  {
    WUuid assetGuid = asset->m_Data.m_Guid;
    WArrayPtr<WUuid> assets(&assetGuid, 1);
    auto mimeData = AssetsToDragMimeData(assets);

    WDragDropInfo info;
    info.m_pMimeData = mimeData.get();
    info.m_TargetDocument = pDoc->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_iTargetObjectSubID = 0;
    info.m_bShiftKeyDown = bShift;
    info.m_bCtrlKeyDown = bCtrl;

    if (W_TEST_BOOL(WDragDropHandler::DropOnly(&info)))
    {
      return pDoc->GetSelectionManager()->GetCurrentObject();
    }
  }
  return {};
}

const WDocumentObject* WEditorTest::CreateGameObject(WScene2Document* pDoc, const WDocumentObject* pParent, WStringView sName)
{
  auto pAccessor = pDoc->GetObjectAccessor();
  pAccessor->StartTransaction("Add Game Object");

  WUuid guid;
  W_TEST_STATUS(pAccessor->AddObjectByName(pParent != nullptr ? pParent : pDoc->GetObjectManager()->GetRootObject(), "Children", -1, WRTTI::FindTypeByName("WGameObject"), guid));
  const WDocumentObject* pObject = pAccessor->GetObject(guid);
  W_TEST_STATUS(pAccessor->SetValueByName(pObject, "Name", sName));

  pAccessor->FinishTransaction();

  return pObject;
}
