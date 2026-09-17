#pragma once

#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Basics.h>

class WViewMarqueePickingResultMsgToEditor;
class WQtGameObjectDocumentWindow;
class WOrthoGizmoContext;
class WContextMenuContext;
class WSelectionContext;
class WCameraMoveContext;

class W_EDITORFRAMEWORK_DLL WQtGameObjectViewWidget : public WQtEngineViewWidget
{
  Q_OBJECT
public:
  WQtGameObjectViewWidget(QWidget* pParent, WQtGameObjectDocumentWindow* pOwnerWindow, WEngineViewConfig* pViewConfig);
  ~WQtGameObjectViewWidget();

  WOrthoGizmoContext* m_pOrthoGizmoContext;
  WSelectionContext* m_pSelectionContext;
  WCameraMoveContext* m_pCameraMoveContext;

  virtual void SyncToEngine() override;

protected:
  virtual void HandleMarqueePickingResult(const WViewMarqueePickingResultMsgToEditor* pMsg) override;

  WUInt32 m_uiLastMarqueeActionID = 0;
  WDeque<WUuid> m_MarqueeBaseSelection;
};
