#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Gravity.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Continuous.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomColor.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_VelocityCone.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>
#include <QBoxLayout>
#include <QComboBox>
#include <QInputDialog>
#include <QToolButton>
#include <SharedPluginAssets/Common/Messages.h>
#include <ToolsFoundation/Command/TreeCommands.h>

WQtParticleEffectAssetDocumentWindow::WQtParticleEffectAssetDocumentWindow(WAssetDocument* pDocument)
  : WQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtParticleEffectAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WQtParticleEffectAssetDocumentWindow::StructureEventHandler, this));


  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "ParticleEffectAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "ParticleEffectAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ParticleEffectAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  WDocumentObject* pRootObject = pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0];

  // WQtDocumentPanel* pMainPropertyPanel = new WQtDocumentPanel(this);
  WQtDocumentPanel* pEffectPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
  WQtDocumentPanel* pReactionsPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
  WQtDocumentPanel* pSystemsPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
  WQtDocumentPanel* pEmitterPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
  WQtDocumentPanel* pInitializerPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
  WQtDocumentPanel* pBehaviorPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
  WQtDocumentPanel* pTypePanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);


  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(WVec3(-1.6f, 0, 0), WVec3(0, 0, 0), WVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new WQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(WVec3(0), WVec3(5.0f), WVec3(-2, 0, 0.5f), 1.0f);
    AddViewWidget(m_pViewWidget);
    WQtViewWidgetContainer* pContainer = new WQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, "ParticleEffectAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  //{
  //  pMainPropertyPanel->setObjectName("ParticleEffectAssetDockWidget");
  //  pMainPropertyPanel->setWindowTitle("Particle Effect Properties");
  //  pMainPropertyPanel->show();

  //  WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pMainPropertyPanel, pDocument);
  //  pMainPropertyPanel->setWidget(pPropertyGrid);

  //  addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pMainPropertyPanel);

  //  pDocument->GetSelectionManager()->SetSelection(pRootObject);
  //}

  // Effect Properties
  {
    pEffectPanel->setObjectName("ParticleEffectAssetDockWidget_Effect");
    pEffectPanel->setWindowTitle("Effect");
    pEffectPanel->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pEffectPanel, pDocument, false);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new WQtAssetStatusIndicator((WAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pEffectPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    WDeque<const WDocumentObject*> sel;
    sel.PushBack(pRootObject);
    pPropertyGrid->SetSelectionIncludeExcludeProperties(nullptr, "EventReactions;ParticleSystems");
    pPropertyGrid->SetSelection(sel);

    m_pDockManager->addDockWidget(ads::RightDockWidgetArea, pEffectPanel);
  }

  // Particle Systems Panel
  {
    pSystemsPanel->setObjectName("ParticleEffectAssetDockWidget_Systems");
    pSystemsPanel->setWindowTitle("Systems");
    pSystemsPanel->show();

    QWidget* pMainWidget = new QFrame(pSystemsPanel);
    pMainWidget->setContentsMargins(0, 0, 0, 0);
    pMainWidget->setLayout(new QVBoxLayout(pMainWidget));
    pMainWidget->layout()->setContentsMargins(0, 0, 0, 0);

    {
      QWidget* pGroup = new QWidget(pMainWidget);
      pGroup->setContentsMargins(0, 0, 0, 0);
      pGroup->setLayout(new QHBoxLayout(pGroup));
      pGroup->layout()->setContentsMargins(0, 0, 0, 0);

      m_pSystemsCombo = new QComboBox(pSystemsPanel);
      connect(m_pSystemsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onSystemSelected(int)));

      m_pAddSystem = new QToolButton(pSystemsPanel);
      connect(m_pAddSystem, &QAbstractButton::clicked, this, &WQtParticleEffectAssetDocumentWindow::onAddSystem);

      m_pRemoveSystem = new QToolButton(pSystemsPanel);
      connect(m_pRemoveSystem, &QAbstractButton::clicked, this, &WQtParticleEffectAssetDocumentWindow::onRemoveSystem);

      m_pRenameSystem = new QToolButton(pSystemsPanel);
      connect(m_pRenameSystem, &QAbstractButton::clicked, this, &WQtParticleEffectAssetDocumentWindow::onRenameSystem);

      m_pAddSystem->setToolTip("Add another particle system to the effect.");
      m_pRemoveSystem->setToolTip("Remove this particle system from the effect.");
      m_pRenameSystem->setToolTip("Rename this particle system.");

      m_pAddSystem->setIcon(QIcon(":/GuiFoundation/Icons/Add.svg"));
      m_pRemoveSystem->setIcon(QIcon(":/GuiFoundation/Icons/Delete.svg"));
      m_pRenameSystem->setIcon(QIcon(":/GuiFoundation/Icons/Rename.svg"));

      pGroup->layout()->addWidget(m_pRenameSystem);
      pGroup->layout()->addWidget(m_pSystemsCombo);
      pGroup->layout()->addWidget(m_pAddSystem);
      pGroup->layout()->addWidget(m_pRemoveSystem);

      pMainWidget->layout()->addWidget(pGroup);
    }

    m_pPropertyGridSystems = new WQtPropertyGridWidget(pSystemsPanel, pDocument);
    m_pPropertyGridSystems->SetSelectionIncludeExcludeProperties(nullptr, "Name;Emitters;Initializers;Behaviors;Types");
    pMainWidget->layout()->addWidget(m_pPropertyGridSystems);

    if (!pRootObject->GetChildren().IsEmpty())
    {
      WDeque<const WDocumentObject*> sel;
      sel.PushBack(pRootObject->GetChildren()[0]);
      m_pPropertyGridSystems->SetSelection(sel);
    }

    pSystemsPanel->setWidget(pMainWidget);

    m_pDockManager->addDockWidget(ads::CenterDockWidgetArea, pSystemsPanel, pEffectPanel->dockAreaWidget());
  }

  // Event Reactions
  {
    pReactionsPanel->setObjectName("ParticleEffectAssetDockWidget_Reactions");
    pReactionsPanel->setWindowTitle("Event Reactions");
    pReactionsPanel->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pReactionsPanel, pDocument, false);
    pReactionsPanel->setWidget(pPropertyGrid);

    WDeque<const WDocumentObject*> sel;
    sel.PushBack(pRootObject);
    pPropertyGrid->SetSelectionIncludeExcludeProperties("EventReactions");
    pPropertyGrid->SetSelection(sel);

    m_pDockManager->addDockWidget(ads::CenterDockWidgetArea, pReactionsPanel, pEffectPanel->dockAreaWidget());
  }

  // System Emitters
  {
    pEmitterPanel->setObjectName("ParticleEffectAssetDockWidget_Emitter");
    pEmitterPanel->setWindowTitle("Emitter");
    pEmitterPanel->show();

    m_pPropertyGridEmitter = new WQtPropertyGridWidget(pEmitterPanel, pDocument, false);
    m_pPropertyGridEmitter->SetSelectionIncludeExcludeProperties("Emitters");
    pEmitterPanel->setWidget(m_pPropertyGridEmitter);

    m_pDockManager->addDockWidget(ads::BottomDockWidgetArea, pEmitterPanel, pEffectPanel->dockAreaWidget());
  }

  // System Initializers
  {
    pInitializerPanel->setObjectName("ParticleEffectAssetDockWidget_Initializer");
    pInitializerPanel->setWindowTitle("Initializers");
    pInitializerPanel->show();

    m_pPropertyGridInitializer = new WQtPropertyGridWidget(pInitializerPanel, pDocument, false);
    m_pPropertyGridInitializer->SetSelectionIncludeExcludeProperties("Initializers");
    pInitializerPanel->setWidget(m_pPropertyGridInitializer);

    m_pDockManager->addDockWidget(ads::CenterDockWidgetArea, pInitializerPanel, pEmitterPanel->dockAreaWidget());
  }

  // System Behaviors
  {
    pBehaviorPanel->setObjectName("ParticleEffectAssetDockWidget_Behavior");
    pBehaviorPanel->setWindowTitle("Behaviors");
    pBehaviorPanel->show();

    m_pPropertyGridBehavior = new WQtPropertyGridWidget(pBehaviorPanel, pDocument, false);
    m_pPropertyGridBehavior->SetSelectionIncludeExcludeProperties("Behaviors");
    pBehaviorPanel->setWidget(m_pPropertyGridBehavior);

    m_pDockManager->addDockWidget(ads::CenterDockWidgetArea, pBehaviorPanel, pEmitterPanel->dockAreaWidget());
  }

  // System Types
  {
    pTypePanel->setObjectName("ParticleEffectAssetDockWidget_Type");
    pTypePanel->setWindowTitle("Renderers");
    pTypePanel->show();

    m_pPropertyGridType = new WQtPropertyGridWidget(pTypePanel, pDocument, false);
    m_pPropertyGridType->SetSelectionIncludeExcludeProperties("Types");
    pTypePanel->setWidget(m_pPropertyGridType);

    m_pDockManager->addDockWidget(ads::CenterDockWidgetArea, pTypePanel, pEmitterPanel->dockAreaWidget());
  }

  m_pAssetDoc = static_cast<WParticleEffectAssetDocument*>(pDocument);

  pSystemsPanel->raise();
  pEmitterPanel->raise();

  FinishWindowCreation();

  UpdateSystemList();

  GetParticleDocument()->m_Events.AddEventHandler(WMakeDelegate(&WQtParticleEffectAssetDocumentWindow::ParticleEventHandler, this));
}

