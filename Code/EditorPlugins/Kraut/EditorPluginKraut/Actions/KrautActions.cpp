#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/Actions/KrautActions.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WKrautAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WKrautActions::s_hCategory;
WActionDescriptorHandle WKrautActions::s_hWindStrengthMenu;
WActionDescriptorHandle WKrautActions::s_hWindStrength[4];
WActionDescriptorHandle WKrautActions::s_hToggleFrondsLeaves;

void WKrautActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("KrautCategory");

  s_hWindStrengthMenu = W_REGISTER_MENU_WITH_ICON("Kraut.Wind.Menu", ":/EditorPluginKraut/Wind.svg");
  s_hWindStrength[0] = W_REGISTER_ACTION_2(
    "Kraut.Wind.Off", WActionScope::Document, "Kraut Tree", "", WKrautAction, WKrautAction::ActionType::WindStrength, WKrautWindStrength::Off);
  s_hWindStrength[1] = W_REGISTER_ACTION_2(
    "Kraut.Wind.Light", WActionScope::Document, "Kraut Tree", "", WKrautAction, WKrautAction::ActionType::WindStrength, WKrautWindStrength::Light);
  s_hWindStrength[2] = W_REGISTER_ACTION_2(
    "Kraut.Wind.Moderate", WActionScope::Document, "Kraut Tree", "", WKrautAction, WKrautAction::ActionType::WindStrength, WKrautWindStrength::Moderate);
  s_hWindStrength[3] = W_REGISTER_ACTION_2(
    "Kraut.Wind.Strong", WActionScope::Document, "Kraut Tree", "", WKrautAction, WKrautAction::ActionType::WindStrength, WKrautWindStrength::Strong);

  s_hToggleFrondsLeaves = W_REGISTER_ACTION_1("Kraut.ToggleFrondsLeaves", WActionScope::Document, "Kraut Tree", "", WKrautAction, WKrautAction::ActionType::ToggleFrondsLeaves);
}

void WKrautActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hWindStrengthMenu);

  for (int i = 0; i < W_ARRAY_SIZE(s_hWindStrength); ++i)
    WActionManager::UnregisterAction(s_hWindStrength[i]);

  WActionManager::UnregisterAction(s_hToggleFrondsLeaves);
}

void WKrautActions::MapActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "KrautCategory";

  pMap->MapAction(s_hWindStrengthMenu, szSubPath, 1.0f);

  WStringBuilder sSubPath(szSubPath, "/Kraut.Wind.Menu");

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(s_hWindStrength); ++i)
    pMap->MapAction(s_hWindStrength[i], sSubPath, i + 1.0f);

  pMap->MapAction(s_hToggleFrondsLeaves, szSubPath, 2.0f);
}

WKrautAction::WKrautAction(const WActionContext& context, const char* szName, WKrautAction::ActionType type, WKrautWindStrength::Enum windStrength)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_WindStrength = windStrength;

  m_pDocument = const_cast<WKrautTreeAssetDocument*>(static_cast<const WKrautTreeAssetDocument*>(context.m_pDocument));
  m_pDocument->m_Events.AddEventHandler(WMakeDelegate(&WKrautAction::KrautEventHandler, this));

  UpdateState();

  if (m_Type == ActionType::ToggleFrondsLeaves)
  {
    SetIconPath(":/EditorPluginKraut/Leaf.svg");
  }
}

WKrautAction::~WKrautAction()
{
  m_pDocument->m_Events.RemoveEventHandler(WMakeDelegate(&WKrautAction::KrautEventHandler, this));
}

void WKrautAction::Execute(const WVariant& value)
{
  if (m_Type == ActionType::WindStrength)
  {
    m_pDocument->SetWindStrength(m_WindStrength);
  }
  else if (m_Type == ActionType::ToggleFrondsLeaves)
  {
    m_pDocument->SetShowFrondsLeaves(!m_pDocument->GetShowFrondsLeaves());
  }
  else
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }
}

void WKrautAction::KrautEventHandler(const WKrautTreeAssetEvent& e)
{
  switch (e.m_Type)
  {
    case WKrautTreeAssetEvent::Type::WindStrengthChanged:
    case WKrautTreeAssetEvent::Type::FrondsLeavesVisibilityChanged:
      UpdateState();
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void WKrautAction::UpdateState()
{
  if (m_Type == ActionType::WindStrength)
  {
    SetCheckable(true);
    SetChecked(m_pDocument->GetWindStrength() == m_WindStrength);
  }
  else if (m_Type == ActionType::ToggleFrondsLeaves)
  {
    SetCheckable(true);
    SetChecked(m_pDocument->GetShowFrondsLeaves());
  }
  else
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }
}
