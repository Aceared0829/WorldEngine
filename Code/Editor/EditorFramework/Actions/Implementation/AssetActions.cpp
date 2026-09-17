#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

WActionDescriptorHandle WAssetActions::s_hAssetCategory;
WActionDescriptorHandle WAssetActions::s_hTransformAsset;
WActionDescriptorHandle WAssetActions::s_hAssetHelp;
WActionDescriptorHandle WAssetActions::s_hTransformAllAssets;
WActionDescriptorHandle WAssetActions::s_hCheckFileSystem;
WActionDescriptorHandle WAssetActions::s_hWriteDependencyDGML;
WActionDescriptorHandle WAssetActions::s_hCopyAssetGuid;
WActionDescriptorHandle WAssetActions::s_hSelectInAssetBrowser;

void WAssetActions::RegisterActions()
{
  s_hAssetCategory = W_REGISTER_CATEGORY("AssetCategory");
  s_hTransformAsset = W_REGISTER_ACTION_1("Asset.Transform", WActionScope::Document, "Assets", "Ctrl+E", WAssetAction, WAssetAction::ButtonType::TransformAsset);
  s_hAssetHelp = W_REGISTER_ACTION_1("Asset.Help", WActionScope::Document, "Assets", "", WAssetAction, WAssetAction::ButtonType::AssetHelp);
  s_hTransformAllAssets = W_REGISTER_ACTION_1("Asset.TransformAll", WActionScope::Global, "Assets", "Ctrl+Shift+E", WAssetAction, WAssetAction::ButtonType::TransformAllAssets);
  s_hCheckFileSystem = W_REGISTER_ACTION_1("Asset.CheckFilesystem", WActionScope::Global, "Assets", "", WAssetAction, WAssetAction::ButtonType::CheckFileSystem);
  s_hWriteDependencyDGML = W_REGISTER_ACTION_1("Asset.WriteDependencyDGML", WActionScope::Document, "Assets", "", WAssetAction, WAssetAction::ButtonType::WriteDependencyDGML);
  s_hCopyAssetGuid = W_REGISTER_ACTION_1("Asset.CopyAssetGuid", WActionScope::Document, "Assets", "", WAssetAction, WAssetAction::ButtonType::CopyAssetGuid);
  s_hSelectInAssetBrowser = W_REGISTER_ACTION_1("Asset.SelectInAssetBrowser", WActionScope::Document, "Assets", "", WAssetAction, WAssetAction::ButtonType::SelectInAssetBrowser);

  {
    WActionMap* pMap = WActionMapManager::GetActionMap("DocumentWindowTabMenu");
    pMap->MapAction(s_hSelectInAssetBrowser, "", 11.0f);
    pMap->MapAction(s_hCopyAssetGuid, "", 12.0f);
  }
}

void WAssetActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hAssetCategory);
  WActionManager::UnregisterAction(s_hTransformAsset);
  WActionManager::UnregisterAction(s_hAssetHelp);
  WActionManager::UnregisterAction(s_hTransformAllAssets);
  WActionManager::UnregisterAction(s_hCheckFileSystem);
  WActionManager::UnregisterAction(s_hWriteDependencyDGML);
  WActionManager::UnregisterAction(s_hCopyAssetGuid);
  WActionManager::UnregisterAction(s_hSelectInAssetBrowser);
}

void WAssetActions::MapMenuActions(WStringView sMapping)
{
  const WStringView sTargetMenu = "G.Asset";

  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hAssetHelp, sTargetMenu, 1.0f);
  pMap->MapAction(s_hTransformAsset, sTargetMenu, 2.0f);
  pMap->MapAction(s_hCopyAssetGuid, sTargetMenu, 3.0f);
  pMap->MapAction(s_hCheckFileSystem, sTargetMenu, 4.0f);
  pMap->MapAction(s_hTransformAllAssets, sTargetMenu, 5.0f);
  pMap->MapAction(s_hWriteDependencyDGML, sTargetMenu, 6.0f);
}

