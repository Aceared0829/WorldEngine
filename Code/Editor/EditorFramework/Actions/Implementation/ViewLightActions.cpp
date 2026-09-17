#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewLightButtonAction, 1, WRTTINoAllocator);
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewLightSliderAction, 1, WRTTINoAllocator);
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WActionDescriptorHandle WViewLightActions::s_hLightMenu;
WActionDescriptorHandle WViewLightActions::s_hSkyBox;
WActionDescriptorHandle WViewLightActions::s_hSkyLight;
WActionDescriptorHandle WViewLightActions::s_hSkyLightCubeMap;
WActionDescriptorHandle WViewLightActions::s_hSkyLightIntensity;
WActionDescriptorHandle WViewLightActions::s_hDirLight;
WActionDescriptorHandle WViewLightActions::s_hDirLightAngle;
WActionDescriptorHandle WViewLightActions::s_hDirLightShadows;
WActionDescriptorHandle WViewLightActions::s_hDirLightIntensity;
WActionDescriptorHandle WViewLightActions::s_hFog;
WActionDescriptorHandle WViewLightActions::s_hSetAsDefault;

void WViewLightActions::RegisterActions()
{
  s_hLightMenu = W_REGISTER_MENU_WITH_ICON("View.LightMenu", ":/EditorFramework/Icons/ViewLightMenu.svg");
  s_hSkyBox = W_REGISTER_ACTION_1(
    "View.SkyBox", WActionScope::Document, "View", "", WViewLightButtonAction, WEngineViewLightSettingsEvent::Type::SkyBoxChanged);
  s_hSkyLight = W_REGISTER_ACTION_1(
    "View.SkyLight", WActionScope::Document, "View", "", WViewLightButtonAction, WEngineViewLightSettingsEvent::Type::SkyLightChanged);
  s_hSkyLightCubeMap = W_REGISTER_ACTION_1(
    "View.SkyLightCubeMap", WActionScope::Document, "View", "", WViewLightButtonAction, WEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged);
  s_hSkyLightIntensity = W_REGISTER_ACTION_1(
    "View.SkyLightIntensity", WActionScope::Document, "View", "", WViewLightSliderAction, WEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged);

  s_hDirLight = W_REGISTER_ACTION_1(
    "View.DirectionalLight", WActionScope::Document, "View", "", WViewLightButtonAction, WEngineViewLightSettingsEvent::Type::DirectionalLightChanged);
  s_hDirLightAngle = W_REGISTER_ACTION_1(
    "View.DirLightAngle", WActionScope::Document, "View", "", WViewLightSliderAction, WEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged);
  s_hDirLightShadows = W_REGISTER_ACTION_1(
    "View.DirectionalLightShadows", WActionScope::Document, "View", "", WViewLightButtonAction, WEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged);
  s_hDirLightIntensity = W_REGISTER_ACTION_1(
    "View.DirLightIntensity", WActionScope::Document, "View", "", WViewLightSliderAction, WEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged);
  s_hFog = W_REGISTER_ACTION_1(
    "View.Fog", WActionScope::Document, "View", "", WViewLightButtonAction, WEngineViewLightSettingsEvent::Type::FogChanged);
  s_hSetAsDefault = W_REGISTER_ACTION_1(
    "View.SetAsDefault", WActionScope::Document, "View", "", WViewLightButtonAction, WEngineViewLightSettingsEvent::Type::DefaultValuesChanged);
}

void WViewLightActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hLightMenu);
  WActionManager::UnregisterAction(s_hSkyBox);
  WActionManager::UnregisterAction(s_hSkyLight);
  WActionManager::UnregisterAction(s_hSkyLightCubeMap);
  WActionManager::UnregisterAction(s_hSkyLightIntensity);
  WActionManager::UnregisterAction(s_hDirLight);
  WActionManager::UnregisterAction(s_hDirLightAngle);
  WActionManager::UnregisterAction(s_hDirLightShadows);
  WActionManager::UnregisterAction(s_hDirLightIntensity);
  WActionManager::UnregisterAction(s_hFog);
  WActionManager::UnregisterAction(s_hSetAsDefault);
}

void WViewLightActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hLightMenu, "", 2.5f);
  const WStringView sSubPath = "View.LightMenu";
  pMap->MapAction(s_hSkyBox, sSubPath, 1.0f);
  pMap->MapAction(s_hSkyLight, sSubPath, 1.0f);
  pMap->MapAction(s_hSkyLightCubeMap, sSubPath, 2.0f);
  pMap->MapAction(s_hSkyLightIntensity, sSubPath, 3.0f);
  pMap->MapAction(s_hDirLight, sSubPath, 4.0f);
  pMap->MapAction(s_hDirLightAngle, sSubPath, 5.0f);
  pMap->MapAction(s_hDirLightShadows, sSubPath, 6.0f);
  pMap->MapAction(s_hDirLightIntensity, sSubPath, 7.0f);
  pMap->MapAction(s_hFog, sSubPath, 8.0f);
  pMap->MapAction(s_hSetAsDefault, sSubPath, 9.0f);
}

//////////////////////////////////////////////////////////////////////////

WViewLightButtonAction::WViewLightButtonAction(const WActionContext& context, const char* szName, WEngineViewLightSettingsEvent::Type button)
  : WButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(m_Context.m_pWindow);
  m_pSettings = static_cast<WEngineViewLightSettings*>(pView->GetDocumentWindow()->GetDocument()->FindSyncObject(WEngineViewLightSettings::GetStaticRTTI()));
  W_ASSERT_DEV(m_pSettings != nullptr, "The asset document does not have a WEngineViewLightSettings sync object.");
  m_SettingsID = m_pSettings->m_EngineViewLightSettingsEvents.AddEventHandler(WMakeDelegate(&WViewLightButtonAction::LightSettingsEventHandler, this));

  switch (m_ButtonType)
  {
    case WEngineViewLightSettingsEvent::Type::SkyBoxChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/WSkyBoxComponent.svg");
      break;
    case WEngineViewLightSettingsEvent::Type::SkyLightChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/WSkyLightComponent.svg");
      break;
    case WEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
      SetIconPath(":/TypeIcons/WSkyLightComponent.svg");
      break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/WDirectionalLightComponent.svg");
      break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/WDirectionalLightComponent.svg");
      break;
    case WEngineViewLightSettingsEvent::Type::FogChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/WFogComponent.svg");
      break;
    case WEngineViewLightSettingsEvent::Type::DefaultValuesChanged:
      SetCheckable(false);
      SetIconPath(":/EditorFramework/Icons/ViewLightMenu.svg");
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  UpdateAction();
}

WViewLightButtonAction::~WViewLightButtonAction()
{
  m_pSettings->m_EngineViewLightSettingsEvents.RemoveEventHandler(m_SettingsID);
}

