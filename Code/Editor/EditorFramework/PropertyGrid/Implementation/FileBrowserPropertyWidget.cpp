#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/QtFileLineEdit.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

WQtFilePropertyWidget::WQtFilePropertyWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new WQtFileLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  W_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  W_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr, "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("... "));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);

  {
    QMenu* pMenu = new QMenu();

    pMenu->setDefaultAction(pMenu->addAction(QIcon(), QLatin1String("Select File"), this, SLOT(on_BrowseFile_clicked())));
    QAction* pActionOpenFile = pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open File"), this, SLOT(OnOpenFile()));
    QAction* pActionOpenWith = pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open With..."), this, SLOT(OnOpenFileWith()));

    pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnOpenExplorer()));

    connect(pMenu, &QMenu::aboutToShow, pMenu, [=]()
      {
        pActionOpenFile->setEnabled(!m_pWidget->text().isEmpty());
        pActionOpenWith->setEnabled(!m_pWidget->text().isEmpty());
        //
      });

    m_pButton->setMenu(pMenu);
  }

  m_pWarningIcon = new QLabel(this);
  m_pWarningIcon->setPixmap(WQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Warning.svg").pixmap(16, 16));
  m_pWarningIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_pWarningIcon->setVisible(false);
  m_pWarningIcon->setToolTip(QStringLiteral("This property is required and must reference a file."));

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);
  m_pLayout->addWidget(m_pWarningIcon);
}

void WQtFilePropertyWidget::UpdateRequiredIndicator(bool bValueEmpty)
{
  const bool bRequired = m_pProp->GetAttributeByType<WRequiredAttribute>() != nullptr;
  m_pWarningIcon->setVisible(bRequired && bValueEmpty);
}

bool WQtFilePropertyWidget::IsValidFileReference(WStringView sFile) const
{
  auto pAttr = m_pProp->GetAttributeByType<WFileBrowserAttribute>();

  WTempHybridArray<WStringView, 8> extensions;
  WStringView sTemp = pAttr->GetTypeFilter();
  sTemp.Split(false, extensions, ";");
  for (WStringView& ext : extensions)
  {
    ext.TrimWordStart("*.");
    if (sFile.GetFileExtension().IsEqual_NoCase(ext))
      return true;
  }

  return false;
}

void WQtFilePropertyWidget::SetReadOnly(bool bReadOnly /*= true*/)
{
  m_pWidget->setReadOnly(bReadOnly);
}

void WQtFilePropertyWidget::OnInit()
{
  auto pAttr = m_pProp->GetAttributeByType<WFileBrowserAttribute>();
  W_ASSERT_DEV(pAttr != nullptr, "WQtFilePropertyWidget was created without a WFileBrowserAttribute!");

  if (!pAttr->GetCreateTitle().IsEmpty())
  {
    m_pButton->menu()->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/DocumentAdd.svg")), QString("Create %1...").arg(WMakeQString(pAttr->GetCreateTitle())), this, SLOT(OnCreateFile()));
  }

  if (!pAttr->GetCustomAction().IsEmpty())
  {
    m_pButton->menu()->addAction(QIcon(), WMakeQString(WTranslate(pAttr->GetCustomAction())), this, SLOT(OnCustomAction()));
  }
}

void WQtFilePropertyWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals b(m_pWidget);
  WQtScopedBlockSignals b2(m_pButton);

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
    m_pWarningIcon->setVisible(false);
  }
  else
  {
    WStringBuilder sText = value.ConvertTo<WString>();

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));

    UpdateRequiredIndicator(sText.IsEmpty());
  }
}

void WQtFilePropertyWidget::on_TextFinished_triggered()
{
  WStringBuilder sText = m_pWidget->text().toUtf8().data();

  UpdateRequiredIndicator(sText.IsEmpty());

  BroadcastValueChanged(sText.GetData());
}

void WQtFilePropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
    on_TextFinished_triggered();
}

void WQtFilePropertyWidget::OnOpenExplorer()
{
  WString sPath = m_pWidget->text().toUtf8().data();
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  WQtUiServices::OpenInExplorer(sPath, true);
}


