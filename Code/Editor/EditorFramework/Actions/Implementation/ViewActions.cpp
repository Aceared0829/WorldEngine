#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

WActionDescriptorHandle WViewActions::s_hRenderMode;
WActionDescriptorHandle WViewActions::s_hPerspective;
WActionDescriptorHandle WViewActions::s_hActivateRemoteProcess;
WActionDescriptorHandle WViewActions::s_hLinkDeviceCamera;


void WViewActions::RegisterActions()
{
  s_hRenderMode = W_REGISTER_DYNAMIC_MENU("View.RenderMode", WRenderModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hPerspective = W_REGISTER_DYNAMIC_MENU("View.RenderPerspective", WPerspectiveAction, ":/EditorFramework/Icons/Perspective.svg");
  s_hActivateRemoteProcess = W_REGISTER_ACTION_1("View.ActivateRemoteProcess", WActionScope::Window, "View", "", WViewAction, WViewAction::ButtonType::ActivateRemoteProcess);
  s_hLinkDeviceCamera = W_REGISTER_ACTION_1("View.LinkDeviceCamera", WActionScope::Window, "View", "", WViewAction, WViewAction::ButtonType::LinkDeviceCamera);
}

void WViewActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hRenderMode);
  WActionManager::UnregisterAction(s_hPerspective);
  WActionManager::UnregisterAction(s_hActivateRemoteProcess);
  WActionManager::UnregisterAction(s_hLinkDeviceCamera);
}

void WViewActions::MapToolbarActions(WStringView sMapping, WUInt32 uiFlags)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  if (uiFlags & Flags::PerspectiveMode)
    pMap->MapAction(s_hPerspective, "", 1.0f);

  if (uiFlags & Flags::RenderMode)
    pMap->MapAction(s_hRenderMode, "", 2.0f);

  if (uiFlags & Flags::ActivateRemoteProcess)
  {
    pMap->MapAction(s_hActivateRemoteProcess, "", 4.0f);
    pMap->MapAction(s_hLinkDeviceCamera, "", 5.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// WRenderModeAction
////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderModeAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WRenderModeAction::WRenderModeAction(const WActionContext& context, const char* szName, const char* szIconPath)
  : WEnumerationMenuAction(context, szName, szIconPath)
{
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(context.m_pWindow);
  W_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'WQtEngineViewWidget'!");
  InitEnumerationType(WGetStaticRTTI<WViewRenderMode>());
}

WInt64 WRenderModeAction::GetValue() const
{
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(m_Context.m_pWindow);
  return (WInt64)pView->m_pViewConfig->m_RenderMode;
}

void WRenderModeAction::Execute(const WVariant& value)
{
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(m_Context.m_pWindow);
  pView->m_pViewConfig->m_RenderMode = (WViewRenderMode::Enum)value.ConvertTo<WInt64>();
  TriggerUpdate();
}

////////////////////////////////////////////////////////////////////////
// WPerspectiveAction
////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPerspectiveAction, 1, WRTTINoAllocator)
  ;
W_END_DYNAMIC_REFLECTED_TYPE;

WPerspectiveAction::WPerspectiveAction(const WActionContext& context, const char* szName, const char* szIconPath)
  : WEnumerationMenuAction(context, szName, szIconPath)
{
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(context.m_pWindow);
  W_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'WQtEngineViewWidget'!");
  InitEnumerationType(WGetStaticRTTI<WSceneViewPerspective>());
}

WInt64 WPerspectiveAction::GetValue() const
{
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(m_Context.m_pWindow);
  return (WInt64)pView->m_pViewConfig->m_Perspective;
}

void WPerspectiveAction::Execute(const WVariant& value)
{
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(m_Context.m_pWindow);
  auto newValue = (WSceneViewPerspective::Enum)value.ConvertTo<WInt64>();

  if (pView->m_pViewConfig->m_Perspective != newValue)
  {
    pView->m_pViewConfig->m_Perspective = newValue;
    pView->m_pViewConfig->ApplyPerspectiveSetting();
    TriggerUpdate();
  }
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewAction, 1, WRTTINoAllocator)
  ;
W_END_DYNAMIC_REFLECTED_TYPE;

WViewAction::WViewAction(const WActionContext& context, const char* szName, ButtonType button)
  : WButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case WViewAction::ButtonType::ActivateRemoteProcess:
      SetIconPath(":/EditorFramework/Icons/SwitchToRemoteProcess.svg");
      break;
    case WViewAction::ButtonType::LinkDeviceCamera:
      SetIconPath(":/EditorFramework/Icons/LinkDeviceCamera.svg");
      SetCheckable(true);
      SetChecked(pView->m_pViewConfig->m_bUseCameraTransformOnDevice);
      break;
  }
}

WViewAction::~WViewAction() = default;

void WViewAction::Execute(const WVariant& value)
{
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case WViewAction::ButtonType::ActivateRemoteProcess:
    {
      WEditorEngineProcessConnection::GetSingleton()->ActivateRemoteProcess(WDynamicCast<WAssetDocument*>(m_Context.m_pDocument), pView->GetViewID());
    }
    break;

    case WViewAction::ButtonType::LinkDeviceCamera:
    {
      pView->m_pViewConfig->m_bUseCameraTransformOnDevice = !pView->m_pViewConfig->m_bUseCameraTransformOnDevice;
      SetChecked(pView->m_pViewConfig->m_bUseCameraTransformOnDevice);
      WEditorEngineProcessConnection::GetSingleton()->ActivateRemoteProcess(WDynamicCast<WAssetDocument*>(m_Context.m_pDocument), pView->GetViewID());
    }
    break;
  }
}
