#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WQtAssetPropertyWidget::WQtAssetPropertyWidget()
  : WQtStandardPropertyWidget()
{
  m_uiThumbnailID = 0;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pWidget = new WQtAssetLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  m_pWidget->m_pOwner = this;
  setFocusProxy(m_pWidget);

  W_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  W_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr, "signal/slot connection failed");
  W_VERIFY(connect(m_pWidget, SIGNAL(OpenAsset()), this, SLOT(OnOpenAssetDocument())) != nullptr, "signal/slot connection failed");
  W_VERIFY(connect(m_pWidget, SIGNAL(SelectAsset()), this, SLOT(on_BrowseFile_clicked())) != nullptr, "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("... "));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setPopupMode(QToolButton::InstantPopup);

  QMenu* pMenu = new QMenu();
  pMenu->setToolTipsVisible(true);
  m_pButton->setMenu(pMenu);

  connect(pMenu, &QMenu::aboutToShow, this, &WQtAssetPropertyWidget::OnShowMenu);

  m_pWarningIcon = new QLabel(this);
  m_pWarningIcon->setPixmap(WQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Warning.svg").pixmap(16, 16));
  m_pWarningIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_pWarningIcon->setVisible(false);

  m_pLayout->addWidget(m_pWidget, 1);
  m_pLayout->addWidget(m_pButton);
  m_pLayout->addWidget(m_pWarningIcon);

  W_VERIFY(connect(WQtImageCache::GetSingleton(), &WQtImageCache::ImageLoaded, this, &WQtAssetPropertyWidget::ThumbnailLoaded) != nullptr, "signal/slot connection failed");
  W_VERIFY(
    connect(WQtImageCache::GetSingleton(), &WQtImageCache::ImageInvalidated, this, &WQtAssetPropertyWidget::ThumbnailInvalidated) != nullptr, "signal/slot connection failed");
}

bool WQtAssetPropertyWidget::IsValidAssetType(const char* szAssetReference) const
{
  WAssetCurator::WLockedSubAsset pAsset;

  if (!WConversionUtils::IsStringUuid(szAssetReference))
  {
    pAsset = WAssetCurator::GetSingleton()->FindSubAsset(szAssetReference);

    if (pAsset == nullptr)
    {
      const WAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<WAssetBrowserAttribute>();

      // if this file type is on the asset whitelist for this asset type, let it through
      return WAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(pAssetAttribute->GetTypeFilter(), szAssetReference);
    }
  }
  else
  {
    const WUuid AssetGuid = WConversionUtils::ConvertStringToUuid(szAssetReference);

    pAsset = WAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);
  }

  // invalid asset in general
  if (pAsset == nullptr)
    return false;

  const WAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<WAssetBrowserAttribute>();

  if (WStringUtils::IsEqual(pAssetAttribute->GetTypeFilter(), ";;")) // empty type list -> allows everything
    return true;

  WStringBuilder sTypeFilter(";", pAsset->m_Data.m_sSubAssetsDocumentTypeName, ";");

  if (WStringUtils::FindSubString_NoCase(pAssetAttribute->GetTypeFilter(), sTypeFilter) != nullptr)
    return true;

  if (const WDocumentTypeDescriptor* pDesc = WDocumentManager::GetDescriptorForDocumentType(pAsset->m_Data.m_sSubAssetsDocumentTypeName))
  {
    for (const WString& comp : pDesc->m_CompatibleTypes)
    {
      sTypeFilter.Set(";", comp, ";");

      if (WStringUtils::FindSubString_NoCase(pAssetAttribute->GetTypeFilter(), sTypeFilter) != nullptr)
        return true;
    }
  }

  return false;
}

void WQtAssetPropertyWidget::OnInit()
{
  W_ASSERT_DEV(m_pProp->GetAttributeByType<WAssetBrowserAttribute>() != nullptr, "WQtAssetPropertyWidget was created without a WAssetBrowserAttribute!");
}

void WQtAssetPropertyWidget::UpdateThumbnail(const WUuid& guid, const char* szThumbnailPath)
{
  if (IsUndead())
    return;

  const QPixmap* pThumbnailPixmap = nullptr;

  if (guid.IsValid())
  {
    WUInt64 uiUserData1, uiUserData2;
    m_AssetGuid.GetValues(uiUserData1, uiUserData2);

    const WAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<WAssetBrowserAttribute>();
    WStringBuilder sTypeFilter = pAssetAttribute->GetTypeFilter();
    sTypeFilter.Trim(" ;");

    pThumbnailPixmap = WQtImageCache::GetSingleton()->QueryPixmapForType(
      sTypeFilter, szThumbnailPath, QModelIndex(), QVariant(uiUserData1), QVariant(uiUserData2), &m_uiThumbnailID);
  }

  if (pThumbnailPixmap)
  {
    m_pButton->setIcon(QIcon(pThumbnailPixmap->scaledToWidth(16, Qt::TransformationMode::SmoothTransformation)));
    m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
  }
  else
  {
    m_pButton->setIcon(QIcon());
    m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  }
}

