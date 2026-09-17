#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Reflection/Reflection.h>

class QWidget;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class WDocument;
class WQtEngineDocumentWindow;
class WQtEngineViewWidget;

enum class WEditorInput
{
  MayBeHandledByOthers,
  WasExclusivelyHandled,
};

class W_EDITORFRAMEWORK_DLL WEditorInputContext : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WEditorInputContext, WReflectedClass);

public:
  WEditorInputContext();

  virtual ~WEditorInputContext();

  void FocusLost(bool bCancel);

  WEditorInput KeyPressEvent(QKeyEvent* e) { return DoKeyPressEvent(e); }
  WEditorInput KeyReleaseEvent(QKeyEvent* e) { return DoKeyReleaseEvent(e); }
  WEditorInput MousePressEvent(QMouseEvent* e) { return DoMousePressEvent(e); }
  WEditorInput MouseReleaseEvent(QMouseEvent* e) { return DoMouseReleaseEvent(e); }
  WEditorInput MouseMoveEvent(QMouseEvent* e);
  WEditorInput WheelEvent(QWheelEvent* e) { return DoWheelEvent(e); }

  static void SetActiveInputContext(WEditorInputContext* pContext);

  void MakeActiveInputContext(bool bActive = true);

  static bool IsAnyInputContextActive() { return s_pActiveInputContext != nullptr; }

  static WEditorInputContext* GetActiveInputContext() { return s_pActiveInputContext; }

  static void UpdateActiveInputContext();

  bool IsActiveInputContext() const;

  void SetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView);

  WQtEngineDocumentWindow* GetOwnerWindow() const;

  WQtEngineViewWidget* GetOwnerView() const;

  bool GetShortcutsDisabled() const { return m_bDisableShortcuts; }

  /// If set to true, the surrounding window will ensure to block all shortcuts and instead send keypress events to the input context
  void SetShortcutsDisabled(bool bDisabled) { m_bDisableShortcuts = bDisabled; }

  virtual bool IsPickingSelectedAllowed() const { return true; }

  /// How the mouse position is updated when the mouse cursor reaches the screen borders.
  enum class MouseMode
  {
    Normal,                     ///< Nothing happens, the mouse will stop at screen borders as usual
    WrapAtScreenBorders,        ///< The mouse is visibly wrapped at screen borders. When this mode is disabled, the mouse stays where it is.
    HideAndWrapAtScreenBorders, ///< The mouse is wrapped at screen borders, which enables infinite movement, but the cursor is invisible. When this
                                ///< mode is disabled the mouse is restored to the position where it was when it was enabled.
  };

  /// Sets how the mouse will act when it reaches the screen border. UpdateMouseMode() must be called on every mouseMoveEvent to update the
  /// state.
  ///
  /// The return value is the current global mouse position. Can be used to initialize a 'Last Mouse Position' variable.
  WVec2I32 SetMouseMode(MouseMode mode);

  /// Updates the mouse position. Can always be called but will only have an effect if SetMouseMode() was called with one of the wrap modes.
  ///
  /// Returns the new global mouse position, which may change drastically if the mouse cursor needed to be wrapped around the screen.
  /// Should be used to update a "Last Mouse Position" variable.
  WVec2I32 UpdateMouseMode(QMouseEvent* e);

  virtual void UpdateStatusBarText(WQtEngineDocumentWindow* pWindow) {}

protected:
  virtual void DoFocusLost(bool bCancel) {}

  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) = 0;

  virtual void OnActivated() {}
  virtual void OnDeactivated() {}
  virtual WEditorInput DoKeyPressEvent(QKeyEvent* e);
  virtual WEditorInput DoKeyReleaseEvent(QKeyEvent* e) { return WEditorInput::MayBeHandledByOthers; }
  virtual WEditorInput DoMousePressEvent(QMouseEvent* e) { return WEditorInput::MayBeHandledByOthers; }
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) { return WEditorInput::MayBeHandledByOthers; }
  virtual WEditorInput DoMouseMoveEvent(QMouseEvent* e) { return WEditorInput::MayBeHandledByOthers; }
  virtual WEditorInput DoWheelEvent(QWheelEvent* e) { return WEditorInput::MayBeHandledByOthers; }

private:
  static WEditorInputContext* s_pActiveInputContext;

  WQtEngineDocumentWindow* m_pOwnerWindow;
  WQtEngineViewWidget* m_pOwnerView;
  bool m_bDisableShortcuts;
  bool m_bJustWrappedMouse;
  MouseMode m_MouseMode;
  WVec2I32 m_vMouseRestorePosition;
  WVec2I32 m_vMousePosBeforeWrap;
  WVec2I32 m_vExpectedMousePosition;
  WRectU32 m_MouseWrapRect;

  virtual void UpdateContext() {}
};
