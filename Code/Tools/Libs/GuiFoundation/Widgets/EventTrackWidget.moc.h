#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec2.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

#include <QBrush>
#include <QPen>
#include <QWidget>

class WQGridBarWidget;
class QRubberBand;

class W_GUIFOUNDATION_DLL WQtEventTrackWidget : public QWidget
{
  Q_OBJECT

public:
  struct SelectedPoint
  {
    W_DECLARE_POD_TYPE();

    WUInt32 m_uiCategory;
    WUInt32 m_uiSortedIdx;
  };

  WQtEventTrackWidget(QWidget* pParent);

  void SetData(const WEventTrackData* pData, double fMinCurveLength);
  void SetGridBarWidget(WQGridBarWidget* pGridBar) { m_pGridBar = pGridBar; }

  void SetScrubberPosition(double fPosition);
  double GetScrubberPosition() const { return m_fScrubberPosition; }

  void FrameCurve();

  QPoint MapFromScene(const QPointF& pos) const;
  QPointF MapToScene(const QPoint& pos) const;

  void ClearSelection();
  void GetSelection(WDynamicArray<WUInt32>& out_selection) const;

Q_SIGNALS:
  void DoubleClickEvent(double fScenePosX, double fEpsilon);
  void DeleteControlPointsEvent();
  void MoveControlPointsEvent(double fMoveX);
  void BeginOperationEvent(QString sName);
  void EndOperationEvent(bool bCommit);
  void ScaleControlPointsEvent(const QPointF& centerPos, double fScaleX);
  void ContextMenuEvent(QPoint pos, QPointF scenePos);
  void SelectionChangedEvent();

protected:
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void keyPressEvent(QKeyEvent* e) override;

private:
  enum class ClickTarget
  {
    Nothing,
    SelectedPoint
  };
  enum class EditState
  {
    None,
    DraggingPoints,
    MultiSelect,
    RightClick,
    Panning,
    ScaleLeftRight
  };
  enum class SelectArea
  {
    None,
    Center,
    Left,
    Right
  };

  struct Point
  {
    W_DECLARE_POD_TYPE();

    WUInt32 m_uiOrgIndex;
    bool m_bSelected;
    double m_fPosX;
  };

  struct PointCategory
  {
    WHashedString m_sName;
    WHybridArray<Point, 32> m_SortedPoints;
  };

  bool IsSelected(SelectedPoint cp) const;
  void SetSelection(SelectedPoint cp);
  void SetSelection(const WArrayPtr<SelectedPoint>& selection);
  void ToggleSelected(SelectedPoint cp);
  void SetSelected(SelectedPoint cp, bool set);

  void PaintOutsideAreaOverlay(QPainter* painter) const;
  void PaintControlPoints(QPainter* painter) const;
  void PaintMultiSelectionSquare(QPainter* painter) const;
  void PaintScrubber(QPainter& p) const;
  void RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity);
  QRectF ComputeViewportSceneRect() const;
  bool PickCpAt(const QPoint& pos, float fMaxPixelDistance, SelectedPoint& out_Result) const;
  ClickTarget DetectClickTarget(const QPoint& pos);
  void ExecMultiSelection(WDynamicArray<SelectedPoint>& out_Selection);
  bool CombineSelection(WDynamicArray<SelectedPoint>& inout_Selection, const WArrayPtr<SelectedPoint>& change, bool add);
  void ComputeSelectionRect();
  SelectArea WhereIsPoint(QPoint pos) const;
  void ClampZoomPan();
  void RecreateSortedData();

  WQGridBarWidget* m_pGridBar = nullptr;

  EditState m_State = EditState::None;

  const WEventTrackData* m_pEditData = nullptr;

  double m_fMaxCurveExtent = 0;
  double m_fSceneTranslationX = 0;
  QPointF m_SceneToPixelScale;
  QPoint m_LastMousePos;

  QBrush m_ControlPointBrush;
  QBrush m_SelectedControlPointBrush;
  QPen m_ControlPointPen;

  bool m_bBegunChanges = false;
  bool m_bFrameBeforePaint = true;

  QPoint m_MultiSelectionStart;
  QRect m_MultiSelectRect;
  QRectF m_SelectionBRect;
  QPointF m_ScaleReferencePoint;
  QPointF m_ScaleStartPoint;
  QPointF m_TotalPointDrag;
  QRubberBand* m_pRubberband = nullptr;

  bool m_bShowScrubber = false;
  double m_fScrubberPosition = 0;

  WHashTable<WHashedString, WUInt32> m_NameToCategory;
  WHybridArray<PointCategory, 8> m_Categories;
  WHybridArray<SelectedPoint, 32> m_SelectedPoints;
};