void WViewLightButtonAction::Execute(const WVariant& value)
{
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case WEngineViewLightSettingsEvent::Type::SkyBoxChanged:
    {
      m_pSettings->SetSkyBox(value.ConvertTo<bool>());
    }
    break;
    case WEngineViewLightSettingsEvent::Type::SkyLightChanged:
    {
      m_pSettings->SetSkyLight(value.ConvertTo<bool>());
    }
    break;
    case WEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
    {
      WStringBuilder sFile = m_pSettings->GetSkyLightCubeMap();
      WUuid assetGuid = WConversionUtils::ConvertStringToUuid(sFile);

      WQtAssetBrowserDlg dlg(pView, assetGuid, "CompatibleAsset_Texture_Cube");
      if (dlg.exec() == 0)
        return;

      assetGuid = dlg.GetSelectedAssetGuid();
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

      m_pSettings->SetSkyLightCubeMap(sFile);
    }
    break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
    {
      m_pSettings->SetDirectionalLight(value.ConvertTo<bool>());
    }
    break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
    {
      m_pSettings->SetDirectionalLightShadows(value.ConvertTo<bool>());
    }
    break;
    case WEngineViewLightSettingsEvent::Type::FogChanged:
    {
      m_pSettings->SetFog(value.ConvertTo<bool>());
    }
    break;
    case WEngineViewLightSettingsEvent::Type::DefaultValuesChanged:
    {
      if (WQtUiServices::MessageBoxQuestion("Do you want to make the current light settings the global default?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
      {
        WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
        pPreferences->SetAsDefaultValues(*m_pSettings);
      }
    }
    break;
    default:
      break;
  }
}

void WViewLightButtonAction::LightSettingsEventHandler(const WEngineViewLightSettingsEvent& e)
{
  if (m_ButtonType == e.m_Type)
  {
    UpdateAction();
  }
}

void WViewLightButtonAction::UpdateAction()
{
  switch (m_ButtonType)
  {
    case WEngineViewLightSettingsEvent::Type::SkyBoxChanged:
    {
      SetChecked(m_pSettings->GetSkyBox());
    }
    break;
    case WEngineViewLightSettingsEvent::Type::SkyLightChanged:
    {
      SetChecked(m_pSettings->GetSkyLight());
    }
    break;
    case WEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
    {
    }
    break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
    {
      SetChecked(m_pSettings->GetDirectionalLight());
    }
    break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
    {
      SetChecked(m_pSettings->GetDirectionalLightShadows());
    }
    break;
    case WEngineViewLightSettingsEvent::Type::FogChanged:
    {
      SetChecked(m_pSettings->GetFog());
    }
    break;
    default:
      break;
  }
}
//////////////////////////////////////////////////////////////////////////

WViewLightSliderAction::WViewLightSliderAction(const WActionContext& context, const char* szName, WEngineViewLightSettingsEvent::Type button)
  : WSliderAction(context, szName)
{
  m_ButtonType = button;
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(m_Context.m_pWindow);
  m_pSettings = static_cast<WEngineViewLightSettings*>(pView->GetDocumentWindow()->GetDocument()->FindSyncObject(WEngineViewLightSettings::GetStaticRTTI()));
  W_ASSERT_DEV(m_pSettings != nullptr, "The asset document does not have a WEngineViewLightSettings sync object.");
  m_SettingsID = m_pSettings->m_EngineViewLightSettingsEvents.AddEventHandler(WMakeDelegate(&WViewLightSliderAction::LightSettingsEventHandler, this));

  switch (m_ButtonType)
  {
    case WEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
      SetIconPath(":/TypeIcons/WSkyLightComponent.svg");
      SetRange(0, 20);
      break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
      SetIconPath(":/TypeIcons/WDirectionalLightComponent.svg");
      SetRange(0, 360);
      break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
      SetIconPath(":/TypeIcons/WDirectionalLightComponent.svg");
      SetRange(0, 200);
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  UpdateAction();
}

WViewLightSliderAction::~WViewLightSliderAction()
{
  m_pSettings->m_EngineViewLightSettingsEvents.RemoveEventHandler(m_SettingsID);
}

void WViewLightSliderAction::Execute(const WVariant& value)
{
  switch (m_ButtonType)
  {
    case WEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
    {
      m_pSettings->SetSkyLightIntensity(value.ConvertTo<float>() / 10.0f);
    }
    break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
    {
      m_pSettings->SetDirectionalLightAngle(WAngle::MakeFromDegree(value.ConvertTo<float>()));
    }
    break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
    {
      m_pSettings->SetDirectionalLightIntensity(value.ConvertTo<float>() / 10.0f);
    }
    break;
    default:
      break;
  }
}

void WViewLightSliderAction::LightSettingsEventHandler(const WEngineViewLightSettingsEvent& e)
{
  if (m_ButtonType == e.m_Type)
  {
    UpdateAction();
  }
}

void WViewLightSliderAction::UpdateAction()
{
  switch (m_ButtonType)
  {
    case WEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
    {
      SetValue(WMath::Clamp((WInt32)(m_pSettings->GetSkyLightIntensity() * 10.0f), 0, 20));
    }
    break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
    {
      SetValue(WMath::Clamp((WInt32)(m_pSettings->GetDirectionalLightAngle().GetDegree()), 0, 360));
    }
    break;
    case WEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
    {
      SetValue(WMath::Clamp((WInt32)(m_pSettings->GetDirectionalLightIntensity() * 10.0f), 1, 200));
    }
    break;
    default:
      break;
  }
}