void WAssetActions::MapToolBarActions(WStringView sMapping, bool bDocument)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hAssetCategory, "", 10.0f);

  if (bDocument)
  {
    pMap->MapAction(s_hTransformAsset, "AssetCategory", 1.0f);
  }
  else
  {
    pMap->MapAction(s_hCheckFileSystem, "AssetCategory", 1.0f);
    pMap->MapAction(s_hTransformAllAssets, "AssetCategory", 2.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// WAssetAction
////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAssetAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WAssetAction::WAssetAction(const WActionContext& context, const char* szName, ButtonType button)
  : WButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  switch (m_ButtonType)
  {
    case WAssetAction::ButtonType::TransformAsset:
      SetIconPath(":/EditorFramework/Icons/TransformAsset.svg");
      break;
    case WAssetAction::ButtonType::TransformAllAssets:
      SetIconPath(":/EditorFramework/Icons/TransformAllAssets.svg");
      break;
    case WAssetAction::ButtonType::CheckFileSystem:
      SetIconPath(":/EditorFramework/Icons/CheckFileSystem.svg");
      break;
    case WAssetAction::ButtonType::AssetHelp:
      SetIconPath(":/GuiFoundation/Icons/Help.svg");
      SetEnabled(!WTranslateHelpURL(context.m_pDocument->GetDocumentTypeName()).IsEmpty());
      break;
    case WAssetAction::ButtonType::WriteDependencyDGML:
      break;
    case WAssetAction::ButtonType::CopyAssetGuid:
      SetIconPath(":/GuiFoundation/Icons/Guid.svg");
      break;
    case WAssetAction::ButtonType::SelectInAssetBrowser:
      break;
  }
}

WAssetAction::~WAssetAction() = default;

void WAssetAction::Execute(const WVariant& value)
{
  switch (m_ButtonType)
  {
    case WAssetAction::ButtonType::TransformAsset:
    {
      if (m_Context.m_pDocument->IsModified())
      {
        WStatus res = const_cast<WDocument*>(m_Context.m_pDocument)->SaveDocument();
        if (res.Failed())
        {
          WLog::Error("Failed to save document '{0}': '{1}'", m_Context.m_pDocument->GetDocumentPath(), res.GetMessageString());
          break;
        }
      }

      WTransformStatus ret = WAssetCurator::GetSingleton()->TransformAsset(m_Context.m_pDocument->GetGuid(), WTransformFlags::ForceTransform | WTransformFlags::TriggeredManually);

      if (ret.Failed())
      {
        WLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, m_Context.m_pDocument->GetDocumentPath());
      }
      else
      {
        WAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
      }
    }
    break;

    case WAssetAction::ButtonType::TransformAllAssets:
    {
      WAssetCurator::GetSingleton()->CheckFileSystem();
      WAssetCurator::GetSingleton()->TransformAllAssets().IgnoreResult();
    }
    break;

    case WAssetAction::ButtonType::CheckFileSystem:
    {
      WAssetCurator::GetSingleton()->CheckFileSystem();
      WAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }
    break;

    case WAssetAction::ButtonType::WriteDependencyDGML:
    {
      WStringBuilder sOutput = QFileDialog::getSaveFileName(QApplication::activeWindow(), "Write to DGML", {}, "DGML (*.dgml)", nullptr, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();

      if (sOutput.IsEmpty())
        return;

      WAssetCurator::GetSingleton()->WriteDependencyDGML(m_Context.m_pDocument->GetGuid(), sOutput);
    }
    break;

    case WAssetAction::ButtonType::AssetHelp:
    {
      WStringView sType = GetContext().m_pDocument->GetDocumentTypeName();
      WString sURL = WTranslateHelpURL(sType);

      if (!sURL.IsEmpty())
      {
        QDesktopServices::openUrl(QUrl(WMakeQString(sURL)));
      }
    }
    break;

    case WAssetAction::ButtonType::CopyAssetGuid:
    {
      WStringBuilder sGuid;
      WConversionUtils::ToString(m_Context.m_pDocument->GetGuid(), sGuid);

      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      mimeData->setText(sGuid.GetData());
      clipboard->setMimeData(mimeData);

      WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(WFmt("Copied asset GUID: {}", sGuid), WTime::MakeFromSeconds(5));
    }
    break;

    case WAssetAction::ButtonType::SelectInAssetBrowser:
    {
      WQtAssetBrowserPanel::GetSingleton()->AssetBrowserWidget->SetSelectedAsset(m_Context.m_pDocument->GetGuid());
      WQtAssetBrowserPanel::GetSingleton()->EnsureVisible();
    }
    break;
  }
}
