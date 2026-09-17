#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginJolt/Actions/MeshColliderActions.h>
#include <EditorPluginJolt/Dialogs/CreateMeshColliderDlg.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshColliderAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WActionDescriptorHandle WMeshColliderActions::s_hCategory;
WActionDescriptorHandle WMeshColliderActions::s_hCreateCollider;
WActionDescriptorHandle WMeshColliderActions::s_hCreateColliderDoc;

void WMeshColliderActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("MeshColliderCategory");

  // Two descriptors, because the scope decides both the action's context and how its proxy is cached:
  // Global once overall for the asset browser, Document once per mesh document.
  s_hCreateCollider = W_REGISTER_ACTION_0("Jolt.CreateColliderFromMesh", WActionScope::Global, "Jolt", "", WMeshColliderAction);
  s_hCreateColliderDoc = W_REGISTER_ACTION_0("Jolt.CreateColliderFromMeshDocument", WActionScope::Document, "Jolt", "", WMeshColliderAction);
}

void WMeshColliderActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hCreateCollider);
  WActionManager::UnregisterAction(s_hCreateColliderDoc);
}

WResult WMeshColliderActions::MapActions(WStringView sActionMap, WStringView sSubPath, bool bDocumentScope)
{
  // Not an assert: the mesh document's action maps belong to EditorPluginAssets, which is not
  // guaranteed to have been loaded first.
  WActionMap* pMap = WActionMapManager::GetActionMap(sActionMap);
  if (pMap == nullptr)
    return W_FAILURE;

  pMap->MapAction(s_hCategory, sSubPath, 11.0f);
  pMap->MapAction(bDocumentScope ? s_hCreateColliderDoc : s_hCreateCollider, "MeshColliderCategory", 1.0f);
  return W_SUCCESS;
}

void WMeshColliderAction::GetTargetAssets(WDynamicArray<WUuid>& out_assets) const
{
  out_assets.Clear();

  if (const WAssetDocument* pAssetDoc = WDynamicCast<const WAssetDocument*>(m_Context.m_pDocument))
  {
    if (WMeshColliderCreator::IsMeshAsset(pAssetDoc->GetGuid()))
    {
      out_assets.PushBack(pAssetDoc->GetGuid());
    }

    return;
  }

  for (const WUuid& guid : WAssetBrowserSelection::GetCurrent().m_AssetGuids)
  {
    if (WMeshColliderCreator::IsMeshAsset(guid))
    {
      out_assets.PushBack(guid);
    }
  }
}

WMeshColliderAction::WMeshColliderAction(const WActionContext& context, const char* szName)
  : WButtonAction(context, szName, false, "")
{
  SetIconPath(":/AssetIcons/Jolt_Collision_Mesh.svg");

  RefreshState();
}

void WMeshColliderAction::RefreshState()
{
  WHybridArray<WUuid, 16> assets;
  GetTargetAssets(assets);

  SetVisible(!assets.IsEmpty(), false);
  SetEnabled(!assets.IsEmpty(), false);
}

void WMeshColliderAction::Execute(const WVariant& value)
{
  WHybridArray<WUuid, 16> assets;
  GetTargetAssets(assets);

  if (assets.IsEmpty())
    return;

  if (assets.GetCount() == 1)
  {
    WMeshColliderSource source;
    if (WMeshColliderCreator::GatherMeshColliderSource(assets[0], source).Failed())
    {
      WQtUiServices::MessageBoxWarning("The selected asset is not a mesh asset.");
      return;
    }

    WQtCreateMeshColliderDlg dlg(source, nullptr);
    if (dlg.exec() != QDialog::Accepted)
      return;

    const WStatus res = WMeshColliderCreator::CreateMeshCollider(source, dlg.GetOptions());
    WQtUiServices::MessageBoxStatus(res, "Failed to create the collision mesh asset.", "", true);
    return;
  }

  WQtCreateMeshColliderDlg dlg(assets.GetCount(), nullptr);
  if (dlg.exec() != QDialog::Accepted)
    return;

  WUInt32 uiCreated = 0;
  WUInt32 uiSkipped = 0;
  const WStatus res = WMeshColliderCreator::CreateMeshColliders(assets, dlg.GetOptions(), uiCreated, uiSkipped);

  if (res.Failed())
  {
    WQtUiServices::MessageBoxStatus(res, "Failed to create the collision mesh assets.", "", true);
    return;
  }

  if (uiSkipped > 0)
  {
    // which meshes were skipped and why is in the log
    WQtUiServices::MessageBoxInformation(WFmt("Created {} collision mesh asset(s), skipped {}.\n\nSee the log for details.", uiCreated, uiSkipped));
  }
}
