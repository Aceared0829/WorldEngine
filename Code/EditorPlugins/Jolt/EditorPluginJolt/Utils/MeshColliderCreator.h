#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

/// Which kind of collision mesh asset to generate from a mesh asset.
struct WMeshColliderKind
{
  using StorageType = WUInt8;

  enum Enum
  {
    /// WJoltConvexCollisionMeshAsset. Required for dynamic actors.
    ConvexHull,

    /// WJoltCollisionMeshAsset. Concave, but only usable for static geometry.
    TriangleMesh,

    Default = ConvexHull
  };
};

/// \see WMeshColliderCreator::CreateMeshCollider()
struct WMeshColliderOptions
{
  /// Where to write the asset. Absolute, or relative to the parent of a data directory
  /// ("Testing Chambers/Objects/Barrel.WJoltCollisionMeshAsset"). Empty means the suggested path,
  /// which is what creating colliders for several meshes at once uses.
  /// \see WMeshColliderCreator::SuggestColliderPath()
  WString m_sColliderPath;

  WEnum<WMeshColliderKind> m_Kind;

  /// The surface asset to assign, as a guid or an asset path. Only convex meshes have a single
  /// surface; a triangle mesh gets one per material slot at transform time, so this is ignored there.
  WString m_sSurface;

  /// Overwrites a collision mesh asset that already exists instead of refusing.
  ///
  /// The contents are rewritten in place, so the asset keeps its guid and references to it still
  /// resolve. Hand-tuned values on it are lost.
  bool m_bOverwriteExisting = false;

  bool m_bOpenAfterCreate = false;
};

/// The import settings of a mesh asset, as far as a collision mesh asset can reproduce them.
///
/// Every value is stored as a variant that is written to the collision mesh asset property of the
/// same name, so that neither side's C++ type has to be known here. Values that the mesh asset does
/// not have (an animated mesh has no transform options) are invalid variants and are then left at
/// the collision mesh asset's own default.
struct W_EDITORPLUGINJOLT_DLL WMeshColliderSource
{
  WUuid m_MeshAssetGuid;
  WString m_sMeshAssetPath;

  /// The mesh asset's "MeshFile". Empty if it could not be read or the mesh is a primitive rather
  /// than an imported file, in which case no collision mesh can be generated from it.
  WString m_sMeshFile;

  /// True for a primitive mesh (WMeshPrimitive other than File), which has no source file to import.
  /// Kept separate from an empty m_sMeshFile so that the reason can be reported.
  bool m_bIsPrimitive = false;

  bool m_bAnimated = false;

  /// The import options shared with the collision mesh asset, keyed by property name.
  WVariantDictionary m_ImportProperties;

  /// An existing collision mesh asset of that kind built from the same source file, if there is one.
  WUuid m_ExistingTriangleColMesh;
  WUuid m_ExistingConvexColMesh;

  /// The existing collision mesh asset of the given kind, or an invalid uuid.
  WUuid GetExisting(WEnum<WMeshColliderKind> kind) const;
};

/// Creates Jolt collision mesh assets from mesh assets. \see WMeshColliderUtils
///
/// Only settings that both asset types have are transferred, everything else stays at the collision
/// mesh asset's default.
class W_EDITORPLUGINJOLT_DLL WMeshColliderCreator
{
public:
  /// Fails if the guid does not belong to a mesh asset.
  ///
  /// Opens the mesh asset document to read its properties, if it is not open already.
  static WResult GatherMeshColliderSource(const WUuid& meshAssetGuid, WMeshColliderSource& out_source);

  /// Creates and saves the collision mesh asset document.
  ///
  /// Fails if a file already exists at the target path, unless WMeshColliderOptions::m_bOverwriteExisting
  /// is set. Reusing an existing collider is otherwise up to the caller.
  /// \see WMeshColliderSource::GetExisting()
  static WStatus CreateMeshCollider(const WMeshColliderSource& source, const WMeshColliderOptions& options);

  /// Creates a collider for each of the given mesh assets, each at its suggested path.
  ///
  /// Meshes that already have a collider at that path, and ones no collider can be built from, are
  /// skipped with a log message rather than failing the whole run. Only an outright error, such as a
  /// document that cannot be written, is reported back.
  static WStatus CreateMeshColliders(WArrayPtr<const WUuid> meshAssetGuids, const WMeshColliderOptions& options, WUInt32& out_uiCreated, WUInt32& out_uiSkipped);

  /// Whether the given guid refers to a mesh or animated mesh asset.
  static bool IsMeshAsset(const WUuid& assetGuid);

  /// The default absolute path for a collider of that kind, next to the mesh asset.
  ///
  /// Appends a number if that file is already taken, unless bAllowExisting is set.
  static WString SuggestColliderPath(const WMeshColliderSource& source, WEnum<WMeshColliderKind> kind, bool bAllowExisting = false);

  /// Turns an absolute path into one relative to the parent of its data directory, for display.
  /// Returns the input unchanged if it is not inside a data directory.
  static WString MakeDisplayPath(WStringView sAbsolutePath);

  /// Resolves what MakeDisplayPath() produced, or any absolute path, back to an absolute path.
  /// Fails if the path names no known data directory.
  static WResult ResolveDisplayPath(WStringView sPath, WStringBuilder& out_sAbsolutePath);
};
