#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QLineEdit>
#include <QModelIndex>

class WQtAssetPropertyWidget;

/// A QLineEdit that is used by WQtAssetPropertyWidget
class W_EDITORFRAMEWORK_DLL WQtAssetLineEdit : public QLineEdit
{
  Q_OBJECT

Q_SIGNALS:
  void OpenAsset();
  void SelectAsset();

public:
  explicit WQtAssetLineEdit(QWidget* pParent = nullptr);
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;

  WQtAssetPropertyWidget* m_pOwner = nullptr;
};
