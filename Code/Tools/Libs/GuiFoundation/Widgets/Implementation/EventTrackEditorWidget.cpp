#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <QGraphicsItem>
#include <QGraphicsSceneEvent>
#include <QInputDialog>
#include <QMenu>
#include <QPainterPath>

WQtEventTrackEditorWidget::WQtEventTrackEditorWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  EventTrackEdit->SetGridBarWidget(GridBarWidget);

  // make sure the track is visible and not completely squashed
  EventTrackEdit->setMinimumHeight(50);

  connect(EventTrackEdit, &WQtEventTrackWidget::DeleteControlPointsEvent, this, &WQtEventTrackEditorWidget::onDeleteControlPoints);
  connect(EventTrackEdit, &WQtEventTrackWidget::DoubleClickEvent, this, &WQtEventTrackEditorWidget::onDoubleClick);
  connect(EventTrackEdit, &WQtEventTrackWidget::MoveControlPointsEvent, this, &WQtEventTrackEditorWidget::onMoveControlPoints);
  connect(EventTrackEdit, &WQtEventTrackWidget::BeginOperationEvent, this, &WQtEventTrackEditorWidget::onBeginOperation);
  connect(EventTrackEdit, &WQtEventTrackWidget::EndOperationEvent, this, &WQtEventTrackEditorWidget::onEndOperation);
  // connect(EventTrackEdit, &WQtEventTrackWidget::ScaleControlPointsEvent, this, &WQtEventTrackEditorWidget::onScaleControlPoints);
  connect(EventTrackEdit, &WQtEventTrackWidget::ContextMenuEvent, this, &WQtEventTrackEditorWidget::onContextMenu);
  connect(EventTrackEdit, &WQtEventTrackWidget::SelectionChangedEvent, this, &WQtEventTrackEditorWidget::onSelectionChanged);

  LinePosition->setEnabled(false);

  DetermineAvailableEvents();
}

WQtEventTrackEditorWidget::~WQtEventTrackEditorWidget() = default;

void WQtEventTrackEditorWidget::SetData(const WEventTrackData& trackData, double fMinCurveLength)
{
  WQtScopedUpdatesDisabled ud(this);
  WQtScopedBlockSignals bs(this);

  m_pData = &trackData;
  EventTrackEdit->SetData(&trackData, fMinCurveLength);

  UpdateSpinBoxes();
}

void WQtEventTrackEditorWidget::SetScrubberPosition(WUInt64 uiTick)
{
  EventTrackEdit->SetScrubberPosition(uiTick / 4800.0);
}

void WQtEventTrackEditorWidget::SetScrubberPosition(WTime time)
{
  EventTrackEdit->SetScrubberPosition(time.GetSeconds());
}

void WQtEventTrackEditorWidget::ClearSelection()
{
  EventTrackEdit->ClearSelection();
}

void WQtEventTrackEditorWidget::FrameCurve()
{
  EventTrackEdit->FrameCurve();
}

void WQtEventTrackEditorWidget::on_AddEventButton_clicked()
{
  QString name = QInputDialog::getText(this, "Add Type", "Event Type Name:");

  m_EventSet.AddAvailableEvent(name.toUtf8().data());

  if (m_EventSet.IsModified())
  {
    m_EventSet.WriteToDDL(":project/Editor/Events.ddl").IgnoreResult();

    FillEventComboBox(name.toUtf8().data());
  }
}

void WQtEventTrackEditorWidget::on_InsertEventButton_clicked()
{
  int curveIdx = 0, cpIdx = 0;
  double posX = WMath::Max(EventTrackEdit->GetScrubberPosition(), 0.0);

  Q_EMIT InsertCpEvent(m_pData->TickFromTime(WTime::MakeFromSeconds(posX)), ComboType->currentText().toUtf8().data());
}

void WQtEventTrackEditorWidget::onDeleteControlPoints()
{
  WTempHybridArray<WUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (selection.IsEmpty())
    return;

  EventTrackEdit->ClearSelection();

  Q_EMIT BeginCpChangesEvent("Delete Events");

  selection.Sort([](WUInt32 lhs, WUInt32 rhs) -> bool
    { return lhs > rhs; });

  // delete sorted from back to front to prevent point indices becoming invalidated
  for (WUInt32 pt : selection)
  {
    Q_EMIT CpDeletedEvent(pt);
  }

  Q_EMIT EndCpChangesEvent();
}

void WQtEventTrackEditorWidget::onDoubleClick(double scenePosX, double epsilon)
{
  InsertCpAt(scenePosX, WMath::Abs(epsilon));
}

void WQtEventTrackEditorWidget::onMoveControlPoints(double x)
{
  m_fControlPointMove += x;

  WTempHybridArray<WUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Move Events");

  for (const auto& cpSel : selection)
  {
    auto& cp = m_DataCopy.m_ControlPoints[cpSel];

    double newPos = cp.GetTickAsTime().GetSeconds() + m_fControlPointMove;
    newPos = WMath::Max(newPos, 0.0);

    Q_EMIT CpMovedEvent(cpSel, m_pData->TickFromTime(WTime::MakeFromSeconds(newPos)));
  }

  Q_EMIT EndCpChangesEvent();
}

