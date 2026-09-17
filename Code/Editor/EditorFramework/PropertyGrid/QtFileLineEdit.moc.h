#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <QLineEdit>

class WQtFilePropertyWidget;

/// A QLineEdit that is used by WQtFilePropertyWidget
class W_EDITORFRAMEWORK_DLL WQtFileLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  explicit WQtFileLineEdit(WQtFilePropertyWidget* pParent = nullptr);
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;

  WQtFilePropertyWidget* m_pOwner = nullptr;
};
