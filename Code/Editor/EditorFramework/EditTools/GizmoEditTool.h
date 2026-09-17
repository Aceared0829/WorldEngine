#pragma once

#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/EditorFrameworkDLL.h>

struct WEngineWindowEvent;
struct WGameObjectEvent;
struct WDocumentObjectStructureEvent;
struct WManipulatorManagerEvent;
struct WSelectionManagerEvent;
struct WCommandHistoryEvent;
struct WGizmoEvent;

class W_EDITORFRAMEWORK_DLL WGameObjectGizmoEditTool : public WGameObjectEditTool
{
  W_ADD_DYNAMIC_REFLECTION(WGameObjectGizmoEditTool, WGameObjectEditTool);

public:
  WGameObjectGizmoEditTool();
  ~WGameObjectGizmoEditTool();

  void TransformationGizmoEventHandler(const WGizmoEvent& e);

protected:
  virtual void OnConfigured() override;

  void UpdateGizmoSelectionList();

  void UpdateGizmoVisibleState();
  virtual void ApplyGizmoVisibleState(bool visible) = 0;

  void UpdateGizmoTransformation();
  virtual void ApplyGizmoTransformation(const WTransform& transform) = 0;

  virtual void TransformationGizmoEventHandlerImpl(const WGizmoEvent& e) = 0;

  WDeque<WSelectedGameObject> m_GizmoSelection;
  bool m_bInGizmoInteraction = false;
  bool m_bMergeTransactions = false;

private:
  void DocumentWindowEventHandler(const WQtDocumentWindowEvent& e);
  void UpdateManipulatorVisibility();
  void GameObjectEventHandler(const WGameObjectEvent& e);
  void CommandHistoryEventHandler(const WCommandHistoryEvent& e);
  void SelectionManagerEventHandler(const WSelectionManagerEvent& e);
  void ManipulatorManagerEventHandler(const WManipulatorManagerEvent& e);
  void EngineWindowEventHandler(const WEngineWindowEvent& e);
  void ObjectStructureEventHandler(const WDocumentObjectStructureEvent& e);
};
