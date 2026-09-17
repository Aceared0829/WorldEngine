#pragma once

#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

class QWidget;
class QHBoxLayout;
class QPushButton;
class WQtEngineViewWidget;
class WAssetDocument;
class WEditorEngineDocumentMsg;
class WQtCuratorControl;
struct WObjectPickingResult;
struct WEngineViewConfig;
struct WCommonAssetUiState;


struct W_EDITORFRAMEWORK_DLL WEngineWindowEvent
{
  enum class Type
  {
    ViewCreated,
    ViewDestroyed,
  };

  Type m_Type;
  WQtEngineViewWidget* m_pView = nullptr;
};

/// Base class for all document windows that need a connection to the engine process, and might want to render 3D content.
///
/// This class has an WEditorEngineConnection object for sending messages between the editor and the engine process.
/// It also allows to embed WQtEngineViewWidget objects into the UI, which enable 3D rendering by the engine process.
class W_EDITORFRAMEWORK_DLL WQtEngineDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtEngineDocumentWindow(WAssetDocument* pDocument);
  virtual ~WQtEngineDocumentWindow();

  WEditorEngineConnection* GetEditorEngineConnection() const;
  const WObjectPickingResult& PickObject(WUInt16 uiScreenPosX, WUInt16 uiScreenPosY, WQtEngineViewWidget* pView) const;

  WAssetDocument* GetDocument() const;

  /// Returns the WQtEngineViewWidget over which the mouse currently hovers
  WQtEngineViewWidget* GetHoveredViewWidget() const;

  /// Returns the WQtEngineViewWidget that has the input focus
  WQtEngineViewWidget* GetFocusedViewWidget() const;

  WQtEngineViewWidget* GetViewWidgetByID(WUInt32 uiViewID) const;

  WArrayPtr<WQtEngineViewWidget* const> GetViewWidgets() const;

  void AddViewWidget(WQtEngineViewWidget* pView);

  virtual void CreateImageCapture(const char* szOutputPath) override;

  /// Returns the active camera mode index, or -1 if the window does not support camera mode switching.
  virtual int GetCameraMode() const { return -1; }

  /// Sets the active camera mode by index. Override together with GetCameraMode() and GetCameraModeNames().
  virtual void SetCameraMode(int iMode) {}

  /// Returns the display names for each supported camera mode, in order matching the indices used by GetCameraMode() and SetCameraMode().
  virtual void GetCameraModeNames(WDynamicArray<WString>& out_names) const
  {
    out_names.PushBack("Orbit Camera");
    out_names.PushBack("Free Camera");
  }

public:
  mutable WEvent<const WEngineWindowEvent&> m_EngineWindowEvent;

protected:
  friend class WQtEngineViewWidget;
  WHybridArray<WQtEngineViewWidget*, 4> m_ViewWidgets;
  WQtCuratorControl* m_pCuratorControl = nullptr;

  virtual void CommonAssetUiEventHandler(const WCommonAssetUiState& e);

  virtual void ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg);
  void RemoveViewWidget(WQtEngineViewWidget* pView);
  void DestroyAllViews();
  virtual void InternalRedraw() override;
};
