#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Actions/MeshPrefabActions.h>
#include <EditorPluginScene/Dialogs/CreateMeshPrefabDlg.moc.h>
#include <EditorPluginScene/Utils/MeshPrefabCreator.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshPrefabAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WActionDescriptorHandle WMeshPrefabActions::s_hCategory;
WActionDescriptorHandle WMeshPrefabActions::s_hCreatePrefabFromMesh;
WActionDescriptorHandle WMeshPrefabActions::s_hCreatePrefabFromMeshDoc;

void WMeshPrefabActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("MeshPrefabCategory");

  s_hCreatePrefabFromMesh = W_REGISTER_ACTION_1("Prefabs.CreateFromMesh", WActionScope::Global, "Prefabs", "", WMeshPrefabAction, WMeshPrefabAction::ActionType::CreatePrefabFromMesh);
  s_hCreatePrefabFromMeshDoc = W_REGISTER_ACTION_1("Prefabs.CreateFromMeshDocument", WActionScope::Document, "Prefabs", "", WMeshPrefabAction, WMeshPrefabAction::ActionType::CreatePrefabFromMesh);
}

void WMeshPrefabActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hCreatePrefabFromMesh);
  WActionManager::UnregisterAction(s_hCreatePrefabFromMeshDoc);
}

WResult WMeshPrefabActions::MapActions(WStringView sActionMap, WStringView sSubPath, bool bDocumentScope)
{
  // Not an assert: the mesh document's action maps belong to EditorPluginAssets, which is not
  // guaranteed to have been loaded first.
  WActionMap* pMap = WActionMapManager::GetActionMap(sActionMap);
  if (pMap == nullptr)
    return W_FAILURE;

  pMap->MapAction(s_hCategory, sSubPath, 10.0f);
  pMap->MapAction(bDocumentScope ? s_hCreatePrefabFromMeshDoc : s_hCreatePrefabFromMesh, "MeshPrefabCategory", 1.0f);
  return W_SUCCESS;
}

void WMeshPrefabAction::GetTargetAssets(WDynamicArray<WUuid>& out_assets) const
{
  out_assets.Clear();

  if (const WAssetDocument* pAssetDoc = WDynamicCast<const WAssetDocument*>(m_Context.m_pDocument))
  {
    if (WMeshPrefabCreator::IsMeshAsset(pAssetDoc->GetGuid()))
    {
      out_assets.PushBack(pAssetDoc->GetGuid());
    }

    return;
  }

  for (const WUuid& guid : WAssetBrowserSelection::GetCurrent().m_AssetGuids)
  {
    if (WMeshPrefabCreator::IsMeshAsset(guid))
    {
      out_assets.PushBack(guid);
    }
  }
}

WMeshPrefabAction::WMeshPrefabAction(const WActionContext& context, const char* szName, WMeshPrefabAction::ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::CreatePrefabFromMesh:
      SetIconPath(":/AssetIcons/Prefab.svg");
      break;
  }

  RefreshState();
}

void WMeshPrefabAction::RefreshState()
{
  switch (m_Type)
  {
    case ActionType::CreatePrefabFromMesh:
    {
      WHybridArray<WUuid, 16> assets;
      GetTargetAssets(assets);

      SetVisible(!assets.IsEmpty(), false);
      SetEnabled(!assets.IsEmpty(), false);
      break;
    }
  }
}

void WMeshPrefabAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::CreatePrefabFromMesh:
    {
      WHybridArray<WUuid, 16> assets;
      GetTargetAssets(assets);

      if (assets.IsEmpty())
        return;

      if (assets.GetCount() == 1)
      {
        WMeshPrefabSource source;
        if (WMeshPrefabCreator::GatherMeshPrefabSource(assets[0], source).Failed())
        {
          WQtUiServices::MessageBoxWarning("The selected asset is not a mesh asset.");
          return;
        }

        WQtCreateMeshPrefabDlg dlg(source, nullptr);
        if (dlg.exec() != QDialog::Accepted)
          return;

        const WStatus res = WMeshPrefabCreator::CreateMeshPrefab(source, dlg.GetOptions());
        WQtUiServices::MessageBoxStatus(res, "Failed to create the prefab.", "", true);
        return;
      }

      WQtCreateMeshPrefabDlg dlg(assets.GetCount(), nullptr);
      if (dlg.exec() != QDialog::Accepted)
        return;

      WUInt32 uiCreated = 0;
      WUInt32 uiSkipped = 0;
      const WStatus res = WMeshPrefabCreator::CreateMeshPrefabs(assets, dlg.GetOptions(), uiCreated, uiSkipped);

      if (res.Failed())
      {
        WQtUiServices::MessageBoxStatus(res, "Failed to create the prefabs.", "", true);
        return;
      }

      // which meshes were skipped and why is in the log
      if (uiSkipped > 0)
      {
        WQtUiServices::MessageBoxInformation(WFmt("Created {} prefab(s), skipped {}.\n\nSee the log for details.", uiCreated, uiSkipped));
      }
      break;
    }
  }
}