WQtParticleEffectAssetDocumentWindow::~WQtParticleEffectAssetDocumentWindow()
{
  GetParticleDocument()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtParticleEffectAssetDocumentWindow::ParticleEventHandler, this));

  RestoreResource();

  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WQtParticleEffectAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtParticleEffectAssetDocumentWindow::PropertyEventHandler, this));
}

WParticleEffectAssetDocument* WQtParticleEffectAssetDocumentWindow::GetParticleDocument()
{
  return static_cast<WParticleEffectAssetDocument*>(GetDocument());
}

void WQtParticleEffectAssetDocumentWindow::SelectSystem(const WDocumentObject* pObject)
{
  if (pObject == nullptr)
  {
    m_sSelectedSystem.Clear();

    m_pPropertyGridSystems->ClearSelection();
    m_pPropertyGridEmitter->ClearSelection();
    m_pPropertyGridInitializer->ClearSelection();
    m_pPropertyGridBehavior->ClearSelection();
    m_pPropertyGridType->ClearSelection();

    if (m_pSystemsCombo->currentIndex() != -1)
    {
      // prevent infinite recursion
      m_pSystemsCombo->setCurrentIndex(-1);
    }
  }
  else
  {
    m_sSelectedSystem = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<WString>();

    WDeque<const WDocumentObject*> sel;
    sel.PushBack(pObject);
    GetDocument()->GetSelectionManager()->SetSelection(pObject);
    m_pPropertyGridSystems->SetSelection(sel);

    m_pPropertyGridEmitter->SetSelection(sel);
    m_pPropertyGridInitializer->SetSelection(sel);
    m_pPropertyGridBehavior->SetSelection(sel);
    m_pPropertyGridType->SetSelection(sel);

    m_pSystemsCombo->setCurrentText(m_sSelectedSystem.GetData());
  }
}

