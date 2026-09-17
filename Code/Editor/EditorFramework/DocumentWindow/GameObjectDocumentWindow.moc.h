#pragma once
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>

class WGameObjectDocument;
class WWorldSettingsMsgToEngine;
class WQtGameObjectViewWidget;
struct WGameObjectEvent;
struct WSnapProviderEvent;

class W_EDITORFRAMEWORK_DLL WQtGameObjectDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT
public:
  WQtGameObjectDocumentWindow(WGameObjectDocument* pDocument);
  ~WQtGameObjectDocumentWindow();

  WGameObjectDocument* GetGameObjectDocument() const;

protected:
  WWorldSettingsMsgToEngine GetWorldSettings() const;
  WGridSettingsMsgToEngine GetGridSettings() const;
  virtual void ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg) override;

private:
  void GameObjectEventHandler(const WGameObjectEvent& e);
  void SnapProviderEventHandler(const WSnapProviderEvent& e);

  void FocusOnSelectionAllViews();
  void FocusOnSelectionHoveredView();

  void HandleFocusOnSelection(const WQuerySelectionBBoxResultMsgToEditor* pMsg, WQtGameObjectViewWidget* pSceneView);
};
