#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

/// Which kind of collision mesh asset to build from a mesh asset.
///
/// The values name Jolt document types, but nothing here links against the Jolt plugin: the asset is
/// built by document type name and by writing properties by name.
struct WCollisionMeshKind
{
  using StorageType = WUInt8;

  enum Enum
  {
    /// WJoltCollisionMeshAsset. Concave, but only usable for static geometry.
    TriangleMesh,

    /// WJoltConvexCollisionMeshAsset. Required for dynamic actors.
    ConvexHull,

    Default = TriangleMesh
  };
};

/// Builds Jolt collision mesh assets out of mesh assets.
///
/// Shared by the "Create Collider" dialog and by prefab creation, so that both build the collision
/// mesh the same way.
///
/// The Jolt plugin does not have to be loaded to find or read collision mesh assets, but it does have
/// to be loaded to create one, as it registers the document type.
struct W_EDITORPLUGINASSETS_DLL WMeshColliderUtils
{
  /// The document types that a mesh asset can have.
  static constexpr WStringView s_sMeshDocType = "Mesh"_wsv;
  static constexpr WStringView s_sAnimatedMeshDocType = "Animated Mesh"_wsv;

  /// Whether the given guid refers to a mesh or animated mesh asset.
  static bool IsMeshAsset(const WUuid& assetGuid);

  /// The mesh asset properties that a collision mesh asset has as well, under the same name.
  ///
  /// Pass these to ReadMeshProperties(). An animated mesh asset only has some of them; the rest come
  /// back as invalid variants and are then not written.
  static WArrayPtr<const WStringView> GetImportPropertyNames();

  /// Written on top of GetImportPropertyNames(), but only to a triangle mesh: a convex hull is built
  /// from the hull of the vertices, so simplifying the source geometry first changes nothing about it.
  static WArrayPtr<const WStringView> GetSimplificationPropertyNames();

  /// The subset of GetImportPropertyNames() that selects which part of the model file is used.
  /// FindExisting() needs at least these in the dictionary it is given.
  static WArrayPtr<const WStringView> GetSubMeshPropertyNames();

  /// The document type name of a collision mesh asset of that kind, e.g. for FindExisting().
  static WStringView GetDocumentType(WEnum<WCollisionMeshKind> kind);

  /// The file extension of a collision mesh asset of that kind, without a dot.
  static WStringView GetExtension(WEnum<WCollisionMeshKind> kind);

  /// Finds the collision mesh asset of that kind that belongs to a specific mesh asset, wherever it
  /// sits in the project. A collider inside a mesh's "_data" folder is found just as one next to it.
  ///
  /// Candidates come from the transform dependencies the curator already holds. The model file alone
  /// is not enough to identify one: several mesh assets commonly import different sub-meshes out of
  /// one model file, and their colliders then all depend on that same file. Those candidates are told
  /// apart by which sub-mesh they select, and only then by their file name.
  ///
  /// \param meshImportProperties
  ///   The mesh asset's properties, as read by ReadMeshProperties(). A candidate that selects a
  ///   different sub-mesh is not a match at all, so passing an empty dictionary here means any
  ///   collider built from the model file is accepted - which is only correct if the model file is
  ///   known to hold a single mesh.
  /// \param sMeshAssetPath
  ///   Used to prefer a collider whose file name matches the mesh asset's, for the case that several
  ///   colliders select the same sub-mesh. May be empty.
  ///
  /// Returns an invalid uuid if there is no match, or if sMeshFile is empty.
  static WUuid FindExisting(WEnum<WCollisionMeshKind> kind, WStringView sMeshFile, const WVariantDictionary& meshImportProperties, WStringView sMeshAssetPath = {});

  /// Reads the given properties from a mesh asset document, for passing to CreateCollisionMesh().
  ///
  /// Opens the document if it is not open already, and closes it again unless someone else was
  /// working with it. Properties the document does not have are not added to the dictionary.
  static WResult ReadMeshProperties(WStringView sAbsMeshAssetPath, WArrayPtr<const WStringView> properties, WVariantDictionary& out_values);

  /// Creates or rewrites the collision mesh asset at that path and saves it.
  ///
  /// importProperties is what ReadMeshProperties() returned; only the entries that this kind of
  /// collision mesh actually has are written.
  ///
  /// sSurface is only used for a convex mesh, which has a single surface. A triangle mesh gets one
  /// per material slot when it is transformed, so it is ignored there.
  ///
  /// Fails if the file exists and bOverwriteExisting is not set. An existing asset is rewritten in
  /// place, so it keeps its guid and references to it still resolve.
  ///
  /// The document is closed again afterwards; opening it is left to the caller.
  static WStatus CreateCollisionMesh(WStringView sAbsColliderPath, WEnum<WCollisionMeshKind> kind, const WVariantDictionary& importProperties, WStringView sSurface, bool bOverwriteExisting, WUuid& out_guid);
};
