#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/TypeWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QWidget>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

class QSpacerItem;
class QVBoxLayout;
class QScrollArea;

class WQtGroupBoxBase;
class WDocument;
class WDocumentObjectManager;
class WCommandHistory;
class WObjectAccessorBase;
struct WDocumentObjectPropertyEvent;
struct WPropertyMetaStateEvent;
struct WObjectAccessorChangeEvent;
struct WPropertyDefaultEvent;
struct WContainerElementMetaStateEvent;

class W_GUIFOUNDATION_DLL WQtPropertyGridWidget : public QWidget
{
  Q_OBJECT
public:
  WQtPropertyGridWidget(QWidget* pParent, WDocument* pDocument = nullptr, bool bBindToSelectionManager = true);
  ~WQtPropertyGridWidget();

  void SetDocument(WDocument* pDocument, bool bBindToSelectionManager = true);

  void ClearSelection();
  void SetSelectionIncludeExcludeProperties(const char* szIncludeProperties = nullptr, const char* szExcludeProperties = nullptr);
  void SetSelection(const WDeque<const WDocumentObject*>& selection);
  const WDocument* GetDocument() const;
  const WDocumentObjectManager* GetObjectManager() const;
  WCommandHistory* GetCommandHistory() const;
  WObjectAccessorBase* GetObjectAccessor() const;

  static WRttiMappedObjectFactory<WQtPropertyWidget>& GetFactory();
  static WQtPropertyWidget* CreateMemberPropertyWidget(const WAbstractProperty* pProp);
  static WQtPropertyWidget* CreatePropertyWidget(const WAbstractProperty* pProp);

  void SetCollapseState(WQtGroupBoxBase* pBox);

Q_SIGNALS:
  void ExtendContextMenu(QMenu& ref_menu, WQtPropertyWidget* pPropWidget);

public Q_SLOTS:
  void OnCollapseStateChanged(bool bCollapsed);

private:
  static WRttiMappedObjectFactory<WQtPropertyWidget> s_Factory;
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, PropertyGrid);

private:
  void ObjectAccessorChangeEventHandler(const WObjectAccessorChangeEvent& e);
  void SelectionEventHandler(const WSelectionManagerEvent& e);
  void FactoryEventHandler(const WRttiMappedObjectFactory<WQtPropertyWidget>::Event& e);
  void TypeEventHandler(const WPhantomRttiManagerEvent& e);
  WUInt32 GetGroupBoxHash(WQtGroupBoxBase* pBox) const;

private:
  WDocument* m_pDocument;
  bool m_bBindToSelectionManager = false;
  WDeque<const WDocumentObject*> m_Selection;
  WMap<WUInt32, bool> m_CollapseState;
  WString m_sSelectionIncludeProperties;
  WString m_sSelectionExcludeProperties;

  QVBoxLayout* m_pLayout;
  QScrollArea* m_pScroll;
  QWidget* m_pContent;
  QVBoxLayout* m_pContentLayout;

  WQtTypeWidget* m_pTypeWidget;
  QSpacerItem* m_pSpacer;
};
