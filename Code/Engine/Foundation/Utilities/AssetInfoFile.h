#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/AssetFileHeader.h>

/// Key/value pairs describing the result of an asset transform, e.g. the vertex count of a mesh or the format of a texture.
///
/// Written as OpenDDL text next to the transformed output in the AssetCache folder. The file records the hash and type
/// version of the output it belongs to, so that a stale file can be detected like a stale output.
///
/// Most assets record nothing and therefore have no such file at all, so a missing file is not an error.
///
/// Values are untyped, because the file is written by several tools (editor, editor processor, TexConv) and consumed
/// generically. Use the key names below where they apply.
class W_FOUNDATION_DLL WAssetInfoFile
{
public:
  /// Key names used across multiple asset types. Asset types may add arbitrary further keys.
  ///
  /// Must be valid DDL identifiers, because that is what they become in the file. For display they are run through
  /// WTranslate, which splits CamelCase into words, so "ConvexParts" shows up as "Convex Parts".
  struct Keys
  {
    static constexpr WStringView NumVertices = "Vertices"_wsv;            ///< WUInt32
    static constexpr WStringView NumTriangles = "Triangles"_wsv;          ///< WUInt32
    static constexpr WStringView NumSubMeshes = "Meshes"_wsv;             ///< WUInt32
    static constexpr WStringView NumSurfaces = "Surfaces"_wsv;            ///< WUInt32
    static constexpr WStringView NumBones = "Bones"_wsv;                  ///< WUInt32
    static constexpr WStringView BoundsCenter = "Center"_wsv;             ///< WVec3, shown together with Extents as the resulting range
    static constexpr WStringView BoundsHalfExtents = "Extents"_wsv;       ///< WVec3, so the full size is twice this
    static constexpr WStringView BoundsRadius = "Radius"_wsv;             ///< float
    static constexpr WStringView ImageWidth = "Width"_wsv;                ///< WUInt32, shown together with Height as the resolution
    static constexpr WStringView ImageHeight = "Height"_wsv;              ///< WUInt32
    static constexpr WStringView Format = "Format"_wsv;                   ///< WString
    static constexpr WStringView CollisionMeshType = "CollisionMesh"_wsv; ///< WString, e.g. "Triangle" or "ConvexHull"
    static constexpr WStringView NumConvexParts = "ConvexParts"_wsv;      ///< WUInt32, only recorded when there is more than one
    static constexpr WStringView AvailableClips = "Clips"_wsv;            ///< WVariantArray of WString: animation clip names in the source file
    static constexpr WStringView AvailableMeshes = "MeshesInSource"_wsv;  ///< WVariantArray of WString: mesh names in the source file
  };

  /// Adds or overwrites a value. An invalid value removes the key.
  void SetValue(WStringView sKey, const WVariant& value);

  /// Returns an invalid variant if the key does not exist.
  WVariant GetValue(WStringView sKey) const;

  bool IsEmpty() const { return m_Values.IsEmpty(); }
  void Clear() { m_Values.Clear(); }

  const WMap<WString, WVariant>& GetValues() const { return m_Values; }

  /// Writes the values and the header as OpenDDL.
  ///
  /// Always writes, even when the map is empty. Prefer WriteToFile(), which skips empty maps.
  WResult Write(WStreamWriter& inout_stream, const WAssetFileHeader& header) const;

  /// Discards previous content. Values whose type this build cannot represent are skipped individually.
  WResult Read(WStreamReader& inout_stream, WAssetFileHeader& out_header);

  /// Writes the file, or deletes any existing one if there is nothing to write.
  WResult WriteToFile(WStringView sAbsolutePath, const WAssetFileHeader& header) const;

  /// Fails if the file does not exist, or if it was written for a different hash or type version, ie. if it is stale.
  WResult ReadFromFile(WStringView sAbsolutePath, WUInt64 uiExpectedHash, WUInt16 uiExpectedTypeVersion);

  /// Returns the path of the info file that belongs to the given transform output.
  static WStringBuilder GetInfoFilePathForOutput(WStringView sAbsoluteOutputPath);

  /// Appends all values, one "Name: value" per line, for display in the UI.
  ///
  /// Nothing is appended when there are no values, not even a separator. This shows everything, which is a lot for some
  /// asset types; for a short summary use WAssetDocumentManager::AppendAssetInfoSummary() instead.
  void AppendToDisplayString(WStringBuilder& ref_sOut, WStringView sLinePrefix = "\n"_wsv) const;

  /// Appends only the given keys, in the given order. Keys that have no value are skipped.
  void AppendValuesToDisplayString(WStringBuilder& ref_sOut, WArrayPtr<const WStringView> keys, WStringView sLinePrefix = "\n"_wsv) const;

  /// Appends a single value. Returns false if there is nothing to show for that key.
  ///
  /// Some keys are formatted together with another one, e.g. ImageWidth prints the full resolution. The absorbed key
  /// (here ImageHeight) returns false on its own, so iterating over all keys does not print it twice.
  bool AppendValueToDisplayString(WStringBuilder& ref_sOut, WStringView sKey, WStringView sLinePrefix = "\n"_wsv) const;

private:
  // A map, not a hash table, so that the written files don't change just because the insertion order did.
  WMap<WString, WVariant> m_Values;
};
