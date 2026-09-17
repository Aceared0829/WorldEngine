#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetWindow.moc.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <GuiFoundation/Widgets/TimeScrubberWidget.moc.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

WQtAnimationClipAssetDocumentWindow::WQtAnimationClipAssetDocumentWindow(WAnimationClipAssetDocument* pDocument)
  : WQtEngineDocumentWindow(pDocument)
  , m_Clock("AssetClip")
{
  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "AnimationClipAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "AnimationClipAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AnimationClipAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  WQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(WVec3(-1.6f, 0, 0), WVec3(0, 0, 0), WVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new WQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(WVec3(0, 0, 1), WVec3(5.0f), WVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new WQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, "AnimationClipAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("AnimationClipAssetDockWidget");
    pPropertyPanel->setWindowTitle("Animation Clip Properties");
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

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  // Time Scrubber
  {
    m_pTimeScrubber = new WQtTimeScrubberWidget(pContainer);
    m_pTimeScrubber->SetDuration(WTime::MakeFromSeconds(1));

    pContainer->GetLayout()->addWidget(m_pTimeScrubber);

    connect(m_pTimeScrubber, &WQtTimeScrubberWidget::ScrubberPosChangedEvent, this, &WQtAnimationClipAssetDocumentWindow::OnScrubberPosChangedEvent);
  }

  // Event Track Panel
  {
    m_pEventTrackPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    m_pEventTrackPanel->setObjectName("AnimClipEventTrackDockWidget");
    m_pEventTrackPanel->setWindowTitle("Event Track");
    m_pEventTrackPanel->show();

    m_pEventTrackEditor = new WQtEventTrackEditorWidget(m_pEventTrackPanel);
    m_pEventTrackPanel->setWidget(m_pEventTrackEditor);

    m_pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pEventTrackPanel);

    UpdateEventTrackEditor();
  }

  // Event track editor events
  {
    connect(m_pEventTrackEditor, &WQtEventTrackEditorWidget::InsertCpEvent, this, &WQtAnimationClipAssetDocumentWindow::onEventTrackInsertCpAt);
    connect(m_pEventTrackEditor, &WQtEventTrackEditorWidget::CpMovedEvent, this, &WQtAnimationClipAssetDocumentWindow::onEventTrackCpMoved);
    connect(m_pEventTrackEditor, &WQtEventTrackEditorWidget::CpDeletedEvent, this, &WQtAnimationClipAssetDocumentWindow::onEventTrackCpDeleted);

    connect(m_pEventTrackEditor, &WQtEventTrackEditorWidget::BeginOperationEvent, this, &WQtAnimationClipAssetDocumentWindow::onEventTrackBeginOperation);
    connect(m_pEventTrackEditor, &WQtEventTrackEditorWidget::EndOperationEvent, this, &WQtAnimationClipAssetDocumentWindow::onEventTrackEndOperation);
    connect(m_pEventTrackEditor, &WQtEventTrackEditorWidget::BeginCpChangesEvent, this, &WQtAnimationClipAssetDocumentWindow::onEventTrackBeginCpChanges);
    connect(m_pEventTrackEditor, &WQtEventTrackEditorWidget::EndCpChangesEvent, this, &WQtAnimationClipAssetDocumentWindow::onEventTrackEndCpChanges);
  }

  // curve editor
  {
    m_pCurveEditPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    m_pCurveEditPanel->setObjectName("AnimClipCustomCurvesPanel");
    m_pCurveEditPanel->setWindowTitle("Curves");
    m_pCurveEditPanel->show();

    m_pCurveEditor = new WQtCurve1DEditorWidget(this);
    m_pCurveEditPanel->setWidget(m_pCurveEditor);

    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::InsertCpEvent, this, &WQtAnimationClipAssetDocumentWindow::onCurveInsertCpAt);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::CpMovedEvent, this, &WQtAnimationClipAssetDocumentWindow::onCurveCpMoved);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::CpDeletedEvent, this, &WQtAnimationClipAssetDocumentWindow::onCurveCpDeleted);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::TangentMovedEvent, this, &WQtAnimationClipAssetDocumentWindow::onCurveTangentMoved);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::TangentLinkEvent, this, &WQtAnimationClipAssetDocumentWindow::onLinkCurveTangents);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::CpTangentModeEvent, this, &WQtAnimationClipAssetDocumentWindow::onCurveTangentModeChanged);

    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::BeginOperationEvent, this, &WQtAnimationClipAssetDocumentWindow::onCurveBeginOperation);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::EndOperationEvent, this, &WQtAnimationClipAssetDocumentWindow::onCurveEndOperation);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::BeginCpChangesEvent, this, &WQtAnimationClipAssetDocumentWindow::onCurveBeginCpChanges);
    connect(m_pCurveEditor, &WQtCurve1DEditorWidget::EndCpChangesEvent, this, &WQtAnimationClipAssetDocumentWindow::onCurveEndCpChanges);

    m_pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pCurveEditPanel);

    UpdateCurveEditor();
  }

  FinishWindowCreation();

  GetAnimationClipDocument()->m_CommonAssetUiChangeEvent.AddEventHandler(WMakeDelegate(&WQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(WMakeDelegate(&WQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler, this));
  pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WQtAnimationClipAssetDocumentWindow::StructureEventHandler, this));
}

