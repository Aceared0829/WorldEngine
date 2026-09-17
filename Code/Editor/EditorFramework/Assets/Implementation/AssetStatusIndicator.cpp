#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>

WQtAssetStatusIndicator::WQtAssetStatusIndicator(WAssetDocument* pDoc, QWidget* pParent)
  : QFrame(pParent)
{
  m_pAsset = pDoc;

  setContentsMargins(0, 0, 0, 0);
  setLayout(new QHBoxLayout());

  layout()->setContentsMargins(0, 0, 0, 0);

  m_pLabel = new QPushButton();
  m_pLabel->setFlat(true);
  connect(m_pLabel, &QPushButton::clicked, this, &WQtAssetStatusIndicator::onClick);

  layout()->addWidget(m_pLabel);

  m_pHelp = new QPushButton();
  connect(m_pHelp, &QPushButton::clicked, this, &WQtAssetStatusIndicator::onHelp);
  m_pHelp->setFlat(true);
  m_pHelp->setIcon(QIcon(":/GuiFoundation/Icons/Help.svg"));
  m_pHelp->setMaximumWidth(32);
  m_pHelp->setToolTip(WMakeQString(WTranslateTooltip("Asset.Help")));
  layout()->addWidget(m_pHelp);

  m_pAsset->m_EventsOne.AddEventHandler(WMakeDelegate(&WQtAssetStatusIndicator::DocumentEventHandler, this));
  WAssetCurator::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WQtAssetStatusIndicator::AssetEventHandler, this));

  UpdateDisplay();
}

WQtAssetStatusIndicator::~WQtAssetStatusIndicator()
{
  m_pAsset->m_EventsOne.RemoveEventHandler(WMakeDelegate(&WQtAssetStatusIndicator::DocumentEventHandler, this));
  WAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtAssetStatusIndicator::AssetEventHandler, this));
}

void WQtAssetStatusIndicator::DocumentEventHandler(const WDocumentEvent& e)
{
  if (e.m_Type == WDocumentEvent::Type::ModifiedChanged)
  {
    UpdateDisplay();
  }
}

void WQtAssetStatusIndicator::AssetEventHandler(const WAssetCuratorEvent& e)
{
  if (e.m_AssetGuid == m_pAsset->GetGuid())
  {
    if (e.m_Type == WAssetCuratorEvent::Type::AssetUpdated)
    {
      UpdateDisplay();
    }
  }
}

void WQtAssetStatusIndicator::UpdateDisplay()
{
  // states:
  // all good
  // document modified - but live preview
  // document modified - no preview
  // saved - waiting for background transform
  // saved - needs manual transform (bg off)
  // transform error

  if (m_pAsset->IsModified())
  {
    auto flags = m_pAsset->GetAssetFlags();
    const bool bTransformOnSave = flags.IsSet(WAssetDocumentFlags::AutoTransformOnSave);
    const bool bBgRunning = WAssetProcessor::GetSingleton()->GetProcessorState() == WAssetProcessor::ProcessorState::Running;

    // no flag for live preview available (ignore)

    if (bTransformOnSave || bBgRunning)
    {
      m_pLabel->setText("Asset Modified: Click to Save");
      m_Action = Action::Save;
    }
    else
    {
      m_pLabel->setText("Asset Modified: Click to Transform");
      m_Action = Action::Transform;
    }

    m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/Attention.svg"));
  }
  else
  {
    auto assetInfo = WAssetCurator::GetSingleton()->GetSubAsset(m_pAsset->GetGuid());
    switch (assetInfo->m_pAssetInfo->m_TransformState)
    {
      case WAssetInfo::TransformState::UpToDate:
      case WAssetInfo::TransformState::NeedsThumbnail:
      {
        m_pLabel->setText("Asset State: All Good");
        m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/AssetOk.svg"));
        m_Action = Action::None;
        break;
      }

      case WAssetInfo::TransformState::NeedsImport:
      case WAssetInfo::TransformState::NeedsTransform:
      {
        const bool bBgRunning = WAssetProcessor::GetSingleton()->GetProcessorState() == WAssetProcessor::ProcessorState::Running;

        if (bBgRunning)
        {
          m_pLabel->setText("Waiting for Transform: Click to Force");
          m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/AssetNeedsTransform.svg"));
        }
        else
        {
          m_pLabel->setText("Asset Changed: Click to Transform");
          m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/Attention.svg"));
        }

        m_Action = Action::Transform;
        break;
      }

      case WAssetInfo::TransformState::TransformError:
      case WAssetInfo::TransformState::MissingTransformDependency:
      case WAssetInfo::TransformState::MissingThumbnailDependency:
      case WAssetInfo::TransformState::MissingPackageDependency:
      case WAssetInfo::TransformState::CircularDependency:
        m_pLabel->setText("Asset Error: Click for Details");
        m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/AssetFailedTransform.svg"));
        m_Action = Action::ShowErrors;
        break;

      default:
        break;
    }
  }
}

