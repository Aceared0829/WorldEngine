#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/VisualGraph/View.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

////////////////////////////////////////////////////////////////////////
// WMaterialModelAction
////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMaterialModelAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WMaterialModelAction::WMaterialModelAction(const WActionContext& context, const char* szName, const char* szIconPath)
  : WEnumerationMenuAction(context, szName, szIconPath)
{
  InitEnumerationType(WGetStaticRTTI<WMaterialAssetPreview>());
}

WInt64 WMaterialModelAction::GetValue() const
{
  return static_cast<const WMaterialAssetDocument*>(m_Context.m_pDocument)->m_PreviewModel.GetValue();
}

void WMaterialModelAction::Execute(const WVariant& value)
{
  ((WMaterialAssetDocument*)m_Context.m_pDocument)->m_PreviewModel.SetValue(value.ConvertTo<WInt32>());
}

//////////////////////////////////////////////////////////////////////////
// WMaterialAssetActions
//////////////////////////////////////////////////////////////////////////

WActionDescriptorHandle WMaterialAssetActions::s_hMaterialModelAction;

void WMaterialAssetActions::RegisterActions()
{
  s_hMaterialModelAction = W_REGISTER_DYNAMIC_MENU("MaterialAsset.Model", WMaterialModelAction, ":/EditorFramework/Icons/Perspective.svg");
}

void WMaterialAssetActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hMaterialModelAction);
}

void WMaterialAssetActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hMaterialModelAction, "", 45.0f);
}


//////////////////////////////////////////////////////////////////////////
// WQtMaterialAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////


WInt32 WQtMaterialAssetDocumentWindow::s_iNodeConfigWatchers = 0;
WHybridArray<WDirectoryWatcher*, 4> WQtMaterialAssetDocumentWindow::s_NodeConfigWatchers;


WQtMaterialAssetDocumentWindow::WQtMaterialAssetDocumentWindow(WMaterialAssetDocument* pDocument)
  : WQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtMaterialAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WQtMaterialAssetDocumentWindow::SelectionEventHandler, this));

  pDocument->m_VisualShaderEvents.AddEventHandler(WMakeDelegate(&WQtMaterialAssetDocumentWindow::VisualShaderEventHandler, this));

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "MaterialAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "MaterialAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("MaterialAssetWindowToolBar");
    addToolBar(pToolBar);
  }


  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(WVec3(+1.6f, 0.5f, 0.3f), WVec3(0, 0, 0), WVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90, 0.01f, 100.0f);

    m_pViewWidget = new WQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureFixed(WVec3(0), WVec3(0.0f), WVec3(+2.3f, -0.4f, 0.2f));

    AddViewWidget(m_pViewWidget);
    WQtViewWidgetContainer* pContainer = new WQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), nullptr, m_pViewWidget, "MaterialAssetViewToolBar");

    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("MaterialAssetDockWidget");
    pPropertyPanel->setWindowTitle("Material Properties");
    pPropertyPanel->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new WQtAssetStatusIndicator(GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);
  }

  // Visual Shader Editor
  {
    m_pVsePanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    m_pVsePanel->setObjectName("VisualShaderDockWidget");
    m_pVsePanel->setWindowTitle("Visual Shader Editor");

    QSplitter* pSplitter = new QSplitter(Qt::Orientation::Horizontal, m_pVsePanel);

    m_pScene = new WQtVisualShaderScene(this);
    m_pScene->InitScene(static_cast<const WVisualGraphObjectManager*>(pDocument->GetObjectManager()));

    m_pNodeView = new WQtVisualGraphView(m_pVsePanel);
    m_pNodeView->SetScene(m_pScene);
    pSplitter->addWidget(m_pNodeView);

    QWidget* pRightGroup = new QWidget(m_pVsePanel);
    pRightGroup->setLayout(new QVBoxLayout());

    QWidget* pButtonGroup = new QWidget(m_pVsePanel);
    pButtonGroup->setLayout(new QHBoxLayout());

    m_pOutputLine = new QTextEdit(m_pVsePanel);
    m_pOutputLine->setText("Transform the material asset to compile the Visual Shader.");
    m_pOutputLine->setReadOnly(true);

    m_pOpenShaderButton = new QPushButton(m_pVsePanel);
    m_pOpenShaderButton->setText("Open Shader File");
    connect(m_pOpenShaderButton, &QPushButton::clicked, this, &WQtMaterialAssetDocumentWindow::OnOpenShaderClicked);

    pButtonGroup->layout()->setContentsMargins(0, 0, 0, 0);
    pButtonGroup->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    pButtonGroup->layout()->addWidget(m_pOpenShaderButton);

    pRightGroup->layout()->setContentsMargins(0, 0, 0, 0);
    pRightGroup->layout()->addWidget(m_pOutputLine);
    pRightGroup->layout()->addWidget(pButtonGroup);

    pSplitter->addWidget(pRightGroup);

    pSplitter->setStretchFactor(0, 10);
    pSplitter->setStretchFactor(1, 1);

    m_bVisualShaderEnabled = false;
    m_pVsePanel->setWidget(pSplitter);

    m_pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pVsePanel);
    m_pVsePanel->toggleView(false);
  }

  pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  UpdatePreview();

  UpdateNodeEditorVisibility();

  FinishWindowCreation();
}

