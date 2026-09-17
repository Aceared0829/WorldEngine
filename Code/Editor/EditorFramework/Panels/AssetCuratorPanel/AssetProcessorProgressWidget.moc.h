#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/Declarations.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <QWidget>

class WQGridBarWidget;
struct WAssetProcessorProgressEvent;
struct WAssetProcessorEvent;
class QPushButton;
class QScrollBar;

/// Visual progress widget that displays an asset processing timeline.
///
/// Shows a timeline view with Y-axis representing each WEditorProcessor and X-axis representing time.
/// Each asset being processed is displayed as a bar on the timeline.
/// The timeline is compacted in that every length of time in which no asset processing happens is reduced to one second in the timeline and a diagonal pattern is drawn to indicate that the timeline cuts off there.
class W_EDITORFRAMEWORK_DLL WQtAssetProcessorProgressWidget : public QWidget
{
  Q_OBJECT

public:
  explicit WQtAssetProcessorProgressWidget(QWidget* pParent = nullptr);
  ~WQtAssetProcessorProgressWidget();

  /// An WQGridBarWidget must be placed over this widget in a vertical layout and then set here.
  void SetGridBarWidget(WQGridBarWidget* pGridBar);
  void SetScrollBarWidget(QScrollBar* pScrollBar);

public Q_SLOTS:
  void ClearHistory();

protected:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void changeEvent(QEvent* e) override;
  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

private:
  enum class EditState
  {
    None,
    Panning
  };

  /// Compact representation of an item in the timeline of the asset processor.
  struct ProcessorTask
  {
    static constexpr WUInt16 SuccessResult = 0xFFFF;
    W_ALWAYS_INLINE bool IsFinished() const { return m_fDurationInSeconds != -1; }
    W_ALWAYS_INLINE bool Failed() const { return m_uiResultIndex != SuccessResult; }
    W_ALWAYS_INLINE float EndTime() const { return IsFinished() ? m_fStartTimeInSeconds + m_fDurationInSeconds : 0.0f; }

    WUuid m_AssetGuid;
    float m_fStartTimeInSeconds = 0.0f;
    float m_fTransformStartTimeInSeconds = 0.0f;
    float m_fDurationInSeconds = -1;
    WUInt16 m_uiResultIndex = -1; //< We only store failed results to safe space. Index points to m_FailedTransforms
    WAssetInfo::TransformState m_TransformState = WAssetInfo::Unknown;
  };

  class HistoryState : public WRefCounted
  {
  public:
    void SetMaxProcessors(WUInt32 uiCount);
    void ClearHistory();
    WTime GetLatestTaskTime() const;
    void OnProgressEvent(const WAssetProcessorProgressEvent& e);
    void OnProcessorEvent(const WAssetProcessorEvent& e);
    const ProcessorTask* FindTaskAtTime(WUInt32 uiProcessorID, double fPointInTimeSec) const;

  public:
    mutable WMutex m_HistoryMutex;
    WQtAssetProcessorProgressWidget* m_pParent = nullptr;
    // Rendering the current time is a bit cumbersome so we instead subtract an offset to make the graph start at zero seconds. If this value is false, the next incoming asset transform will set m_CurrentOffset.
    bool m_bCurrentOffsetValid = false;
    // To compact various runs of asset processing in the timeline, this value is subtracted from the actual start / end time of each asset transform.
    WTime m_CurrentOffset;
    // At which points in time the timeline got compacted. Just used for rendering a pattern to indicate that the timeline was cut off at these points.
    WDynamicArray<WTime> m_SkipOffsets;
    WDynamicArray<WDynamicArray<ProcessorTask>> m_ProcessorHistory; // [processorId][taskIndex]
    WDeque<WTransformStatus> m_FailedTransforms;
    WDynamicArray<WEditorProcessorState> m_ProcessStates;
  };

  static constexpr int s_iRowHeight = 30;
  static constexpr int s_iRowSpacing = 5;
  static constexpr int s_iLeftMargin = 100;
  static constexpr int s_iIndicatorSize = 20;
  static constexpr int s_iTopMargin = 0;

private Q_SLOTS:
  void OnUpdateTimer();
  void OnHistoryChanged();
  void OnProcessorStateChanged();
  void OnProcessStateChanged(WUInt8 uiProcessID);

private:
  QPoint MapFromScene(const QPointF& pos) const;
  QPointF MapToScene(const QPoint& pos) const;
  QRectF ComputeViewportSceneRect() const;
  void ClampZoomPan();
  void UpdateGridBarConfig() const;

  const WString& GetAssetPath(const WUuid& assetGuid) const;
  const ProcessorTask* FindTaskAtPosition(const QPoint& pos, WUInt32& out_uiProcessorID) const;
  void ShowTooltip(QMouseEvent* e);

  void DrawTimeline(QPainter& painter) const;
  void DrawProcessorRow(QPainter& painter, WUInt32 uiProcessorID, int y) const;
  void DrawProcessorTask(QPainter& painter, const ProcessorTask& task, const QRect& rect, const QRect& actualWork) const;

private:
  WEventSubscriptionID m_ProgressEventsID;
  WEventSubscriptionID m_ProcessorEventsID;
  WSharedPtr<HistoryState> m_pHistoryState;
  WUInt32 m_uiMaxProcessors = 0;

  QTimer* m_pUpdateTimer = nullptr;
  WQGridBarWidget* m_pGridBar = nullptr;
  QScrollBar* m_pScrollBar = nullptr;

  // Display and interaction settings
  EditState m_EditState = EditState::None;

  WTime m_TimelineLength = WTime::MakeFromMinutes(1); // Multiple of 1min, resize when current time exceeds this.
  double m_fSceneTranslationX = 0;                      // Scene horizontal pan offset (in seconds)
  QPointF m_SceneToPixelScale = QPointF(20, 1);
  QPoint m_StartMousePos = {0, 0};
  QPoint m_LastMousePos = {0, 0};

  // Cache for asset names so we don't have to store the name in ProcessorTask and also don't SPAM the WAssetCurator.
  mutable WMap<WUuid, WString> m_AssetNameCache;
  WString m_sUnknownAsset = "<DELETED>";

  // Paint performance tracking
  mutable double m_fLastPaintTimeMs = 0.0;

  // Paint performance caches
  mutable QPixmap m_SkipOffsetPattern;
  mutable QFont m_ProcessorLabelFont;
  mutable QFont m_TaskLabelFont;
  mutable QColor m_NeedsTransformColor[2];
  mutable QColor m_NeedsThumbnailColor[2];
  mutable QColor m_ErrorColor[2];
  mutable bool m_bCachesInitialized = false;
  mutable WDynamicArray<QString> m_ProcessorLabels; // Cached "Process N" strings

  void InitializePaintCaches() const;
  void InvalidatePaintCaches();
};
