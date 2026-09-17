#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>

WQtColorGradientAssetDocumentWindow::WQtColorGradientAssetDocumentWindow(WDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtColorGradientAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WQtColorGradientAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "ColorGradientAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "ColorGradientAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ColorGradientAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_bShowFirstTime = true;
  m_pGradientEditor = new WQtColorGradientEditorWidget(this);


  // Central Widget
  {
    QWidget* pContainer = new QWidget(this);
    pContainer->setLayout(new QVBoxLayout());
    pContainer->layout()->addWidget(new WQtAssetStatusIndicator((WAssetDocument*)GetDocument()));
    pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    pContainer->layout()->addWidget(m_pGradientEditor);
    pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

    WQtDocumentPanel* pCentral = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("WQtDocumentPanel");
    pCentral->setWindowTitle("Gradient");
    pCentral->setWidget(pContainer);

    m_pDockManager->setCentralWidget(pCentral);
  }

  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::ColorCpAdded, this, &WQtColorGradientAssetDocumentWindow::onGradientColorCpAdded);
  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::ColorCpMoved, this, &WQtColorGradientAssetDocumentWindow::onGradientColorCpMoved);
  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::ColorCpDeleted, this, &WQtColorGradientAssetDocumentWindow::onGradientColorCpDeleted);
  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::ColorCpChanged, this, &WQtColorGradientAssetDocumentWindow::onGradientColorCpChanged);

  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::AlphaCpAdded, this, &WQtColorGradientAssetDocumentWindow::onGradientAlphaCpAdded);
  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::AlphaCpMoved, this, &WQtColorGradientAssetDocumentWindow::onGradientAlphaCpMoved);
  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::AlphaCpDeleted, this, &WQtColorGradientAssetDocumentWindow::onGradientAlphaCpDeleted);
  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::AlphaCpChanged, this, &WQtColorGradientAssetDocumentWindow::onGradientAlphaCpChanged);

  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::IntensityCpAdded, this, &WQtColorGradientAssetDocumentWindow::onGradientIntensityCpAdded);
  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::IntensityCpMoved, this, &WQtColorGradientAssetDocumentWindow::onGradientIntensityCpMoved);
  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::IntensityCpDeleted, this, &WQtColorGradientAssetDocumentWindow::onGradientIntensityCpDeleted);
  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::IntensityCpChanged, this, &WQtColorGradientAssetDocumentWindow::onGradientIntensityCpChanged);

  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::BeginOperation, this, &WQtColorGradientAssetDocumentWindow::onGradientBeginOperation);
  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::EndOperation, this, &WQtColorGradientAssetDocumentWindow::onGradientEndOperation);

  connect(m_pGradientEditor, &WQtColorGradientEditorWidget::NormalizeRange, this, &WQtColorGradientAssetDocumentWindow::onGradientNormalizeRange);

  // property grid, if needed
  if (false)
  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("ColorGradientAssetDockWidget");
    pPropertyPanel->setWindowTitle("ColorGradient Properties");
    pPropertyPanel->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();
}

WQtColorGradientAssetDocumentWindow::~WQtColorGradientAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtColorGradientAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WQtColorGradientAssetDocumentWindow::StructureEventHandler, this));

  RestoreResource();
}

void WQtColorGradientAssetDocumentWindow::onGradientColorCpAdded(double posX, const WColorGammaUB& color)
{
  WColorGradientAssetDocument* pDoc = static_cast<WColorGradientAssetDocument*>(GetDocument());

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Color Control Point");

  // Get the Gradient sub-object GUID
  WUuid gradientGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();

  WAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = gradientGuid;
  cmdAdd.m_NewObjectGuid = WUuid::MakeUuid();
  cmdAdd.m_sParentProperty = "ColorCPs";
  cmdAdd.m_pType = WGetStaticRTTI<WColorGradientColorCP>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd).AssertSuccess();

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = WColorGradient::TimeToTick(posX);
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue = color.r;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue = color.g;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue = color.b;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}


