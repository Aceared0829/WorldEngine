#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginRmlUi/EnginePluginRmlUiDLL.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>

class WObjectSelectionMsgToEngine;
class WRenderContext;

class W_ENGINEPLUGINRMLUI_DLL WRmlUiDocumentContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WRmlUiDocumentContext, WEngineProcessDocumentContext);

public:
  WRmlUiDocumentContext();
  ~WRmlUiDocumentContext();

  const WRmlUiResourceHandle& GetResource() const { return m_hMainResource; }

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext) override;

private:
  WGameObject* m_pMainObject = nullptr;
  WRmlUiResourceHandle m_hMainResource;
};