void WQtAssetPropertyWidget::UpdateRequiredIndicator(bool bValueEmpty, bool bValueValid)
{
  const bool bRequired = m_pProp->GetAttributeByType<WRequiredAttribute>() != nullptr;
  const bool bShow = (bRequired && bValueEmpty) || (!bValueEmpty && !bValueValid);

  m_pWarningIcon->setVisible(bShow);

  if (bShow)
  {
    m_pWarningIcon->setToolTip(bValueEmpty ? QStringLiteral("This property is required and must reference a valid asset.") : QStringLiteral("The selected file is not a valid asset."));
  }
}

void WQtAssetPropertyWidget::InternalSetValue(const WVariant& value)
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
    m_AssetGuid = WUuid();
    WStringBuilder sThumbnailPath;

    if (WConversionUtils::IsStringUuid(sText))
    {
      if (!IsValidAssetType(sText))
      {
        m_uiThumbnailID = 0;

        m_pWidget->setText(WMakeQString(sText));

        m_pButton->setIcon(QIcon());
        m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);

        m_Pal.setColor(QPalette::Active, QPalette::Text, Qt::red);
        m_Pal.setColor(QPalette::Inactive, QPalette::Text, Qt::red);
        m_pWidget->setPalette(m_Pal);

        UpdateRequiredIndicator(false, false);

        return;
      }

      WUuid newAssetGuid = WConversionUtils::ConvertStringToUuid(sText);

      // If this is a thumbnail or transform dependency, make sure the target is not in our inverse hull, i.e. we don't create a circular dependency.
      const WAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<WAssetBrowserAttribute>();
      if (pAssetAttribute->GetDependencyFlags().IsAnySet(WDependencyFlags::Thumbnail | WDependencyFlags::Transform))
      {
        WUuid documentGuid = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetGuid();
        WAssetCurator::WLockedSubAsset asset = WAssetCurator::GetSingleton()->GetSubAsset(documentGuid);
        if (asset.isValid())
        {
          WSet<WUuid> inverseHull;
          WAssetCurator::GetSingleton()->GenerateInverseTransitiveHull(asset->m_pAssetInfo, inverseHull, true, true);
          if (inverseHull.Contains(newAssetGuid))
          {
            WQtUiServices::GetSingleton()->MessageBoxWarning("This asset can't be used here, as that would create a circular dependency.");
            return;
          }
        }
      }

      m_AssetGuid = newAssetGuid;
      auto pAsset = WAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid);

      if (pAsset)
      {
        pAsset->GetSubAssetIdentifier(sText);

        sThumbnailPath = pAsset->m_pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAsset->m_pAssetInfo->m_Path, pAsset->m_Data.m_sName);
      }
      else
      {
        m_AssetGuid = WUuid();
      }
    }

    UpdateThumbnail(m_AssetGuid, sThumbnailPath);

    {
      const QColor validColor = WToQtColor(WColorScheme::LightUI(WColorScheme::Green));
      const QColor invalidColor = WToQtColor(WColorScheme::LightUI(WColorScheme::Red));

      m_Pal.setColor(QPalette::Active, QPalette::Text, m_AssetGuid.IsValid() ? validColor : invalidColor);
      m_Pal.setColor(QPalette::Inactive, QPalette::Text, m_AssetGuid.IsValid() ? validColor : invalidColor);
      m_pWidget->setPalette(m_Pal);

      if (m_AssetGuid.IsValid())
        m_pWidget->setToolTip(QStringLiteral("Valid asset selected.\n\nCTRL+LMB or MMB to open the asset document.\nSHIFT+LMB to select a different asset."));
      else
        m_pWidget->setToolTip(QStringLiteral("The selected file is not a valid asset."));
    }

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));

    UpdateRequiredIndicator(sText.IsEmpty(), m_AssetGuid.IsValid());
  }
}

void WQtAssetPropertyWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  m_pWidget->setPalette(m_Pal);
  WQtStandardPropertyWidget::showEvent(event);
}