void WQtFilePropertyWidget::OnCustomAction()
{
  auto pAttr = m_pProp->GetAttributeByType<WFileBrowserAttribute>();

  if (pAttr->GetCustomAction() == nullptr)
    return;

  auto it = WDocumentManager::s_CustomActions.Find(pAttr->GetCustomAction());

  if (!it.IsValid())
    return;

  WVariant res = it.Value()(m_pGrid->GetDocument());

  if (!res.IsValid() || !res.IsA<WString>())
    return;

  m_pWidget->setText(res.Get<WString>().GetData());
  on_TextFinished_triggered();
}

void WQtFilePropertyWidget::OnOpenFile()
{
  WString sPath = m_pWidget->text().toUtf8().data();
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  if (WQtUiServices::OpenFileInDefaultProgram(sPath).Failed())
  {
    WQtUiServices::MessageBoxInformation(WFmt("File could not be opened:\n{0}\nCheck that the file exists, that a program is associated "
                                                "with this file type and that access to this file is not denied.",
      sPath));
  }
}

void WQtFilePropertyWidget::OnOpenFileWith()
{
  WString sPath = m_pWidget->text().toUtf8().data();
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  WQtUiServices::OpenWith(sPath);
}

void WQtFilePropertyWidget::OnCreateFile()
{
  const WFileBrowserAttribute* pFileAttribute = m_pProp->GetAttributeByType<WFileBrowserAttribute>();

  WStringBuilder sStartDir = m_pGrid->GetDocument()->GetDocumentPath();
  sStartDir.RemoveFileExtension();

  const QString sTitle = QString("Create %1").arg(WMakeQString(pFileAttribute->GetCreateTitle()));
  const QString sExt = QString("%1 %2").arg(WMakeQString(pFileAttribute->GetCreateTitle())).arg(WMakeQString(pFileAttribute->GetTypeFilter()));

  QString sResult = QFileDialog::getSaveFileName(this, sTitle, sStartDir.GetData(), sExt, nullptr);

  if (sResult.isEmpty())
    return;


  WStringBuilder sPath = sResult.toUtf8().data();

  if (!WOSFile::ExistsFile(sPath))
  {
    WStringBuilder sTemplateDoc = "Editor/DocumentTemplates/Default";
    sTemplateDoc.ChangeFileExtension(sPath.GetFileExtension());

    bool bCreate = true;

    WStringBuilder sAbs;
    if (WFileSystem::ResolvePath(sTemplateDoc, &sAbs, nullptr).Succeeded())
    {
      if (WOSFile::CopyFile(sAbs, sPath).Succeeded())
      {
        bCreate = false;
      }
    }

    if (bCreate)
    {
      WOSFile file;
      file.Open(sPath, WFileOpenMode::Write).IgnoreResult();
    }
  }

  if (!WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sPath))
    return;

  m_pWidget->setText(WMakeQString(sPath));
  on_TextFinished_triggered();
}

static WMap<WString, WString> s_StartDirs;

void WQtFilePropertyWidget::on_BrowseFile_clicked()
{
  WString sFile = m_pWidget->text().toUtf8().data();
  const WFileBrowserAttribute* pFileAttribute = m_pProp->GetAttributeByType<WFileBrowserAttribute>();

  auto& sStartDir = s_StartDirs[pFileAttribute->GetTypeFilter()];

  if (!sFile.IsEmpty())
  {
    WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sFile);

    WStringBuilder st = sFile;
    st = st.GetFileDirectory();

    sStartDir = st;
  }

  if (sStartDir.IsEmpty())
    sStartDir = WToolsProject::GetSingleton()->GetProjectFile();

  WQtAssetBrowserDlg dlg(this, pFileAttribute->GetDialogTitle(), sFile, pFileAttribute->GetTypeFilter());
  if (dlg.exec() == QDialog::Rejected)
    return;

  WStringView sResult = dlg.GetSelectedAssetPathRelative();

  if (sResult.IsEmpty())
    return;

  // the returned path is a "datadir parent relative path" and we must remove the first folder
  if (const char* nextSep = sResult.FindSubString("/"))
  {
    sResult.SetStartPosition(nextSep + 1);
  }

  sStartDir = sResult;

  m_pWidget->setText(WMakeQString(sResult));
  on_TextFinished_triggered();
}

//////////////////////////////////////////////////////////////////////////

