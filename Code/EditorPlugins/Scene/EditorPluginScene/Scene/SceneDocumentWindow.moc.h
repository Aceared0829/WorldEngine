#pragma once

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/PropertyGrid/Declarations.h>

struct WEngineViewPreferences;
class QGridLayout;
class WQtViewWidgetContainer;
class WQtSceneViewWidget;
class QSettings;
struct WManipulatorManagerEvent;
class WPreferences;
class WQtQuadViewWidget;
struct WEngineWindowEvent;
class WSceneDocument;
class QMenu;
class WQtPropertyWidget;

Q_DECLARE_OPAQUE_POINTER(WQtSceneViewWidget*);

class WQtSceneDocumentWindowBase : public WQtGameObjectDocumentWindow, public WGameObjectGizmoInterface
{
  Q_OBJECT

public:
  WQtSceneDocumentWindowBase(WSceneDocument* pDocument);
  ~WQtSceneDocumentWindowBase();

  WSceneDocument* GetSceneDocument() const;

  virtual void CreateImageCapture(const char* szOutputPath) override;

public Q_SLOTS:
  void ToggleViews(QWidget* pView);

public:
  /// \name WGameObjectGizmoInterface implementation
  ///@{
  virtual WObjectAccessorBase* GetObjectAccessor() override;
  virtual bool CanDuplicateSelection() const override;
  virtual void DuplicateSelection() override;
  ///@}

protected:
  virtual void ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg) override;
  virtual void InternalRedraw() override;

  void GameObjectEventHandler(const WGameObjectEvent& e);
  void SnapSelectionToPosition(bool bSnapEachObject);
  void SendRedrawMsg();
  void ExtendPropertyGridContextMenu(QMenu& menu, WQtPropertyWidget* pPropWidget);

protected:
  WQtQuadViewWidget* m_pQuadViewWidget = nullptr;
};

class WQtSceneDocumentWindow : public WQtSceneDocumentWindowBase
{
  Q_OBJECT

public:
  WQtSceneDocumentWindow(WSceneDocument* pDocument);
  ~WQtSceneDocumentWindow();
};