void WQtAssetPropertyWidget::FillAssetMenu(QMenu& menu)
{
  if (!menu.isEmpty())
    menu.addSeparator();

  const bool bAsset = m_AssetGuid.IsValid();
  menu.setDefaultAction(menu.addAction(QIcon(), QLatin1String("Select Asset"), this, SLOT(on_BrowseFile_clicked())));
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open Asset"), this, SLOT(OnOpenAssetDocument()))->setEnabled(bAsset);
  menu.addAction(QIcon(), QLatin1String("Select in Asset Browser"), this, SLOT(OnSelectInAssetBrowser()))->setEnabled(bAsset);
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnOpenExplorer()))->setEnabled(bAsset);
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Guid.svg")), QLatin1String("Copy Asset Guid"), this, SLOT(OnCopyAssetGuid()))->setEnabled(bAsset);
  menu.addAction(QIcon(), QLatin1String("Create New Asset"), this, SLOT(OnCreateNewAsset()));
  menu.addAction(QIcon(":/GuiFoundation/Icons/Clear.svg"), QLatin1String("Clear Asset Reference"), this, SLOT(OnClearReference()))->setEnabled(bAsset);
}

void WQtAssetPropertyWidget::on_TextFinished_triggered()
{
  WStringBuilder sText = m_pWidget->text().toUtf8().data();

  auto pAsset = WAssetCurator::GetSingleton()->FindSubAsset(sText);

  if (pAsset)
  {
    WConversionUtils::ToString(pAsset->m_Data.m_Guid, sText);
  }

  BroadcastValueChanged(sText.GetData());
}


void WQtAssetPropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
    on_TextFinished_triggered();
}

void WQtAssetPropertyWidget::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
{
  const WUuid guid(UserData1.toULongLong(), UserData2.toULongLong());

  if (guid == m_AssetGuid)
  {
    UpdateThumbnail(guid, sPath.toUtf8().data());
  }
}


void WQtAssetPropertyWidget::ThumbnailInvalidated(QString sPath, WUInt32 uiImageID)
{
  if (m_uiThumbnailID == uiImageID)
  {
    UpdateThumbnail(WUuid(), "");
  }
}

void WQtAssetPropertyWidget::OnOpenAssetDocument()
{
  if (!m_AssetGuid.IsValid())
    return;

  if (auto asset = WAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid))
  {
    WQtEditorApp::GetSingleton()->OpenDocumentQueued(asset->m_pAssetInfo->m_Path.GetAbsolutePath(), GetSelection()[0].m_pObject);
  }
}

void WQtAssetPropertyWidget::OnSelectInAssetBrowser()
{
  WQtAssetBrowserPanel::GetSingleton()->AssetBrowserWidget->SetSelectedAsset(m_AssetGuid);
  WQtAssetBrowserPanel::GetSingleton()->EnsureVisible();
}

void WQtAssetPropertyWidget::OnOpenExplorer()
{
  WString sPath;

  if (m_AssetGuid.IsValid())
  {
    sPath = WAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_Path.GetAbsolutePath();
  }
  else
  {
    sPath = m_pWidget->text().toUtf8().data();
    if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      return;
  }

  WQtUiServices::OpenInExplorer(sPath, true);
}

void WQtAssetPropertyWidget::OnCopyAssetGuid()
{
  WStringBuilder sGuid;

  if (m_AssetGuid.IsValid())
  {
    WConversionUtils::ToString(m_AssetGuid, sGuid);
  }
  else
  {
    sGuid = m_pWidget->text().toUtf8().data();
    if (!WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sGuid))
      return;
  }

  QClipboard* clipboard = QApplication::clipboard();
  QMimeData* mimeData = new QMimeData();
  mimeData->setText(QString::fromUtf8(sGuid.GetData()));
  clipboard->setMimeData(mimeData);

  WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(WFmt("Copied asset GUID: {}", sGuid), WTime::MakeFromSeconds(5));
}