WQtExternalFilePropertyWidget::WQtExternalFilePropertyWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  W_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  W_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr, "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("... "));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);

  {
    QMenu* pMenu = new QMenu();

    pMenu->setDefaultAction(pMenu->addAction(QIcon(), QLatin1String("Select File"), this, SLOT(on_BrowseFile_clicked())));
    QAction* pActionOpenFile = pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open File"), this, SLOT(OnOpenFile()));
    QAction* pActionOpenWith = pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open With..."), this, SLOT(OnOpenFileWith()));
    pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnOpenExplorer()));

    connect(pMenu, &QMenu::aboutToShow, pMenu, [=]()
      {
        pActionOpenFile->setEnabled(!m_pWidget->text().isEmpty());
        pActionOpenWith->setEnabled(!m_pWidget->text().isEmpty());
        //
      });
    m_pButton->setMenu(pMenu);
  }

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);
}

bool WQtExternalFilePropertyWidget::IsValidFileReference(WStringView sFile) const
{
  auto pAttr = m_pProp->GetAttributeByType<WExternalFileBrowserAttribute>();

  WTempHybridArray<WStringView, 8> extensions;
  WStringView sTemp = pAttr->GetTypeFilter();
  sTemp.Split(false, extensions, ";");
  for (WStringView& ext : extensions)
  {
    ext.TrimWordStart("*.");
    if (sFile.GetFileExtension().IsEqual_NoCase(ext))
      return true;
  }

  return false;
}

void WQtExternalFilePropertyWidget::OnInit()
{
  auto pAttr = m_pProp->GetAttributeByType<WExternalFileBrowserAttribute>();
  W_ASSERT_DEV(pAttr != nullptr, "WQtFilePropertyWidget was created without a WExternalFileBrowserAttribute!");
}

void WQtExternalFilePropertyWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals b(m_pWidget);
  WQtScopedBlockSignals b2(m_pButton);

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    WStringBuilder sText = value.ConvertTo<WString>();

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));
  }
}

void WQtExternalFilePropertyWidget::on_TextFinished_triggered()
{
  WStringBuilder sText = m_pWidget->text().toUtf8().data();

  BroadcastValueChanged(sText.GetData());
}

void WQtExternalFilePropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
    on_TextFinished_triggered();
}

void WQtExternalFilePropertyWidget::OnOpenExplorer()
{
  WString sPath = m_pWidget->text().toUtf8().data();
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  WQtUiServices::OpenInExplorer(sPath, true);
}

void WQtExternalFilePropertyWidget::OnOpenFile()
{
  WString sPath = m_pWidget->text().toUtf8().data();
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  if (WQtUiServices::OpenFileInDefaultProgram(sPath).Failed())
  {
    WQtUiServices::MessageBoxInformation(WFmt("File could not be opened:\n{0}\nCheck that the file exists, that a program is associated "
                                                "with this file type and that access to this file is not denied.",
      sPath));
  }
}

void WQtExternalFilePropertyWidget::OnOpenFileWith()
{
  WString sPath = m_pWidget->text().toUtf8().data();
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  WQtUiServices::OpenWith(sPath);
}

void WQtExternalFilePropertyWidget::on_BrowseFile_clicked()
{
  WString sFile = m_pWidget->text().toUtf8().data();
  const WExternalFileBrowserAttribute* pFileAttribute = m_pProp->GetAttributeByType<WExternalFileBrowserAttribute>();

  auto& sStartDir = s_StartDirs[pFileAttribute->GetTypeFilter()];

  if (sStartDir.IsEmpty())
  {
    sStartDir = sFile.GetFileDirectory();
  }

  if (sStartDir.IsEmpty())
  {
    sStartDir = WToolsProject::GetSingleton()->GetProjectFile();
  }

  QString sResult = QFileDialog::getOpenFileName(this, WMakeQString(pFileAttribute->GetDialogTitle()), sStartDir.GetData(), WMakeQString(pFileAttribute->GetTypeFilter()), nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sResult.isEmpty())
    return;

  sFile = sResult.toUtf8().data();

  // doesn't matter if this fails
  WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile);

  sStartDir = sFile.GetFileDirectory();

  m_pWidget->setText(sResult);
  on_TextFinished_triggered();
}
