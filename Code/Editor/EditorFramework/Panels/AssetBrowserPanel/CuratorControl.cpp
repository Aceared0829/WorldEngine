#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>

WQtCuratorControl::WQtCuratorControl(QWidget* pParent)
  : QWidget(pParent)

{
  QHBoxLayout* pLayout = new QHBoxLayout();
  setLayout(pLayout);
  layout()->setContentsMargins(0, 0, 0, 0);
  m_pBackgroundProcess = new QToolButton(this);
  pLayout->addWidget(m_pBackgroundProcess);
  connect(m_pBackgroundProcess, &QAbstractButton::clicked, this, &WQtCuratorControl::BackgroundProcessClicked);
  pLayout->addSpacing(200);

  UpdateBackgroundProcessState();
  WAssetCurator::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WQtCuratorControl::AssetCuratorEvents, this));
  WAssetProcessor::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WQtCuratorControl::AssetProcessorEvents, this));
  WToolsProject::GetSingleton()->s_Events.AddEventHandler(WMakeDelegate(&WQtCuratorControl::ProjectEvents, this));
}

WQtCuratorControl::~WQtCuratorControl()
{
  WAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtCuratorControl::AssetCuratorEvents, this));
  WAssetProcessor::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtCuratorControl::AssetProcessorEvents, this));
  WToolsProject::GetSingleton()->s_Events.RemoveEventHandler(WMakeDelegate(&WQtCuratorControl::ProjectEvents, this));
}

void WQtCuratorControl::paintEvent(QPaintEvent* e)
{
  QRect rect = contentsRect();
  QRect rectButton = m_pBackgroundProcess->geometry();
  rect.setLeft(rectButton.right());

  QPainter painter(this);
  painter.setPen(QPen(Qt::NoPen));

  WUInt32 uiNumAssets;
  WTempHybridArray<WUInt32, WAssetInfo::TransformState::COUNT> sections;
  WAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);
  QColor colors[WAssetInfo::TransformState::COUNT];
  colors[WAssetInfo::TransformState::Unknown] = WToQtColor(WColorScheme::DarkUI(WColorScheme::Gray));
  colors[WAssetInfo::TransformState::NeedsImport] = WToQtColor(WColorScheme::DarkUI(WColorScheme::Yellow));
  colors[WAssetInfo::TransformState::NeedsTransform] = WToQtColor(WColorScheme::DarkUI(WColorScheme::Blue));
  colors[WAssetInfo::TransformState::NeedsThumbnail] = WToQtColor(WColorScheme::DarkUI(float(WColorScheme::Blue + WColorScheme::Green) * 0.5f * WColorScheme::s_fIndexNormalizer));
  colors[WAssetInfo::TransformState::UpToDate] = WToQtColor(WColorScheme::DarkUI(WColorScheme::Green));
  colors[WAssetInfo::TransformState::MissingTransformDependency] = WToQtColor(WColorScheme::DarkUI(WColorScheme::Red));
  colors[WAssetInfo::TransformState::MissingPackageDependency] = WToQtColor(WColorScheme::DarkUI(WColorScheme::Orange));
  colors[WAssetInfo::TransformState::MissingThumbnailDependency] = WToQtColor(WColorScheme::DarkUI(WColorScheme::Orange));
  colors[WAssetInfo::TransformState::CircularDependency] = WToQtColor(WColorScheme::DarkUI(WColorScheme::Red));
  colors[WAssetInfo::TransformState::TransformError] = WToQtColor(WColorScheme::DarkUI(WColorScheme::Red));

  const WUInt32 uiProblems = sections[WAssetInfo::TransformState::MissingTransformDependency] + sections[WAssetInfo::TransformState::MissingThumbnailDependency] + sections[WAssetInfo::TransformState::MissingPackageDependency] + sections[WAssetInfo::TransformState::TransformError] + sections[WAssetInfo::TransformState::CircularDependency];

  if (uiProblems > 0)
  {
    for (auto& col : colors)
    {
      col.setRed(255);
      col.setGreen(WMath::Min(50, col.green()));
      col.setBlue(WMath::Min(50, col.blue()));
    }
  }

  const float fTotalCount = uiNumAssets;
  const WInt32 iTargetWidth = rect.width();
  WInt32 iCurrentCount = 0;
  for (WInt32 i = 0; i < WAssetInfo::TransformState::COUNT; ++i)
  {
    WInt32 iStartX = WInt32((iCurrentCount / fTotalCount) * iTargetWidth);
    iCurrentCount += sections[i];
    WInt32 iEndX = WInt32((iCurrentCount / fTotalCount) * iTargetWidth);

    if (sections[i])
    {
      QRect area = rect;
      area.setLeft(rect.left() + iStartX);
      area.setRight(rect.left() + iEndX);
      painter.setBrush(QBrush(colors[i]));
      painter.drawRect(area);
    }
  }

  WStringBuilder s;

  if (uiProblems > 0)
  {
    s.SetFormat("{} Problems (click)", uiProblems);
  }
  else
  {
    s = "Asset Status";
  }

  painter.setPen(QPen(Qt::white));
  painter.drawText(rect, s.GetData(), QTextOption(Qt::AlignCenter));
}

