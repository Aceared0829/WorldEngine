#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/CameraModeSwitchActions.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCameraModeSwitchAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WActionDescriptorHandle WCameraModeSwitchActions::s_hCameraMode;

void WCameraModeSwitchActions::RegisterActions()
{
  s_hCameraMode = W_REGISTER_DYNAMIC_MENU("Asset.CameraMode", WCameraModeSwitchAction, ":/EditorFramework/Icons/Camera.svg");
}

void WCameraModeSwitchActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCameraMode);
}

void WCameraModeSwitchActions::MapToolbarActions(const char* szMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(szMapping);
  W_ASSERT_DEV(pMap != nullptr, "Toolbar action map '{}' does not exist", szMapping);
  pMap->MapAction(s_hCameraMode, "", 10.0f);
}

//////////////////////////////////////////////////////////////////////////

WCameraModeSwitchAction::WCameraModeSwitchAction(const WActionContext& context, const char* szName, const char* szIconPath)
  : WDynamicMenuAction(context, szName, szIconPath)
{
}

void WCameraModeSwitchAction::GetEntries(WDynamicArray<Item>& out_entries)
{
  out_entries.Clear();

  auto* pWindow = qobject_cast<WQtEngineDocumentWindow*>(m_Context.m_pWindow);
  if (pWindow == nullptr)
    return;

  const int iMode = pWindow->GetCameraMode();

  WTempHybridArray<WString, 8> names;
  pWindow->GetCameraModeNames(names);

  for (WUInt32 i = 0; i < names.GetCount(); ++i)
  {
    auto& item = out_entries.ExpandAndGetRef();
    item.m_sDisplay = names[i];
    item.m_UserValue = (int)i;
    item.m_CheckState = (iMode == (int)i) ? Item::CheckMark::Checked : Item::CheckMark::Unchecked;
  }
}

void WCameraModeSwitchAction::Execute(const WVariant& value)
{
  auto* pWindow = qobject_cast<WQtEngineDocumentWindow*>(m_Context.m_pWindow);
  if (pWindow)
    pWindow->SetCameraMode(value.ConvertTo<int>());
}