// void WQtEventTrackEditorWidget::onScaleControlPoints(QPointF refPt, double scaleX, double scaleY)
//{
//  const auto selection = EventTrackEdit->GetSelection();
//
//  if (selection.IsEmpty())
//    return;
//
//  const WVec2d ref(refPt.x(), refPt.y());
//  const WVec2d scale(scaleX, scaleY);
//
//  Q_EMIT BeginCpChangesEvent("Scale Points");
//
//  for (const auto& cpSel : selection)
//  {
//    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
//    WVec2d newPos = ref + (WVec2d(cp.GetTickAsTime(), cp.m_fValue) - ref).CompMul(scale);
//    newPos.x = WMath::Max(newPos.x, 0.0);
//    newPos.y = WMath::Clamp(newPos.y, -100000.0, +100000.0);
//
//    Q_EMIT CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(newPos.x), newPos.y);
//  }
//
//  Q_EMIT EndCpChangesEvent();
//}

void WQtEventTrackEditorWidget::onBeginOperation(QString name)
{
  m_fControlPointMove = 0;
  m_DataCopy = *m_pData;

  Q_EMIT BeginOperationEvent(name);
}

void WQtEventTrackEditorWidget::onEndOperation(bool commit)
{
  Q_EMIT EndOperationEvent(commit);
}

void WQtEventTrackEditorWidget::onContextMenu(QPoint pos, QPointF scenePos)
{
  m_ContextMenuScenePos = scenePos;

  QMenu m(this);
  m.setDefaultAction(m.addAction("Add Event", this, SLOT(onAddPoint())));

  WTempHybridArray<WUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (!selection.IsEmpty())
  {
    m.addAction("Delete Events", QKeySequence(Qt::Key_Delete), this, SLOT(onDeleteControlPoints()));
  }

  m.addSeparator();

  m.addAction("Frame", QKeySequence(Qt::ControlModifier | Qt::Key_F), this, [this]()
    { FrameCurve(); });

  m.exec(pos);
}

void WQtEventTrackEditorWidget::onAddPoint()
{
  InsertCpAt(m_ContextMenuScenePos.x(), 0.0f);
}

void WQtEventTrackEditorWidget::InsertCpAt(double posX, double epsilon)
{
  int curveIdx = 0, cpIdx = 0;
  posX = WMath::Max(posX, 0.0);

  Q_EMIT InsertCpEvent(m_pData->TickFromTime(WTime::MakeFromSeconds(posX)), ComboType->currentText().toUtf8().data());
}

void WQtEventTrackEditorWidget::onSelectionChanged()
{
  UpdateSpinBoxes();
}

void WQtEventTrackEditorWidget::UpdateSpinBoxes()
{
  WTempHybridArray<WUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  WQtScopedBlockSignals _1(LinePosition, SelectedTypeLabel);

  if (selection.IsEmpty())
  {
    LinePosition->setText(QString());
    LinePosition->setEnabled(false);
    SelectedTypeLabel->setText("Event: none");
    return;
  }

  const double fPos = m_pData->m_ControlPoints[selection[0]].GetTickAsTime().GetSeconds();

  LinePosition->setEnabled(true);

  WStringBuilder labelText("Event: ", m_pData->m_ControlPoints[selection[0]].m_sEvent.GetString());

  bool bMultipleTicks = false;
  for (WUInt32 i = 1; i < selection.GetCount(); ++i)
  {
    const WString& sName = m_pData->m_ControlPoints[selection[i]].m_sEvent.GetString();
    const double fPos2 = m_pData->m_ControlPoints[selection[i]].GetTickAsTime().GetSeconds();

    if (!labelText.FindSubString(sName))
    {
      labelText.Append(", ", sName);
    }

    if (fPos2 != fPos)
    {
      bMultipleTicks = true;
      break;
    }
  }

  LinePosition->setText(bMultipleTicks ? QString() : QString::number(fPos, 'f', 2));
  SelectedTypeLabel->setText(labelText.GetData());
}

void WQtEventTrackEditorWidget::DetermineAvailableEvents()
{
  m_EventSet.ReadFromDDL(":project/Editor/Events.ddl").IgnoreResult();

  FillEventComboBox(nullptr);
}

void WQtEventTrackEditorWidget::FillEventComboBox(const char* szCurrent)
{
  QString prev = szCurrent;

  if (prev.isEmpty())
    prev = ComboType->currentText();

  ComboType->clear();

  for (const WString& type : m_EventSet.GetAvailableEvents())
  {
    ComboType->addItem(type.GetData());
  }

  ComboType->setCurrentText(prev);
}

void WQtEventTrackEditorWidget::on_LinePosition_editingFinished()
{
  QString sValue = LinePosition->text();

  bool ok = false;
  const double value = sValue.toDouble(&ok);
  if (!ok)
    return;

  if (value < 0)
    return;

  WTempHybridArray<WUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);
  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Set Event Time");

  WInt64 tick = m_pData->TickFromTime(WTime::MakeFromSeconds(value));

  for (const auto& cpSel : selection)
  {
    if (m_pData->m_ControlPoints[cpSel].m_iTick != tick)
      Q_EMIT CpMovedEvent(cpSel, tick);
  }

  Q_EMIT EndCpChangesEvent();
}
