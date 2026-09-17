#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtCurve1DEditorWidget;

class WQtCurve1DAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtCurve1DAssetDocumentWindow(WDocument* pDocument);
  ~WQtCurve1DAssetDocumentWindow();

private Q_SLOTS:
  void onInsertCpAt(WUInt32 uiCurveIdx, WInt64 tickX, double newPosY);
  void onCurveCpMoved(WUInt32 curveIdx, WUInt32 cpIdx, WInt64 iTickX, double newPosY);
  void onCurveCpDeleted(WUInt32 curveIdx, WUInt32 cpIdx);
  void onCurveTangentMoved(WUInt32 curveIdx, WUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void onLinkCurveTangents(WUInt32 curveIdx, WUInt32 cpIdx, bool bLink);
  void onCurveTangentModeChanged(WUInt32 curveIdx, WUInt32 cpIdx, bool rightTangent, int mode);

  void onCurveBeginOperation(QString name);
  void onCurveEndOperation(bool commit);
  void onCurveBeginCpChanges(QString name);
  void onCurveEndCpChanges();

private:
  void UpdatePreview();

  void SendLiveResourcePreview();
  void RestoreResource();

  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);

  WQtCurve1DEditorWidget* m_pCurveEditor = nullptr;
};