void WQtColorGradientAssetDocumentWindow::onGradientAlphaCpAdded(double posX, WUInt8 alpha)
{
  WColorGradientAssetDocument* pDoc = static_cast<WColorGradientAssetDocument*>(GetDocument());

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Alpha Control Point");

  // Get the Gradient sub-object GUID
  WUuid gradientGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();

  WAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = gradientGuid;
  cmdAdd.m_NewObjectGuid = WUuid::MakeUuid();
  cmdAdd.m_sParentProperty = "AlphaCPs";
  cmdAdd.m_pType = WGetStaticRTTI<WColorGradientAlphaCP>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd).AssertSuccess();

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = WColorGradient::TimeToTick(posX);
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}


void WQtColorGradientAssetDocumentWindow::onGradientIntensityCpAdded(double posX, float intensity)
{
  WColorGradientAssetDocument* pDoc = static_cast<WColorGradientAssetDocument*>(GetDocument());

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Intensity Control Point");

  // Get the Gradient sub-object GUID
  WUuid gradientGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();

  WAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = gradientGuid;
  cmdAdd.m_NewObjectGuid = WUuid::MakeUuid();
  cmdAdd.m_sParentProperty = "IntensityCPs";
  cmdAdd.m_pType = WGetStaticRTTI<WColorGradientIntensityCP>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd).AssertSuccess();

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = WColorGradient::TimeToTick(posX);
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void WQtColorGradientAssetDocumentWindow::MoveCP(WInt32 idx, double newPosX, const char* szArrayName)
{
  WColorGradientAssetDocument* pDoc = static_cast<WColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  // First get the Gradient sub-object
  WUuid gradientGuid = pProp->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* pGradientObj = pDoc->GetObjectManager()->GetObject(gradientGuid);

  // Now access the array on the Gradient object
  WVariant objGuid = pGradientObj->GetTypeAccessor().GetValue(szArrayName, idx);

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Move Control Point");

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = WColorGradient::TimeToTick(newPosX);
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void WQtColorGradientAssetDocumentWindow::onGradientColorCpMoved(WInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "ColorCPs");
}

void WQtColorGradientAssetDocumentWindow::onGradientAlphaCpMoved(WInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "AlphaCPs");
}


void WQtColorGradientAssetDocumentWindow::onGradientIntensityCpMoved(WInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "IntensityCPs");
}

void WQtColorGradientAssetDocumentWindow::RemoveCP(WInt32 idx, const char* szArrayName)
{
  WColorGradientAssetDocument* pDoc = static_cast<WColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  // First get the Gradient sub-object
  WUuid gradientGuid = pProp->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* pGradientObj = pDoc->GetObjectManager()->GetObject(gradientGuid);

  // Now access the array on the Gradient object
  WVariant objGuid = pGradientObj->GetTypeAccessor().GetValue(szArrayName, idx);

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  WRemoveObjectCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<WUuid>();
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void WQtColorGradientAssetDocumentWindow::onGradientColorCpDeleted(WInt32 idx)
{
  RemoveCP(idx, "ColorCPs");
}


void WQtColorGradientAssetDocumentWindow::onGradientAlphaCpDeleted(WInt32 idx)
{
  RemoveCP(idx, "AlphaCPs");
}


void WQtColorGradientAssetDocumentWindow::onGradientIntensityCpDeleted(WInt32 idx)
{
  RemoveCP(idx, "IntensityCPs");
}


void WQtColorGradientAssetDocumentWindow::onGradientColorCpChanged(WInt32 idx, const WColorGammaUB& color)
{
  WColorGradientAssetDocument* pDoc = static_cast<WColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  // First get the Gradient sub-object
  WUuid gradientGuid = pProp->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* pGradientObj = pDoc->GetObjectManager()->GetObject(gradientGuid);

  // Now access the array on the Gradient object
  WVariant objGuid = pGradientObj->GetTypeAccessor().GetValue("ColorCPs", idx);

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Color");

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue = color.r;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue = color.g;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue = color.b;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}


void WQtColorGradientAssetDocumentWindow::onGradientAlphaCpChanged(WInt32 idx, WUInt8 alpha)
{
  WColorGradientAssetDocument* pDoc = static_cast<WColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  // First get the Gradient sub-object
  WUuid gradientGuid = pProp->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* pGradientObj = pDoc->GetObjectManager()->GetObject(gradientGuid);

  // Now access the array on the Gradient object
  WVariant objGuid = pGradientObj->GetTypeAccessor().GetValue("AlphaCPs", idx);

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Alpha");

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void WQtColorGradientAssetDocumentWindow::onGradientIntensityCpChanged(WInt32 idx, float intensity)
{
  WColorGradientAssetDocument* pDoc = static_cast<WColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  // First get the Gradient sub-object
  WUuid gradientGuid = pProp->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* pGradientObj = pDoc->GetObjectManager()->GetObject(gradientGuid);

  // Now access the array on the Gradient object
  WVariant objGuid = pGradientObj->GetTypeAccessor().GetValue("IntensityCPs", idx);

  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Intensity");

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}


void WQtColorGradientAssetDocumentWindow::onGradientBeginOperation()
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Gradient");
}


void WQtColorGradientAssetDocumentWindow::onGradientEndOperation(bool commit)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}


