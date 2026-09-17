#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/ActionMap.h>

/// A central place for creating and retrieving action maps.
class W_GUIFOUNDATION_DLL WActionMapManager
{
public:
  /// Adds a new action map with the given name and an optional inherited parent mapping. Asserts if the name was already used before.
  /// \param sMapping Name of the new mapping. This string has to be used when adding actions to the map.
  /// \param sParentMapping If set, the new mapping will inherit all actions of this mapping. The name must exist and resolve to a valid mapping once WActionMap::BuildActionTree is called to generate the action tree.
  static void RegisterActionMap(WStringView sActionMapName, WStringView sParentActionMapName = {});

  /// Deletes the action map with the given name. Asserts, if no such map exists.
  static void UnregisterActionMap(WStringView sActionMapName);

  /// Returns the action map with the given name, or nullptr, if it doesn't exist.
  static WActionMap* GetActionMap(WStringView sActionMapName);

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionMapManager);

  static void Startup();
  static void Shutdown();

private:
  static WMap<WString, WActionMap*> s_Mappings;
};