void WQtAssetPropertyWidget::OnCreateNewAsset()
{
  WString sPath;

  // try to pick a good path
  {
    if (m_AssetGuid.IsValid())
    {
      sPath = WAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_Path.GetAbsolutePath();
    }
    else
    {
      sPath = m_pWidget->text().toUtf8().data();

      if (sPath.IsEmpty())
      {
        sPath = ":project/";
      }

      WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
    }
  }

  const WAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<WAssetBrowserAttribute>();
  WStringBuilder sTypeFilter = pAssetAttribute->GetTypeFilter();

  WTempHybridArray<WString, 4> allowedTypes;
  sTypeFilter.Split(false, allowedTypes, ";");

  WStringBuilder tmp;

  for (WString& type : allowedTypes)
  {
    tmp = type;
    tmp.Trim(" ");
    type = tmp;
  }

  struct info
  {
    WAssetDocumentManager* pAssetMan = nullptr;
    const WDocumentTypeDescriptor* pDocType = nullptr;
  };

  WMap<WString, info> typesToUse;

  {
    const WHybridArray<WDocumentManager*, 16>& managers = WDocumentManager::GetAllDocumentManagers();

    for (WDocumentManager* pMan : managers)
    {
      if (auto pAssetMan = WDynamicCast<WAssetDocumentManager*>(pMan))
      {
        WTempHybridArray<const WDocumentTypeDescriptor*, 4> documentTypes;
        pAssetMan->GetSupportedDocumentTypes(documentTypes);

        for (const WDocumentTypeDescriptor* pType : documentTypes)
        {
          if (allowedTypes.IndexOf(pType->m_sDocumentTypeName) == WInvalidIndex)
          {
            for (const WString& compType : pType->m_CompatibleTypes)
            {
              if (allowedTypes.IndexOf(compType) != WInvalidIndex)
                goto allowed;
            }

            continue;
          }

        allowed:

          auto& toUse = typesToUse[pType->m_sDocumentTypeName];

          toUse.pAssetMan = pAssetMan;
          toUse.pDocType = pType;
        }
      }
    }
  }

  if (typesToUse.IsEmpty())
    return;

  WStringBuilder sFilter;
  QString sSelectedFilter;

  for (auto it : typesToUse)
  {
    const auto& ttu = it.Value();

    const WString sAssetType = ttu.pDocType->m_sDocumentTypeName;
    const WString sExtension = ttu.pDocType->m_sFileExtension;

    sFilter.AppendWithSeparator(";;", sAssetType, " (*.", sExtension, ")");

    if (sSelectedFilter.isEmpty())
    {
      sSelectedFilter = sExtension.GetData();
    }
  }


  WStringBuilder sOutput = sPath;
  {

    QString sStartDir = sOutput.GetFileDirectory().GetData(tmp);
    sOutput = QFileDialog::getSaveFileName(
      QApplication::activeWindow(), "Create Asset", sStartDir, sFilter.GetData(), &sSelectedFilter, QFileDialog::Option::DontResolveSymlinks)
                .toUtf8()
                .data();

    if (sOutput.IsEmpty())
      return;
  }

  sFilter = sOutput.GetFileExtension();

  for (auto it : typesToUse)
  {
    const auto& ttu = it.Value();

    if (sFilter.IsEqual_NoCase(ttu.pDocType->m_sFileExtension))
    {
      WDocument* pDoc = nullptr;

      const WStatus res = ttu.pAssetMan->CreateDocument(ttu.pDocType->m_sDocumentTypeName, sOutput, pDoc, WDocumentFlags::RequestWindow | WDocumentFlags::AddToRecentFilesList);

      WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Creating the document failed.");

      if (res.Succeeded())
      {
        // if this is an asset, make sure it gets transformed, so that the output file exists
        // and make sure the filesystem knows about it (the asset lookup table is written)
        // so that redirections inside the resource manager will work right away
        // otherwise they may only work after a while (the world gets set up again) which would be irritating
        if (WAssetDocument* pAsset = WDynamicCast<WAssetDocument*>(pDoc))
        {
          WAssetCurator::GetSingleton()->NotifyOfAssetChange(pAsset->GetGuid());

          if (pAsset->TransformAsset(WTransformFlags::Default).Failed())
          {
            WLog::Error("Failed to transform newly created asset '{}'", pDoc->GetDocumentPath());
            break;
          }

          WAssetCurator::GetSingleton()->MainThreadTick(false);
          WAssetCurator::GetSingleton()->WriteAssetTables(nullptr, true).IgnoreResult();
        }

        pDoc->EnsureVisible();

        InternalSetValue(sOutput.GetData());
        on_TextFinished_triggered();
      }
      break;
    }
  }
}

void WQtAssetPropertyWidget::OnClearReference()
{
  InternalSetValue("");
  on_TextFinished_triggered();
}

void WQtAssetPropertyWidget::OnShowMenu()
{
  m_pButton->menu()->clear();
  FillAssetMenu(*m_pButton->menu());
}

void WQtAssetPropertyWidget::on_BrowseFile_clicked()
{
  WStringBuilder sFile = m_pWidget->text().toUtf8().data();
  const WAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<WAssetBrowserAttribute>();

  WQtAssetBrowserDlg dlg(this, m_AssetGuid, pAssetAttribute->GetTypeFilter(), {}, pAssetAttribute->GetRequiredTag());
  if (dlg.exec() == 0)
    return;

  WUuid assetGuid = dlg.GetSelectedAssetGuid();
  if (assetGuid.IsValid())
    WConversionUtils::ToString(assetGuid, sFile);

  if (sFile.IsEmpty())
  {
    sFile = dlg.GetSelectedAssetPathRelative();

    if (sFile.IsEmpty())
    {
      sFile = dlg.GetSelectedAssetPathAbsolute();

      WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile);
    }
  }

  if (sFile.IsEmpty())
    return;

  InternalSetValue(sFile.GetData());

  on_TextFinished_triggered();
}
