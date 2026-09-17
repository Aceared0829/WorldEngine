#pragma once

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <Foundation/Basics.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <QTreeView>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtPropertyAnimModel;
class QItemSelection;
class QItemSelectionModel;
class WQtCurve1DEditorWidget;
class WQtEventTrackEditorWidget;
struct WDocumentObjectPropertyEvent;
struct WDocumentObjectStructureEvent;
class WPropertyAnimAssetDocument;
class WQtColorGradientEditorWidget;
class WColorGradientAssetData;
class WQtQuadViewWidget;
class WQtTimeScrubberToolbar;
struct WPropertyAnimAssetDocumentEvent;
class QKeyEvent;
class WQtDocumentPanel;

class WQtPropertyAnimAssetTreeView : public QTreeView
{
  Q_OBJECT

public:
  WQtPropertyAnimAssetTreeView(QWidget* pParent);
  void initialize();

Q_SIGNALS:
  void DeleteSelectedItemsEvent();
  void FrameSelectedItemsEvent();
  void RebindSelectedItemsEvent();

protected slots:
  void onBeforeModelReset();
  void onAfterModelReset();

protected:
  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void contextMenuEvent(QContextMenuEvent* event) override;
  void storeExpandState(const QModelIndex& parent);
  void restoreExpandState(const QModelIndex& parent, QModelIndexList& newSelection);

  QSet<QString> m_NotExpandedState;
  QSet<QString> m_SelectedItems;
};

class WQtPropertyAnimAssetDocumentWindow : public WQtGameObjectDocumentWindow, public WGameObjectGizmoInterface
{
  Q_OBJECT

public:
  WQtPropertyAnimAssetDocumentWindow(WPropertyAnimAssetDocument* pDocument);
  ~WQtPropertyAnimAssetDocumentWindow();

public Q_SLOTS:
  void ToggleViews(QWidget* pView);

public:
  /// \name WGameObjectGizmoInterface implementation
  ///@{
  virtual WObjectAccessorBase* GetObjectAccessor() override;
  virtual bool CanDuplicateSelection() const override;
  virtual void DuplicateSelection() override;
  ///@}

protected:
  virtual void InternalRedraw() override;
  void PropertyAnimAssetEventHandler(const WPropertyAnimAssetDocumentEvent& e);

private Q_SLOTS:
  void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void onScrubberPosChanged(WUInt64 uiTick);
  void onDeleteSelectedItems();
  void onRebindSelectedItems();
  void onPlaybackTick();
  void onPlayPauseClicked();
  void onRepeatClicked();
  void onAdjustDurationClicked();
  void onDurationChangedEvent(double duration);
  void onTreeItemDoubleClicked(const QModelIndex& index);
  void onFrameSelectedTracks();

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
  // Color gradient editor events

  void onGradientColorCpAdded(double posX, const WColorGammaUB& color);
  void onGradientAlphaCpAdded(double posX, WUInt8 alpha);
  void onGradientIntensityCpAdded(double posX, float intensity);
  void MoveGradientCP(WInt32 idx, double newPosX, const char* szArrayName);
  void onGradientColorCpMoved(WInt32 idx, double newPosX);
  void onGradientAlphaCpMoved(WInt32 idx, double newPosX);
  void onGradientIntensityCpMoved(WInt32 idx, double newPosX);
  void RemoveGradientCP(WInt32 idx, const char* szArrayName);
  void onGradientColorCpDeleted(WInt32 idx);
  void onGradientAlphaCpDeleted(WInt32 idx);
  void onGradientIntensityCpDeleted(WInt32 idx);
  void onGradientColorCpChanged(WInt32 idx, const WColorGammaUB& color);
  void onGradientAlphaCpChanged(WInt32 idx, WUInt8 alpha);
  void onGradientIntensityCpChanged(WInt32 idx, float intensity);
  void onGradientBeginOperation();
  void onGradientEndOperation(bool commit);
  // void onGradientNormalizeRange();

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

private:
  WPropertyAnimAssetDocument* GetPropertyAnimDocument();
  // void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);
  void SelectionEventHandler(const WSelectionManagerEvent& e);
  void CommandHistoryEventHandler(const WCommandHistoryEvent& e);
  void UpdateCurveEditor();
  void UpdateGradientEditor();
  void UpdateEventTrackEditor();
  void UpdateSelectionData();

  WQtQuadViewWidget* m_pQuadViewWidget;
  WCurveGroupData m_CurvesToDisplay;
  WColorGradientAssetData* m_pGradientToDisplay = nullptr;
  WInt32 m_iMapGradientToTrack = -1;
  WDynamicArray<WInt32> m_MapSelectionToTrack;
  WQtPropertyAnimAssetTreeView* m_pPropertyTreeView = nullptr;
  WQtPropertyAnimModel* m_pPropertiesModel;
  QItemSelectionModel* m_pSelectionModel = nullptr;
  WQtCurve1DEditorWidget* m_pCurveEditor = nullptr;
  WQtEventTrackEditorWidget* m_pEventTrackEditor = nullptr;
  WQtColorGradientEditorWidget* m_pGradientEditor = nullptr;
  WQtTimeScrubberToolbar* m_pScrubberToolbar = nullptr;
  WQtDocumentPanel* m_pCurvePanel = nullptr;
  WQtDocumentPanel* m_pColorGradientPanel = nullptr;
  WQtDocumentPanel* m_pEventTrackPanel = nullptr;
  bool m_bAnimTimerInFlight = false;
};
