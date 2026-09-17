#include <AiPlugin/Navigation/NavMesh.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

void DrawMeshTilePolygons(const dtMeshTile& meshTile, WDynamicArray<WDebugRendererTriangle>& out_triangles, WArrayPtr<WColor> areaColors);
void DrawMeshTileEdges(const dtMeshTile& meshTile, bool bOuterEdges, bool bInnerEdges, bool bInnerDetailEdges, WDynamicArray<WDebugRendererLine>& out_lines);

WAiNavMeshSector::WAiNavMeshSector()
{
  m_FlagRequested = 0;
  m_FlagInvalidate = 0;
  m_FlagUpdateAvailable = 0;
  m_FlagUsable = 0;
}

WAiNavMeshSector::~WAiNavMeshSector() = default;

WAiNavMesh::WAiNavMesh(const WAiNavmeshConfig& navmeshConfig)
{
  m_uiNumSectorsX = navmeshConfig.m_uiNumSectorsX;
  m_uiNumSectorsY = navmeshConfig.m_uiNumSectorsY;
  m_fSectorMetersXY = navmeshConfig.m_fSectorSize;
  m_fInvSectorMetersXY = 1.0f / navmeshConfig.m_fSectorSize;
  m_NavmeshConfig = navmeshConfig;

  m_pNavMesh = W_DEFAULT_NEW(dtNavMesh);

  dtNavMeshParams np;
  np.tileWidth = navmeshConfig.m_fSectorSize;
  np.tileHeight = navmeshConfig.m_fSectorSize;
  np.orig[0] = -(navmeshConfig.m_uiNumSectorsX * 0.5f) * navmeshConfig.m_fSectorSize;
  np.orig[1] = 0.0f;
  np.orig[2] = -(navmeshConfig.m_uiNumSectorsY * 0.5f) * navmeshConfig.m_fSectorSize;
  np.maxTiles = navmeshConfig.m_uiNumSectorsX * navmeshConfig.m_uiNumSectorsY;
  np.maxPolys = 1 << 16;

  m_pNavMesh->init(&np);
}

WAiNavMesh::~WAiNavMesh()
{
  W_DEFAULT_DELETE(m_pNavMesh);
}

WVec2I32 WAiNavMesh::CalculateSectorCoord(float fPositionX, float fPositionY) const
{
  fPositionX *= m_fInvSectorMetersXY;
  fPositionY *= m_fInvSectorMetersXY;

  fPositionX += m_uiNumSectorsX * 0.5f;
  fPositionY += m_uiNumSectorsY * 0.5f;

  return WVec2I32((WInt32)WMath::Floor(fPositionX), (WInt32)WMath::Floor(fPositionY));
}

WVec2I32 WAiNavMesh::CalculateSectorCoord(SectorID sectorID) const
{
  W_ASSERT_DEBUG(sectorID < m_uiNumSectorsX * m_uiNumSectorsY, "Invalid navmesh sector ID");

  return WVec2I32(sectorID % m_uiNumSectorsX, sectorID / m_uiNumSectorsX);
}

const WAiNavMeshSector* WAiNavMesh::GetSector(SectorID sectorID) const
{
  auto it = m_Sectors.Find(sectorID);
  if (it.IsValid())
    return &it.Value();

  return nullptr;
}

bool WAiNavMesh::RequestSector(SectorID sectorID)
{
  auto& sector = m_Sectors.FindOrAdd(sectorID).Value();

  if (sector.m_FlagUsable == 0)
  {
    if (sector.m_FlagRequested == 0)
    {
      sector.m_FlagRequested = 1;
      m_RequestedSectors.PushBack(sectorID);
    }

    return false;
  }

  return true;
}

