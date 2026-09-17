#pragma once

#include <Foundation/Math/CurveFunctions.h>
#include <Foundation/Tracks/Curve1D.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_Curve1DEditorWidget.h>

#include <QWidget>

class W_GUIFOUNDATION_DLL WQtCurve1DEditorWidget : public QWidget, public Ui_Curve1DEditorWidget
{
  Q_OBJECT

public:
  explicit WQtCurve1DEditorWidget(QWidget* pParent);
  ~WQtCurve1DEditorWidget();

  void SetCurveExtents(double fLowerBound, double fUpperBound, bool bLowerIsFixed, bool bUpperIsFixed);
  void SetCurveRanges(double fLowerRange, double fUpperRange);

  void SetCurves(const WCurveGroupData& curveData);
  void SetScrubberPosition(WUInt64 uiTick);
  void SetScrubberPosition(WTime time);
  void ClearSelection();

  void FrameCurve();
  void FrameSelection();
  void MakeRepeatable(bool bAdjustLastPoint);
  void NormalizeCurveX(WUInt32 uiActiveCurve);
  void NormalizeCurveY(WUInt32 uiActiveCurve);
  void ClearAllPoints();
  void MirrorHorizontally(WUInt32 uiActiveCurve);
  void MirrorVertically(WUInt32 uiActiveCurve);

Q_SIGNALS:
  void CpMovedEvent(WUInt32 uiCurveIdx, WUInt32 uiIdx, WInt64 iTickX, double fNewPosY);
  void CpDeletedEvent(WUInt32 uiCurveIdx, WUInt32 uiIdx);
  void TangentMovedEvent(WUInt32 uiCurveIdx, WUInt32 uiIdx, float fNewPosX, float fNewPosY, bool bRightTangent);
  void InsertCpEvent(WUInt32 uiCurveIdx, WInt64 iTickX, double value);
  void TangentLinkEvent(WUInt32 uiCurveIdx, WUInt32 uiIdx, bool bLink);
  void CpTangentModeEvent(WUInt32 uiCurveIdx, WUInt32 uiIdx, bool bRightTangent, int iMode); // WCurveTangentMode

  void BeginCpChangesEvent(QString sName);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString sName);
  void EndOperationEvent(bool bCommit);

private Q_SLOTS:
  void on_LinePosition_editingFinished();
  void on_LineValue_editingFinished();
  void onDeleteControlPoints();
  void onDoubleClick(const QPointF& scenePos, const QPointF& epsilon);
  void onMoveControlPoints(double x, double y);
  void onMoveTangents(float x, float y);
  void onBeginOperation(QString name);
  void onEndOperation(bool commit);
  void onScaleControlPoints(QPointF refPt, double scaleX, double scaleY);
  void onContextMenu(QPoint pos, QPointF scenePos);
  void onAddPoint();
  void onLinkTangents();
  void onBreakTangents();
  void onFlattenTangents();
  void onSelectionChanged();
  void onMoveCurve(WInt32 iCurve, double moveY);
  void onGenerateCurve(WCurveFunction::Enum function, bool inverse);
  void onSaveAsPreset();
  void onLoadPreset();

private:
  void InsertCpAt(double posX, double value, WVec2d epsilon);
  bool PickCurveAt(double x, double y, double fMaxDistanceY, WInt32& out_iCurveIdx, double& out_ValueY) const;
  bool PickControlPointAt(double x, double y, WVec2d vMaxDistance, WInt32& out_iCurveIdx, WInt32& out_iCpIdx) const;
  void UpdateSpinBoxes();
  void SetTangentMode(WCurveTangentMode::Enum mode, bool bLeft, bool bRight);
  void ClampPoint(double& x, double& y) const;
  void SaveCurvePreset(const char* szFile) const;
  WResult LoadCurvePreset(const char* szFile);
  void FindAllPresets();

  double m_fCurveDuration;
  WVec2 m_vTangentMove;
  WVec2d m_vControlPointMove;
  bool m_bControlPointsScaled = false;
  WCurveGroupData m_Curves;
  WCurveGroupData m_CurvesBackup;
  QPointF m_ContextMenuScenePos;

  static WDynamicArray<WString> s_CurvePresets;
};