void WQtColorGradientAssetDocumentWindow::onGradientNormalizeRange()
{
  if (WQtUiServices::GetSingleton()->MessageBoxQuestion("This will adjust the positions of all control points, such that the minimum is at 0 and the maximum at 1.\n\nContinue?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Yes) !=
      QMessageBox::StandardButton::Yes)
    return;

  WColorGradientAssetDocument* pDoc = static_cast<WColorGradientAssetDocument*>(GetDocument());

  WColorGradient GradientData;
  pDoc->GetProperties()->FillGradientData(GradientData);

  double minX, maxX;
  if (!GradientData.GetExtents(minX, maxX))
    return;

  if ((minX == 0 && maxX == 1) || (minX >= maxX))
    return;

  WCommandHistory* history = GetDocument()->GetCommandHistory();

  const float rangeNorm = 1.0f / (maxX - minX);

  history->StartTransaction("Normalize Gradient Range");

  WUInt32 numRgb, numAlpha, numInt;
  GradientData.GetNumControlPoints(numRgb, numAlpha, numInt);

  for (WUInt32 i = 0; i < numRgb; ++i)
  {
    float x = WColorGradient::TickToTime(GradientData.GetColorControlPoint(i).m_iTick);
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "ColorCPs");
  }

  for (WUInt32 i = 0; i < numAlpha; ++i)
  {
    float x = WColorGradient::TickToTime(GradientData.GetAlphaControlPoint(i).m_iTick);
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "AlphaCPs");
  }

  for (WUInt32 i = 0; i < numInt; ++i)
  {
    float x = WColorGradient::TickToTime(GradientData.GetIntensityControlPoint(i).m_iTick);
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "IntensityCPs");
  }

  history->FinishTransaction();

  m_pGradientEditor->FrameGradient();
}

void WQtColorGradientAssetDocumentWindow::UpdatePreview()
{
  WColorGradient GradientData;

  WColorGradientAssetDocument* pDoc = static_cast<WColorGradientAssetDocument*>(GetDocument());
  pDoc->GetProperties()->FillGradientData(GradientData);

  m_pGradientEditor->SetColorGradient(GradientData);

  if (m_bShowFirstTime)
  {
    m_bShowFirstTime = false;
    m_pGradientEditor->FrameGradient();
  }

  SendLiveResourcePreview();
}

void WQtColorGradientAssetDocumentWindow::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void WQtColorGradientAssetDocumentWindow::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  UpdatePreview();
}

void WQtColorGradientAssetDocumentWindow::SendLiveResourcePreview()
{
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  WResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "ColorGradient";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WContiguousMemoryStreamStorage streamStorage;
  WMemoryStreamWriter memoryWriter(&streamStorage);

  WColorGradientAssetDocument* pDoc = WDynamicCast<WColorGradientAssetDocument*>(GetDocument());

  // Write Path
  WStringBuilder sAbsFilePath = pDoc->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("WColorGradient");

  // Write Header
  memoryWriter << sAbsFilePath;
  const WUInt64 uiHash = WAssetCurator::GetSingleton()->GetAssetTransformHash(pDoc->GetGuid());
  WAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, pDoc->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  pDoc->WriteResource(memoryWriter);
  msg.m_Data = WArrayPtr<const WUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtColorGradientAssetDocumentWindow::RestoreResource()
{
  WRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "ColorGradient";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