WQtAnimationClipAssetDocumentWindow::~WQtAnimationClipAssetDocumentWindow()
{
  GetAnimationClipDocument()->m_CommonAssetUiChangeEvent.RemoveEventHandler(WMakeDelegate(&WQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WQtAnimationClipAssetDocumentWindow::StructureEventHandler, this));
}

WAnimationClipAssetDocument* WQtAnimationClipAssetDocumentWindow::GetAnimationClipDocument()
{
  return static_cast<WAnimationClipAssetDocument*>(GetDocument());
}

void WQtAnimationClipAssetDocumentWindow::ExtractRootMotionFromFeet()
{
  WSimpleDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo = "ExtractRootMotionFromFeet";

  GetDocument()->SendMessageToEngine(&msg);
}

void WQtAnimationClipAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PlaybackPos";
    msg.m_PayloadValue = (double)(m_PlaybackPosition.GetSeconds() / m_ClipDuration.GetSeconds());
    GetDocument()->SendMessageToEngine(&msg);
  }

  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";
    msg.m_sPayload = GetAnimationClipDocument()->GetProperties()->m_sPreviewMesh;
    GetDocument()->SendMessageToEngine(&msg);
  }
  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewAnim";
    msg.m_sPayload = GetAnimationClipDocument()->GetProperties()->m_sPreviewAnim;
    GetDocument()->SendMessageToEngine(&msg);
  }

  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "SimulationSpeed";

    if (GetAnimationClipDocument()->GetCommonAssetUiState(WCommonAssetUiState::Pause) != 0.0f)
      msg.m_PayloadValue = 0.0;
    else
      msg.m_PayloadValue = GetAnimationClipDocument()->GetCommonAssetUiState(WCommonAssetUiState::SimulationSpeed);

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void WQtAnimationClipAssetDocumentWindow::QueryObjectBBox(WInt32 iPurpose /*= 0*/)
{
  WQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void WQtAnimationClipAssetDocumentWindow::UpdateEventTrackEditor()
{
  auto* pDoc = GetAnimationClipDocument();

  m_pEventTrackEditor->SetData(pDoc->GetProperties()->m_EventTrack, m_ClipDuration.GetSeconds());
}

static WColorGammaUB GetColorForCurveName(WStringView sName, WInt32 iOffset)
{
  return WColorScheme::LightUI(static_cast<WColorScheme::Enum>((iOffset + WHashingUtils::StringHash(sName)) % WColorScheme::Count));
}

void WQtAnimationClipAssetDocumentWindow::UpdateCurveEditor()
{
  auto* pDoc = GetAnimationClipDocument();

  m_Curves.Clear();
  m_Curves.m_bOwnsData = false;

  auto& curves = pDoc->GetProperties()->m_Curves;

  WInt32 iOffset = 0;
  for (auto& namedCurve : curves)
  {
    namedCurve.m_Curve.m_CurveColor = GetColorForCurveName(namedCurve.m_sName, iOffset);
    m_Curves.m_Curves.PushBack(&namedCurve.m_Curve);
    ++iOffset;
  }

  m_pCurveEditor->SetCurveExtents(0.0f, m_ClipDuration.GetSeconds(), true, true);
  m_pCurveEditor->SetCurves(m_Curves);
}

void WQtAnimationClipAssetDocumentWindow::InternalRedraw()
{
  if (m_pTimeScrubber == nullptr)
    return;

  if (m_ClipDuration.IsPositive())
  {
    m_Clock.Update();

    const double fSpeed = GetAnimationClipDocument()->GetCommonAssetUiState(WCommonAssetUiState::SimulationSpeed);

    if (GetAnimationClipDocument()->GetCommonAssetUiState(WCommonAssetUiState::Pause) == 0)
    {
      m_PlaybackPosition += m_Clock.GetTimeDiff() * fSpeed;
    }

    if (m_PlaybackPosition > m_ClipDuration)
    {
      if (GetAnimationClipDocument()->GetCommonAssetUiState(WCommonAssetUiState::Loop) != 0)
      {
        m_PlaybackPosition -= m_ClipDuration;
      }
      else
      {
        m_PlaybackPosition = m_ClipDuration;
      }
    }
  }

  m_PlaybackPosition = WMath::Clamp(m_PlaybackPosition, WTime::MakeZero(), m_ClipDuration);
  m_pTimeScrubber->SetScrubberPosition(m_PlaybackPosition);
  m_pEventTrackEditor->SetScrubberPosition(m_PlaybackPosition);
  m_pCurveEditor->SetScrubberPosition(m_PlaybackPosition);

  WEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  WQtEngineDocumentWindow::InternalRedraw();
}

void WQtAnimationClipAssetDocumentWindow::ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = WDynamicCast<const WQuerySelectionBBoxResultMsgToEditor*>(pMsg0))
  {
    const WQuerySelectionBBoxResultMsgToEditor* pMessage = static_cast<const WQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (pMessage->m_vCenter.IsValid() && pMessage->m_vHalfExtents.IsValid())
    {
      m_pViewWidget->SetOrbitVolume(pMessage->m_vCenter, pMessage->m_vHalfExtents.CompMax(WVec3(0.1f)));
    }
    else
    {
      // try again
      QueryObjectBBox(pMessage->m_iPurpose);
    }

    return;
  }

  if (auto pMsg = WDynamicCast<const WSimpleDocumentConfigMsgToEditor*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "ClipDuration")
    {
      const WTime newDuration = pMsg->m_PayloadValue.Get<WTime>();

      if (m_ClipDuration != newDuration)
      {
        m_ClipDuration = newDuration;

        m_pTimeScrubber->SetDuration(m_ClipDuration);

        UpdateEventTrackEditor();
        UpdateCurveEditor();

        m_pEventTrackEditor->FrameCurve();
        m_pCurveEditor->FrameCurve();
      }

      return;
    }
    else if (pMsg->m_sWhatToDo == "ExtractRootMotionFromFeet")
    {
      const WVec4 vResult = pMsg->m_PayloadValue.Get<WVec4>();

      auto pPropObj = GetAnimationClipDocument()->GetPropertyObject();

      WObjectCommandAccessor acc(GetAnimationClipDocument()->GetCommandHistory());
      acc.StartTransaction("Extract Root Motion From Feet");

      acc.SetValueByName(pPropObj, "RootMotion", (WInt32)WRootMotionSource::Constant).AssertSuccess();
      acc.SetValueByName(pPropObj, "ConstantRootMotion", vResult.GetAsVec3()).AssertSuccess();
      acc.SetValueByName(pPropObj, "RootMotionDistance", vResult.w).AssertSuccess();

      acc.FinishTransaction();

      GetAnimationClipDocument()->GetCommandHistory();

      return;
    }
    else if (pMsg->m_sWhatToDo == "ReportError")
    {
      QString text = WMakeQString(pMsg->m_sPayload);
      QTimer::singleShot(1, [=]()
        {
          // we have to break out of this callback, and execute the UI stuff on the main thread
          WQtUiServices::MessageBoxInformation(WFmt(text.toUtf8().data()));
          //
        });
      return;
    }
  }

  WQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg0);
}

void WQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler(const WCommonAssetUiState& e)
{
  WQtEngineDocumentWindow::CommonAssetUiEventHandler(e);

  if (e.m_State == WCommonAssetUiState::Restart)
  {
    m_PlaybackPosition = WTime::MakeFromSeconds(-1);
  }
}

void WQtAnimationClipAssetDocumentWindow::OnScrubberPosChangedEvent(WUInt64 uiNewScrubberTickPos)
{
  if (m_pTimeScrubber == nullptr || m_ClipDuration.IsZeroOrNegative())
    return;

  m_PlaybackPosition = WTime::MakeFromSeconds(uiNewScrubberTickPos / 4800.0);
}

void WQtAnimationClipAssetDocumentWindow::onEventTrackInsertCpAt(WInt64 tickX, QString value)
{
  auto* pDoc = GetAnimationClipDocument();
  pDoc->InsertEventTrackCpAt(tickX, value.toUtf8().data());
}

void WQtAnimationClipAssetDocumentWindow::onEventTrackCpMoved(WUInt32 cpIdx, WInt64 iTickX)
{
  iTickX = WMath::Max<WInt64>(iTickX, 0);

  auto* pDoc = GetAnimationClipDocument();

  WObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const WAbstractProperty* pTrackProp = WGetStaticRTTI<WAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  const WUuid trackGuid = accessor.Get<WUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const WDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const WVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void WQtAnimationClipAssetDocumentWindow::onEventTrackCpDeleted(WUInt32 cpIdx)
{
  auto* pDoc = GetAnimationClipDocument();

  WObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const WAbstractProperty* pTrackProp = WGetStaticRTTI<WAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  const WUuid trackGuid = accessor.Get<WUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const WDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const WVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  WRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void WQtAnimationClipAssetDocumentWindow::onEventTrackBeginOperation(QString name)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Events");
}

void WQtAnimationClipAssetDocumentWindow::onEventTrackEndOperation(bool commit)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void WQtAnimationClipAssetDocumentWindow::onEventTrackBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void WQtAnimationClipAssetDocumentWindow::onEventTrackEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateEventTrackEditor();
}

