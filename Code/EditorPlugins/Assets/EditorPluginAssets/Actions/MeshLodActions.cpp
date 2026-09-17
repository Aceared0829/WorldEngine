#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginAssets/Actions/MeshLodActions.h>
#include <EditorPluginAssets/Dialogs/CreateMeshLodsDlg.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshLodAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WActionDescriptorHandle WMeshLodActions::s_hCategory;
WActionDescriptorHandle WMeshLodActions::s_hCreateLods;
WActionDescriptorHandle WMeshLodActions::s_hCreateLodsDoc;

void WMeshLodActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("MeshLodCategory");

  s_hCreateLods = W_REGISTER_ACTION_0("Meshes.CreateLods", WActionScope::Global, "Meshes", "", WMeshLodAction);
  s_hCreateLodsDoc = W_REGISTER_ACTION_0("Meshes.CreateLodsDocument", WActionScope::Document, "Meshes", "", WMeshLodAction);
}

void WMeshLodActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hCreateLods);
  WActionManager::UnregisterAction(s_hCreateLodsDoc);
}

WResult WMeshLodActions::MapActions(WStringView sActionMap, WStringView sSubPath, bool bDocumentScope)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sActionMap);
  if (pMap == nullptr)
    return W_FAILURE;

  // 9.0f, so that creating LODs sits above creating a prefab (10.0f) and a collider (11.0f)
  pMap->MapAction(s_hCategory, sSubPath, 9.0f);
  pMap->MapAction(bDocumentScope ? s_hCreateLodsDoc : s_hCreateLods, "MeshLodCategory", 1.0f);
  return W_SUCCESS;
}

bool WMeshLodAction::IsLodAsset(const WUuid& assetGuid)
{
  auto pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (!pSubAsset.isValid() || pSubAsset->m_pAssetInfo == nullptr)
    return false;

  return WPathUtils::GetFileName(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath()).StartsWith_NoCase("LOD-");
}

void WMeshLodAction::GetTargetAssets(WDynamicArray<WUuid>& out_assets) const
{
  out_assets.Clear();

  if (const WAssetDocument* pAssetDoc = WDynamicCast<const WAssetDocument*>(m_Context.m_pDocument))
  {
    if (WMeshLodCreator::IsMeshAsset(pAssetDoc->GetGuid()) && !IsLodAsset(pAssetDoc->GetGuid()))
    {
      out_assets.PushBack(pAssetDoc->GetGuid());
    }

    return;
  }

  for (const WUuid& guid : WAssetBrowserSelection::GetCurrent().m_AssetGuids)
  {
    if (WMeshLodCreator::IsMeshAsset(guid) && !IsLodAsset(guid))
    {
      out_assets.PushBack(guid);
    }
  }
}

WMeshLodAction::WMeshLodAction(const WActionContext& context, const char* szName)
  : WButtonAction(context, szName, false, "")
{
  SetIconPath(":/AssetIcons/Mesh.svg");

  RefreshState();
}

void WMeshLodAction::RefreshState()
{
  WHybridArray<WUuid, 16> assets;
  GetTargetAssets(assets);

  SetVisible(!assets.IsEmpty(), false);
  SetEnabled(!assets.IsEmpty(), false);
}

void WMeshLodAction::Execute(const WVariant& value)
{
  WHybridArray<WUuid, 16> assets;
  GetTargetAssets(assets);

  if (assets.IsEmpty())
    return;

  if (assets.GetCount() == 1)
  {
    WMeshLodSource source;
    if (WMeshLodCreator::GatherMeshLodSource(assets[0], source).Failed())
    {
      WQtUiServices::MessageBoxWarning("The selected asset is not a mesh asset.");
      return;
    }

    WQtCreateMeshLodsDlg dlg(source, nullptr);
    if (dlg.exec() != QDialog::Accepted)
      return;

    WUInt32 uiCreated = 0;
    WUInt32 uiSkipped = 0;
    const WStatus res = WMeshLodCreator::CreateMeshLods(source, dlg.GetOptions(), uiCreated, uiSkipped);

    if (res.Failed())
    {
      WQtUiServices::MessageBoxStatus(res, "Failed to create the LOD assets.", "", true);
      return;
    }

    if (uiSkipped > 0)
    {
      WQtUiServices::MessageBoxInformation(WFmt("Created {} LOD asset(s), skipped {}.", uiCreated, uiSkipped));
    }

    return;
  }

  WQtCreateMeshLodsDlg dlg(assets.GetCount(), nullptr);
  if (dlg.exec() != QDialog::Accepted)
    return;

  WUInt32 uiCreated = 0;
  WUInt32 uiSkipped = 0;
  const WStatus res = WMeshLodCreator::CreateMeshLodsForAll(assets, dlg.GetOptions(), uiCreated, uiSkipped);

  if (res.Failed())
  {
    WQtUiServices::MessageBoxStatus(res, "Failed to create the LOD assets.", "", true);
    return;
  }

  // which meshes were skipped and why is in the log
  if (uiSkipped > 0)
  {
    WQtUiServices::MessageBoxInformation(WFmt("Created {} LOD asset(s), skipped {}.\n\nSee the log for details.", uiCreated, uiSkipped));
  }
}
