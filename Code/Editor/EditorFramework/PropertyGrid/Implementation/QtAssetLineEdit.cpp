#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>

WQtAssetLineEdit::WQtAssetLineEdit(QWidget* pParent /*= nullptr*/)
  : QLineEdit(pParent)
{
}

void WQtAssetLineEdit::dragMoveEvent(QDragMoveEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidAssetType(str.toUtf8().data()))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragMoveEvent(e);
}

void WQtAssetLineEdit::dragEnterEvent(QDragEnterEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidAssetType(str.toUtf8().data()))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragEnterEvent(e);
}

void WQtAssetLineEdit::dropEvent(QDropEvent* e)
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
      setText(QString::fromUtf8(sPath.GetData()));
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

void WQtAssetLineEdit::paintEvent(QPaintEvent* e)
{
  if (hasFocus())
  {
    QLineEdit::paintEvent(e);
  }
  else
  {
    QPainter p(this);

    // Paint background
    QStyleOptionFrame panel;
    initStyleOption(&panel);
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);

    // Clip to line edit contents
    QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
    auto margins = textMargins();
    r = r.marginsRemoved(margins);
    p.setClipRect(r);

    // Render asset name
    WStringBuilder sText = qtToEzString(text());
    if (sText.IsEmpty())
    {
      sText = qtToEzString(placeholderText());
    }

    WStringView sFinalText = sText;

    if (m_pOwner->IsValidAssetType(sText))
    {
      if (const char* szPipe = sFinalText.FindLastSubString("|"))
      {
        sFinalText = WStringView(szPipe + 1);
      }
      else
      {
        sFinalText = sFinalText.GetFileName();
      }
    }

    r.adjust(2, 0, 2, 0);
    QTextOption opt(Qt::AlignLeft | Qt::AlignVCenter);
    opt.setWrapMode(QTextOption::NoWrap);
    p.drawText(r, WMakeQString(sFinalText), opt);
  }
}

void WQtAssetLineEdit::mousePressEvent(QMouseEvent* e)
{
  QLineEdit::mousePressEvent(e);

  if ((e->button() == Qt::MouseButton::LeftButton && e->modifiers().testFlag(Qt::ControlModifier)) ||
      (e->button() == Qt::MouseButton::MiddleButton))
  {
    Q_EMIT OpenAsset();
    return;
  }

  if ((e->button() == Qt::MouseButton::LeftButton && e->modifiers().testFlag(Qt::ShiftModifier)))
  {
    Q_EMIT SelectAsset();
    return;
  }
}
