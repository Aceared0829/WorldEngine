#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

class QWidget;
class WCamera;
struct WObjectPickingResult;
class WDocumentObject;

class W_EDITORFRAMEWORK_DLL WSelectionContext : public WEditorInputContext
{
public:
  WSelectionContext(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView, const WCamera* pCamera);
  ~WSelectionContext();

  void SetWindowConfig(const WVec2I32& vViewport) { m_vViewport = vViewport; }

  /// Adds a delegate that gets called whenever an object is picked, as long as the override is active.
  ///
  /// It also changes the owner view's cursor to a cross-hair.
  /// If something gets picked, the override is called with a non-null object.
  /// In case the user presses ESC or the view gets destroyed while the override is active,
  /// the delegate is called with nullptr.
  /// This indicates that all picking should be stopped and the registered user should clean up.
  void SetPickObjectOverride(WDelegate<void(const WDocumentObject*)> pickOverride);
  void ResetPickObjectOverride();

protected:
  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;

  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual WEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual WEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override {}

  const WDocumentObject* determineObjectToSelect(const WDocumentObject* pickedObject, bool bToggle, bool bDirect) const;

  virtual void DoFocusLost(bool bCancel) override;

  virtual void OpenDocumentForPickedObject(const WObjectPickingResult& res) const;
  virtual void SelectPickedObject(const WObjectPickingResult& res, bool bToggle, bool bDirect) const;

protected:
  void SendMarqueeMsg(QMouseEvent* e, WUInt8 uiWhatToDo);

  WDelegate<void(const WDocumentObject*)> m_PickObjectOverride;
  const WCamera* m_pCamera;
  WVec2I32 m_vViewport;
  WEngineGizmoHandle m_hMarqueeGizmo;
  WVec3 m_vMarqueeStartPos;
  WUInt32 m_uiMarqueeID;
  bool m_bPressedSpace = false;

  enum class Mode
  {
    None,
    Single,
    MarqueeAdd,
    MarqueeRemove
  };

  Mode m_Mode = Mode::None;
};
