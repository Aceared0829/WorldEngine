#pragma once

#include <Core/Graphics/Camera.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Size.h>
#include <QWidget>
#include <ads/DockWidget.h>

class WQtEngineDocumentWindow;
class WEditorInputContext;
class QHBoxLayout;
class QPushButton;
class QTimer;
class QVBoxLayout;
class WViewMarqueePickingResultMsgToEditor;

struct W_EDITORFRAMEWORK_DLL WObjectPickingResult
{
  WObjectPickingResult() { Reset(); }
  void Reset();

  WUuid m_PickedObject;
  WUuid m_PickedComponent;
  WUuid m_PickedOther;
  WUInt32 m_uiPartIndex;
  WVec3 m_vPickedPosition;
  WVec3 m_vPickedNormal;
  WVec3 m_vPickingRayStart;
};

/// Base class for views that show engine output
class W_EDITORFRAMEWORK_DLL WQtEngineViewWidget : public QWidget
{
  Q_OBJECT

public:
  WQtEngineViewWidget(QWidget* pParent, WQtEngineDocumentWindow* pDocumentWindow, WEngineViewConfig* pViewConfig);
  ~WQtEngineViewWidget();

  /// Add input contexts in the order in which they are supposed to be processed
  WHybridArray<WEditorInputContext*, 8> m_InputContexts;

  /// Returns the ID of this view
  WUInt32 GetViewID() const { return m_uiViewID; }
  WQtEngineDocumentWindow* GetDocumentWindow() const { return m_pDocumentWindow; }

  /// Sends the redraw message to the engine
  virtual void SyncToEngine();

  void GetCameraMatrices(WMat4& out_mViewMatrix, WMat4& out_mProjectionMatrix) const;

  WEngineViewConfig* m_pViewConfig;

  /// Called every frame to move the camera to its current target (focus on selection, etc.)
  void UpdateCameraInterpolation();

  /// The view's camera will be interpolated to the given coordinates
  void InterpolateCameraTo(
    const WVec3& vPosition, const WVec3& vDirection, float fFovOrDim, const WVec3* pNewUpDirection = nullptr, bool bImmediate = false);

  /// If disabled, no picking takes place in this view.
  ///
  /// Disabled in views that do not need picking (material asset, particle asset, etc.)
  /// and when the mouse is outside a view, to prevent useless picking.
  void SetEnablePicking(bool bEnable);

  void SetPickTransparent(bool bEnable);

  /// Disabled during drag&drop operations, to prevent picking against the dragged object.
  virtual bool IsPickingAgainstSelectionAllowed() const { return !m_bInDragAndDropOperation; }

  /// Holds information about the viewport that the user just now hovered over and what object was picked last
  struct InteractionContext
  {
    WQtEngineViewWidget* m_pLastHoveredViewWidget = nullptr;
    const WObjectPickingResult* m_pLastPickingResult = nullptr;
  };

  /// Returns the latest information about what viewport the user interacted with.
  static const InteractionContext& GetInteractionContext() { return s_InteractionContext; }

  /// Overrides the InteractionContext with custom values. Mostly useful for injecting procedural user interaction for unit tests.
  static void SetInteractionContext(const InteractionContext& ctxt) { s_InteractionContext = ctxt; }

  /// Supposed to open a context menu at the given position. Derived classes must implement OnOpenContextMenu and do the actual work there.
  void OpenContextMenu(QPoint globalPos);

  /// Starts a picking operation for the given pixel position in this view. Returns the most recent picking information in the meantime.
  const WObjectPickingResult& PickObject(WUInt16 uiScreenPosX, WUInt16 uiScreenPosY) const;

  /// Clears the last stored picking position. Only needed when it is vital that no stale picking data can be used next time.
  void ClearLastPickedObject();

  /// Similar to PickObject, but computes the intersection with the given plane instead.
  WResult PickPlane(WUInt16 uiScreenPosX, WUInt16 uiScreenPosY, const WPlane& plane, WVec3& out_vPosition) const;

  /// Processes incoming messages from the engine that are meant for this particular view. Mostly picking results.
  void HandleViewMessage(const WEditorEngineViewMsg* pMsg);

  /// Returns a plane that can be used for picking, when nothing else is available
  /// Orthographic views would typically return their projection planes, perspective views may return the ground plane
  virtual WPlane GetFallbackPickingPlane(WVec3 vPointOnPlane = WVec3(0)) const;

  /// If this is set to a non-zero value, all rendering will use a fixed resolution, instead of the actual window size.
  /// This is useful for unit tests, to guarantee a specific output size, to be able to do image comparisons.
  static WSizeU32 s_FixedResolution;

  void TakeScreenshot(const char* szOutputPath) const;

protected:
  /// Used to deactivate shortcuts
  virtual bool eventFilter(QObject* object, QEvent* event) override;

  virtual void paintEvent(QPaintEvent* event) override;
  virtual QPaintEngine* paintEngine() const override { return nullptr; }

  virtual void resizeEvent(QResizeEvent* event) override;

  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void keyReleaseEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void focusOutEvent(QFocusEvent* e) override;
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;

protected:
  void EngineViewProcessEventHandler(const WEditorEngineProcessConnection::Event& e);
  void ShowRestartButton(bool bShow);
  void ShowProcessStuckIndicator(bool bShow);
  void RecreateEngineViewport();
  virtual void OnOpenContextMenu(QPoint globalPos) {}
  virtual void HandleMarqueePickingResult(const WViewMarqueePickingResultMsgToEditor* pMsg) {}

private Q_SLOTS:
  void SlotRestartEngineProcess();

protected:
  bool m_bUpdatePickingData;
  bool m_bPickTransparent = true;
  bool m_bInDragAndDropOperation;
  WUInt32 m_uiViewID;
  WQtEngineDocumentWindow* m_pDocumentWindow = nullptr;

  static WUInt32 s_uiNextViewID;

  // Camera Interpolation
  float m_fCameraLerp;
  float m_fCameraStartFovOrDim;
  float m_fCameraTargetFovOrDim;
  WVec3 m_vCameraStartPosition;
  WVec3 m_vCameraTargetPosition;
  WVec3 m_vCameraStartDirection;
  WVec3 m_vCameraTargetDirection;
  WVec3 m_vCameraUp;
  WTime m_LastCameraUpdate;

  QHBoxLayout* m_pMainLayout = nullptr;
  QPushButton* m_pRestartButton = nullptr;
  QWidget* m_pViewportWidget = nullptr;
  QWidget* m_pStuckIndicator = nullptr;
  QTimer* m_pResizeTimer = nullptr;

  mutable WObjectPickingResult m_LastPickingResult;

  static InteractionContext s_InteractionContext;
};

/// Wraps and decorates a view widget with a toolbar and layout.
class W_EDITORFRAMEWORK_DLL WQtViewWidgetContainer : public ads::CDockWidget
{
  Q_OBJECT

public:
  WQtViewWidgetContainer(ads::CDockManager* pDockManager, QWidget* pParent, WQtEngineViewWidget* pViewWidget, const char* szToolBarMapping);
  ~WQtViewWidgetContainer();

  WQtEngineViewWidget* GetViewWidget() const { return m_pViewWidget; }
  QVBoxLayout* GetLayout() const { return m_pLayout; }

private:
  WQtEngineViewWidget* m_pViewWidget;
  QVBoxLayout* m_pLayout;
};
