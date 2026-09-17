#pragma once

#include <AiPlugin/Navigation/NavigationConfig.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Recast.h>
#include <RendererCore/Debug/DebugRendererContext.h>

using WDataBuffer = WDynamicArray<WUInt8>;

class WNavmeshGeoWorldModuleInterface;
class dtNavMesh;

/// Stores indices for a triangle.
struct WAiNavMeshTriangle final
{
  W_DECLARE_POD_TYPE();

  WAiNavMeshTriangle() = default;
  WAiNavMeshTriangle(WInt32 a, WInt32 b, WInt32 c)
  {
    m_VertexIdx[0] = a;
    m_VertexIdx[1] = b;
    m_VertexIdx[2] = c;
  }

  WInt32 m_VertexIdx[3];
};

/// Stores the geometry from which a navmesh should be generated.
struct WAiNavMeshInputGeo final
{
  WDynamicArray<WVec3> m_Vertices;
  WDynamicArray<WAiNavMeshTriangle> m_Triangles;
  WDynamicArray<WUInt8> m_TriangleAreaIDs;
};

/// State about a single sector (tile / cell) of an WAiNavMesh
struct WAiNavMeshSector final
{
  WAiNavMeshSector();
  ~WAiNavMeshSector();

  WUInt8 m_FlagRequested : 1;
  WUInt8 m_FlagInvalidate : 1;
  WUInt8 m_FlagUpdateAvailable : 1;
  WUInt8 m_FlagUsable : 1;

  WDataBuffer m_NavmeshDataCur;
  WDataBuffer m_NavmeshDataNew;
  dtTileRef m_TileRef = 0;
};

/// A navmesh generated with a specific configuration.
///
/// Each game may use multiple navmeshes for different character types (large, small, etc).
/// All navmeshes always exist, but only some may contain data.
/// You get access to a navmesh through the WAiNavMeshWorldModule.
///
/// To do a path search, use WAiNavigation.
/// Since the navmesh is built in the background, a path search may need to run for multiple frames,
/// before it can return any result.
class W_AIPLUGIN_DLL WAiNavMesh final
{
  W_DISALLOW_COPY_AND_ASSIGN(WAiNavMesh);

public:
  WAiNavMesh(const WAiNavmeshConfig& navmeshConfig);
  ~WAiNavMesh();

  using SectorID = WUInt32;

  WVec2I32 CalculateSectorCoord(float fPositionX, float fPositionY) const;
  WVec2 GetSectorPositionOffset(WVec2I32 vCoord) const;
  WBoundingBox GetSectorBounds(WVec2I32 vCoord, float fMinZ = 0.0f, float fMaxZ = 1.0f) const;
  float GetSectorSize() const { return m_fSectorMetersXY; }
  SectorID CalculateSectorID(WVec2I32 vCoord) const { return vCoord.y * m_uiNumSectorsX + vCoord.x; }
  WVec2I32 CalculateSectorCoord(SectorID sectorID) const;

  const WAiNavMeshSector* GetSector(SectorID sectorID) const;

  /// Marks the sector as requested.
  ///
  /// Returns true, if the sector is already available, false when it needs to be built first.
  bool RequestSector(SectorID sectorID);

  /// Marks all sectors within the given rectangle as requested.
  ///
  /// Returns true, if all the sectors are already available, false when any of them needs to be built first.
  bool RequestSector(const WVec2& vCenter, const WVec2& vHalfExtents);

  /// Marks the sector as invalidated.
  ///
  /// Invalidated sectors are considered out of date and must be rebuilt before they can be used again.
  /// If bRebuildAsSoonAsPossible is true, the sector is queued to be rebuilt as soon as possible.
  /// Otherwise, it will be unloaded and will not be rebuilt until it is requested again.
  void InvalidateSector(SectorID sectorID, bool bRebuildAsSoonAsPossible);

  /// Marks all sectors within the given rectangle as invalidated.
  ///
  /// Invalidated sectors are considered out of date and must be rebuilt before they can be used again.
  /// If bRebuildAsSoonAsPossible is true, the sector is queued to be rebuilt as soon as possible.
  /// Otherwise, it will be unloaded and will not be rebuilt until it is requested again.
  void InvalidateSector(const WVec2& vCenter, const WVec2& vHalfExtents, bool bRebuildAsSoonAsPossible);

  void FinalizeSectorUpdates();

  SectorID RetrieveRequestedSector();
  void BuildSector(SectorID sectorID, const WNavmeshGeoWorldModuleInterface* pGeo);

  const dtNavMesh* GetDetourNavMesh() const { return m_pNavMesh; }

  void DebugDraw(WDebugRendererContext context, const WAiNavigationConfig& config);

  const WAiNavmeshConfig& GetConfig() const { return m_NavmeshConfig; }

private:
  void DebugDrawSector(WDebugRendererContext context, const WAiNavigationConfig& config, int iTileIdx);

  WAiNavmeshConfig m_NavmeshConfig;

  WUInt32 m_uiNumSectorsX = 0;
  WUInt32 m_uiNumSectorsY = 0;
  float m_fSectorMetersXY = 0;
  float m_fInvSectorMetersXY = 0;

  dtNavMesh* m_pNavMesh = nullptr;
  WMap<SectorID, WAiNavMeshSector> m_Sectors;
  WDeque<SectorID> m_RequestedSectors;

  WMutex m_Mutex;
  WDynamicArray<SectorID> m_UpdatingSectors;

  WDynamicArray<SectorID> m_UnloadingSectors;
};
