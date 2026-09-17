#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/QtFileLineEdit.moc.h>

WQtFileLineEdit::WQtFileLineEdit(WQtFilePropertyWidget* pParent)
  : QLineEdit(pParent)
{
  m_pOwner = pParent;
}

void WQtFileLineEdit::dragMoveEvent(QDragMoveEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidFileReference(qtToEzString(str)))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragMoveEvent(e);
}

void WQtFileLineEdit::dragEnterEvent(QDragEnterEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidFileReference(qtToEzString(str)))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragEnterEvent(e);
}

void WQtFileLineEdit::dropEvent(QDropEvent* e)
{
  if (e->source() == this)
  {
    QLineEdit::dropEvent(e);
    return;
  }

  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    WString sPath = str.toUtf8().data();
    if (WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sPath))
    {
      setText(WMakeQString(sPath));
    }
    else
      setText(QString());

    return;
  }


  if (e->mimeData()->hasText())
  {
    QString str = e->mimeData()->text();

    WString sPath = str.toUtf8().data();
    if (WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sPath))
    {
      setText(QString::fromUtf8(sPath.GetData()));
    }
    else
      setText(QString());

    return;
  }
}