void WQtParticleEffectAssetDocumentWindow::onSystemSelected(int index)
{
  if (index >= 0)
  {
    WDocumentObject* pObject = static_cast<WDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

    SelectSystem(pObject);
  }
  else
  {
    SelectSystem(nullptr);
  }
}

WStatus WQtParticleEffectAssetDocumentWindow::SetupSystem(WStringView sName)
{
  const WDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];
  WObjectAccessorBase* pAccessor = GetDocument()->GetObjectAccessor();

  WUuid systemGuid = WUuid::MakeUuid();

  W_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pRootObject, "ParticleSystems", -1, WGetStaticRTTI<WParticleSystemDescriptor>(), systemGuid));

  const WDocumentObject* pSystemObject = pAccessor->GetObject(systemGuid);

  W_SUCCEED_OR_RETURN(pAccessor->SetValueByName(pSystemObject, "Name", sName));

  // default system setup
  {
    {
      WVarianceTypeTime val;
      val.m_Value = WTime::MakeFromSeconds(1.0f);
      W_SUCCEED_OR_RETURN(pAccessor->SetValueByName(pSystemObject, "LifeTime", val));
    }

    // add emitter
    {
      WUuid emitterGuid = WUuid::MakeUuid();
      W_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pSystemObject, "Emitters", -1, WGetStaticRTTI<WParticleEmitterFactory_Continuous>(), emitterGuid));
    }

    // add cone velocity initializer
    {
      WUuid velocityGuid = WUuid::MakeUuid();
      W_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pSystemObject, "Initializers", -1, WGetStaticRTTI<WParticleInitializerFactory_VelocityCone>(), velocityGuid));

      const WDocumentObject* pConeObject = pAccessor->GetObject(velocityGuid);

      // default speed
      {
        WVarianceTypeFloat val;
        val.m_Value = 4.0f;
        W_SUCCEED_OR_RETURN(pAccessor->SetValueByName(pConeObject, "Speed", val));
      }
    }

    // add color initializer
    {
      WUuid colorInitGuid = WUuid::MakeUuid();
      W_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pSystemObject, "Initializers", -1, WGetStaticRTTI<WParticleInitializerFactory_RandomColor>(), colorInitGuid));

      const WDocumentObject* pColorObject = pAccessor->GetObject(colorInitGuid);

      W_SUCCEED_OR_RETURN(pAccessor->SetValueByName(pColorObject, "Color1", WColor::Red));
      W_SUCCEED_OR_RETURN(pAccessor->SetValueByName(pColorObject, "Color2", WColor::Yellow));
    }

    // add gravity behavior
    {
      WUuid gravityGuid = WUuid::MakeUuid();
      W_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pSystemObject, "Behaviors", -1, WGetStaticRTTI<WParticleBehaviorFactory_Gravity>(), gravityGuid));
    }

    // add quad renderer
    {
      WUuid quadGuid = WUuid::MakeUuid();
      W_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pSystemObject, "Types", -1, WGetStaticRTTI<WParticleTypeQuadFactory>(), quadGuid));
    }
  }

  m_sSelectedSystem = sName;
  UpdateSystemList();
  SelectSystem(pSystemObject);

  return WStatus(W_SUCCESS);
}