WQtMaterialAssetDocumentWindow::~WQtMaterialAssetDocumentWindow()
{
  GetMaterialDocument()->m_VisualShaderEvents.RemoveEventHandler(WMakeDelegate(&WQtMaterialAssetDocumentWindow::VisualShaderEventHandler, this));

  RestoreResource();

  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtMaterialAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtMaterialAssetDocumentWindow::PropertyEventHandler, this));

  const bool bCustom = GetMaterialDocument()->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<WInt64>() == WMaterialShaderMode::Custom;

  if (bCustom)
  {
    SetupDirectoryWatcher(false);
  }
}

void WQtMaterialAssetDocumentWindow::SetupDirectoryWatcher(bool needIt)
{
  if (needIt)
  {
    ++s_iNodeConfigWatchers;

    if (s_NodeConfigWatchers.IsEmpty())
    {
      // the editor's own nodes, and the nodes that the open project ships in its data directories
      WHybridArray<WStringBuilder, 4> folders;

      {
        WStringBuilder& sAppDir = folders.ExpandAndGetRef();
        sAppDir = WApplicationServices::GetSingleton()->GetApplicationDataFolder();
        sAppDir.AppendPath("VisualShader");
      }

      for (const auto& dd : WQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
      {
        WStringBuilder sDataDir;
        if (WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).Succeeded())
        {
          sDataDir.AppendPath("Editor/VisualShader");
          folders.PushBack(sDataDir);
        }
      }

      for (WStringBuilder& sFolder : folders)
      {
        if (!WOSFile::ExistsDirectory(sFolder))
          continue;

        WDirectoryWatcher* pWatcher = W_DEFAULT_NEW(WDirectoryWatcher);

        if (pWatcher->OpenDirectory(sFolder, WDirectoryWatcher::Watch::Writes).Failed())
        {
          WLog::Warning("Could not register a file system watcher for changes to '{0}'", sFolder);
          W_DEFAULT_DELETE(pWatcher);
          continue;
        }

        s_NodeConfigWatchers.PushBack(pWatcher);
      }
    }
  }
  else
  {
    --s_iNodeConfigWatchers;

    if (s_iNodeConfigWatchers == 0)
    {
      for (WDirectoryWatcher* pWatcher : s_NodeConfigWatchers)
      {
        W_DEFAULT_DELETE(pWatcher);
      }

      s_NodeConfigWatchers.Clear();
    }
  }
}

WMaterialAssetDocument* WQtMaterialAssetDocumentWindow::GetMaterialDocument()
{
  return static_cast<WMaterialAssetDocument*>(GetDocument());
}

void WQtMaterialAssetDocumentWindow::InternalRedraw()
{
  WEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  for (WDirectoryWatcher* pWatcher : s_NodeConfigWatchers)
  {
    pWatcher->EnumerateChanges(WMakeDelegate(&WQtMaterialAssetDocumentWindow::OnVseConfigChanged, this));
  }
  WQtEngineDocumentWindow::InternalRedraw();
}


void WQtMaterialAssetDocumentWindow::showEvent(QShowEvent* event)
{
  WQtEngineDocumentWindow::showEvent(event);

  m_pVsePanel->toggleView(m_bVisualShaderEnabled);
}

void WQtMaterialAssetDocumentWindow::OnOpenShaderClicked(bool)
{
  WAssetDocumentManager* pManager = (WAssetDocumentManager*)GetMaterialDocument()->GetDocumentManager();

  WString sAutoGenShader = pManager->GetAbsoluteOutputFileName(GetMaterialDocument()->GetAssetDocumentTypeDescriptor(), GetMaterialDocument()->GetDocumentPath(), WMaterialAssetDocumentManager::s_szShaderOutputTag);

  if (WOSFile::ExistsFile(sAutoGenShader))
  {
    WQtUiServices::OpenFileInDefaultProgram(sAutoGenShader).IgnoreResult();
  }
  else
  {
    WStringBuilder msg;
    msg.SetFormat("The auto generated file does not exist (yet).\nThe supposed location is '{0}'", sAutoGenShader);

    WQtUiServices::GetSingleton()->MessageBoxInformation(msg);
  }
}

