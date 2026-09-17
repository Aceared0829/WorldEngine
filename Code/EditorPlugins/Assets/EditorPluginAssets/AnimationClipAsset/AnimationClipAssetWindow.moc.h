#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Time/Clock.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtOrbitCamViewWidget;
class WQtTimeScrubberWidget;
class WQtEventTrackEditorWidget;
class WQtDocumentPanel;
struct WCommandHistoryEvent;

class WQtAnimationClipAssetDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtAnimationClipAssetDocumentWindow(WAnimationClipAssetDocument* pDocument);
  ~WQtAnimationClipAssetDocumentWindow();

  WAnimationClipAssetDocument* GetAnimationClipDocument();

  void ExtractRootMotionFromFeet();

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg) override;

  virtual void CommonAssetUiEventHandler(const WCommonAssetUiState& e) override;
  void CommandHistoryEventHandler(const WCommandHistoryEvent& e);
  virtual void OnAfterDocumentLayoutRestored() override;

protected Q_SLOTS:
  void OnScrubberPosChangedEvent(WUInt64 uiNewScrubberTickPos);

  //////////////////////////////////////////////////////////////////////////
  // Event track editor events
  void onEventTrackInsertCpAt(WInt64 tickX, QString value);
  void onEventTrackCpMoved(WUInt32 cpIdx, WInt64 iTickX);
  void onEventTrackCpDeleted(WUInt32 cpIdx);
  void onEventTrackBeginOperation(QString name);
  void onEventTrackEndOperation(bool commit);
  void onEventTrackBeginCpChanges(QString name);
  void onEventTrackEndCpChanges();

  //////////////////////////////////////////////////////////////////////////
  // Curve editor events
  void onCurveInsertCpAt(WUInt32 uiCurveIdx, WInt64 tickX, double newPosY);
  void onCurveCpMoved(WUInt32 curveIdx, WUInt32 cpIdx, WInt64 iTickX, double newPosY);
  void onCurveCpDeleted(WUInt32 curveIdx, WUInt32 cpIdx);
  void onCurveTangentMoved(WUInt32 curveIdx, WUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void onLinkCurveTangents(WUInt32 curveIdx, WUInt32 cpIdx, bool bLink);
  void onCurveTangentModeChanged(WUInt32 curveIdx, WUInt32 cpIdx, bool rightTangent, int mode);

  void onCurveBeginOperation(QString name);
  void onCurveEndOperation(bool commit);
  void onCurveBeginCpChanges(QString name);
  void onCurveEndCpChanges();

  //////////////////////////////////////////////////////////////////////////
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);

private:
  void SendRedrawMsg();
  void QueryObjectBBox(WInt32 iPurpose = 0);
  void UpdateEventTrackEditor();
  void UpdateCurveEditor();

  WClock m_Clock;
  WEngineViewConfig m_ViewConfig;
  WQtOrbitCamViewWidget* m_pViewWidget = nullptr;
  WQtTimeScrubberWidget* m_pTimeScrubber = nullptr;
  WTime m_ClipDuration;
  WTime m_PlaybackPosition;

  WQtDocumentPanel* m_pEventTrackPanel = nullptr;
  WQtEventTrackEditorWidget* m_pEventTrackEditor = nullptr;

  WQtDocumentPanel* m_pCurveEditPanel = nullptr;
  WQtCurve1DEditorWidget* m_pCurveEditor = nullptr;
  WCurveGroupData m_Curves;
};