void WQtParticleEffectAssetDocumentWindow::onAddSystem(bool)
{
  bool ok = false;
  QString sName;

  while (true)
  {
    sName = QInputDialog::getText(this, "New Particle System", "Name:", QLineEdit::Normal, QString(), &ok);

    if (!ok)
      return;

    if (sName.isEmpty())
    {
      WQtUiServices::GetSingleton()->MessageBoxInformation("Invalid particle system name.");
      continue;
    }

    if (m_ParticleSystems.Find(sName.toUtf8().data()).IsValid())
    {
      WQtUiServices::GetSingleton()->MessageBoxInformation("A particle system with this name exists already.");
      continue;
    }

    break;
  }

  WObjectAccessorBase* pAccessor = GetDocument()->GetObjectAccessor();

  pAccessor->StartTransaction("Add Particle System");

  if (SetupSystem(qtToEzString(sName)).Failed())
  {
    pAccessor->CancelTransaction();
  }
  else
  {
    pAccessor->FinishTransaction();
  }

  m_bDoLiveResourceUpdate = true;
}

void WQtParticleEffectAssetDocumentWindow::onRemoveSystem(bool)
{
  const int index = m_pSystemsCombo->findText(m_sSelectedSystem.GetData());
  if (index < 0)
    return;

  const WDocumentObject* pObject = static_cast<WDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

  GetDocument()->GetObjectAccessor()->StartTransaction("Rename Particle System");

  WRemoveObjectCommand cmd;
  cmd.m_Object = pObject->GetGuid();

  if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
  {
    GetDocument()->GetObjectAccessor()->CancelTransaction();
    return;
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();

  UpdateSystemList();
}

void WQtParticleEffectAssetDocumentWindow::onRenameSystem(bool)
{
  const int index = m_pSystemsCombo->findText(m_sSelectedSystem.GetData());
  if (index < 0)
    return;

  const WDocumentObject* pObject = static_cast<WDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

  bool ok = false;
  const QString sOrgName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<WString>().GetData();
  QString sName;

  while (true)
  {
    sName = QInputDialog::getText(this, "Rename Particle System", "Name:", QLineEdit::Normal, sOrgName, &ok);

    if (!ok || sName == sOrgName)
      return;

    if (sName.isEmpty())
    {
      WQtUiServices::GetSingleton()->MessageBoxInformation("Invalid particle system name.");
      continue;
    }

    if (m_ParticleSystems.Find(sName.toUtf8().data()).IsValid())
    {
      WQtUiServices::GetSingleton()->MessageBoxInformation("A particle system with this name exists already.");
      continue;
    }

    break;
  }

  m_sSelectedSystem = sName.toUtf8().data();

  GetDocument()->GetObjectAccessor()->StartTransaction("Rename Particle System");

  WSetObjectPropertyCommand cmd2;
  cmd2.m_Object = pObject->GetGuid();
  cmd2.m_NewValue = sName.toUtf8().data();
  cmd2.m_sProperty = "Name";

  if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
  {
    GetDocument()->GetObjectAccessor()->CancelTransaction();
    return;
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();

  UpdateSystemList();
}

void WQtParticleEffectAssetDocumentWindow::SendLiveResourcePreview()
{
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  WResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Particle Effect";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WContiguousMemoryStreamStorage streamStorage;
  WMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  WStringBuilder sAbsFilePath = GetParticleDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("WParticleEffect");

  // Write Header
  memoryWriter << sAbsFilePath;
  const WUInt64 uiHash = WAssetCurator::GetSingleton()->GetAssetTransformHash(GetParticleDocument()->GetGuid());
  WAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetParticleDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  GetParticleDocument()->WriteResource(memoryWriter);
  msg.m_Data = WArrayPtr<const WUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtParticleEffectAssetDocumentWindow::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  m_bDoLiveResourceUpdate = true;

  if (e.m_sProperty == "Name" && e.m_pObject->GetParentProperty() == "ParticleSystems")
  {
    UpdateSystemList();
  }
}

void WQtParticleEffectAssetDocumentWindow::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      m_bDoLiveResourceUpdate = true;
      break;

    default:
      break;
  }
}


