#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation3D/VoxelGrid.h>
#include <Foundation/Math/Math.h>
#include <RendererCore/Debug/DebugRenderer.h>

namespace
{
  /// SAT-based triangle-AABB overlap test (Tomas Akenine-Moller algorithm).
  ///
  /// Tests 13 potential separating axes: 3 box face normals, 1 triangle normal,
  /// and 9 cross products of box edge directions with triangle edge directions.
  bool TriangleBoxOverlap(const WVec3& v0, const WVec3& v1, const WVec3& v2,
    const WVec3& vBoxCenter, float fBoxHalf)
  {
    // Translate triangle so box center is at origin
    const WVec3 a = v0 - vBoxCenter;
    const WVec3 b = v1 - vBoxCenter;
    const WVec3 c = v2 - vBoxCenter;
    const float h = fBoxHalf;

    // Test 3 AABB face normals
    if (WMath::Min(a.x, WMath::Min(b.x, c.x)) > h || WMath::Max(a.x, WMath::Max(b.x, c.x)) < -h)
      return false;
    if (WMath::Min(a.y, WMath::Min(b.y, c.y)) > h || WMath::Max(a.y, WMath::Max(b.y, c.y)) < -h)
      return false;
    if (WMath::Min(a.z, WMath::Min(b.z, c.z)) > h || WMath::Max(a.z, WMath::Max(b.z, c.z)) < -h)
      return false;

    // Triangle edges
    const WVec3 f0 = b - a;
    const WVec3 f1 = c - b;
    const WVec3 f2 = a - c;

    // 9 cross product axes: box_axis_i x triangle_edge_j
    // For each axis, project all 3 vertices and check against box projection radius.
    float p0, p1, p2, r;

    // (1,0,0) x f0 = (0, -f0.z, f0.y)
    p0 = -a.y * f0.z + a.z * f0.y;
    p1 = -b.y * f0.z + b.z * f0.y;
    p2 = -c.y * f0.z + c.z * f0.y;
    r = h * (WMath::Abs(f0.z) + WMath::Abs(f0.y));
    if (WMath::Min(p0, WMath::Min(p1, p2)) > r || WMath::Max(p0, WMath::Max(p1, p2)) < -r)
      return false;

    // (1,0,0) x f1 = (0, -f1.z, f1.y)
    p0 = -a.y * f1.z + a.z * f1.y;
    p1 = -b.y * f1.z + b.z * f1.y;
    p2 = -c.y * f1.z + c.z * f1.y;
    r = h * (WMath::Abs(f1.z) + WMath::Abs(f1.y));
    if (WMath::Min(p0, WMath::Min(p1, p2)) > r || WMath::Max(p0, WMath::Max(p1, p2)) < -r)
      return false;

    // (1,0,0) x f2 = (0, -f2.z, f2.y)
    p0 = -a.y * f2.z + a.z * f2.y;
    p1 = -b.y * f2.z + b.z * f2.y;
    p2 = -c.y * f2.z + c.z * f2.y;
    r = h * (WMath::Abs(f2.z) + WMath::Abs(f2.y));
    if (WMath::Min(p0, WMath::Min(p1, p2)) > r || WMath::Max(p0, WMath::Max(p1, p2)) < -r)
      return false;

    // (0,1,0) x f0 = (f0.z, 0, -f0.x)
    p0 = a.x * f0.z - a.z * f0.x;
    p1 = b.x * f0.z - b.z * f0.x;
    p2 = c.x * f0.z - c.z * f0.x;
    r = h * (WMath::Abs(f0.z) + WMath::Abs(f0.x));
    if (WMath::Min(p0, WMath::Min(p1, p2)) > r || WMath::Max(p0, WMath::Max(p1, p2)) < -r)
      return false;

    // (0,1,0) x f1 = (f1.z, 0, -f1.x)
    p0 = a.x * f1.z - a.z * f1.x;
    p1 = b.x * f1.z - b.z * f1.x;
    p2 = c.x * f1.z - c.z * f1.x;
    r = h * (WMath::Abs(f1.z) + WMath::Abs(f1.x));
    if (WMath::Min(p0, WMath::Min(p1, p2)) > r || WMath::Max(p0, WMath::Max(p1, p2)) < -r)
      return false;

    // (0,1,0) x f2 = (f2.z, 0, -f2.x)
    p0 = a.x * f2.z - a.z * f2.x;
    p1 = b.x * f2.z - b.z * f2.x;
    p2 = c.x * f2.z - c.z * f2.x;
    r = h * (WMath::Abs(f2.z) + WMath::Abs(f2.x));
    if (WMath::Min(p0, WMath::Min(p1, p2)) > r || WMath::Max(p0, WMath::Max(p1, p2)) < -r)
      return false;

    // (0,0,1) x f0 = (-f0.y, f0.x, 0)
    p0 = -a.x * f0.y + a.y * f0.x;
    p1 = -b.x * f0.y + b.y * f0.x;
    p2 = -c.x * f0.y + c.y * f0.x;
    r = h * (WMath::Abs(f0.y) + WMath::Abs(f0.x));
    if (WMath::Min(p0, WMath::Min(p1, p2)) > r || WMath::Max(p0, WMath::Max(p1, p2)) < -r)
      return false;

    // (0,0,1) x f1 = (-f1.y, f1.x, 0)
    p0 = -a.x * f1.y + a.y * f1.x;
    p1 = -b.x * f1.y + b.y * f1.x;
    p2 = -c.x * f1.y + c.y * f1.x;
    r = h * (WMath::Abs(f1.y) + WMath::Abs(f1.x));
    if (WMath::Min(p0, WMath::Min(p1, p2)) > r || WMath::Max(p0, WMath::Max(p1, p2)) < -r)
      return false;

    // (0,0,1) x f2 = (-f2.y, f2.x, 0)
    p0 = -a.x * f2.y + a.y * f2.x;
    p1 = -b.x * f2.y + b.y * f2.x;
    p2 = -c.x * f2.y + c.y * f2.x;
    r = h * (WMath::Abs(f2.y) + WMath::Abs(f2.x));
    if (WMath::Min(p0, WMath::Min(p1, p2)) > r || WMath::Max(p0, WMath::Max(p1, p2)) < -r)
      return false;

    // Triangle normal
    const WVec3 normal = f0.CrossRH(f1);
    const float d = normal.Dot(a);
    r = h * (WMath::Abs(normal.x) + WMath::Abs(normal.y) + WMath::Abs(normal.z));
    if (WMath::Abs(d) > r)
      return false;

    return true;
  }
} // namespace

