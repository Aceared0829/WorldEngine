#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Declarations.h>


using WRenderPipelineResourceHandle = WTypedResourceHandle<class WRenderPipelineResource>;

enum class WEditorEngineProcessMode
{
  Primary,
  Remote,
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEditorEngineProcessApp
{
  W_DECLARE_SINGLETON(WEditorEngineProcessApp);

public:
  WEditorEngineProcessApp();
  ~WEditorEngineProcessApp();

  void SetRemoteMode();

  bool IsRemoteMode() const { return m_Mode == WEditorEngineProcessMode::Remote; }

  virtual WViewHandle CreateRemoteWindowAndView(WCamera* pCamera);
  void DestroyRemoteWindow();

  virtual WRenderPipelineResourceHandle CreateDefaultMainRenderPipeline();
  virtual WRenderPipelineResourceHandle CreateDefaultDebugRenderPipeline();

protected:
  virtual void CreateRemoteWindow();

  WEditorEngineProcessMode m_Mode = WEditorEngineProcessMode::Primary;

  WRegisteredWndHandle m_hWindow;
  WViewHandle m_hRemoteView;
};
