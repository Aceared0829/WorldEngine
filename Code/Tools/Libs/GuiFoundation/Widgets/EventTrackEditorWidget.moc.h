#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_EventTrackEditorWidget.h>

#include <QWidget>

class W_GUIFOUNDATION_DLL WQtEventTrackEditorWidget : public QWidget, public Ui_EventTrackEditorWidget
{
  Q_OBJECT

public:
  explicit WQtEventTrackEditorWidget(QWidget* pParent);
  ~WQtEventTrackEditorWidget();

  void SetData(const WEventTrackData& data, double fMinCurveLength);
  void SetScrubberPosition(WUInt64 uiTick);
  void SetScrubberPosition(WTime time);
  void ClearSelection();

  void FrameCurve();

Q_SIGNALS:
  void CpMovedEvent(WUInt32 uiIdx, WInt64 iTickX);
  void CpDeletedEvent(WUInt32 uiIdx);
  void InsertCpEvent(WInt64 iTickX, const char* value);

  void BeginCpChangesEvent(QString sName);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString sName);
  void EndOperationEvent(bool bCommit);

private Q_SLOTS:
  void on_LinePosition_editingFinished();
  void on_AddEventButton_clicked();
  void on_InsertEventButton_clicked();
  void onDeleteControlPoints();
  void onDoubleClick(double scenePosX, double epsilon);
  void onMoveControlPoints(double x);
  void onBeginOperation(QString name);
  void onEndOperation(bool commit);
  // void onScaleControlPoints(QPointF refPt, double scaleX);
  void onContextMenu(QPoint pos, QPointF scenePos);
  void onAddPoint();
  void onSelectionChanged();

private:
  void InsertCpAt(double posX, double epsilon);
  void UpdateSpinBoxes();
  void DetermineAvailableEvents();
  void FillEventComboBox(const char* szCurrent = nullptr);

  const WEventTrackData* m_pData = nullptr;
  WEventTrackData m_DataCopy;

  double m_fControlPointMove;
  QPointF m_ContextMenuScenePos;
  WEventSet m_EventSet;
};
