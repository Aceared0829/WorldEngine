#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>


WQtCurve1DAssetDocumentWindow::WQtCurve1DAssetDocumentWindow(WDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WQtCurve1DAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "Curve1DAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "Curve1DAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("Curve1DAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    m_pCurveEditor = new WQtCurve1DEditorWidget(this);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new WQtAssetStatusIndicator((WAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(m_pCurveEditor);

    WQtDocumentPanel* pCentral = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("WQtDocumentPanel");
    pCentral->setWindowTitle("Curve");
    pCentral->setWidget(pWidget);

    m_pDockManager->setCentralWidget(pCentral);
  }

  connect(m_pCurveEditor, &WQtCurve1DEditorWidget::InsertCpEvent, this, &WQtCurve1DAssetDocumentWindow::onInsertCpAt);
  connect(m_pCurveEditor, &WQtCurve1DEditorWidget::CpMovedEvent, this, &WQtCurve1DAssetDocumentWindow::onCurveCpMoved);
  connect(m_pCurveEditor, &WQtCurve1DEditorWidget::CpDeletedEvent, this, &WQtCurve1DAssetDocumentWindow::onCurveCpDeleted);
  connect(m_pCurveEditor, &WQtCurve1DEditorWidget::TangentMovedEvent, this, &WQtCurve1DAssetDocumentWindow::onCurveTangentMoved);
  connect(m_pCurveEditor, &WQtCurve1DEditorWidget::TangentLinkEvent, this, &WQtCurve1DAssetDocumentWindow::onLinkCurveTangents);
  connect(m_pCurveEditor, &WQtCurve1DEditorWidget::CpTangentModeEvent, this, &WQtCurve1DAssetDocumentWindow::onCurveTangentModeChanged);

  connect(m_pCurveEditor, &WQtCurve1DEditorWidget::BeginOperationEvent, this, &WQtCurve1DAssetDocumentWindow::onCurveBeginOperation);
  connect(m_pCurveEditor, &WQtCurve1DEditorWidget::EndOperationEvent, this, &WQtCurve1DAssetDocumentWindow::onCurveEndOperation);
  connect(m_pCurveEditor, &WQtCurve1DEditorWidget::BeginCpChangesEvent, this, &WQtCurve1DAssetDocumentWindow::onCurveBeginCpChanges);
  connect(m_pCurveEditor, &WQtCurve1DEditorWidget::EndCpChangesEvent, this, &WQtCurve1DAssetDocumentWindow::onCurveEndCpChanges);

  if (false)
  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("Curve1DAssetDockWidget");
    pPropertyPanel->setWindowTitle("Curve1D Properties");
    pPropertyPanel->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();
}

WQtCurve1DAssetDocumentWindow::~WQtCurve1DAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WQtCurve1DAssetDocumentWindow::StructureEventHandler, this));

  RestoreResource();
}

void WQtCurve1DAssetDocumentWindow::onCurveBeginOperation(QString name)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands(name.toUtf8().data());
}

void WQtCurve1DAssetDocumentWindow::onCurveEndOperation(bool commit)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();

  UpdatePreview();
}

void WQtCurve1DAssetDocumentWindow::onCurveBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void WQtCurve1DAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdatePreview();
}

void WQtCurve1DAssetDocumentWindow::onInsertCpAt(WUInt32 uiCurveIdx, WInt64 tickX, double clickPosY)
{
  WCurve1DAssetDocument* pDoc = static_cast<WCurve1DAssetDocument*>(GetDocument());

  WCommandHistory* history = pDoc->GetCommandHistory();

  if (pDoc->GetPropertyObject()->GetTypeAccessor().GetCount("Curves") == 0)
  {
    // no curves allocated yet, add one

    WAddObjectCommand cmdAddCurve;
    cmdAddCurve.m_Parent = pDoc->GetPropertyObject()->GetGuid();
    cmdAddCurve.m_NewObjectGuid = WUuid::MakeUuid();
    cmdAddCurve.m_sParentProperty = "Curves";
    cmdAddCurve.m_pType = WGetStaticRTTI<WSingleCurveData>();
    cmdAddCurve.m_Index = -1;

    history->AddCommand(cmdAddCurve).AssertSuccess();
  }

  const WVariant curveGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Curves", uiCurveIdx);

  WAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = curveGuid.Get<WUuid>();
  cmdAdd.m_NewObjectGuid = WUuid::MakeUuid();
  cmdAdd.m_sParentProperty = "ControlPoints";
  cmdAdd.m_pType = WGetStaticRTTI<WCurveControlPointData>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd).AssertSuccess();

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = tickX;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = clickPosY;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "LeftTangent";
  cmdSet.m_NewValue = WVec2(-0.1f, 0.0f);
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "RightTangent";
  cmdSet.m_NewValue = WVec2(+0.1f, 0.0f);
  history->AddCommand(cmdSet).AssertSuccess();
}

