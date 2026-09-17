#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QLineEdit>
#include <QModelIndex>

class WSelectionContext;
struct WSelectionManagerEvent;

class W_EDITORFRAMEWORK_DLL WQtGameObjectReferencePropertyWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtGameObjectReferencePropertyWidget();

private Q_SLOTS:
  void on_PickObject_clicked();

protected slots:
  void on_customContextMenuRequested(const QPoint& pt);
  void OnSelectReferencedObject();
  void OnCopyReference();
  void OnClearReference();
  void OnPasteReference();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;
  void FillContextMenu(QMenu& menu);
  void PickObjectOverride(const WDocumentObject* pObject);
  void SetValue(const QString& sText);
  void ClearPicking();
  void SelectionManagerEventHandler(const WSelectionManagerEvent& e);
  virtual void showEvent(QShowEvent* event) override;

protected:
  QPalette m_Pal;
  QHBoxLayout* m_pLayout = nullptr;
  QLabel* m_pWidget = nullptr;
  QString m_sInternalValue;
  QToolButton* m_pButton = nullptr;
  WHybridArray<WSelectionContext*, 8> m_SelectionContextsToUnsubscribe;
};