void WQtAssetStatusIndicator::onClick(bool)
{
  switch (m_Action)
  {
    case Action::Save:
      m_pAsset->SaveDocument().IgnoreResult();
      break;

    case Action::None:
      // also transform in this case
      [[fallthrough]];

    case Action::Transform:
      m_pAsset->TransformAsset(WTransformFlags::TriggeredManually | WTransformFlags::ForceTransform);
      break;

    case Action::ShowErrors:
    {
      auto assetInfo = WAssetCurator::GetSingleton()->GetSubAsset(m_pAsset->GetGuid());

      WStringBuilder output;
      output.Set("Asset transform failed.\n\n");

      if (!assetInfo->m_pAssetInfo->m_LogEntries.IsEmpty())
      {
        output.Append("Errors:\n\n");

        for (const WLogEntry& logEntry : assetInfo->m_pAssetInfo->m_LogEntries)
        {
          output.AppendFormat("{}\n", logEntry.m_sMsg);
        }
      }

      auto getNiceName = [](const WString& sDep) -> WStringBuilder
      {
        if (WConversionUtils::IsStringUuid(sDep))
        {
          WUuid guid = WConversionUtils::ConvertStringToUuid(sDep);
          auto assetInfoDep = WAssetCurator::GetSingleton()->GetSubAsset(guid);
          if (assetInfoDep)
          {
            return assetInfoDep->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
          }

          WUInt64 uiLow;
          WUInt64 uiHigh;
          guid.GetValues(uiLow, uiHigh);
          WStringBuilder sTmp;
          sTmp.SetFormat("{} - u4{{},{}}", sDep, uiLow, uiHigh);

          return sTmp;
        }

        return sDep;
      };

      WSet<WString> missingDeps;

      if (!assetInfo->m_pAssetInfo->m_MissingTransformDeps.IsEmpty())
      {
        for (const WString& dep : assetInfo->m_pAssetInfo->m_MissingTransformDeps)
        {
          missingDeps.Insert(getNiceName(dep));
        }
      }

      if (!assetInfo->m_pAssetInfo->m_MissingPackageDeps.IsEmpty())
      {
        for (const WString& dep : assetInfo->m_pAssetInfo->m_MissingPackageDeps)
        {
          missingDeps.Insert(getNiceName(dep));
        }
      }

      if (!assetInfo->m_pAssetInfo->m_MissingThumbnailDeps.IsEmpty())
      {
        for (const WString& dep : assetInfo->m_pAssetInfo->m_MissingThumbnailDeps)
        {
          missingDeps.Insert(getNiceName(dep));
        }
      }

      if (!missingDeps.IsEmpty())
      {
        output.Append("Missing Dependencies:\n\n");

        for (const WString& dep : missingDeps)
        {
          output.AppendFormat("{}\n", dep);
        }
      }

      WQtUiServices::GetSingleton()->MessageBoxInformation(output);

      break;
    }
  }
}

void WQtAssetStatusIndicator::onHelp(bool)
{
  WStringView sType = m_pAsset->GetDocumentTypeName();
  WString sURL = WTranslateHelpURL(sType);

  if (!sURL.IsEmpty())
  {
    QDesktopServices::openUrl(QUrl(WMakeQString(sURL)));
  }
  else
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation(WFmt("There is no known online documentation for the asset type '{}'.\n\nPlease report this to the developers.", sType));
  }
}