bool WAiNavMesh::RequestSector(const WVec2& vCenter, const WVec2& vHalfExtents)
{
  WVec2I32 coordMin = CalculateSectorCoord(vCenter.x - vHalfExtents.x, vCenter.y - vHalfExtents.y);
  WVec2I32 coordMax = CalculateSectorCoord(vCenter.x + vHalfExtents.x, vCenter.y + vHalfExtents.y);

  coordMin.x = WMath::Clamp<WInt32>(coordMin.x, 0, m_uiNumSectorsX - 1);
  coordMax.x = WMath::Clamp<WInt32>(coordMax.x, 0, m_uiNumSectorsX - 1);
  coordMin.y = WMath::Clamp<WInt32>(coordMin.y, 0, m_uiNumSectorsY - 1);
  coordMax.y = WMath::Clamp<WInt32>(coordMax.y, 0, m_uiNumSectorsY - 1);

  bool res = true;

  for (WInt32 y = coordMin.y; y <= coordMax.y; ++y)
  {
    for (WInt32 x = coordMin.x; x <= coordMax.x; ++x)
    {
      if (!RequestSector(CalculateSectorID(WVec2I32(x, y))))
      {
        res = false;
      }
    }
  }

  return res;
}

void WAiNavMesh::InvalidateSector(const WVec2& vCenter, const WVec2& vHalfExtents, bool bRebuildAsSoonAsPossible)
{
  WVec2I32 coordMin = CalculateSectorCoord(vCenter.x - vHalfExtents.x, vCenter.y - vHalfExtents.y);
  WVec2I32 coordMax = CalculateSectorCoord(vCenter.x + vHalfExtents.x, vCenter.y + vHalfExtents.y);

  coordMin.x = WMath::Clamp<WInt32>(coordMin.x, 0, m_uiNumSectorsX - 1);
  coordMax.x = WMath::Clamp<WInt32>(coordMax.x, 0, m_uiNumSectorsX - 1);
  coordMin.y = WMath::Clamp<WInt32>(coordMin.y, 0, m_uiNumSectorsY - 1);
  coordMax.y = WMath::Clamp<WInt32>(coordMax.y, 0, m_uiNumSectorsY - 1);

  for (WInt32 y = coordMin.y; y <= coordMax.y; ++y)
  {
    for (WInt32 x = coordMin.x; x <= coordMax.x; ++x)
    {
      InvalidateSector(CalculateSectorID(WVec2I32(x, y)), bRebuildAsSoonAsPossible);
    }
  }
}

void WAiNavMesh::InvalidateSector(SectorID sectorID, bool bRebuildAsSoonAsPossible)
{
  auto it = m_Sectors.Find(sectorID);
  if (!it.IsValid())
    return;

  auto& sector = it.Value();

  if (sector.m_FlagInvalidate == 0 && (sector.m_FlagUsable == 1 || sector.m_FlagUpdateAvailable == 1))
  {
    if (bRebuildAsSoonAsPossible)
    {
      sector.m_FlagInvalidate = 1;
      m_RequestedSectors.PushBack(sectorID);
    }
    else
    {
      sector.m_FlagRequested = 0;
      m_UnloadingSectors.PushBack(sectorID);
    }
  }
}