void WQtCurve1DAssetDocumentWindow::onCurveCpMoved(WUInt32 curveIdx, WUInt32 cpIdx, WInt64 iTickX, double newPosY)
{
  iTickX = WMath::Max<WInt64>(iTickX, 0);

  WCurve1DAssetDocument* pDoc = static_cast<WCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const WVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const WDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<WUuid>());
  const WVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = newPosY;
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void WQtCurve1DAssetDocumentWindow::onCurveCpDeleted(WUInt32 curveIdx, WUInt32 cpIdx)
{
  WCurve1DAssetDocument* pDoc = static_cast<WCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const WVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const WDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<WUuid>());
  const WVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  WRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void WQtCurve1DAssetDocumentWindow::onCurveTangentMoved(WUInt32 curveIdx, WUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  WCurve1DAssetDocument* pDoc = static_cast<WCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const WVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const WDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<WUuid>());
  const WVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();

  // clamp tangents to one side
  if (rightTangent)
    newPosX = WMath::Max(newPosX, 0.0f);
  else
    newPosX = WMath::Min(newPosX, 0.0f);

  cmdSet.m_sProperty = rightTangent ? "RightTangent" : "LeftTangent";
  cmdSet.m_NewValue = WVec2(newPosX, newPosY);
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void WQtCurve1DAssetDocumentWindow::onLinkCurveTangents(WUInt32 curveIdx, WUInt32 cpIdx, bool bLink)
{
  WCurve1DAssetDocument* pDoc = static_cast<WCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const WVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const WDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<WUuid>());
  const WVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmdLink;
  cmdLink.m_Object = cpGuid.Get<WUuid>();
  cmdLink.m_sProperty = "Linked";
  cmdLink.m_NewValue = bLink;
  GetDocument()->GetCommandHistory()->AddCommand(cmdLink).AssertSuccess();

  if (bLink)
  {
    const WVec2 leftTangent = pDoc->GetProperties()->m_Curves[curveIdx]->m_ControlPoints[cpIdx].m_LeftTangent;
    const WVec2 rightTangent(-leftTangent.x, -leftTangent.y);

    onCurveTangentMoved(curveIdx, cpIdx, rightTangent.x, rightTangent.y, true);
  }
}

void WQtCurve1DAssetDocumentWindow::onCurveTangentModeChanged(WUInt32 curveIdx, WUInt32 cpIdx, bool rightTangent, int mode)
{
  WCurve1DAssetDocument* pDoc = static_cast<WCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const WVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const WDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<WUuid>());
  const WVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmd;
  cmd.m_Object = cpGuid.Get<WUuid>();
  cmd.m_sProperty = rightTangent ? "RightTangentMode" : "LeftTangentMode";
  cmd.m_NewValue = mode;
  GetDocument()->GetCommandHistory()->AddCommand(cmd).AssertSuccess();

  // sync current curve back
  if (false)
  {
    // generally works, but would need some work to make it perfect

    WCurve1D curve;
    pDoc->GetProperties()->m_Curves[curveIdx]->ConvertToRuntimeData(curve);
    curve.SortControlPoints();
    curve.ApplyTangentModes();

    for (WUInt32 i = 0; i < curve.GetNumControlPoints(); ++i)
    {
      const auto& cp = curve.GetControlPoint(i);
      if (cp.m_uiOriginalIndex == cpIdx)
      {
        if (rightTangent)
          onCurveTangentMoved(curveIdx, cpIdx, cp.m_RightTangent.x, cp.m_RightTangent.y, true);
        else
          onCurveTangentMoved(curveIdx, cpIdx, cp.m_LeftTangent.x, cp.m_LeftTangent.y, false);

        break;
      }
    }
  }
}

void WQtCurve1DAssetDocumentWindow::UpdatePreview()
{
  WCurve1DAssetDocument* pDoc = static_cast<WCurve1DAssetDocument*>(GetDocument());

  m_pCurveEditor->SetCurveExtents(0, 0.1f, true, false);
  m_pCurveEditor->SetCurves(*pDoc->GetProperties());

  SendLiveResourcePreview();
}

void WQtCurve1DAssetDocumentWindow::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void WQtCurve1DAssetDocumentWindow::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::AfterReset:
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      UpdatePreview();
      break;

    default:
      break;
  }
}

void WQtCurve1DAssetDocumentWindow::SendLiveResourcePreview()
{
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  WResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Curve1D";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WContiguousMemoryStreamStorage streamStorage;
  WMemoryStreamWriter memoryWriter(&streamStorage);

  WCurve1DAssetDocument* pDoc = WDynamicCast<WCurve1DAssetDocument*>(GetDocument());

  // Write Path
  WStringBuilder sAbsFilePath = pDoc->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("WCurve1D");

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

void WQtCurve1DAssetDocumentWindow::RestoreResource()
{
  WRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Curve1D";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