WVoxelGrid::WVoxelGrid() = default;
WVoxelGrid::~WVoxelGrid() = default;

void WVoxelGrid::Initialize(const WVec3U32& vDimensions, const WVec3& vCenter, float fVoxelSize)
{
  W_ASSERT_DEV(fVoxelSize > 0.0f, "Voxel size must be positive");

  m_vDimensions.x = WMath::Max(4u, vDimensions.x);
  m_vDimensions.y = WMath::Max(4u, vDimensions.y);
  m_vDimensions.z = WMath::Max(4u, vDimensions.z);

  m_vNumBlocks.x = (m_vDimensions.x + 3u) / 4u;
  m_vNumBlocks.y = (m_vDimensions.y + 3u) / 4u;
  m_vNumBlocks.z = (m_vDimensions.z + 3u) / 4u;

  // Round dims up to block boundaries
  m_vDimensions.x = m_vNumBlocks.x * 4u;
  m_vDimensions.y = m_vNumBlocks.y * 4u;
  m_vDimensions.z = m_vNumBlocks.z * 4u;

  m_Blocks.Clear();
  m_Blocks.SetCount(m_vNumBlocks.x * m_vNumBlocks.y * m_vNumBlocks.z);

  m_vCenter = vCenter;
  m_fVoxelSize = fVoxelSize;
  m_fInvVoxelSize = 1.0f / fVoxelSize;

  const WVec3 vHalfExtents(
    m_vDimensions.x * 0.5f * m_fVoxelSize,
    m_vDimensions.y * 0.5f * m_fVoxelSize,
    m_vDimensions.z * 0.5f * m_fVoxelSize);

  m_vOrigin = m_vCenter - vHalfExtents;
}

void WVoxelGrid::ClearData()
{
  WMemoryUtils::ZeroFill(m_Blocks.GetData(), m_Blocks.GetCount());
}