void WQtCuratorControl::mouseReleaseEvent(QMouseEvent* e)
{
  QWidget::mouseReleaseEvent(e);

  WQtAssetCuratorPanel::GetSingleton()->EnsureVisible();
}

void WQtCuratorControl::UpdateBackgroundProcessState()
{
  WAssetProcessor::ProcessorState state = WAssetProcessor::GetSingleton()->GetProcessorState();
  switch (state)
  {
    case WAssetProcessor::ProcessorState::Stopped:
      m_pBackgroundProcess->setToolTip("Start background asset processing");
      m_pBackgroundProcess->setIcon(WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingStart.svg"));
      break;
    case WAssetProcessor::ProcessorState::Running:
      m_pBackgroundProcess->setToolTip("Stop background asset processing");
      m_pBackgroundProcess->setIcon(WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingPause.svg"));
      break;
    case WAssetProcessor::ProcessorState::Stopping:
      m_pBackgroundProcess->setToolTip("Force stop background asset processing");
      m_pBackgroundProcess->setIcon(WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingForceStop.svg"));
      break;
    default:
      break;
  }

  m_pBackgroundProcess->setCheckable(true);
  m_pBackgroundProcess->setChecked(state == WAssetProcessor::ProcessorState::Running);
}

void WQtCuratorControl::BackgroundProcessClicked(bool checked)
{
  WAssetProcessor::ProcessorState state = WAssetProcessor::GetSingleton()->GetProcessorState();

  if (state == WAssetProcessor::ProcessorState::Stopped)
  {
    WAssetCurator::GetSingleton()->CheckFileSystem();
    WAssetProcessor::GetSingleton()->StartProcessor();
  }
  else
  {
    bool bForce = state == WAssetProcessor::ProcessorState::Stopping;
    WAssetProcessor::GetSingleton()->StopProcessor(bForce);
  }
}

void WQtCuratorControl::SlotUpdateTransformStats()
{
  m_bScheduled = false;

  WUInt32 uiNumAssets;
  WTempHybridArray<WUInt32, WAssetInfo::TransformState::COUNT> sections;
  WAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

  WStringBuilder s;

  if (uiNumAssets > 0)
  {
    s.SetFormat("Unknown: {}\nImport Needed: {}\nTransform Needed: {}\nThumbnail Needed: {}\nMissing Dependency: {}\nCircular Dependency: {}\nFailed Transform: {}",
      sections[WAssetInfo::TransformState::Unknown],
      sections[WAssetInfo::TransformState::NeedsImport],
      sections[WAssetInfo::TransformState::NeedsTransform],
      sections[WAssetInfo::TransformState::NeedsThumbnail],
      sections[WAssetInfo::TransformState::MissingTransformDependency] + sections[WAssetInfo::TransformState::MissingThumbnailDependency] + sections[WAssetInfo::TransformState::MissingPackageDependency],
      sections[WAssetInfo::TransformState::CircularDependency],
      sections[WAssetInfo::TransformState::TransformError]);
    setToolTip(s.GetData());
  }
  else
  {
    setToolTip("");
  }
  update();
}

void WQtCuratorControl::ScheduleUpdateTransformStats()
{
  if (m_bScheduled)
    return;

  m_bScheduled = true;

  QTimer::singleShot(200, this, SLOT(SlotUpdateTransformStats()));
}

void WQtCuratorControl::AssetCuratorEvents(const WAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case WAssetCuratorEvent::Type::AssetUpdated:
      ScheduleUpdateTransformStats();
      break;
    default:
      break;
  }
}

void WQtCuratorControl::AssetProcessorEvents(const WAssetProcessorEvent& e)
{
  switch (e.m_Type)
  {
    case WAssetProcessorEvent::Type::AssetProcessorStateChanged:
    {
      QMetaObject::invokeMethod(this, "UpdateBackgroundProcessState", Qt::QueuedConnection);
    }
    break;
    default:
      break;
  }
}

void WQtCuratorControl::ProjectEvents(const WToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case WToolsProjectEvent::Type::ProjectClosing:
    case WToolsProjectEvent::Type::ProjectClosed:
    case WToolsProjectEvent::Type::ProjectOpened:
      ScheduleUpdateTransformStats();
      break;

    default:
      break;
  }
}
