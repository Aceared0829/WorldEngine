#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ActionManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WActionManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WActionManager::Shutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WEvent<const WActionManager::Event&> WActionManager::s_Events;
WIdTable<WActionId, WActionDescriptor*> WActionManager::s_ActionTable;
WMap<WString, WActionManager::CategoryData> WActionManager::s_CategoryPathToActions;
WMap<WString, WString> WActionManager::s_ShortcutOverride;

////////////////////////////////////////////////////////////////////////
// WActionManager public functions
////////////////////////////////////////////////////////////////////////

WActionDescriptorHandle WActionManager::RegisterAction(const WActionDescriptor& desc)
{
  WActionDescriptorHandle hType = GetActionHandle(desc.m_sCategoryPath, desc.m_sActionName);
  W_ASSERT_DEV(hType.IsInvalidated(), "The action '{0}' in category '{1}' was already registered!", desc.m_sActionName, desc.m_sCategoryPath);

  WActionDescriptor* pDesc = CreateActionDesc(desc);

  // apply shortcut override
  {
    auto ovride = s_ShortcutOverride.Find(desc.m_sActionName);
    if (ovride.IsValid())
      pDesc->m_sShortcut = ovride.Value();
  }

  hType = WActionDescriptorHandle(s_ActionTable.Insert(pDesc));
  pDesc->m_Handle = hType;

  auto it = s_CategoryPathToActions.FindOrAdd(pDesc->m_sCategoryPath);
  it.Value().m_Actions.Insert(hType);
  it.Value().m_ActionNameToHandle[pDesc->m_sActionName] = hType;

  {
    Event msg;
    msg.m_Type = Event::Type::ActionAdded;
    msg.m_pDesc = pDesc;
    msg.m_Handle = hType;
    s_Events.Broadcast(msg);
  }
  return hType;
}

bool WActionManager::UnregisterAction(WActionDescriptorHandle& ref_hAction)
{
  WActionDescriptor* pDesc = nullptr;
  if (!s_ActionTable.TryGetValue(ref_hAction, pDesc))
  {
    ref_hAction.Invalidate();
    return false;
  }

  auto it = s_CategoryPathToActions.Find(pDesc->m_sCategoryPath);
  W_ASSERT_DEV(it.IsValid(), "Action is present but not mapped in its category path!");
  W_VERIFY(it.Value().m_Actions.Remove(ref_hAction), "Action is present but not in its category data!");
  W_VERIFY(it.Value().m_ActionNameToHandle.Remove(pDesc->m_sActionName), "Action is present but its name is not in the map!");
  if (it.Value().m_Actions.IsEmpty())
  {
    s_CategoryPathToActions.Remove(it);
  }

  s_ActionTable.Remove(ref_hAction);
  DeleteActionDesc(pDesc);
  ref_hAction.Invalidate();
  return true;
}

const WActionDescriptor* WActionManager::GetActionDescriptor(WActionDescriptorHandle hAction)
{
  WActionDescriptor* pDesc = nullptr;
  if (s_ActionTable.TryGetValue(hAction, pDesc))
    return pDesc;

  return nullptr;
}

const WIdTable<WActionId, WActionDescriptor*>::ConstIterator WActionManager::GetActionIterator()
{
  return s_ActionTable.GetIterator();
}

WActionDescriptorHandle WActionManager::GetActionHandle(WStringView sCategoryPath, WStringView sActionName)
{
  WActionDescriptorHandle hAction;
  auto it = s_CategoryPathToActions.Find(sCategoryPath);
  if (!it.IsValid())
    return hAction;

  it.Value().m_ActionNameToHandle.TryGetValue(sActionName, hAction);

  return hAction;
}

WString WActionManager::FindActionCategory(WStringView sActionName)
{
  for (auto itCat : s_CategoryPathToActions)
  {
    if (itCat.Value().m_ActionNameToHandle.Contains(sActionName))
      return itCat.Key();
  }

  return WString();
}