void WVoxelGrid::SetVoxel(const WVec3I32& vCoord, bool bSolid)
{
  W_ASSERT_DEBUG(IsCoordValid(vCoord), "Invalid coordinate");

  const WUInt32 uiX = (WUInt32)vCoord.x;
  const WUInt32 uiY = (WUInt32)vCoord.y;
  const WUInt32 uiZ = (WUInt32)vCoord.z;

  const WUInt32 uiBlockIdx = GetBlockIndex(uiX / 4u, uiY / 4u, uiZ / 4u);
  const WUInt32 uiBit = GetBitIndex(uiX % 4u, uiY % 4u, uiZ % 4u);
  const WUInt64 uiMask = 1ull << uiBit;

  if (bSolid)
  {
    m_Blocks[uiBlockIdx] |= uiMask;
  }
  else
  {
    m_Blocks[uiBlockIdx] &= ~uiMask;
  }
}

bool WVoxelGrid::IsVoxelSet(const WVec3I32& vCoord) const
{
  W_ASSERT_DEBUG(IsCoordValid(vCoord), "Invalid coordinate");

  const WUInt32 uiX = (WUInt32)vCoord.x;
  const WUInt32 uiY = (WUInt32)vCoord.y;
  const WUInt32 uiZ = (WUInt32)vCoord.z;

  const WUInt32 uiBlockIdx = GetBlockIndex(uiX / 4u, uiY / 4u, uiZ / 4u);
  const WUInt64 uiBlock = m_Blocks[uiBlockIdx];

  if (uiBlock == 0ull)
    return false;

  const WUInt32 uiBit = GetBitIndex(uiX % 4u, uiY % 4u, uiZ % 4u);
  const WUInt64 uiMask = 1ull << uiBit;

  return (uiBlock & uiMask) != 0ull;
}

void WVoxelGrid::SetVoxelsOnTriangle(const WVec3& v0, const WVec3& v1, const WVec3& v2, bool bSet /*= true*/)
{
  // Triangle AABB in world space
  WVec3 triMin;
  triMin.x = WMath::Min(v0.x, WMath::Min(v1.x, v2.x));
  triMin.y = WMath::Min(v0.y, WMath::Min(v1.y, v2.y));
  triMin.z = WMath::Min(v0.z, WMath::Min(v1.z, v2.z));

  WVec3 triMax;
  triMax.x = WMath::Max(v0.x, WMath::Max(v1.x, v2.x));
  triMax.y = WMath::Max(v0.y, WMath::Max(v1.y, v2.y));
  triMax.z = WMath::Max(v0.z, WMath::Max(v1.z, v2.z));

  // Convert to voxel coordinates and clamp to grid bounds
  WVec3I32 coordMin = WorldToCoord(triMin);
  WVec3I32 coordMax = WorldToCoord(triMax);

  coordMin.x = WMath::Max(coordMin.x, 0);
  coordMin.y = WMath::Max(coordMin.y, 0);
  coordMin.z = WMath::Max(coordMin.z, 0);
  coordMax.x = WMath::Min(coordMax.x, (WInt32)GetDimensions().x - 1);
  coordMax.y = WMath::Min(coordMax.y, (WInt32)GetDimensions().y - 1);
  coordMax.z = WMath::Min(coordMax.z, (WInt32)GetDimensions().z - 1);

  if (coordMin.x > coordMax.x || coordMin.y > coordMax.y || coordMin.z > coordMax.z)
    return;

  const float fHalfSize = GetVoxelSize() * 0.5f;

  for (WInt32 z = coordMin.z; z <= coordMax.z; ++z)
  {
    for (WInt32 y = coordMin.y; y <= coordMax.y; ++y)
    {
      for (WInt32 x = coordMin.x; x <= coordMax.x; ++x)
      {
        const WVec3I32 vCoord(x, y, z);

        if (IsVoxelSet(vCoord) == bSet)
          continue;

        const WVec3 voxelCenter = CoordToWorld(vCoord);

        if (TriangleBoxOverlap(v0, v1, v2, voxelCenter, fHalfSize))
        {
          SetVoxel(vCoord, bSet);
        }
      }
    }
  }
}