void WAiNavMesh::FinalizeSectorUpdates()
{
  W_LOCK(m_Mutex);

  for (auto sectorID : m_UpdatingSectors)
  {
    const auto coord = CalculateSectorCoord(sectorID);

    auto& sector = m_Sectors[sectorID];

    W_ASSERT_DEV(sector.m_FlagUpdateAvailable == 1, "Invalid sector update state");

    if (!sector.m_NavmeshDataCur.IsEmpty())
    {
      sector.m_FlagUsable = 0;

      const auto res = m_pNavMesh->removeTile(sector.m_TileRef, nullptr, nullptr);

      if (res != DT_SUCCESS)
      {
        WLog::Error("NavMesh removeTile error: {}", res);
      }
    }

    sector.m_NavmeshDataCur.Swap(sector.m_NavmeshDataNew);
    sector.m_NavmeshDataNew.Clear();
    sector.m_NavmeshDataNew.Compact();

    if (!sector.m_NavmeshDataCur.IsEmpty())
    {
      auto res = m_pNavMesh->addTile(sector.m_NavmeshDataCur.GetData(), (int)sector.m_NavmeshDataCur.GetCount(), 0, 0, &sector.m_TileRef);

      if (res == DT_SUCCESS)
      {
        WLog::Success("Loaded navmesh tile {}|{}", coord.x, coord.y);
        sector.m_FlagUsable = 1;
      }
      else
      {
        WLog::Error("NavMesh addTile error: {}", res);
      }
    }
    else
    {
      WLog::Success("Loaded empty navmesh tile {}|{}", coord.x, coord.y);
      sector.m_FlagUsable = 1;
    }

    sector.m_FlagInvalidate = 0;
    sector.m_FlagUpdateAvailable = 0;
    // sector.m_FlagRequested = 0; // do not reset the requested flag
  }

  m_UpdatingSectors.Clear();

  for (auto sectorID : m_UnloadingSectors)
  {
    auto& sector = m_Sectors[sectorID];

    // Sector has been requested since then, don't unload it.
    if (sector.m_FlagRequested == 1)
      continue;

    if (!sector.m_NavmeshDataCur.IsEmpty())
    {
      const auto res = m_pNavMesh->removeTile(sector.m_TileRef, nullptr, nullptr);

      if (res != DT_SUCCESS)
      {
        WLog::Error("NavMesh removeTile error: {}", res);
      }

      sector.m_NavmeshDataCur.Clear();
      sector.m_NavmeshDataCur.Compact();
    }

    sector.m_FlagRequested = 0;
    sector.m_FlagInvalidate = 0;
    sector.m_FlagUpdateAvailable = 0;
    sector.m_FlagUsable = 0;
  }

  m_UnloadingSectors.Clear();
}

WAiNavMesh::SectorID WAiNavMesh::RetrieveRequestedSector()
{
  if (m_RequestedSectors.IsEmpty())
    return WInvalidIndex;

  WAiNavMesh::SectorID id = m_RequestedSectors.PeekFront();
  m_RequestedSectors.PopFront();

  return id;
}

WVec2 WAiNavMesh::GetSectorPositionOffset(WVec2I32 vCoord) const
{
  return WVec2((vCoord.x - m_uiNumSectorsX * 0.5f) * m_fSectorMetersXY, (vCoord.y - m_uiNumSectorsY * 0.5f) * m_fSectorMetersXY);
}

WBoundingBox WAiNavMesh::GetSectorBounds(WVec2I32 vCoord, float fMinZ /*= 0.0f*/, float fMaxZ /*= 1.0f*/) const
{
  const WVec3 min = GetSectorPositionOffset(vCoord).GetAsVec3(fMinZ);
  const WVec3 max = min + WVec3(m_fSectorMetersXY, m_fSectorMetersXY, fMaxZ - fMinZ);

  return WBoundingBox::MakeFromMinMax(min, max);
}

void WAiNavMesh::DebugDraw(WDebugRendererContext context, const WAiNavigationConfig& config)
{
  const auto& dtm = *GetDetourNavMesh();

  for (int i = 0; i < dtm.getMaxTiles(); ++i)
  {
    DebugDrawSector(context, config, i);
  }
}

void WAiNavMesh::DebugDrawSector(WDebugRendererContext context, const WAiNavigationConfig& config, int iTileIdx)
{
  const auto& mesh = *GetDetourNavMesh();

  const dtMeshTile* pTile = mesh.getTile(iTileIdx);

  if (pTile == nullptr || pTile->header == nullptr)
    return;

  WColor areaColors[WAiNumGroundTypes];
  for (WUInt32 i = 0; i < WAiNumGroundTypes; ++i)
  {
    areaColors[i] = config.m_GroundTypes[i].m_Color;
  }

  {
    WDynamicArray<WDebugRendererTriangle> triangles;
    triangles.Reserve(pTile->header->polyCount * 2);

    DrawMeshTilePolygons(*pTile, triangles, areaColors);
    WDebugRenderer::DrawSolidTriangles(context, triangles, WColor::White);
  }

  {
    WDynamicArray<WDebugRendererLine> lines;
    lines.Reserve(pTile->header->polyCount * 10);
    DrawMeshTileEdges(*pTile, true, true, false, lines);
    WDebugRenderer::DrawLines(context, lines, WColor::White);
  }
}