WResult WActionManager::ExecuteAction(WStringView sCategory0, WStringView sActionName, const WActionContext& context, const WVariant& value /*= WVariant()*/)
{
  WStringBuilder sCategory = sCategory0;

  if (sCategory.IsEmpty())
  {
    sCategory = FindActionCategory(sActionName);
  }

  auto hAction = WActionManager::GetActionHandle(sCategory, sActionName);

  if (hAction.IsInvalidated())
    return W_FAILURE;

  const WActionDescriptor* pDesc = WActionManager::GetActionDescriptor(hAction);

  if (pDesc == nullptr)
    return W_FAILURE;

  WAction* pAction = pDesc->CreateAction(context);

  if (pAction == nullptr)
    return W_FAILURE;

  pAction->Execute(value);
  pDesc->DeleteAction(pAction);

  return W_SUCCESS;
}

void WActionManager::SaveShortcutAssignment()
{
  WStringBuilder sFile = WApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
  sFile.AppendPath("Settings/Shortcuts.ddl");

  W_LOG_BLOCK("LoadShortcutAssignment", sFile.GetData());

  WDeferredFileWriter file;
  file.SetOutput(sFile);

  WOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(WOpenDdlWriter::TypeStringMode::Compliant);

  WStringBuilder sKey;

  for (auto it = GetActionIterator(); it.IsValid(); ++it)
  {
    auto pAction = it.Value();

    if (pAction->m_Type != WActionType::Action)
      continue;

    if (pAction->m_sShortcut == pAction->m_sDefaultShortcut)
      sKey.Set("default: ", pAction->m_sShortcut);
    else
      sKey = pAction->m_sShortcut;

    writer.BeginPrimitiveList(WOpenDdlPrimitiveType::String, pAction->m_sActionName);
    writer.WriteString(sKey);
    writer.EndPrimitiveList();
  }

  if (file.Close().Failed())
  {
    WLog::Error("Failed to write shortcuts config file '{0}'", sFile);
  }
}

void WActionManager::LoadShortcutAssignment()
{
  WStringBuilder sFile = WApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
  sFile.AppendPath("Settings/Shortcuts.ddl");

  W_LOG_BLOCK("LoadShortcutAssignment", sFile.GetData());

  WFileReader file;
  if (file.Open(sFile).Failed())
  {
    WLog::Dev("No shortcuts file '{0}' was found", sFile);
    return;
  }

  WOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, WLog::GetThreadLocalLogSystem()).Failed())
    return;

  const auto obj = reader.GetRootElement();

  WStringBuilder sKey, sValue;

  for (auto pElement = obj->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (!pElement->HasName() || !pElement->HasPrimitives(WOpenDdlPrimitiveType::String))
      continue;

    sKey = pElement->GetName();
    sValue = pElement->GetPrimitivesString()[0];

    if (sValue.FindSubString_NoCase("default") != nullptr)
      continue;

    s_ShortcutOverride[sKey] = sValue;
  }

  // apply overrides
  for (auto it = GetActionIterator(); it.IsValid(); ++it)
  {
    auto pAction = it.Value();

    if (pAction->m_Type != WActionType::Action)
      continue;

    auto ovride = s_ShortcutOverride.Find(pAction->m_sActionName);
    if (ovride.IsValid())
      pAction->m_sShortcut = ovride.Value();
  }
}

////////////////////////////////////////////////////////////////////////
// WActionManager private functions
////////////////////////////////////////////////////////////////////////

void WActionManager::Startup()
{
  WDocumentActions::RegisterActions();
  WStandardMenus::RegisterActions();
  WCommandHistoryActions::RegisterActions();
  WEditActions::RegisterActions();
}

void WActionManager::Shutdown()
{
  WDocumentActions::UnregisterActions();
  WStandardMenus::UnregisterActions();
  WCommandHistoryActions::UnregisterActions();
  WEditActions::UnregisterActions();

  W_ASSERT_DEV(s_ActionTable.IsEmpty(), "Some actions were registered but not unregistred.");
  W_ASSERT_DEV(s_CategoryPathToActions.IsEmpty(), "Some actions were registered but not unregistred.");

  s_ActionTable.Clear();
  s_CategoryPathToActions.Clear();
  s_ShortcutOverride.Clear();
}

WActionDescriptor* WActionManager::CreateActionDesc(const WActionDescriptor& desc)
{
  WActionDescriptor* pDesc = W_DEFAULT_NEW(WActionDescriptor);
  *pDesc = desc;
  return pDesc;
}

void WActionManager::DeleteActionDesc(WActionDescriptor* pDesc)
{
  W_DEFAULT_DELETE(pDesc);
}