void WQtParticleEffectAssetDocumentWindow::ParticleEventHandler(const WParticleEffectAssetEvent& e)
{
  switch (e.m_Type)
  {
    case WParticleEffectAssetEvent::RestartEffect:
    {
      WEditorEngineRestartSimulationMsg msg;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;

    case WParticleEffectAssetEvent::AutoRestartChanged:
    {
      WEditorEngineLoopAnimationMsg msg;
      msg.m_bLoop = GetParticleDocument()->GetAutoRestart();
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;

    default:
      break;
  }
}

void WQtParticleEffectAssetDocumentWindow::UpdateSystemList()
{
  WMap<WString, WDocumentObject*> newParticleSystems;

  WDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  WStringBuilder s;

  for (WDocumentObject* pChild : pRootObject->GetChildren())
  {
    if (pChild->GetParentProperty() == "ParticleSystems"_wsv)
    {
      s = pChild->GetTypeAccessor().GetValue("Name").ConvertTo<WString>();
      newParticleSystems[s] = pChild;
    }
  }

  // early out
  if (m_ParticleSystems == newParticleSystems)
    return;

  m_ParticleSystems.Swap(newParticleSystems);

  {
    WQtScopedBlockSignals _1(m_pSystemsCombo);
    m_pSystemsCombo->clear();

    for (auto it = m_ParticleSystems.GetIterator(); it.IsValid(); ++it)
    {
      m_pSystemsCombo->addItem(it.Key().GetData(), QVariant::fromValue<void*>(it.Value()));
    }
  }

  if (!m_ParticleSystems.Find(m_sSelectedSystem).IsValid())
    m_sSelectedSystem.Clear();

  if (m_sSelectedSystem.IsEmpty() && !m_ParticleSystems.IsEmpty())
    m_sSelectedSystem = m_ParticleSystems.GetIterator().Key();

  if (!m_ParticleSystems.IsEmpty())
  {
    SelectSystem(m_ParticleSystems[m_sSelectedSystem]);
  }
  else
  {
    SelectSystem(nullptr);
  }

  const bool hasSelection = !m_ParticleSystems.IsEmpty();

  m_pSystemsCombo->setEnabled(hasSelection);
  m_pRemoveSystem->setEnabled(hasSelection);
  m_pRenameSystem->setEnabled(hasSelection);
}


void WQtParticleEffectAssetDocumentWindow::InternalRedraw()
{
  WEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  WQtEngineDocumentWindow::InternalRedraw();
}


void WQtParticleEffectAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  if (m_bDoLiveResourceUpdate)
  {
    SendLiveResourcePreview();
    m_bDoLiveResourceUpdate = false;
  }

  {
    WSimulationSettingsMsgToEngine msg;
    msg.m_bSimulateWorld = !GetParticleDocument()->GetSimulationPaused();
    msg.m_fSimulationSpeed = GetParticleDocument()->GetSimulationSpeed();
    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void WQtParticleEffectAssetDocumentWindow::RestoreResource()
{
  WRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Particle Effect";

  WStringBuilder tmp;
  msg.m_sResourceID = WConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
