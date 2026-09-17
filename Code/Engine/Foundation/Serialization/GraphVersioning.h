#pragma once

/// \file

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Strings/HashedString.h>

class WRTTI;
class WAbstractObjectNode;
class WAbstractObjectGraph;
class WGraphPatch;
class WGraphPatchContext;
class WGraphVersioning;

/// Identifier for graph patches combining type name and version number.
///
/// This structure uniquely identifies which patch should be applied to which type version.
/// The versioning system uses this to track patch progression and avoid duplicate applications.
struct WVersionKey
{
  WVersionKey() = default;
  WVersionKey(WStringView sType, WUInt32 uiTypeVersion)
  {
    m_sType.Assign(sType);
    m_uiTypeVersion = uiTypeVersion;
  }
  W_DECLARE_POD_TYPE();
  WHashedString m_sType;
  WUInt32 m_uiTypeVersion;
};

/// Hash helper class for WVersionKey
struct WGraphVersioningHash
{
  W_FORCE_INLINE static WUInt32 Hash(const WVersionKey& a)
  {
    auto typeNameHash = a.m_sType.GetHash();
    WUInt32 uiHash = WHashingUtils::xxHash32(&typeNameHash, sizeof(typeNameHash));
    uiHash = WHashingUtils::xxHash32(&a.m_uiTypeVersion, sizeof(a.m_uiTypeVersion), uiHash);
    return uiHash;
  }

  W_ALWAYS_INLINE static bool Equal(const WVersionKey& a, const WVersionKey& b)
  {
    return a.m_sType == b.m_sType && a.m_uiTypeVersion == b.m_uiTypeVersion;
  }
};

/// Stores type version information required for graph patching operations.
///
/// This structure contains the metadata needed to apply version patches, including
/// type names and version numbers for both the type and its parent class hierarchy.
/// It overlaps with WReflectedTypeDescriptor to enable efficient patch processing.
struct W_FOUNDATION_DLL WTypeVersionInfo
{
  const char* GetTypeName() const;
  void SetTypeName(const char* szName);
  const char* GetParentTypeName() const;
  void SetParentTypeName(const char* szName);

  WHashedString m_sTypeName;
  WHashedString m_sParentTypeName;
  WUInt32 m_uiTypeVersion;
};
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WTypeVersionInfo);

/// Context object that manages the patching process for individual nodes.
///
/// This class is passed to patch implementations to provide utility functions and track
/// the patching progress of a node. It handles base class patching, type renaming, and
/// hierarchy changes while maintaining consistency across the entire patching process.
class W_FOUNDATION_DLL WGraphPatchContext
{
public:
  /// Ensures a base class is patched to the specified version before continuing.
  ///
  /// This function forces the base class to be at the specified version, applying patches if necessary.
  /// Use bForcePatch for backwards compatibility when base class type information wasn't originally
  /// serialized. This ensures proper patch ordering in inheritance hierarchies.
  void PatchBaseClass(const char* szType, WUInt32 uiTypeVersion, bool bForcePatch = false); // [tested]

  /// Renames the current node's type to a new type name.
  ///
  /// Used when types are renamed or moved to different namespaces. The version number
  /// is preserved unless explicitly changed with the overload that takes a version parameter.
  void RenameClass(const char* szTypeName); // [tested]

  /// Renames the current node's type and sets a new version number.
  ///
  /// Use this when both the type name and version change during a patch operation.
  /// This is common when types are refactored or split into multiple classes.
  void RenameClass(const char* szTypeName, WUInt32 uiVersion);

  /// Replaces the entire base class hierarchy with a new one.
  ///
  /// This is used for major refactoring where the inheritance structure changes.
  /// The array should contain the complete new inheritance chain from most derived
  /// to most base class. Handle with care as this affects serialization compatibility.
  void ChangeBaseClass(WArrayPtr<WVersionKey> baseClasses); // [tested]

private:
  friend class WGraphVersioning;
  WGraphPatchContext(WGraphVersioning* pParent, WAbstractObjectGraph* pGraph, WAbstractObjectGraph* pTypesGraph);
  void Patch(WAbstractObjectNode* pNode);
  void Patch(WUInt32 uiBaseClassIndex, WUInt32 uiTypeVersion, bool bForcePatch);
  void UpdateBaseClasses();

private:
  WGraphVersioning* m_pParent = nullptr;
  WAbstractObjectGraph* m_pGraph = nullptr;
  WAbstractObjectNode* m_pNode = nullptr;
  WDynamicArray<WVersionKey> m_BaseClasses;
  WUInt32 m_uiBaseClassIndex = 0;
  mutable WHashTable<WHashedString, WTypeVersionInfo> m_TypeToInfo;
};

/// Singleton system that manages version patching for WAbstractObjectGraph instances.
///
/// This system automatically applies version patches during deserialization to handle data migration
/// when type definitions change between versions. It supports both node-level patches (specific type
/// transformations) and graph-level patches (global transformations affecting multiple types).
///
/// The system automatically executes during WAbstractObjectGraph deserialization,
/// ensuring that older serialized data can be loaded into newer application versions.
class W_FOUNDATION_DLL WGraphVersioning
{
  W_DECLARE_SINGLETON(WGraphVersioning);

public:
  WGraphVersioning();
  ~WGraphVersioning();

  /// Applies all necessary patches to bring the graph to the current version.
  ///
  /// This is the main entry point for graph patching. It processes all nodes in the graph,
  /// applying patches in dependency order to ensure consistency.
  ///
  /// \param pGraph The object graph to patch (modified in-place)
  /// \param pTypesGraph Optional type information graph from serialization time.
  ///        Contains the exact type versions that were serialized. If not provided,
  ///        base classes are assumed to be at their maximum patchable version.
  ///
  /// The patching process:
  /// 1. Discovers all required patches for each node type
  /// 2. Sorts patches by dependency order (base classes first)
  /// 3. Applies patches incrementally until all nodes reach current versions
  /// 4. Validates that no circular dependencies exist
  void PatchGraph(WAbstractObjectGraph* pGraph, WAbstractObjectGraph* pTypesGraph = nullptr);

private:
  friend class WGraphPatchContext;

  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, GraphVersioning);

  void PluginEventHandler(const WPluginEvent& EventData);
  void UpdatePatches();
  WUInt32 GetMaxPatchVersion(const WHashedString& sType) const;

  WHashTable<WHashedString, WUInt32> m_MaxPatchVersion; ///< Max version the given type can be patched to.
  WDynamicArray<const WGraphPatch*> m_GraphPatches;
  WHashTable<WVersionKey, const WGraphPatch*, WGraphVersioningHash> m_NodePatches;
};
