#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Pipeline/Declarations.h>

class WEngineProcessDocumentContext;
class WEditorEngineDocumentMsg;
class WViewRedrawMsgToEngine;
class WEditorEngineViewMsg;
struct WGALRenderTargets;

using WRenderPipelineResourceHandle = WTypedResourceHandle<class WRenderPipelineResource>;

/// Represents the window inside the editor process, into which the engine process renders
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEditorProcessViewWindow : public WWindowBase
{
public:
  WEditorProcessViewWindow()
  {
    m_hWnd = INVALID_WINDOW_HANDLE_VALUE;
    m_uiWidth = 0;
    m_uiHeight = 0;
  }

  ~WEditorProcessViewWindow();

  WResult UpdateWindow(WWindowHandle hParentWindow, WUInt16 uiWidth, WUInt16 uiHeight);

  // Inherited via WWindowBase
  virtual WSizeU32 GetClientAreaSize() const override { return WSizeU32(m_uiWidth, m_uiHeight); }
  virtual WWindowHandle GetNativeWindowHandle() const override { return m_hWnd; }
  virtual void ProcessWindowMessages() override {}
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override { return false; }
  virtual bool IsVisible() const override { return true; }
  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }


  WUInt16 m_uiWidth;
  WUInt16 m_uiHeight;

private:
  WWindowHandle m_hWnd;
  WAtomicInteger32 m_iReferenceCount = 0;
};

/// Represents the view/window on the engine process side, holds all data necessary for rendering
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEngineProcessViewContext
{
public:
  WEngineProcessViewContext(WEngineProcessDocumentContext* pContext);
  virtual ~WEngineProcessViewContext();

  void SetViewID(WUInt32 uiId);

  WEngineProcessDocumentContext* GetDocumentContext() const { return m_pDocumentContext; }

  virtual void HandleViewMessage(const WEditorEngineViewMsg* pMsg);
  virtual void SetupRenderTarget(WGALSwapChainHandle hSwapChain, const WGALRenderTargets* pRenderTargets, WUInt16 uiWidth, WUInt16 uiHeight);
  virtual void Redraw(bool bRenderEditorGizmos);
  virtual bool PendingOperationInProgress() const;

  /// Focuses camera on the given object
  static bool FocusCameraOnObject(WCamera& inout_camera, const WBoundingBoxSphere& objectBounds, float fFov, const WVec3& vViewDir);

  WViewHandle GetViewHandle() const { return m_hView; }

  void DrawSimpleGrid() const;

protected:
  void SendViewMessage(WEditorEngineViewMsg* pViewMsg);
  void HandleWindowUpdate(WWindowHandle hWnd, WUInt16 uiWidth, WUInt16 uiHeight);
  void OnSwapChainChanged(WGALSwapChainHandle hSwapChain, WSizeU32 size);

  virtual void SetCamera(const WViewRedrawMsgToEngine* pMsg);
  virtual void SetViewProperties(WView* pView);

  /// Returns the handle to the default render pipeline.
  virtual WRenderPipelineResourceHandle CreateDefaultRenderPipeline();

  /// Returns the handle to the debug render pipeline.
  virtual WRenderPipelineResourceHandle CreateDebugRenderPipeline();

  /// Create the actual view.
  virtual WViewHandle CreateView() = 0;

private:
  WEngineProcessDocumentContext* m_pDocumentContext;
  WRegisteredWndHandle m_hEditorWindow;
  WString m_sPendingScreenshotPath;

protected:
  WView* CreateDefaultView(WStringView sName);

  WCamera m_Camera;
  WViewHandle m_hView;
  WUInt32 m_uiViewID;
};
