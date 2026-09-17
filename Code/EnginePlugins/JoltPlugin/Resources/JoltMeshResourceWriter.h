#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/String.h>

class WStreamWriter;

struct W_JOLTPLUGIN_DLL WJoltHeightfieldWriteDesc
{
  /// Hash of the source data. Written into the file so the export modifier can skip
  /// regeneration when the file is already up to date.
  WUInt64 uiContentHash = 0;

  /// Grid vertex counts. Must satisfy Jolt requirements: even, >= 4, and uiSizeX == uiSizeY.
  WUInt32 uiSizeX = 0;
  WUInt32 uiSizeY = 0;

  /// X/Y half-extents of the heightfield in world units.
  WVec2 vHalfExtent;

  /// Height samples, row-major with uiSizeX * uiSizeY entries.
  WArrayPtr<const float> heights;

  /// Per-quad material indices, row-major with (uiSizeX-1)*(uiSizeY-1) entries.
  /// May be empty if there are no materials.
  WArrayPtr<const WUInt8> matIndices;

  /// Surface resource paths indexed by material index values.
  WArrayPtr<const WString> surfacePaths;

  /// Jolt collision layer stored in the file and forwarded to the collider component.
  WUInt8 uiCollisionLayer = 0;
};

struct WJoltMeshDesc
{
  enum class Type : WUInt8
  {
    Triangle,
    ConvexHull,
    ConvexDecomposition,
    ConvexHullGroup,
  };

  Type m_Type = Type::Triangle;
  bool m_bFlipNormals = false;
  WUInt32 m_uiMaxConvexPieces = 1;

  /// Hash of the source data. Written uncompressed at the front of the file so the export
  /// modifier can check whether the file is up to date without decompressing it.
  WUInt64 m_uiContentHash = 0;

  WDynamicArray<WVec3> m_Vertices;
  WDynamicArray<WUInt32> m_TriangleIndices;
  WDynamicArray<WUInt16> m_TriangleSurfaceID;

  WDynamicArray<WString> m_Surfaces;
};

/// Stats for the geometry that cooking produced.
struct WJoltCookedMeshStats
{
  WUInt32 m_uiNumVertices = 0;  ///< Summed over all parts, for a decomposition or hull group.
  WUInt32 m_uiNumTriangles = 0; ///< Summed over all parts, for a decomposition or hull group.
  WUInt32 m_uiNumParts = 0;     ///< How many convex pieces were produced. 1 for a single hull, 0 for a triangle mesh.
};

/// Helper class for writing WJoltMeshResource and WJoltHeightfieldResource files.
class W_JOLTPLUGIN_DLL WJoltMeshResourceWriter
{
public:
  /// Writes the given mesh description to the provided stream so that it can be loaded as an WJoltMeshResource.
  ///
  /// Set bWriteAssetHeader to false if the asset header has already been written to the stream, e.g. in case of an asset transformation.
  static WResult WriteMeshResource(const WJoltMeshDesc& meshDesc, WStreamWriter& inout_stream, bool bWriteAssetHeader = true, WUInt64 uiAssetHash = 0, WJoltCookedMeshStats* out_pStats = nullptr);

  /// Cooks the Jolt heightfield shape and writes it to the provided stream so that it can be loaded as an WJoltHeightfieldResource.
  ///
  /// Set bWriteAssetHeader to false if the asset header has already been written to the stream.
  static WResult WriteHeightfieldResource(const WJoltHeightfieldWriteDesc& desc, WStreamWriter& inout_stream, bool bWriteAssetHeader = true, WUInt64 uiAssetHash = 0);

private:
  static WResult ComputeConvexHull(const WDynamicArray<WVec3>& vertices, WDynamicArray<WVec3>& out_hullVertices);

  static WResult CookSingleConvexJoltMesh(const WDynamicArray<WVec3>& vertices, WStreamWriter& inout_stream, WJoltCookedMeshStats& ref_stats);

  static WResult CookTriangleMesh(const WJoltMeshDesc& meshDesc, WStreamWriter& inout_stream);
  static WResult CookConvexMesh(const WJoltMeshDesc& meshDesc, WStreamWriter& inout_stream, WJoltCookedMeshStats& ref_stats);
  static WResult CookDecomposedConvexMesh(const WJoltMeshDesc& meshDesc, WStreamWriter& inout_stream, WJoltCookedMeshStats& ref_stats);
  static WResult CookConvexHullGroup(const WJoltMeshDesc& meshDesc, WStreamWriter& inout_stream, WJoltCookedMeshStats& ref_stats);
};