/// Returns the WSingleCurveData document object for Curves[uiCurveIdx].m_Curve
static const WDocumentObject* GetCurveSubObject(WAnimationClipAssetDocument* pDoc, WUInt32 uiCurveIdx)
{
  const WVariant namedCurveGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Curves", uiCurveIdx);
  const WDocumentObject* pNamedCurve = pDoc->GetObjectManager()->GetObject(namedCurveGuid.Get<WUuid>());
  const WVariant curveGuid = pNamedCurve->GetTypeAccessor().GetValue("Curve");
  return pDoc->GetObjectManager()->GetObject(curveGuid.Get<WUuid>());
}

void WQtAnimationClipAssetDocumentWindow::onCurveInsertCpAt(WUInt32 uiCurveIdx, WInt64 tickX, double newPosY)
{
  auto* pDoc = static_cast<WAnimationClipAssetDocument*>(GetDocument());

  WCommandHistory* history = pDoc->GetCommandHistory();

  // If there is no curve at uiCurveIdx yet, add a new named curve entry
  while (pDoc->GetPropertyObject()->GetTypeAccessor().GetCount("Curves") <= static_cast<WInt32>(uiCurveIdx))
  {
    WAddObjectCommand cmdAdd;
    cmdAdd.m_Parent = pDoc->GetPropertyObject()->GetGuid();
    cmdAdd.m_sParentProperty = "Curves";
    cmdAdd.m_pType = WGetStaticRTTI<WAnimationClipCurveData>();
    cmdAdd.m_Index = -1;
    cmdAdd.m_NewObjectGuid = WUuid::MakeUuid();
    history->AddCommand(cmdAdd).AssertSuccess();
  }

  const WDocumentObject* pCurveObj = GetCurveSubObject(pDoc, uiCurveIdx);

  WAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = pCurveObj->GetGuid();
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
  cmdSet.m_NewValue = newPosY;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "LeftTangent";
  cmdSet.m_NewValue = WVec2(-0.1f, 0.0f);
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "RightTangent";
  cmdSet.m_NewValue = WVec2(+0.1f, 0.0f);
  history->AddCommand(cmdSet).AssertSuccess();
}

void WQtAnimationClipAssetDocumentWindow::onCurveCpMoved(WUInt32 curveIdx, WUInt32 cpIdx, WInt64 iTickX, double newPosY)
{
  iTickX = WMath::Max<WInt64>(iTickX, 0);

  auto* pDoc = static_cast<WAnimationClipAssetDocument*>(GetDocument());

  const WDocumentObject* pCurveObj = GetCurveSubObject(pDoc, curveIdx);
  const WVariant cpGuid = pCurveObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = newPosY;
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}


