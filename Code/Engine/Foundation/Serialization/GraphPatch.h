#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Utilities/EnumerableClass.h>

class WRTTI;
class WAbstractObjectNode;
class WAbstractObjectGraph;
class WGraphVersioning;
class WGraphPatchContext;

/// Base class for implementing data migration patches for object graphs.
///
/// Graph patches enable automatic data migration when type definitions change between application
/// versions. By creating static instances of derived classes, patches are automatically registered
/// and applied during deserialization.
///
/// Patch implementation pattern:
/// 1. Create a class derived from WGraphPatch
/// 2. Implement the Patch() method to transform data
/// 3. Create a static instance to auto-register the patch
/// 4. The versioning system automatically applies patches during load
///
/// Example:
/// \code
///   class MyTypePatch_1_to_2 : public WGraphPatch
///   {
///   public:
///     MyTypePatch_1_to_2() : WGraphPatch("MyType", 2) {}
///     virtual void Patch(WGraphPatchContext& ctx, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
///     {
///       // Migrate data from version 1 to version 2
///       pNode->RenameProperty("oldName", "newName");
///     }
///   };
///   static MyTypePatch_1_to_2 g_MyTypePatch_1_to_2; // Auto-registers patch
/// \endcode
class W_FOUNDATION_DLL WGraphPatch : public WEnumerable<WGraphPatch>
{
public:
  enum class PatchType : WUInt8
  {
    NodePatch,  ///< Patch applies to individual nodes of a specific type and version
    GraphPatch, ///< Patch applies to the entire graph, processes all nodes regardless of type
  };

  /// Constructs a patch for the specified type and target version.
  ///
  /// Parameters:
  /// - szType: Type name to patch (ignored for GraphPatch type)
  /// - uiTypeVersion: Target version to patch to
  /// - type: Whether this is a NodePatch (per-instance) or GraphPatch (global)
  ///
  /// Patch execution order:
  /// - Patches are applied incrementally from (uiTypeVersion-1) to uiTypeVersion
  /// - If gaps exist in patch versions, input may be older than (uiTypeVersion-1)
  /// - NodePatch: Applied once per instance of the specified type
  /// - GraphPatch: Applied once per graph, processes all relevant nodes internally
  ///
  /// For GraphPatch type, szType and uiTypeVersion are ignored during execution,
  /// and the patch implementation must determine what to process.
  WGraphPatch(const char* szType, WUInt32 uiTypeVersion, PatchType type = PatchType::NodePatch);

  /// Main patch implementation - transforms data from old version to new version.
  ///
  /// Implementation requirements:
  /// - NodePatch: Transform pNode from its current version to m_uiTypeVersion
  /// - GraphPatch: Process entire pGraph, pNode will be nullptr
  ///
  /// Available operations through context:
  /// - Property manipulation (rename, add, remove, change type)
  /// - Type renaming and hierarchy changes
  ///
  /// Important: Patches should be idempotent and handle missing properties gracefully
  /// for robustness against incomplete version chains.
  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const = 0;
  /// Returns the type to patch.
  const char* GetType() const;
  /// Returns the type version to patch to.
  WUInt32 GetTypeVersion() const;
  PatchType GetPatchType() const;

  W_DECLARE_ENUMERABLE_CLASS(WGraphPatch);

private:
  const char* m_szType = nullptr;
  WUInt32 m_uiTypeVersion;
  PatchType m_PatchType;
};