void WQtMaterialAssetDocumentWindow::UpdatePreview()
{
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  WResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Material";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WContiguousMemoryStreamStorage streamStorage;
  WMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  WStringBuilder sAbsFilePath = GetMaterialDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("WBinMaterial");
  // Write Header
  memoryWriter << sAbsFilePath;
  const WUInt64 uiHash = WAssetCurator::GetSingleton()->GetAssetTransformHash(GetMaterialDocument()->GetGuid());
  WAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetMaterialDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  if (GetMaterialDocument()->WriteMaterialAsset(memoryWriter, WAssetCurator::GetSingleton()->GetActiveAssetProfile(), false).Failed())
    return;

  msg.m_Data = WArrayPtr<const WUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtMaterialAssetDocumentWindow::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (e.m_pObject == GetMaterialDocument()->GetPropertyObject() && e.m_sProperty == "ShaderMode")
  {
    UpdateNodeEditorVisibility();
  }

  UpdatePreview();

  if (e.m_sProperty == "ShaderMode" ||
      e.m_sProperty == "BLEND_MODE" ||
      e.m_sProperty == "BaseMaterial")
  {
    WDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "InvalidateCache";

    GetEditorEngineConnection()->SendMessage(&msg);
  }
}


void WQtMaterialAssetDocumentWindow::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // delayed execution
    QTimer::singleShot(1, [this]()
      {
      // Check again if the selection is empty. This could have changed due to the delayed execution.
      if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
      {
        GetDocument()->GetSelectionManager()->SetSelection(GetMaterialDocument()->GetPropertyObject());
      } });
  }
}

void WQtMaterialAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const WMaterialAssetDocument* pDoc = static_cast<const WMaterialAssetDocument*>(GetDocument());

    WDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewModel";
    msg.m_iValue = pDoc->m_PreviewModel.GetValue();

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void WQtMaterialAssetDocumentWindow::RestoreResource()
{
  WRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Material";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtMaterialAssetDocumentWindow::UpdateNodeEditorVisibility()
{
  const bool bCustom = GetMaterialDocument()->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<WInt64>() == WMaterialShaderMode::Custom;

  m_pVsePanel->toggleView(bCustom);

  // when this is called during construction, it seems to be overridden again (probably by the dock widget code or the splitter)
  // by delaying it a bit, we have the last word
  QTimer::singleShot(100, this, [this, bCustom]()
    { m_pVsePanel->toggleView(bCustom); });

  if (m_bVisualShaderEnabled != bCustom)
  {
    m_bVisualShaderEnabled = bCustom;

    SetupDirectoryWatcher(bCustom);
  }
}

void WQtMaterialAssetDocumentWindow::OnVseConfigChanged(WStringView sFilename, WDirectoryWatcherAction action, WDirectoryWatcherType type)
{
  if (type != WDirectoryWatcherType::File || !WPathUtils::HasExtension(sFilename, "DDL"))
    return;

  // lalala ... this is to allow writes to the file to 'hopefully' finish before we try to read it
  WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));

  WVisualShaderTypeRegistry::GetSingleton()->UpdateNodeData(sFilename);

  // TODO: We write an invalid hash in the file, should maybe compute the correct one on the fly
  // but that would involve the asset curator which would also save / transform everything which is
  // not what we want.
  WAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(0, GetMaterialDocument()->GetAssetTypeVersion());
  GetMaterialDocument()->RecreateVisualShaderFile(AssetHeader).LogFailure();
}

void WQtMaterialAssetDocumentWindow::VisualShaderEventHandler(const WMaterialVisualShaderEvent& e)
{
  WStringBuilder text;

  if (e.m_Type == WMaterialVisualShaderEvent::VisualShaderNotUsed)
  {
    text = "<span style=\"color:#bbbb00;\">Visual Shader is not used by the material.</span><br><br>Change the ShaderMode in the asset "
           "properties to enable Visual Shader mode.";
  }
  else
  {
    if (e.m_Type == WMaterialVisualShaderEvent::TransformSucceeded)
      text = "<span style=\"color:#00ff00;\">Visual Shader was transformed successfully.</span><br><br>";
    else
      text = "<span style=\"color:#ff8800;\">Visual Shader is invalid:</span><br><br>";

    WStringBuilder err = e.m_sTransformError;

    WTempHybridArray<WStringView, 16> lines;
    err.Split(false, lines, "\n");

    for (const WStringView& line : lines)
    {
      if (line.StartsWith("Error:"))
        text.AppendFormat("<span style=\"color:#ff2200;\">{0}</span><br>", line);
      else if (line.StartsWith("Warning:"))
        text.AppendFormat("<span style=\"color:#ffaa00;\">{0}</span><br>", line);
      else
        text.Append(line);
    }
    UpdatePreview();
  }

  m_pOutputLine->setAcceptRichText(true);
  m_pOutputLine->setHtml(text.GetData());
}
