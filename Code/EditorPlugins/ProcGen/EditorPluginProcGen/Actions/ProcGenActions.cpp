#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/Actions/ProcGenActions.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <GuiFoundation/Action/ActionManager.h>

WActionDescriptorHandle WProcGenActions::s_hCategory;
WActionDescriptorHandle WProcGenActions::s_hDumpAST;
WActionDescriptorHandle WProcGenActions::s_hDumpDisassembly;

void WProcGenActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("ProcGen");
  s_hDumpAST = W_REGISTER_ACTION_1("ProcGen.DumpAST", WActionScope::Document, "ProcGen Graph", "", WProcGenAction, WProcGenAction::ActionType::DumpAST);
  s_hDumpDisassembly = W_REGISTER_ACTION_1("ProcGen.DumpDisassembly", WActionScope::Document, "ProcGen Graph", "", WProcGenAction, WProcGenAction::ActionType::DumpDisassembly);
}

void WProcGenActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hDumpAST);
  WActionManager::UnregisterAction(s_hDumpDisassembly);
}

void WProcGenActions::MapMenuActions()
{
  WActionMap* pMap = WActionMapManager::GetActionMap("ProcGenAssetMenuBar");
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategory, "G.Tools.Document", 10.0f);
  pMap->MapAction(s_hDumpAST, "G.Tools.Document", "ProcGen", 1.0f);
  pMap->MapAction(s_hDumpDisassembly, "G.Tools.Document", "ProcGen", 2.0f);

  pMap = WActionMapManager::GetActionMap("ProcGenAssetToolBar");
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategory, "", 10.0f);
  pMap->MapAction(s_hDumpAST, "ProcGen", 1.0f);
  pMap->MapAction(s_hDumpDisassembly, "ProcGen", 2.0f);
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGenAction, 0, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WProcGenAction::WProcGenAction(const WActionContext& context, const char* szName, ActionType type)
  : WButtonAction(context, szName, false, "")
  , m_Type(type)
{
}

WProcGenAction::~WProcGenAction() = default;

void WProcGenAction::Execute(const WVariant& value)
{
  if (auto pAssetDocument = WDynamicCast<WProcGenGraphAssetDocument*>(GetContext().m_pDocument))
  {
    pAssetDocument->DumpSelectedOutput(m_Type == ActionType::DumpAST, m_Type == ActionType::DumpDisassembly);
  }
}
