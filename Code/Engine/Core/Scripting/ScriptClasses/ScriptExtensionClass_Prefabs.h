#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class WWorld;
class WGameObject;

/// Script extension class providing prefab instantiation functionality for scripts.
class W_CORE_DLL WScriptExtensionClass_Prefabs
{
public:
  /// Spawns a prefab instance at the specified global transform.
  ///
  /// \param sPrefab Path or name of the prefab to spawn
  /// \param globalTransform World position, rotation and scale for the prefab
  /// \param uiUniqueID Unique identifier for deterministic spawning, use 0 for random
  /// \param bSetCreatedByPrefab Whether to mark spawned objects as created by prefab
  /// \param bSetHideShapeIcon Whether to hide shape icons in the editor for spawned objects
  /// \return Array of game object handles for the spawned prefab's top-level objects
  static WVariantArray SpawnPrefab(WWorld* pWorld, WStringView sPrefab, const WTransform& globalTransform, WUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon);

  /// Spawns a prefab instance as a child of the specified parent object.
  ///
  /// \param sPrefab Path or name of the prefab to spawn
  /// \param pParent Parent game object for the spawned prefab
  /// \param localTransform Local transform relative to the parent
  /// \param uiUniqueID Unique identifier for deterministic spawning, use 0 for random
  /// \param bSetCreatedByPrefab Whether to mark spawned objects as created by prefab
  /// \param bSetHideShapeIcon Whether to hide shape icons in the editor for spawned objects
  /// \return Array of game object handles for the spawned prefab's top-level objects
  static WVariantArray SpawnPrefabAsChild(WWorld* pWorld, WStringView sPrefab, WGameObject* pParent, const WTransform& localTransform, WUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon);
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WScriptExtensionClass_Prefabs);
