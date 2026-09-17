#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>

WMap<WString, WActionMap*> WActionMapManager::s_Mappings;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ActionMapManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ActionManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WActionMapManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WActionMapManager::Shutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// WActionMapManager public functions
////////////////////////////////////////////////////////////////////////

void WActionMapManager::RegisterActionMap(WStringView sActionMapName, WStringView sParentActionMapName)
{
  auto it = s_Mappings.Find(sActionMapName);
  W_ASSERT_ALWAYS(!it.IsValid(), "Mapping '{}' already exists", sActionMapName);
  s_Mappings.Insert(sActionMapName, W_DEFAULT_NEW(WActionMap, sParentActionMapName));
}

void WActionMapManager::UnregisterActionMap(WStringView sActionMapName)
{
  auto it = s_Mappings.Find(sActionMapName);
  W_ASSERT_ALWAYS(it.IsValid(), "Mapping '{}' not found", sActionMapName);
  W_DEFAULT_DELETE(it.Value());
  s_Mappings.Remove(it);
}

WActionMap* WActionMapManager::GetActionMap(WStringView sActionMapName)
{
  auto it = s_Mappings.Find(sActionMapName);
  if (!it.IsValid())
    return nullptr;

  return it.Value();
}


////////////////////////////////////////////////////////////////////////
// WActionMapManager private functions
////////////////////////////////////////////////////////////////////////

void WActionMapManager::Startup()
{
  WActionMapManager::RegisterActionMap("DocumentWindowTabMenu");
  WDocumentActions::MapMenuActions("DocumentWindowTabMenu", "");
}

void WActionMapManager::Shutdown()
{
  WActionMapManager::UnregisterActionMap("DocumentWindowTabMenu");

  while (!s_Mappings.IsEmpty())
  {
    UnregisterActionMap(s_Mappings.GetIterator().Key());
  }
}