void WQtAnimationClipAssetDocumentWindow::onCurveCpDeleted(WUInt32 curveIdx, WUInt32 cpIdx)
{
  auto* pDoc = static_cast<WAnimationClipAssetDocument*>(GetDocument());

  const WDocumentObject* pCurveObj = GetCurveSubObject(pDoc, curveIdx);
  const WVariant cpGuid = pCurveObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  WRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<WUuid>();
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}


void WQtAnimationClipAssetDocumentWindow::onCurveTangentMoved(WUInt32 curveIdx, WUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  auto* pDoc = static_cast<WAnimationClipAssetDocument*>(GetDocument());

  const WDocumentObject* pCurveObj = GetCurveSubObject(pDoc, curveIdx);
  const WVariant cpGuid = pCurveObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

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


void WQtAnimationClipAssetDocumentWindow::onLinkCurveTangents(WUInt32 curveIdx, WUInt32 cpIdx, bool bLink)
{
  auto* pDoc = static_cast<WAnimationClipAssetDocument*>(GetDocument());

  const WDocumentObject* pCurveObj = GetCurveSubObject(pDoc, curveIdx);
  const WVariant cpGuid = pCurveObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmdLink;
  cmdLink.m_Object = cpGuid.Get<WUuid>();
  cmdLink.m_sProperty = "Linked";
  cmdLink.m_NewValue = bLink;
  GetDocument()->GetCommandHistory()->AddCommand(cmdLink).AssertSuccess();

  if (bLink)
  {
    const WVec2 leftTangent = pDoc->GetProperties()->m_Curves[curveIdx].m_Curve.m_ControlPoints[cpIdx].m_LeftTangent;
    const WVec2 rightTangent = -leftTangent;

    onCurveTangentMoved(curveIdx, cpIdx, rightTangent.x, rightTangent.y, true);
  }
}


void WQtAnimationClipAssetDocumentWindow::onCurveTangentModeChanged(WUInt32 curveIdx, WUInt32 cpIdx, bool rightTangent, int mode)
{
  auto* pDoc = static_cast<WAnimationClipAssetDocument*>(GetDocument());

  const WDocumentObject* pCurveObj = GetCurveSubObject(pDoc, curveIdx);
  const WVariant cpGuid = pCurveObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  WSetObjectPropertyCommand cmd;
  cmd.m_Object = cpGuid.Get<WUuid>();
  cmd.m_sProperty = rightTangent ? "RightTangentMode" : "LeftTangentMode";
  cmd.m_NewValue = mode;
  GetDocument()->GetCommandHistory()->AddCommand(cmd).AssertSuccess();
}


void WQtAnimationClipAssetDocumentWindow::onCurveBeginOperation(QString name)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands(name.toUtf8().data());
}

void WQtAnimationClipAssetDocumentWindow::onCurveEndOperation(bool commit)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();

  UpdateCurveEditor();
}

void WQtAnimationClipAssetDocumentWindow::onCurveBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void WQtAnimationClipAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateCurveEditor();
}

void WQtAnimationClipAssetDocumentWindow::OnAfterDocumentLayoutRestored()
{
  // ADS flags dock widgets not found in the saved layout as "unassigned" (closed, detached from all dock areas).
  // Re-add the panel to its default location.
  if (m_pCurveEditPanel->isClosed())
  {
    if (auto* pArea = m_pEventTrackPanel->dockAreaWidget())
    {
      m_pDockManager->addDockWidgetTabToArea(m_pCurveEditPanel, pArea);
    }
    else
    {
      m_pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pCurveEditPanel);
    }
  }
}

void WQtAnimationClipAssetDocumentWindow::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::AfterReset:
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      UpdateCurveEditor();
      break;

    default:
      break;
  }
}

void WQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler(const WCommandHistoryEvent& e)
{
  // also listen to TransactionCanceled, which is sent when a no-op happens (e.g. asset transform with no change)
  // because the event track data object may still get replaced, and we have to get the new pointer
  if (e.m_Type == WCommandHistoryEvent::Type::TransactionEnded || e.m_Type == WCommandHistoryEvent::Type::UndoEnded ||
      e.m_Type == WCommandHistoryEvent::Type::RedoEnded ||
      e.m_Type == WCommandHistoryEvent::Type::TransactionCanceled)
  {
    UpdateEventTrackEditor();
    UpdateCurveEditor();
  }
}
