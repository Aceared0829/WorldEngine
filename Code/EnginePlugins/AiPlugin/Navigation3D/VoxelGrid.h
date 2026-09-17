#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Vec3.h>

class WDebugRendererContext;

/// Stores a 3D voxel grid using packed 4x4x4 bit blocks.
///
/// Each block is stored as a single WUInt64 (64 bits = 4*4*4 voxels).
/// A set bit means the voxel is occupied (solid). A cleared bit means the voxel is free (passable).
///
/// The grid has a fixed resolution and is centered at a configurable world position.
/// Use WorldToCoord / CoordToWorld to convert between world space and voxel coordinates.
/// Use InjectBox / InjectSphere to mark voxels as occupied based on world-space shapes.
class W_AIPLUGIN_DLL WVoxelGrid
{
public:
  WVoxelGrid();
  ~WVoxelGrid();

  /// Initializes the grid with the given resolution (in voxels).
  ///
  /// Each dimension is rounded up to a multiple of 4 internally.
  /// The grid is cleared to all-free after init.
  void Initialize(const WVec3U32& vDimensions, const WVec3& vCenter, float fVoxelSize);

  /// Clears all voxel data (sets everything to free). Keeps allocated memory.
  void ClearData();

  /// Converts a world-space position to integer voxel coordinates.
  W_FORCE_INLINE WVec3I32 WorldToCoord(const WVec3& vWorldPos) const
  {
    const WVec3 vLocal = (vWorldPos - m_vOrigin) * m_fInvVoxelSize;
    return {WMath::FloorToInt(vLocal.x), WMath::FloorToInt(vLocal.y), WMath::FloorToInt(vLocal.z)};
  }

  /// Converts integer voxel coordinates to the world-space center of that voxel.
  W_FORCE_INLINE WVec3 CoordToWorld(const WVec3I32& vCoord) const
  {
    return m_vOrigin + WVec3((vCoord.x + 0.5f) * m_fVoxelSize, (vCoord.y + 0.5f) * m_fVoxelSize, (vCoord.z + 0.5f) * m_fVoxelSize);
  }

  /// Returns true if the coordinate is inside the grid bounds.
  W_FORCE_INLINE bool IsCoordValid(const WVec3I32& vCoord) const
  {
    return vCoord.x >= 0 && vCoord.y >= 0 && vCoord.z >= 0 &&
           (WUInt32)vCoord.x < m_vDimensions.x &&
           (WUInt32)vCoord.y < m_vDimensions.y &&
           (WUInt32)vCoord.z < m_vDimensions.z;
  }

  /// Sets a voxel to occupied (true) or free (false).
  void SetVoxel(const WVec3I32& vCoord, bool bSolid);

  /// Returns whether the voxel at the given coordinate is occupied.
  bool IsVoxelSet(const WVec3I32& vCoord) const;

  /// Rasterizes a world-space triangle into the grid, setting (or clearing) every voxel it overlaps.
  ///
  /// Cost scales with the triangle's voxel-space bounding box, so rasterizing geometry that is large
  /// relative to the voxel size is expensive. Parts of the triangle outside the grid are ignored.
  void SetVoxelsOnTriangle(const WVec3& v0, const WVec3& v1, const WVec3& v2, bool bSet = true);

  /// Ray-march visibility test between two grid-space coordinates.
  ///
  /// Returns true if no occupied voxel blocks the line of sight.
  bool CheckLineOfSight(const WVec3I32& vStart, const WVec3I32& vGoal) const;

  /// Returns the world-space AABB of the entire grid.
  WBoundingBox GetAABB() const;

  /// Returns the approximate memory usage in bytes.
  WUInt64 GetHeapMemoryUsage() const;

  /// Draws a debug visualization of occupied surface voxels.
  void DebugDraw(const WDebugRendererContext& context, const WColor& color) const;

  WVec3U32 GetDimensions() const { return m_vDimensions; }
  float GetVoxelSize() const { return m_fVoxelSize; }
  const WVec3& GetOrigin() const { return m_vOrigin; }
  const WVec3& GetCenter() const { return m_vCenter; }

private:
  W_FORCE_INLINE WUInt32 GetBlockIndex(WUInt32 uiBlockX, WUInt32 uiBlockY, WUInt32 uiBlockZ) const
  {
    return uiBlockZ * (m_vNumBlocks.x * m_vNumBlocks.y) + uiBlockY * m_vNumBlocks.x + uiBlockX;
  }

  W_FORCE_INLINE static WUInt32 GetBitIndex(WUInt32 uiLocalX, WUInt32 uiLocalY, WUInt32 uiLocalZ)
  {
    return uiLocalZ * 16u + uiLocalY * 4u + uiLocalX;
  }

  WVec3U32 m_vDimensions;
  WVec3U32 m_vNumBlocks;

  float m_fVoxelSize = 1.0f;
  float m_fInvVoxelSize = 1.0f;
  WVec3 m_vCenter = WVec3::MakeZero();
  WVec3 m_vOrigin = WVec3::MakeZero();

  WDynamicArray<WUInt64> m_Blocks;
};