bool WVoxelGrid::CheckLineOfSight(const WVec3I32& vStart, const WVec3I32& vGoal) const
{
  const WInt32 dx = vGoal.x - vStart.x;
  const WInt32 dy = vGoal.y - vStart.y;
  const WInt32 dz = vGoal.z - vStart.z;

  const WInt32 iSteps = WMath::Max(WMath::Abs(dx), WMath::Max(WMath::Abs(dy), WMath::Abs(dz)));

  if (iSteps == 0)
    return true;

  const float fInvSteps = 1.0f / (float)iSteps;
  const float fXIncr = (float)dx * fInvSteps;
  const float fYIncr = (float)dy * fInvSteps;
  const float fZIncr = (float)dz * fInvSteps;

  float fX = (float)vStart.x;
  float fY = (float)vStart.y;
  float fZ = (float)vStart.z;

  for (WInt32 i = 0; i < iSteps; ++i)
  {
    const WVec3I32 vCoord(
      (WInt32)WMath::Round(fX),
      (WInt32)WMath::Round(fY),
      (WInt32)WMath::Round(fZ));

    if (vCoord.x == vGoal.x && vCoord.y == vGoal.y && vCoord.z == vGoal.z)
      return true;

    if (IsVoxelSet(vCoord))
      return false;

    fX += fXIncr;
    fY += fYIncr;
    fZ += fZIncr;
  }

  return true;
}

WBoundingBox WVoxelGrid::GetAABB() const
{
  const WVec3 vHalfExtents(
    m_vDimensions.x * 0.5f * m_fVoxelSize,
    m_vDimensions.y * 0.5f * m_fVoxelSize,
    m_vDimensions.z * 0.5f * m_fVoxelSize);

  return WBoundingBox::MakeFromCenterAndHalfExtents(m_vCenter, vHalfExtents);
}

WUInt64 WVoxelGrid::GetHeapMemoryUsage() const
{
  return m_Blocks.GetHeapMemoryUsage();
}

void WVoxelGrid::DebugDraw(const WDebugRendererContext& context, const WColor& color) const
{
  if (m_Blocks.IsEmpty())
    return;

  // Shrink boxes slightly so individual voxels are visually distinguishable
  const float fHalf = m_fVoxelSize * 0.48f;
  const WVec3 vHalfExtents(fHalf, fHalf, fHalf);

  for (WUInt32 bz = 0; bz < m_vNumBlocks.z; ++bz)
  {
    for (WUInt32 by = 0; by < m_vNumBlocks.y; ++by)
    {
      for (WUInt32 bx = 0; bx < m_vNumBlocks.x; ++bx)
      {
        const WUInt32 uiBlockIdx = GetBlockIndex(bx, by, bz);
        WUInt64 uiBlock = m_Blocks[uiBlockIdx];

        if (uiBlock == 0ull)
          continue;

        while (uiBlock != 0ull)
        {
          const WUInt32 uiBit = WMath::FirstBitLow(uiBlock);
          uiBlock ^= (1ull << uiBit);

          const WUInt32 lx = uiBit % 4u;
          const WUInt32 ly = (uiBit / 4u) % 4u;
          const WUInt32 lz = uiBit / 16u;

          const WVec3I32 vGlobalCoord(
            (WInt32)(bx * 4u + lx),
            (WInt32)(by * 4u + ly),
            (WInt32)(bz * 4u + lz));

          // Only draw surface voxels (at least one free neighbor)
          bool bIsSurface = false;
          const WVec3I32 vNeighbors[] = {
            {vGlobalCoord.x - 1, vGlobalCoord.y, vGlobalCoord.z},
            {vGlobalCoord.x + 1, vGlobalCoord.y, vGlobalCoord.z},
            {vGlobalCoord.x, vGlobalCoord.y - 1, vGlobalCoord.z},
            {vGlobalCoord.x, vGlobalCoord.y + 1, vGlobalCoord.z},
            {vGlobalCoord.x, vGlobalCoord.y, vGlobalCoord.z - 1},
            {vGlobalCoord.x, vGlobalCoord.y, vGlobalCoord.z + 1},
          };

          for (const auto& vNeighbor : vNeighbors)
          {
            if (!IsCoordValid(vNeighbor) || !IsVoxelSet(vNeighbor))
            {
              bIsSurface = true;
              break;
            }
          }

          if (bIsSurface)
          {
            const WVec3 vWorldPos = CoordToWorld(vGlobalCoord);
            const WBoundingBox voxelBox = WBoundingBox::MakeFromCenterAndHalfExtents(vWorldPos, vHalfExtents);
            WDebugRenderer::DrawLineBox(context, voxelBox, color);
          }
        }
      }
    }
  }
}
