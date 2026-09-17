#include <TerrainPlugin/TerrainPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Math/Float16.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/RenderContext/BindGroupBuilder.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphContext.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>
#include <RendererFoundation/Utils/ResourceStateTracker.h>
#include <Shaders/Terrain/Generation/HeightfieldBakeConstants.h>
#include <Shaders/Terrain/Generation/VoxelBakeConstants.h>
#include <TerrainPlugin/Components/TerrainBrushBaseComponent.h>
#include <TerrainPlugin/TerrainSystem.h>
#include <Texture/Image/ImageUtils.h>

static void ExecuteGraphSync(WRenderGraph& ref_graph, WGALDevice* pDevice)
{
  W_VERIFY(ref_graph.Compile().Succeeded(), "Terrain sync render graph compilation failed");

  pDevice->BeginFrame();

  WGALResourceStateTracker tracker(pDevice);
  ref_graph.ComputeBarriers(tracker);

  auto* pEncoder = pDevice->BeginCommands("TerrainSync");
  WRenderGraphContext ctx(pEncoder, pDevice, WRenderContext::GetDefaultInstance());
  ref_graph.Execute(ctx);

  WHybridArray<WGALBufferBarrier, 4> bufBarriers;
  tracker.RevertBufferState([&](const WGALBufferBarrier& b)
    { bufBarriers.PushBack(b); });
  if (!bufBarriers.IsEmpty())
    pEncoder->BufferBarrier(bufBarriers);

  pDevice->EndCommands(pEncoder);

  pDevice->EndFrame();
}

// clang-format off
W_IMPLEMENT_WORLD_MODULE(WTerrainSystem);

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTerrainSystem, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WTerrainResolution, 1)
  W_ENUM_CONSTANT(WTerrainResolution::Res32),
  W_ENUM_CONSTANT(WTerrainResolution::Res64),
  W_ENUM_CONSTANT(WTerrainResolution::Res128),
  W_ENUM_CONSTANT(WTerrainResolution::Res256),
  W_ENUM_CONSTANT(WTerrainResolution::Res512),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WTerrainPatchColliderMode, 1)
  W_ENUM_CONSTANT(WTerrainPatchColliderMode::None),
  W_ENUM_CONSTANT(WTerrainPatchColliderMode::FullResolution),
  W_ENUM_CONSTANT(WTerrainPatchColliderMode::HalfResolution),
  W_ENUM_CONSTANT(WTerrainPatchColliderMode::QuarterResolution),
  W_ENUM_CONSTANT(WTerrainPatchColliderMode::EighthResolution),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WDeque<WTerrainSystem*> WTerrainSystem::s_TerrainSystems;

WTerrainSystem::WTerrainSystem(WWorld* pWorld)
  : SUPER(pWorld)
{
}

WTerrainSystem::~WTerrainSystem() = default;

void WTerrainSystem::Initialize()
{
  SUPER::Initialize();

  if (s_TerrainSystems.IsEmpty())
  {
    WRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
  }

  s_TerrainSystems.PushBack(this);

  m_pRenderGraph = WRenderGraphManager::CreateRenderGraph("TerrainBake", WRenderGraphPhase::PreRender);

  m_hTerrainBakeStep1Shader = WResourceManager::LoadResource<WShaderResource>("Shaders/Terrain/Generation/HeightfieldTerrainBakeStep1CS.WShader");
  m_hTerrainBakeStep2Shader = WResourceManager::LoadResource<WShaderResource>("Shaders/Terrain/Generation/HeightfieldTerrainBakeStep2CS.WShader");
  m_hTerrainBakeStep3Shader = WResourceManager::LoadResource<WShaderResource>("Shaders/Terrain/Generation/HeightfieldTerrainBakeStep3CS.WShader");
  m_hHeightfieldBakeConstants = WRenderContext::CreateConstantBufferStorage<HeightfieldBakeConstants>();

  m_hVoxelBakeShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Terrain/Generation/VoxelBakeCS.WShader");
  m_hVoxelMeshClearShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Terrain/Generation/VoxelMeshClearCS.WShader");
  m_hVoxelSurfaceNetsPass1Shader = WResourceManager::LoadResource<WShaderResource>("Shaders/Terrain/Generation/VoxelSurfaceNetsPass1CS.WShader");
  m_hVoxelSurfaceNetsPass2Shader = WResourceManager::LoadResource<WShaderResource>("Shaders/Terrain/Generation/VoxelSurfaceNetsPass2CS.WShader");
  m_hVoxelBlurDistShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Terrain/Generation/VoxelBlurDistCS.WShader");
  m_hVoxelCleanupShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Terrain/Generation/VoxelCleanupCS.WShader");
  m_hVoxelFillCompactCopyArgsShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Terrain/Generation/VoxelFillCompactCopyArgsCS.WShader");
  m_hVoxelCompactCopyShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Terrain/Generation/VoxelCompactCopyCS.WShader");
  m_hVoxelBakeConstants = WRenderContext::CreateConstantBufferStorage<VoxelBakeConstants>();
}

void WTerrainSystem::Deinitialize()
{
  FrameCleanup();

  s_TerrainSystems.RemoveAndSwap(this);

  if (s_TerrainSystems.IsEmpty())
  {
    WRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);
  }

  m_pRenderGraph = nullptr;

  WRenderContext::DeleteConstantBufferStorage(m_hHeightfieldBakeConstants);
  DestroyHeightfields();
  DestroySharedHeightfieldScratch();

  WRenderContext::DeleteConstantBufferStorage(m_hVoxelBakeConstants);
  DestroyVoxelVolumes();
  DestroySharedVoxelScratch();

  SUPER::Deinitialize();
}

void WTerrainSystem::FrameCleanup()
{
  W_LOCK(m_Mutex);

  for (WUInt32 idx : m_QueuedHeightfieldsToDelete)
  {
    DestroyHeightfieldTerrain(idx);
  }
  m_QueuedHeightfieldsToDelete.Clear();

  for (WUInt32 idx : m_QueuedVoxelVolumesToDelete)
  {
    DestroyVoxelTerrain(idx);
  }
  m_QueuedVoxelVolumesToDelete.Clear();
}

void WTerrainSystem::OnRenderEvent(const WRenderWorldRenderEvent& e)
{
  if (e.m_Type != WRenderWorldRenderEvent::Type::BeginRender)
    return;

  for (auto* pTerrain : s_TerrainSystems)
  {
    pTerrain->UpdateTerrain();
  }
}

void WTerrainSystem::UpdateTerrain()
{
  FrameCleanup();

  bool bAnyDirty = m_bBrushesDirty;

  if (!bAnyDirty)
  {
    for (const auto& patch : m_Heightfields)
    {
      if (patch.m_bInUse && patch.m_bDirty)
      {
        bAnyDirty = true;
        break;
      }
    }
  }

  if (!bAnyDirty)
  {
    for (const auto& vol : m_VoxelVolumes)
    {
      if (vol.m_bInUse && vol.m_bDirty)
      {
        bAnyDirty = true;
        break;
      }
    }
  }

  if (!bAnyDirty)
    return;

  W_PROFILE_SCOPE("UpdateTerrain");

  m_pRenderGraph->Reset();

  for (WUInt32 i = 0; i < m_Heightfields.GetCount(); ++i)
  {
    WTerrainData_Heightfield& patch = m_Heightfields[i];

    if (!patch.m_bInUse)
      continue;

    bool bShouldBake = patch.m_bDirty;

    if (m_bBrushesDirty)
    {
      const WUInt64 uiNewHash = ComputeHeightfieldBrushOverlapHash(patch);
      if (uiNewHash != patch.m_uiBrushOverlapHash)
        bShouldBake = true;

      patch.m_uiBrushOverlapHash = uiNewHash;
    }

    if (bShouldBake)
    {
      UpdateHeightfield(i, *m_pRenderGraph);
    }
  }

  for (WUInt32 i = 0; i < m_VoxelVolumes.GetCount(); ++i)
  {
    WTerrainData_Voxel& vol = m_VoxelVolumes[i];
    if (!vol.m_bInUse)
      continue;

    bool bShouldBake = vol.m_bDirty;
    WUInt64 uiNewHash = vol.m_uiBrushOverlapHash;
    if (m_bBrushesDirty)
    {
      uiNewHash = ComputeVoxelBrushOverlapHash(vol);

      if (uiNewHash != vol.m_uiBrushOverlapHash)
        bShouldBake = true;
    }

    if (bShouldBake)
    {
      vol.m_uiBrushOverlapHash = uiNewHash;
      UpdateVoxels(i, *m_pRenderGraph);
    }
  }

  WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);

  m_bBrushesDirty = false;
}

//////////////////////////////////////////////////////////////////////////
// Brushes
//////////////////////////////////////////////////////////////////////////

WUInt32 WTerrainSystem::CreateBrushData()
{
  for (WUInt32 idx = 0; idx < m_Brushes.GetCount(); ++idx)
  {
    if (!m_Brushes[idx].m_bInUse)
    {
      m_Brushes[idx].m_bInUse = true;
      return idx;
    }
  }

  m_Brushes.ExpandAndGetRef().m_bInUse = true;
  return m_Brushes.GetCount() - 1;
}

void WTerrainSystem::RemoveBrushData(WUInt32& ref_uiIdx)
{
  if (ref_uiIdx == WInvalidIndex)
    return;

  W_ASSERT_DEV(ref_uiIdx < m_Brushes.GetCount(), "Invalid brush index");

  m_bBrushesDirty = true;
  m_Brushes[ref_uiIdx].m_bInUse = false;
  ref_uiIdx = WInvalidIndex;
}

const WTerrainData_Brush& WTerrainSystem::ReadBrushData(WUInt32 uiIdx) const
{
  return m_Brushes[uiIdx];
}

WTerrainData_Brush& WTerrainSystem::ModifyBrushData(WUInt32 uiIdx)
{
  m_bBrushesDirty = true;
  return m_Brushes[uiIdx];
}

WGALBufferHandle WTerrainSystem::CreateBrushBuffer(WDynamicArray<TerrainBrushData>& brushes, WGALDevice* pDevice) const
{
  // Always allocate at least one element so the binding is always valid.
  // BrushCount=0 prevents the shader from indexing into it.
  if (brushes.IsEmpty())
    brushes.ExpandAndGetRef();

  WGALBufferCreationDescription brushDesc;
  brushDesc.m_uiStructSize = sizeof(TerrainBrushData);
  brushDesc.m_uiTotalSize = brushes.GetCount() * sizeof(TerrainBrushData);
  brushDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;

  return pDevice->CreateBuffer(brushDesc, WArrayPtr<const WUInt8>(reinterpret_cast<const WUInt8*>(brushes.GetData()), brushes.GetCount() * sizeof(TerrainBrushData)));
}

//////////////////////////////////////////////////////////////////////////
// Heightfield Terrain
//////////////////////////////////////////////////////////////////////////

WUInt32 WTerrainSystem::CreateHeightfieldTerrain(WUInt32 uiCellsPerSide)
{
  W_PROFILE_SCOPE("CreateHeightfieldTerrain");

  // Find a free slot first
  WUInt32 uiIndex = WInvalidIndex;
  for (WUInt32 i = 0; i < m_Heightfields.GetCount(); ++i)
  {
    if (!m_Heightfields[i].m_bInUse)
    {
      uiIndex = i;
      break;
    }
  }

  if (uiIndex == WInvalidIndex)
  {
    uiIndex = m_Heightfields.GetCount();
    m_Heightfields.ExpandAndGetRef();
  }

  WTerrainData_Heightfield& patch = m_Heightfields[uiIndex];
  patch.m_uiCellsPerSide = uiCellsPerSide;

  patch.m_bInUse = true;
  patch.m_bDirty = true;

  // Stored grid = render vertices (CellsPerSide+1) plus 4 extra rings on each side.
  // Extra rings let normals CS use correct central-differences at patch edges,
  // and provide enough padding for Jolt's required multiple-of-4 vertex count.
  const WUInt32 uiStoredSize = uiCellsPerSide + 9;
  // Create persistent GPU buffer (UAV for CS, SRV in VS)
  WGALBufferCreationDescription bufDesc;
  bufDesc.m_uiStructSize = sizeof(float);
  bufDesc.m_uiTotalSize = uiStoredSize * uiStoredSize * sizeof(float);
  bufDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
  bufDesc.m_ResourceAccess.m_bImmutable = false;

  patch.m_hBakedHeights = WGALDevice::GetDefaultDevice()->CreateBuffer(bufDesc);

  // Create persistent normal buffer (uint per vertex: XY as packed 16-bit floats)
  WGALBufferCreationDescription normDesc;
  normDesc.m_uiStructSize = sizeof(WUInt32);
  normDesc.m_uiTotalSize = uiStoredSize * uiStoredSize * sizeof(WUInt32);
  normDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
  normDesc.m_ResourceAccess.m_bImmutable = false;

  patch.m_hBakedNormals = WGALDevice::GetDefaultDevice()->CreateBuffer(normDesc);

  // Per-cell top-4 material indices (uint per cell), written by Step3 and bound as SRV in the VS.
  // Covers the border cells too, so skirt cells have real data instead of reusing the nearest inner
  // cell's. The stored grid is a power of two plus 8 cells wide, so it divides evenly by the step.
  const WUInt32 uiStoredCellsPerSide = (uiStoredSize - 1) / WTerrainMaterialCellStep;

  WGALBufferCreationDescription cellMatDesc;
  cellMatDesc.m_uiStructSize = sizeof(WUInt32);
  cellMatDesc.m_uiTotalSize = uiStoredCellsPerSide * uiStoredCellsPerSide * sizeof(WUInt32);
  cellMatDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
  cellMatDesc.m_ResourceAccess.m_bImmutable = false;

  patch.m_hCellMaterials = WGALDevice::GetDefaultDevice()->CreateBuffer(cellMatDesc);

  // Per-cell-corner f16 blend weights (uint per corner), written by Step3 and bound as SRV in the VS.
  // Layout: (cellIndex * 4 + cornerSlot), cornerSlot = TL:0, TR:1, BL:2, BR:3.
  // Each corner slot is unique — no inter-thread write conflicts in Step3.
  WGALBufferCreationDescription weightDesc;
  weightDesc.m_uiStructSize = sizeof(WUInt32);
  weightDesc.m_uiTotalSize = uiStoredCellsPerSide * uiStoredCellsPerSide * 4 * sizeof(WUInt32);
  weightDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
  weightDesc.m_ResourceAccess.m_bImmutable = false;

  patch.m_hVertexWeights = WGALDevice::GetDefaultDevice()->CreateBuffer(weightDesc);

  // One carve bit per stored-grid vertex, written by Step2 and bound as SRV in the VS. Full
  // resolution regardless of the material cell step, because carving is a per-vertex decision.
  // Rounded up to whole words; trailing bits of the last word are written as not carved.
  WGALBufferCreationDescription carveDesc;
  carveDesc.m_uiStructSize = sizeof(WUInt32);
  carveDesc.m_uiTotalSize = ((uiStoredSize * uiStoredSize + 31) / 32) * sizeof(WUInt32);
  carveDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
  carveDesc.m_ResourceAccess.m_bImmutable = false;

  patch.m_hCarveMask = WGALDevice::GetDefaultDevice()->CreateBuffer(carveDesc);

  return uiIndex;
}

void WTerrainSystem::RemoveHeightfieldTerrain(WUInt32& ref_uiPatchIndex)
{
  W_LOCK(m_Mutex);
  m_QueuedHeightfieldsToDelete.PushBack(ref_uiPatchIndex);
}

void WTerrainSystem::DestroyHeightfieldTerrain(WUInt32& uiIdx)
{
  if (uiIdx >= m_Heightfields.GetCount())
    return;

  auto& data = m_Heightfields[uiIdx];
  uiIdx = WInvalidIndex;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  pDevice->DestroyBuffer(data.m_hBakedHeights);
  pDevice->DestroyBuffer(data.m_hBakedNormals);
  pDevice->DestroyBuffer(data.m_hCellMaterials);
  pDevice->DestroyBuffer(data.m_hVertexWeights);
  pDevice->DestroyBuffer(data.m_hCarveMask);

  data.m_bInUse = false;
}

WTerrainData_Heightfield& WTerrainSystem::ModifyHeightfieldTerrain(WUInt32 uiIdx)
{
  auto& data = m_Heightfields[uiIdx];
  data.m_bDirty = true;
  return data;
}

WGALBufferHandle WTerrainSystem::GetHeightfieldHeightBuffer(WUInt32 uiPatchIndex) const
{
  if (uiPatchIndex < m_Heightfields.GetCount() && m_Heightfields[uiPatchIndex].m_bInUse)
    return m_Heightfields[uiPatchIndex].m_hBakedHeights;

  return WGALBufferHandle();
}

WGALBufferHandle WTerrainSystem::GetHeightfieldNormalBuffer(WUInt32 uiPatchIndex) const
{
  if (uiPatchIndex < m_Heightfields.GetCount() && m_Heightfields[uiPatchIndex].m_bInUse)
    return m_Heightfields[uiPatchIndex].m_hBakedNormals;

  return WGALBufferHandle();
}

WGALBufferHandle WTerrainSystem::GetHeightfieldCellMaterialBuffer(WUInt32 uiPatchIndex) const
{
  if (uiPatchIndex < m_Heightfields.GetCount() && m_Heightfields[uiPatchIndex].m_bInUse)
    return m_Heightfields[uiPatchIndex].m_hCellMaterials;

  return WGALBufferHandle();
}

WGALBufferHandle WTerrainSystem::GetHeightfieldMaterialVertexWeightBuffer(WUInt32 uiPatchIndex) const
{
  if (uiPatchIndex < m_Heightfields.GetCount() && m_Heightfields[uiPatchIndex].m_bInUse)
    return m_Heightfields[uiPatchIndex].m_hVertexWeights;

  return WGALBufferHandle();
}

WGALBufferHandle WTerrainSystem::GetHeightfieldCarveMaskBuffer(WUInt32 uiPatchIndex) const
{
  if (uiPatchIndex < m_Heightfields.GetCount() && m_Heightfields[uiPatchIndex].m_bInUse)
    return m_Heightfields[uiPatchIndex].m_hCarveMask;

  return WGALBufferHandle();
}

WUInt32 WTerrainSystem::GetHeightfieldCellsPerSide(WUInt32 uiPatchIndex) const
{
  return (uiPatchIndex < m_Heightfields.GetCount()) ? m_Heightfields[uiPatchIndex].m_uiCellsPerSide : 128;
}

WUInt64 WTerrainSystem::GetHeightfieldBrushOverlapHash(WUInt32 uiPatchIndex) const
{
  if (uiPatchIndex >= m_Heightfields.GetCount() || !m_Heightfields[uiPatchIndex].m_bInUse)
    return 0;
  return ComputeHeightfieldBrushOverlapHash(m_Heightfields[uiPatchIndex]);
}

WUInt64 WTerrainSystem::ComputeHeightfieldBrushOverlapHash(const WTerrainData_Heightfield& heightfield) const
{
  const WTransform invTrans = heightfield.m_GlobalTransform.GetInverse();
  const float fSize = (float)heightfield.m_uiCellsPerSide * heightfield.m_fGridSpacing;

  WHashStreamWriter64 writer;

  for (WUInt32 i = 0; i < m_Brushes.GetCount(); ++i)
  {
    const auto& brush = m_Brushes[i];
    if (!brush.m_bInUse || !brush.m_bAffectHeightfields)
      continue;

    if (!brush.m_Tags.IsEmpty() && !brush.m_Tags.IsAnySet(heightfield.m_Tags))
      continue;

    const WVec3 vLocalCenter = invTrans * brush.m_vPosition;
    const float fConservativeRadius = WMath::Sqrt(brush.m_vHalfExtents.x * brush.m_vHalfExtents.x + brush.m_vHalfExtents.y * brush.m_vHalfExtents.y) + brush.m_fInnerRadius + brush.m_fOuterRadius;

    const float fClampedX = WMath::Clamp(vLocalCenter.x, 0.0f, fSize);
    const float fClampedY = WMath::Clamp(vLocalCenter.y, 0.0f, fSize);
    const float fDx = vLocalCenter.x - fClampedX;
    const float fDy = vLocalCenter.y - fClampedY;

    if (fDx * fDx + fDy * fDy > fConservativeRadius * fConservativeRadius)
      continue;

    writer << i;
    writer << brush.m_vPosition.x << brush.m_vPosition.y << brush.m_vPosition.z;
    writer << brush.m_qRotation.x << brush.m_qRotation.y << brush.m_qRotation.z << brush.m_qRotation.w;
    writer << brush.m_vHalfExtents.x << brush.m_vHalfExtents.y;
    writer << brush.m_fHalfExtentZ << brush.m_fHalfExtentYTop;
    writer << brush.m_fInnerRadius << brush.m_fOuterRadius << brush.m_fFalloff;
    writer << brush.m_ModifyMode.GetValue();
    writer << brush.m_uiMaterialIndex << brush.m_fMaterialStrength;
    writer << brush.m_fNoiseStrength;
    writer << brush.m_fNoiseFrequency;
    writer << brush.m_iPriority;
    brush.m_Tags.Save(writer);
  }

  writer << heightfield.m_uiDefaultMaterialIndex;
  heightfield.m_Tags.Save(writer);

  return writer.GetHashValue();
}

void WTerrainSystem::FindHeightfieldOverlappingBrushes(const WTerrainData_Heightfield& heightfield, WDynamicArray<TerrainBrushData>& brushes) const
{
  const WTransform invTrans = heightfield.m_GlobalTransform.GetInverse();
  const float fSize = (float)heightfield.m_uiCellsPerSide * heightfield.m_fGridSpacing;

  brushes.Clear();
  brushes.Reserve(m_Brushes.GetCount());

  const WMat3 mInvTerrainRot = heightfield.m_GlobalTransform.m_qRotation.GetAsMat3().GetTranspose();

  for (auto& brush : m_Brushes)
  {
    if (!brush.m_bInUse || !brush.m_bAffectHeightfields)
      continue;

    if (!brush.m_Tags.IsEmpty() && !brush.m_Tags.IsAnySet(heightfield.m_Tags))
      continue;

    if ((brush.m_ModifyMode == WTerrainModifyMode::OnlyPaint2D || brush.m_ModifyMode == WTerrainModifyMode::OnlyPaint3D) && brush.m_fMaterialStrength <= 0.0f)
      continue;

    // Skip brushes whose conservative footprint does not overlap the heightfield XY extent.
    const WVec3 vLocalCenter = invTrans * brush.m_vPosition;
    const float fConservativeRadius = WMath::Sqrt(brush.m_vHalfExtents.x * brush.m_vHalfExtents.x + brush.m_vHalfExtents.y * brush.m_vHalfExtents.y) + brush.m_fInnerRadius + brush.m_fOuterRadius;
    const float fClampedX = WMath::Clamp(vLocalCenter.x, 0.0f, fSize);
    const float fClampedY = WMath::Clamp(vLocalCenter.y, 0.0f, fSize);
    const float fDx = vLocalCenter.x - fClampedX;
    const float fDy = vLocalCenter.y - fClampedY;
    if (fDx * fDx + fDy * fDy > fConservativeRadius * fConservativeRadius)
      continue;

    TerrainBrushData& bd = brushes.ExpandAndGetRef();
    bd.Position = vLocalCenter;
    // bd.Position.z -= 0.5f; // keep the brush component slightly above the baked terrain so its icon remains visible
    bd.HalfExtentX = brush.m_vHalfExtents.x;
    bd.HalfExtentYBottom = brush.m_vHalfExtents.y;
    bd.HalfExtentYTop = brush.m_fHalfExtentYTop;
    bd.HalfExtentZ = brush.m_fHalfExtentZ;
    bd.InnerRadius = brush.m_fInnerRadius;
    bd.OuterRadius = WMath::Max(brush.m_fOuterRadius, 0.0001f);
    bd.Falloff = brush.m_fFalloff;
    bd.ModifyMode = brush.m_ModifyMode.GetValue();
    bd.MaterialIndex = brush.m_uiMaterialIndex;
    bd.MaterialStrength = brush.m_fMaterialStrength;
    bd.NoiseStrength = brush.m_fNoiseStrength;
    bd.NoiseFrequency = WMath::Max(0.0001f, brush.m_fNoiseFrequency);
    bd.CpuPriority = static_cast<float>(brush.m_iPriority);

    // Build the inverse rotation from terrain-local space into brush-local space.
    // mLocalBrushRot = invTerrainRot * brushWorldRot  (takes brush-local -> terrain-local)
    // mInvLocalBrushRot = transpose of above          (takes terrain-local -> brush-local)
    const WMat3 mInvLocalBrushRot = (mInvTerrainRot * brush.m_qRotation.GetAsMat3()).GetTranspose();

    bd.InvRotRow0 = mInvLocalBrushRot.GetRow(0);
    bd.InvRotRow1 = mInvLocalBrushRot.GetRow(1);
    bd.InvRotRow2 = mInvLocalBrushRot.GetRow(2);
  }

  // Primary sort: priority ascending (higher priority = applied later = wins).
  // Within the same priority: Carve last, then Max < Min < Set, then by height.
  brushes.Sort([](const TerrainBrushData& a, const TerrainBrushData& b) -> bool
    {
      if (a.CpuPriority != b.CpuPriority)
        return a.CpuPriority < b.CpuPriority;
      const bool aCarve = a.ModifyMode == WTerrainModifyMode::Carve;
      const bool bCarve = b.ModifyMode == WTerrainModifyMode::Carve;
      if (aCarve != bCarve)
        return !aCarve;
      if (a.ModifyMode != b.ModifyMode)
        return a.ModifyMode < b.ModifyMode;
      if (a.ModifyMode == 1u)             // Min: descending height
        return a.Position.z > b.Position.z;
      return a.Position.z < b.Position.z; // Max / Set / Ignore: ascending height
    });
}

void WTerrainSystem::EnsureSharedHeightfieldScratch(WUInt32 uiStoredSize)
{
  if (uiStoredSize <= m_uiHeightfieldSharedMaskStoredSize && !m_hHeightfieldSharedMask.IsInvalidated())
    return;

  DestroySharedHeightfieldScratch();

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  // Intermediate material mask (uint2 per vertex): written by Step1/2, read by Step3.
  WGALBufferCreationDescription maskDesc;
  maskDesc.m_uiStructSize = sizeof(WUInt64);
  maskDesc.m_uiTotalSize = uiStoredSize * uiStoredSize * sizeof(WUInt64);
  maskDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
  maskDesc.m_ResourceAccess.m_bImmutable = false;

  m_hHeightfieldSharedMask = pDevice->CreateBuffer(maskDesc);
  m_uiHeightfieldSharedMaskStoredSize = uiStoredSize;
}

void WTerrainSystem::DestroyHeightfields()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  for (auto& patch : m_Heightfields)
  {
    pDevice->DestroyBuffer(patch.m_hBakedHeights);
    pDevice->DestroyBuffer(patch.m_hBakedNormals);
    pDevice->DestroyBuffer(patch.m_hCellMaterials);
    pDevice->DestroyBuffer(patch.m_hVertexWeights);
  }
  m_Heightfields.Clear();
}

void WTerrainSystem::DestroySharedHeightfieldScratch()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  pDevice->DestroyBuffer(m_hHeightfieldSharedMask);

  m_uiHeightfieldSharedMaskStoredSize = 0;
}

void WTerrainSystem::UpdateHeightfield(WUInt32 uiIndex, WRenderGraph& graph)
{
  W_PROFILE_SCOPE("UpdateHeightfield");

  auto& patch = m_Heightfields[uiIndex];
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  WTempHybridArray<TerrainBrushData, 16> brushCPUData;
  FindHeightfieldOverlappingBrushes(patch, brushCPUData);
  const WUInt32 uiNumBrushes = brushCPUData.GetCount();

  WGALBufferHandle hBrushBuffer = CreateBrushBuffer(brushCPUData, pDevice);

  // Build source height data by sampling the height image (or zeros if no image is assigned).
  // Stored grid = (CellsPerSide+9)² — 4 border rings on each side beyond the render vertices.
  const WUInt32 uiStoredSize = patch.m_uiCellsPerSide + 9;
  const WUInt32 uiVertexCount = patch.m_uiCellsPerSide + 1; // vertices per side (quads+1)
  WTempArray<float> srcHeights;
  srcHeights.SetCount(uiStoredSize * uiStoredSize, 0.0f);

  if (patch.m_hHeightImage.IsValid() && patch.m_fHeightScale > 0.0f)
  {
    WResourceLock<WImageDataResource> imgLock(patch.m_hHeightImage, WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (imgLock.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      const WImage& img = imgLock->GetDescriptor().m_Image;
      const WColor* pPixels = img.GetPixelPointer<WColor>();
      const WUInt32 uiImgW = img.GetWidth();
      const WUInt32 uiImgH = img.GetHeight();
      const float fInvVertexCount = (uiVertexCount > 1) ? 1.0f / static_cast<float>(uiVertexCount - 1) : 0.0f;

      for (WUInt32 sRow = 0; sRow < uiStoredSize; ++sRow)
      {
        for (WUInt32 sCol = 0; sCol < uiStoredSize; ++sCol)
        {
          const float u = patch.m_vImageOffset.x + static_cast<float>(static_cast<WInt32>(sCol) - 4) * fInvVertexCount * patch.m_vImageSize.x;
          const float v = patch.m_vImageOffset.y + static_cast<float>(static_cast<WInt32>(sRow) - 4) * fInvVertexCount * patch.m_vImageSize.y;
          srcHeights[sRow * uiStoredSize + sCol] = WImageUtils::BilinearSample(pPixels, uiImgW, uiImgH, WImageAddressMode::Clamp, WVec2(u, v)).r * patch.m_fHeightScale;
        }
      }
    }
  }

  // Create source heights buffer with initial data (no separate UpdateBuffer needed).
  WGALBufferCreationDescription srcDesc;
  srcDesc.m_uiStructSize = sizeof(float);
  srcDesc.m_uiTotalSize = srcHeights.GetCount() * sizeof(float);
  srcDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
  srcDesc.m_ResourceAccess.m_bImmutable = false;

  WGALBufferHandle hSourceBuffer = pDevice->CreateBuffer(srcDesc, WArrayPtr<const WUInt8>(reinterpret_cast<const WUInt8*>(srcHeights.GetData()), srcHeights.GetCount() * sizeof(float)));

  const WUInt32 uiGroups = (uiStoredSize + 15) / 16;

  // The intermediate mask is shared bake scratch — grow it to fit this patch's stored grid.
  EnsureSharedHeightfieldScratch(uiStoredSize);

  // Import all buffers into the graph.
  auto hGraphSrc = graph.ImportBuffer(hSourceBuffer);
  auto hGraphBrush = graph.ImportBuffer(hBrushBuffer);
  auto hGraphCarve = graph.ImportBuffer(patch.m_hCarveMask);
  auto hGraphBakedH = graph.ImportBuffer(patch.m_hBakedHeights);
  auto hGraphBakedM = graph.ImportBuffer(m_hHeightfieldSharedMask);
  auto hGraphBakedN = graph.ImportBuffer(patch.m_hBakedNormals);

  // Pass 1: bake heights from source + brushes into BakedHeights (UAV) and BakedMask (UAV).
  {
    HeightfieldBakeConstants c1;
    c1.GridSpacing = patch.m_fGridSpacing;
    c1.VertexIdxPitch = uiStoredSize;
    c1.BrushCount = uiNumBrushes;
    c1.PatchOrigin = patch.m_GlobalTransform.m_vPosition.GetAsVec2();

    auto pass = graph.AddComputePass("TerrainHFBakeStep1");
    pass.ReadBuffer(hGraphSrc);
    pass.ReadBuffer(hGraphBrush);
    pass.WriteBuffer(hGraphBakedH);
    pass.WriteBuffer(hGraphBakedM);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c1, uiGroups, hGraphSrc, hGraphBrush, hGraphBakedH, hGraphBakedM](const WRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrevAsync = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrevAsync));

        HeightfieldBakeConstants* constants = WRenderContext::GetConstantBufferData<HeightfieldBakeConstants>(m_hHeightfieldBakeConstants);
        *constants = c1;
        pRC->BindShader(m_hTerrainBakeStep1Shader);
        WBindGroupBuilder& bg = pRC->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("SourceHeights", ctx.ResolveBuffer(hGraphSrc));
        bg.BindBuffer("BakedHeights", ctx.ResolveBuffer(hGraphBakedH));
        bg.BindBuffer("BakedMask", ctx.ResolveBuffer(hGraphBakedM));
        bg.BindBuffer("Brushes", ctx.ResolveBuffer(hGraphBrush));
        bg.BindBuffer("HeightfieldBakeConstants", m_hHeightfieldBakeConstants);
        pRC->Dispatch(uiGroups, uiGroups, 1).AssertSuccess(); });
  }

  // Pass 2: derive normals from BakedHeights (SRV) into BakedNormals (UAV).
  // The graph inserts a UAV→SRV barrier on BakedHeights between pass 1 and pass 2.
  {
    HeightfieldBakeConstants c2;
    c2.GridSpacing = patch.m_fGridSpacing;
    c2.VertexIdxPitch = uiStoredSize;
    c2.DefaultMaterialIndex = patch.m_uiDefaultMaterialIndex;

    auto pass = graph.AddComputePass("TerrainHFBakeStep2");
    pass.ReadBuffer(hGraphBakedH);
    pass.WriteBuffer(hGraphBakedM); // RWStructuredBuffer: normalizes weights in-place, needs UAV state
    pass.WriteBuffer(hGraphCarve);
    pass.WriteBuffer(hGraphBakedN);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c2, uiGroups, hGraphBakedH, hGraphBakedN, hGraphBakedM, hGraphCarve](const WRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrevAsync = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrevAsync));

        HeightfieldBakeConstants* constants = WRenderContext::GetConstantBufferData<HeightfieldBakeConstants>(m_hHeightfieldBakeConstants);
        *constants = c2;
        pRC->BindShader(m_hTerrainBakeStep2Shader);
        WBindGroupBuilder& bg = pRC->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedHeights", ctx.ResolveBuffer(hGraphBakedH));
        bg.BindBuffer("BakedNormals", ctx.ResolveBuffer(hGraphBakedN));
        bg.BindBuffer("BakedMask", ctx.ResolveBuffer(hGraphBakedM));
        bg.BindBuffer("BakedCarveMask", ctx.ResolveBuffer(hGraphCarve));
        bg.BindBuffer("HeightfieldBakeConstants", m_hHeightfieldBakeConstants);
        pRC->Dispatch(uiGroups, uiGroups, 1).AssertSuccess(); });
  }

  // Pass 3: read finalized BakedMask (SRV) → write CellMaterials + VertexWeights (UAVs).
  // The graph inserts a UAV→SRV barrier on BakedMask between Pass2 and Pass3.
  {
    HeightfieldBakeConstants c3;
    c3.VertexIdxPitch = uiStoredSize;
    // Step3 covers the border cells too, so skirt cells get their own material set and weights.
    c3.CellsPerSide = (uiStoredSize - 1) / WTerrainMaterialCellStep;

    const WUInt32 uiCellGroups = (c3.CellsPerSide + 15) / 16;

    auto hGraphCellMat = graph.ImportBuffer(patch.m_hCellMaterials);
    auto hGraphVtxW = graph.ImportBuffer(patch.m_hVertexWeights);

    auto pass = graph.AddComputePass("TerrainHFBakeStep3");
    pass.ReadBuffer(hGraphBakedM);
    pass.WriteBuffer(hGraphCellMat);
    pass.WriteBuffer(hGraphVtxW);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c3, uiCellGroups, hGraphBakedM, hGraphCellMat, hGraphVtxW](const WRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrevAsync = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrevAsync));

        HeightfieldBakeConstants* constants = WRenderContext::GetConstantBufferData<HeightfieldBakeConstants>(m_hHeightfieldBakeConstants);
        *constants = c3;
        pRC->BindShader(m_hTerrainBakeStep3Shader);
        WBindGroupBuilder& bg = pRC->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedMask", ctx.ResolveBuffer(hGraphBakedM));
        bg.BindBuffer("CellMaterials", ctx.ResolveBuffer(hGraphCellMat));
        bg.BindBuffer("VertexWeights", ctx.ResolveBuffer(hGraphVtxW));
        bg.BindBuffer("HeightfieldBakeConstants", m_hHeightfieldBakeConstants);
        pRC->Dispatch(uiCellGroups, uiCellGroups, 1).AssertSuccess(); });
  }

  // Deferred deletion: GAL delays destruction by several frames, so the callbacks will still see valid data.
  pDevice->DestroyBuffer(hSourceBuffer);
  pDevice->DestroyBuffer(hBrushBuffer);

  patch.m_bDirty = false;
}

WResult WTerrainSystem::ReadbackHeightfieldData(WUInt32 uiPatchIndex, WDynamicArray<float>& out_heights, WDynamicArray<WUInt8>& out_dominantMat, WTime timeout)
{
  if (uiPatchIndex >= m_Heightfields.GetCount())
    return W_FAILURE;

  auto& patch = m_Heightfields[uiPatchIndex];
  if (!patch.m_bInUse || patch.m_hBakedHeights.IsInvalidated())
    return W_FAILURE;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  // The mask is shared scratch, so it is not persistent — always re-bake and read back within one sync graph.
  patch.m_bDirty = true;

  WGALReadbackBufferHelper heightRB;
  WGALReadbackBufferHelper maskRB;

  auto pGraph = WRenderGraphManager::CreateRenderGraph("TerrainHFExportReadback", WRenderGraphPhase::PreRender);
  pGraph->Reset();

  UpdateHeightfield(uiPatchIndex, *pGraph);

  // Re-import the same buffers (ImportBuffer dedupes by handle) to add the readback transfer pass.
  auto hGraphBakedH = pGraph->ImportBuffer(patch.m_hBakedHeights);
  auto hGraphBakedM = pGraph->ImportBuffer(m_hHeightfieldSharedMask);

  const WGALBufferHandle hBH = patch.m_hBakedHeights;
  const WGALBufferHandle hBM = m_hHeightfieldSharedMask;

  auto pass = pGraph->AddTransferPass("TerrainHFReadback");
  pass.ReadBuffer(hGraphBakedH, WGALResourceState::CopySource);
  pass.ReadBuffer(hGraphBakedM, WGALResourceState::CopySource);
  pass.HasSideEffects();
  pass.SetExecuteCallback([hBH, hBM, &heightRB, &maskRB](const WRenderGraphContext& ctx)
    {
      heightRB.ReadbackBuffer(*ctx.GetCommandEncoder(), hBH);
      maskRB.ReadbackBuffer(*ctx.GetCommandEncoder(), hBM); });

  ExecuteGraphSync(*pGraph, pDevice);

  const WTime tDeadline = WTime::Now() + timeout;

  auto PollReadback = [&](WGALReadbackBufferHelper& readback, const char* szName) -> WResult
  {
    while (true)
    {
      const auto result = readback.GetReadbackResult(WTime::MakeFromMilliseconds(2));
      if (result == WGALAsyncResult::Expired)
      {
        WLog::Error("ReadbackHeightfieldData: {} readback expired for patch {}.", szName, uiPatchIndex);
        return W_FAILURE;
      }
      if (result == WGALAsyncResult::Ready)
        return W_SUCCESS;
      if (WTime::Now() >= tDeadline)
      {
        WLog::Error("ReadbackHeightfieldData: timed out waiting for {} of patch {}.", szName, uiPatchIndex);
        return W_FAILURE;
      }
    }
  };

  if (PollReadback(heightRB, "heights").Failed())
    return W_FAILURE;
  if (PollReadback(maskRB, "mask").Failed())
    return W_FAILURE;

  const WUInt32 S = patch.m_uiCellsPerSide + 9;

  {
    WArrayPtr<const WUInt8> rawMemory;
    auto lock = heightRB.LockBuffer(rawMemory);
    if (!lock)
      return W_FAILURE;

    const float* pStoredHeights = reinterpret_cast<const float*>(rawMemory.GetPtr());
    out_heights.SetCountUninitialized(S * S);
    WMemoryUtils::Copy(out_heights.GetData(), pStoredHeights, S * S);
  }

  {
    WArrayPtr<const WUInt8> rawMemory;
    auto lock = maskRB.LockBuffer(rawMemory);
    if (lock)
    {
      // Step2 sorts only the four explicit brush slots, so slot 0 is the strongest brush material,
      // which is not necessarily the strongest material overall: the base material is implicit and
      // covers whatever weight the brushes leave over. Compare it against slot 0 here so a faint
      // brush does not win over a base material that visually dominates the surface.
      const WUInt32* pStoredMask = reinterpret_cast<const WUInt32*>(rawMemory.GetPtr());
      out_dominantMat.SetCountUninitialized(S * S);
      for (WUInt32 i = 0; i < S * S; ++i)
      {
        const WUInt32 uiIndices = pStoredMask[i * 2];
        const WUInt32 uiWeights = pStoredMask[i * 2 + 1];

        const WUInt8 uiTopBrushMat = static_cast<WUInt8>(uiIndices & 0xFFu);

        // 0xFF in slot 0 is the sentinel for a carved (non-colliding) vertex and must be preserved.
        if (uiTopBrushMat == 0xFFu)
        {
          out_dominantMat[i] = 0xFFu;
          continue;
        }

        const WUInt32 uiTopBrushWeight = uiWeights & 0xFFu;
        WUInt32 uiTotalBrushWeight = 0;
        for (WUInt32 s = 0; s < 4; ++s)
          uiTotalBrushWeight += (uiWeights >> (s * 8)) & 0xFFu;

        // Step1 composites brushes with alpha semantics, so the weights sum to <= 1 and the remainder
        // is the base material's share. A brush painting at full strength leaves no remainder and wins here.
        const WUInt32 uiBaseWeight = (uiTotalBrushWeight >= 255) ? 0 : (255 - uiTotalBrushWeight);

        out_dominantMat[i] = (uiBaseWeight > uiTopBrushWeight) ? patch.m_uiDefaultMaterialIndex : uiTopBrushMat;
      }
    }
  }

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
// Voxel Terrain
//////////////////////////////////////////////////////////////////////////

W_DEFINE_AS_POD_TYPE(VoxelGpuVertex);

WUInt32 WTerrainSystem::CreateVoxelTerrain(WUInt32 uiResolution, float fVoxelSize)
{
  W_PROFILE_SCOPE("CreateVoxelTerrain");

  WUInt32 uiIndex = WInvalidIndex;
  for (WUInt32 i = 0; i < m_VoxelVolumes.GetCount(); ++i)
  {
    if (!m_VoxelVolumes[i].m_bInUse)
    {
      uiIndex = i;
      break;
    }
  }
  if (uiIndex == WInvalidIndex)
  {
    uiIndex = m_VoxelVolumes.GetCount();
    m_VoxelVolumes.ExpandAndGetRef();
  }

  constexpr WUInt32 c_uiVoxelBorderVoxels = 4u; // border voxels on each side of the inner volume
  WTerrainData_Voxel& vol = m_VoxelVolumes[uiIndex];
  vol.m_uiResolution = uiResolution;
  // X: add border on both sides, then align to multiple of 8 (required for voxel packing — 8 per uint).
  // Y/Z: add border on both sides, no alignment needed.
  const WUInt32 uiBufX = ((uiResolution + 2u * c_uiVoxelBorderVoxels + 7u) & ~7u);
  vol.m_uiPackPitch = uiBufX / 8u;
  vol.m_fVoxelSize = fVoxelSize;
  vol.m_bInUse = true;
  vol.m_bDirty = true;
  vol.m_uiBrushOverlapHash = 0;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  // Only the final render buffers are held per piece; all bake scratch lives in the shared set on the system.
  // Cells = (N+1)³ where N = uiResolution; indices worst-case = cells * 18.
  const WUInt32 uiCells = vol.m_uiResolution + 1u;
  const WUInt32 uiMaxCells = uiCells * uiCells * uiCells;
  const WUInt32 uiMaxIndices = uiMaxCells * 18u;

  // Final render buffers — worst-case sized so rendering can begin on the first bake without a count readback;
  // the GPU-driven compact-copy only writes the used entries and DrawArgs controls the draw count.
  {
    WGALBufferCreationDescription bufDesc;
    bufDesc.m_uiStructSize = sizeof(VoxelGpuVertex);
    bufDesc.m_uiTotalSize = uiMaxCells * sizeof(VoxelGpuVertex);
    bufDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::UnorderedAccess | WGALBufferUsageFlags::ShaderResource;
    bufDesc.m_ResourceAccess.m_bImmutable = false;
    vol.m_hFinalVertices = pDevice->CreateBuffer(bufDesc);
  }
  {
    WGALBufferCreationDescription bufDesc;
    bufDesc.m_uiStructSize = sizeof(WUInt32);
    bufDesc.m_uiTotalSize = uiMaxIndices * sizeof(WUInt32);
    bufDesc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::UnorderedAccess | WGALBufferUsageFlags::ShaderResource;
    bufDesc.m_ResourceAccess.m_bImmutable = false;
    vol.m_hFinalIndices = pDevice->CreateBuffer(bufDesc);
  }
  {
    // DrawArgs: 4 uints {IndexCount, 1, 0, 0}. ByteAddressBuffer + DrawIndirect.
    WGALBufferCreationDescription bufDesc;
    bufDesc.m_uiStructSize = sizeof(WUInt32);
    bufDesc.m_uiTotalSize = 4u * sizeof(WUInt32);
    bufDesc.m_BufferFlags = WGALBufferUsageFlags::ByteAddressBuffer | WGALBufferUsageFlags::UnorderedAccess | WGALBufferUsageFlags::DrawIndirect;
    bufDesc.m_ResourceAccess.m_bImmutable = false;
    const WUInt32 zero[4] = {0, 1, 0, 0};
    vol.m_hFinalDrawArgs = pDevice->CreateBuffer(bufDesc, WMakeArrayPtr(zero).ToByteArray());
  }

  return uiIndex;
}

void WTerrainSystem::RemoveVoxelTerrain(WUInt32 uiIndex)
{
  W_LOCK(m_Mutex);
  m_QueuedVoxelVolumesToDelete.PushBack(uiIndex);
}

WTerrainData_Voxel& WTerrainSystem::ModifyVoxelTerrain(WUInt32 uiIndex)
{
  auto& vol = m_VoxelVolumes[uiIndex];
  vol.m_bDirty = true;
  return vol;
}

void WTerrainSystem::DestroyVoxelTerrain(WUInt32& uiIdx)
{
  if (uiIdx >= m_VoxelVolumes.GetCount())
    return;

  auto& data = m_VoxelVolumes[uiIdx];
  uiIdx = WInvalidIndex;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  if (!data.m_hFinalVertices.IsInvalidated())
  {
    pDevice->DestroyBuffer(data.m_hFinalVertices);
    data.m_hFinalVertices.Invalidate();
  }
  if (!data.m_hFinalIndices.IsInvalidated())
  {
    pDevice->DestroyBuffer(data.m_hFinalIndices);
    data.m_hFinalIndices.Invalidate();
  }
  if (!data.m_hFinalDrawArgs.IsInvalidated())
  {
    pDevice->DestroyBuffer(data.m_hFinalDrawArgs);
    data.m_hFinalDrawArgs.Invalidate();
  }

  data.m_bInUse = false;
}

void WTerrainSystem::EnsureSharedVoxelScratch(WUInt32 uiPackPitch, WUInt32 uiBufYZ, WUInt32 uiResolution)
{
  if (uiPackPitch <= m_uiSharedVoxelPackPitch && uiBufYZ <= m_uiSharedVoxelBufYZ && uiResolution <= m_uiSharedVoxelResolution && !m_hSharedVoxels.IsInvalidated())
    return;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  DestroySharedVoxelScratch();

  // Grow to the maximum of the requested and previous dimensions so a smaller later bake never shrinks it.
  uiPackPitch = WMath::Max(uiPackPitch, m_uiSharedVoxelPackPitch);
  uiBufYZ = WMath::Max(uiBufYZ, m_uiSharedVoxelBufYZ);
  uiResolution = WMath::Max(uiResolution, m_uiSharedVoxelResolution);

  const WUInt32 uiBufX = uiPackPitch * 8u;
  const WUInt32 uiPackedCount = uiPackPitch * uiBufYZ * uiBufYZ;
  const WUInt32 uiCells = uiResolution + 1u;
  const WUInt32 uiMaxCells = uiCells * uiCells * uiCells;
  const WUInt32 uiMaxIndices = uiMaxCells * 18u;

  auto CreateBuf = [pDevice](WUInt32 uiStructSize, WUInt32 uiCount) -> WGALBufferHandle
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = uiStructSize;
    desc.m_uiTotalSize = uiStructSize * uiCount;
    desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    return pDevice->CreateBuffer(desc);
  };

  m_hSharedVoxels = CreateBuf(sizeof(WUInt32), uiPackedCount);
  m_hSharedVoxelDist = CreateBuf(sizeof(float), uiBufX * uiBufYZ * uiBufYZ);
  m_hSharedVoxelDistScratch = CreateBuf(sizeof(float), uiBufX * uiBufYZ * uiBufYZ);
  m_hSharedMeshRemap = CreateBuf(sizeof(WUInt32), uiMaxCells);
  m_hSharedMeshCompactVertices = CreateBuf(sizeof(VoxelGpuVertex), uiMaxCells);
  m_hSharedMeshIndices = CreateBuf(sizeof(WUInt32), uiMaxIndices);
  m_hSharedMeshCounts = CreateBuf(sizeof(VoxelMeshCounts), 1);

  // DispatchIndirect args for VoxelCompactCopyCS: 3 uints {GroupsX, 1, 1}. Re-filled each bake.
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(WUInt32);
    desc.m_uiTotalSize = 3u * sizeof(WUInt32);
    desc.m_BufferFlags = WGALBufferUsageFlags::ByteAddressBuffer | WGALBufferUsageFlags::UnorderedAccess | WGALBufferUsageFlags::DrawIndirect;
    desc.m_ResourceAccess.m_bImmutable = false;
    const WUInt32 one[3] = {1, 1, 1};
    m_hSharedCompactCopyDispatchArgs = pDevice->CreateBuffer(desc, WMakeArrayPtr(one).ToByteArray());
  }

  m_uiSharedVoxelPackPitch = uiPackPitch;
  m_uiSharedVoxelBufYZ = uiBufYZ;
  m_uiSharedVoxelResolution = uiResolution;
}

void WTerrainSystem::DestroyVoxelVolumes()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  for (auto& vol : m_VoxelVolumes)
  {
    if (!vol.m_hFinalVertices.IsInvalidated())
      pDevice->DestroyBuffer(vol.m_hFinalVertices);
    if (!vol.m_hFinalIndices.IsInvalidated())
      pDevice->DestroyBuffer(vol.m_hFinalIndices);
    if (!vol.m_hFinalDrawArgs.IsInvalidated())
      pDevice->DestroyBuffer(vol.m_hFinalDrawArgs);
  }
  m_VoxelVolumes.Clear();
}

void WTerrainSystem::DestroySharedVoxelScratch()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  WGALBufferHandle* handles[] = {&m_hSharedVoxels, &m_hSharedVoxelDist, &m_hSharedVoxelDistScratch, &m_hSharedMeshRemap, &m_hSharedMeshCompactVertices, &m_hSharedMeshIndices, &m_hSharedMeshCounts, &m_hSharedCompactCopyDispatchArgs};
  for (WGALBufferHandle* pHandle : handles)
  {
    if (!pHandle->IsInvalidated())
    {
      pDevice->DestroyBuffer(*pHandle);
      pHandle->Invalidate();
    }
  }

  m_uiSharedVoxelPackPitch = 0;
  m_uiSharedVoxelBufYZ = 0;
  m_uiSharedVoxelResolution = 0;
}

WGALBufferHandle WTerrainSystem::GetVoxelVolumeGpuMeshVertexBuffer(WUInt32 uiIndex) const
{
  if (uiIndex < m_VoxelVolumes.GetCount())
  {
    const auto& vol = m_VoxelVolumes[uiIndex];
    return vol.m_hFinalVertices;
  }
  return {};
}

WGALBufferHandle WTerrainSystem::GetVoxelVolumeGpuMeshDrawArgsBuffer(WUInt32 uiIndex) const
{
  if (uiIndex < m_VoxelVolumes.GetCount())
  {
    const auto& vol = m_VoxelVolumes[uiIndex];
    return vol.m_hFinalDrawArgs;
  }
  return {};
}

WGALBufferHandle WTerrainSystem::GetVoxelVolumeGpuMeshIndexBuffer(WUInt32 uiIndex) const
{
  if (uiIndex < m_VoxelVolumes.GetCount())
  {
    const auto& vol = m_VoxelVolumes[uiIndex];
    return vol.m_hFinalIndices;
  }
  return {};
}

WUInt64 WTerrainSystem::GetVoxelBrushOverlapHash(WUInt32 uiIndex) const
{
  if (uiIndex >= m_VoxelVolumes.GetCount())
    return 0;
  return m_VoxelVolumes[uiIndex].m_uiBrushOverlapHash;
}

WUInt64 WTerrainSystem::ComputeVoxelBrushOverlapHash(const WTerrainData_Voxel& vol) const
{
  const WTransform invTrans = vol.m_GlobalTransform.GetInverse();
  const float fSize = (float)vol.m_uiResolution * vol.m_fVoxelSize;

  WHashStreamWriter64 writer;

  for (WUInt32 i = 0; i < m_Brushes.GetCount(); ++i)
  {
    const auto& brush = m_Brushes[i];
    if (!brush.m_bInUse || !brush.m_bAffectVoxels)
      continue;

    if (!brush.m_Tags.IsEmpty() && !brush.m_Tags.IsAnySet(vol.m_Tags))
      continue;

    const WVec3 vLocalCenter = invTrans * brush.m_vPosition;
    const float fConservativeRadius = WMath::Sqrt(brush.m_vHalfExtents.x * brush.m_vHalfExtents.x + brush.m_vHalfExtents.y * brush.m_vHalfExtents.y + brush.m_fHalfExtentZ * brush.m_fHalfExtentZ) + brush.m_fInnerRadius + brush.m_fOuterRadius;

    const float fDx = vLocalCenter.x - WMath::Clamp(vLocalCenter.x, 0.0f, fSize);
    const float fDy = vLocalCenter.y - WMath::Clamp(vLocalCenter.y, 0.0f, fSize);
    // OnlyPaint2D projects from above with no Z extent — skip Z bounds check.
    const float fDz = (brush.m_ModifyMode == WTerrainModifyMode::OnlyPaint2D) ? 0.0f : (vLocalCenter.z - WMath::Clamp(vLocalCenter.z, 0.0f, fSize));
    if (fDx * fDx + fDy * fDy + fDz * fDz > fConservativeRadius * fConservativeRadius)
      continue;

    writer << i;
    writer << brush.m_vPosition.x << brush.m_vPosition.y << brush.m_vPosition.z;
    writer << brush.m_qRotation.x << brush.m_qRotation.y << brush.m_qRotation.z << brush.m_qRotation.w;
    writer << brush.m_vHalfExtents.x << brush.m_vHalfExtents.y;
    writer << brush.m_fHalfExtentZ << brush.m_fHalfExtentYTop;
    writer << brush.m_fInnerRadius << brush.m_fOuterRadius << brush.m_fFalloff;
    writer << brush.m_ModifyMode.GetValue();
    writer << brush.m_uiMaterialIndex << brush.m_fMaterialStrength;
    writer << brush.m_fNoiseStrength << brush.m_fNoiseFrequency;
    writer << brush.m_iPriority;
    brush.m_Tags.Save(writer);
  }

  writer << vol.m_fFillHeight;
  vol.m_Tags.Save(writer);

  return writer.GetHashValue();
}

void WTerrainSystem::FindVoxelOverlappingBrushes(const WTerrainData_Voxel& vol, WDynamicArray<TerrainBrushData>& brushes) const
{
  const WTransform invTrans = vol.m_GlobalTransform.GetInverse();
  const float fSize = (float)vol.m_uiResolution * vol.m_fVoxelSize;

  brushes.Clear();
  brushes.Reserve(m_Brushes.GetCount());

  const WMat3 mInvVolumeRot = vol.m_GlobalTransform.m_qRotation.GetAsMat3().GetTranspose();

  for (const auto& brush : m_Brushes)
  {
    if (!brush.m_bInUse || !brush.m_bAffectVoxels)
      continue;

    if (!brush.m_Tags.IsEmpty() && !brush.m_Tags.IsAnySet(vol.m_Tags))
      continue;

    if ((brush.m_ModifyMode == WTerrainModifyMode::OnlyPaint2D || brush.m_ModifyMode == WTerrainModifyMode::OnlyPaint3D) && brush.m_fMaterialStrength <= 0.0f)
      continue;

    const WVec3 vLocalCenter = invTrans * brush.m_vPosition;
    const float fConservativeRadius = WMath::Sqrt(brush.m_vHalfExtents.x * brush.m_vHalfExtents.x + brush.m_vHalfExtents.y * brush.m_vHalfExtents.y + brush.m_fHalfExtentZ * brush.m_fHalfExtentZ) + brush.m_fInnerRadius + brush.m_fOuterRadius;

    const float fDx = vLocalCenter.x - WMath::Clamp(vLocalCenter.x, 0.0f, fSize);
    const float fDy = vLocalCenter.y - WMath::Clamp(vLocalCenter.y, 0.0f, fSize);
    // OnlyPaint2D projects from above with no Z extent — skip Z bounds check.
    const float fDz = (brush.m_ModifyMode == WTerrainModifyMode::OnlyPaint2D) ? 0.0f : (vLocalCenter.z - WMath::Clamp(vLocalCenter.z, 0.0f, fSize));
    if (fDx * fDx + fDy * fDy + fDz * fDz > fConservativeRadius * fConservativeRadius)
      continue;

    TerrainBrushData& bd = brushes.ExpandAndGetRef();
    bd.Position = vLocalCenter;
    bd.HalfExtentX = brush.m_vHalfExtents.x;
    bd.HalfExtentYBottom = brush.m_vHalfExtents.y;
    bd.HalfExtentYTop = brush.m_fHalfExtentYTop;
    bd.HalfExtentZ = brush.m_fHalfExtentZ;
    bd.InnerRadius = brush.m_fInnerRadius;
    bd.OuterRadius = WMath::Max(brush.m_fOuterRadius, 0.0001f);
    bd.Falloff = brush.m_fFalloff;
    bd.ModifyMode = brush.m_ModifyMode.GetValue();
    bd.MaterialIndex = brush.m_uiMaterialIndex;
    bd.MaterialStrength = brush.m_fMaterialStrength;
    bd.NoiseStrength = brush.m_fNoiseStrength;
    bd.NoiseFrequency = WMath::Max(0.0001f, brush.m_fNoiseFrequency);
    bd.CpuPriority = static_cast<float>(brush.m_iPriority);

    const WMat3 mInvLocalBrushRot = (mInvVolumeRot * brush.m_qRotation.GetAsMat3()).GetTranspose();
    bd.InvRotRow0 = mInvLocalBrushRot.GetRow(0);
    bd.InvRotRow1 = mInvLocalBrushRot.GetRow(1);
    bd.InvRotRow2 = mInvLocalBrushRot.GetRow(2);
  }

  // Primary sort: priority ascending (higher priority = applied later = wins).
  // Within same priority: Carve last, then by mode number, then by height.
  brushes.Sort([](const TerrainBrushData& a, const TerrainBrushData& b) -> bool
    {
      if (a.CpuPriority != b.CpuPriority)
        return a.CpuPriority < b.CpuPriority;
      const bool aCarve = a.ModifyMode == WTerrainModifyMode::Carve;
      const bool bCarve = b.ModifyMode == WTerrainModifyMode::Carve;
      if (aCarve != bCarve)
        return !aCarve;
      if (a.ModifyMode != b.ModifyMode)
        return a.ModifyMode < b.ModifyMode;
      if (a.ModifyMode == 1u)
        return a.Position.z > b.Position.z;
      return a.Position.z < b.Position.z; });
}

void WTerrainSystem::UpdateVoxels(WUInt32 uiIndex, WRenderGraph& graph)
{
  W_PROFILE_SCOPE("UpdateVoxels");

  auto& vol = m_VoxelVolumes[uiIndex];
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  WTempHybridArray<TerrainBrushData, 16> brushCPUData;
  FindVoxelOverlappingBrushes(vol, brushCPUData);

  const WUInt32 uiNumBrushes = brushCPUData.GetCount();
  WGALBufferHandle hBrushBuffer = CreateBrushBuffer(brushCPUData, pDevice);

  constexpr WUInt32 c_uiVoxelBorderVoxels = 4u;

  const WUInt32 uiResolution = vol.m_uiResolution;
  const WUInt32 uiPackPitch = vol.m_uiPackPitch;
  const WUInt32 uiBufYZ = uiResolution + 2u * c_uiVoxelBorderVoxels;
  const WUInt32 uiGroupsX = (uiPackPitch + 3u) / 4u;
  const WUInt32 uiGroupsYZ = (uiBufYZ + 3u) / 4u;

  // The bake scratch is shared across all volumes — grow it to fit this volume before importing.
  EnsureSharedVoxelScratch(uiPackPitch, uiBufYZ, uiResolution);

  // Import all buffers. Default states: UAV for SRV|UAV buffers, SRV for brush. Scratch is the shared set;
  // only the Final* buffers are per volume.
  auto hGraphBrush = graph.ImportBuffer(hBrushBuffer, WGALResourceState::ShaderResource);
  auto hGraphBakedVoxels = graph.ImportBuffer(m_hSharedVoxels);
  auto hGraphBakedVoxelDist = graph.ImportBuffer(m_hSharedVoxelDist);
  auto hGraphBakedVoxelDistScratch = graph.ImportBuffer(m_hSharedVoxelDistScratch);
  auto hGraphGpuMeshRemap = graph.ImportBuffer(m_hSharedMeshRemap);
  auto hGraphGpuMeshCompactVerts = graph.ImportBuffer(m_hSharedMeshCompactVertices);
  auto hGraphGpuMeshIndices = graph.ImportBuffer(m_hSharedMeshIndices);
  auto hGraphGpuMeshCounts = graph.ImportBuffer(m_hSharedMeshCounts);
  auto hGraphCompactCopyDispatchArgs = graph.ImportBuffer(m_hSharedCompactCopyDispatchArgs);
  auto hGraphFinalVertices = graph.ImportBuffer(vol.m_hFinalVertices);
  auto hGraphFinalIndices = graph.ImportBuffer(vol.m_hFinalIndices);
  auto hGraphFinalDrawArgs = graph.ImportBuffer(vol.m_hFinalDrawArgs);

  // Bake pass: writes BakedVoxels (UAV) and BakedVoxelDist (UAV) from brushes.
  {
    VoxelBakeConstants c = {};
    c.GridSpacing = vol.m_fVoxelSize;
    c.VoxelResolution = WVec3U32(uiResolution, uiResolution, uiResolution);
    c.BufferSize = WVec3U32(uiPackPitch * 8u, uiBufYZ, uiBufYZ);
    c.NumBorderVoxels = c_uiVoxelBorderVoxels;
    c.BrushCount = uiNumBrushes;
    c.InitialSolid = vol.m_bInitialSolid ? 1 : 0;
    c.FillHeight = vol.m_fFillHeight;
    c.PatchOrigin = vol.m_GlobalTransform.m_vPosition;

    auto pass = graph.AddComputePass("VoxelBake");
    pass.ReadBuffer(hGraphBrush);
    pass.WriteBuffer(hGraphBakedVoxels);
    pass.WriteBuffer(hGraphBakedVoxelDist);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c, uiGroupsX, uiGroupsYZ, hGraphBrush, hGraphBakedVoxels, hGraphBakedVoxelDist](const WRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        VoxelBakeConstants* constants = WRenderContext::GetConstantBufferData<VoxelBakeConstants>(m_hVoxelBakeConstants);
        *constants = c;
        pRC->BindShader(m_hVoxelBakeShader);
        WBindGroupBuilder& bg = pRC->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedVoxels",    ctx.ResolveBuffer(hGraphBakedVoxels));
        bg.BindBuffer("BakedVoxelDist", ctx.ResolveBuffer(hGraphBakedVoxelDist));
        bg.BindBuffer("Brushes",        ctx.ResolveBuffer(hGraphBrush));
        bg.BindBuffer("VoxelBakeConstants", m_hVoxelBakeConstants);
        pRC->Dispatch(uiGroupsX, uiGroupsYZ, uiGroupsYZ).AssertSuccess(); });
  }

  pDevice->DestroyBuffer(hBrushBuffer);
  vol.m_bDirty = false;

  // SDF blur: reads BakedVoxelDist (SRV), writes BakedVoxelDistScratch (UAV), reads BakedVoxels (SRV).
  {
    VoxelBakeConstants c = {};
    c.VoxelResolution = WVec3U32(uiResolution, uiResolution, uiResolution);
    c.BufferSize = WVec3U32(uiPackPitch * 8u, uiBufYZ, uiBufYZ);
    c.NumBorderVoxels = c_uiVoxelBorderVoxels;
    c.SmoothFactor = 0.3f;

    auto pass = graph.AddComputePass("VoxelBlurDist");
    pass.ReadBuffer(hGraphBakedVoxelDist);
    pass.ReadBuffer(hGraphBakedVoxels);
    pass.WriteBuffer(hGraphBakedVoxelDistScratch);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c, uiGroupsX, uiGroupsYZ, hGraphBakedVoxelDist, hGraphBakedVoxelDistScratch, hGraphBakedVoxels](const WRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        VoxelBakeConstants* constants = WRenderContext::GetConstantBufferData<VoxelBakeConstants>(m_hVoxelBakeConstants);
        *constants = c;
        pRC->BindShader(m_hVoxelBlurDistShader);
        WBindGroupBuilder& bg = pRC->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedVoxelDistIn",  ctx.ResolveBuffer(hGraphBakedVoxelDist));
        bg.BindBuffer("BakedVoxelDistOut", ctx.ResolveBuffer(hGraphBakedVoxelDistScratch));
        bg.BindBuffer("BakedVoxels",       ctx.ResolveBuffer(hGraphBakedVoxels));
        bg.BindBuffer("VoxelBakeConstants", m_hVoxelBakeConstants);
        pRC->Dispatch(uiGroupsX, uiGroupsYZ, uiGroupsYZ).AssertSuccess(); });
  }

  // Topology cleanup iterations: ping-pong between the two dist buffers.
  // After blur, Scratch holds the smoothed result so that is the initial src.
  auto hGraphCleanupSrc = hGraphBakedVoxelDistScratch;
  auto hGraphCleanupDst = hGraphBakedVoxelDist;
  const WUInt32 uiCleanupIterations = vol.m_uiCleanupIterations;

  for (WUInt32 it = 0; it < uiCleanupIterations; ++it)
  {
    VoxelBakeConstants c = {};
    c.GridSpacing = vol.m_fVoxelSize;
    c.VoxelResolution = WVec3U32(uiResolution, uiResolution, uiResolution);
    c.BufferSize = WVec3U32(uiPackPitch * 8u, uiBufYZ, uiBufYZ);
    c.NumBorderVoxels = c_uiVoxelBorderVoxels;

    auto hSrc = hGraphCleanupSrc;
    auto hDst = hGraphCleanupDst;

    auto pass = graph.AddComputePass("VoxelCleanup");
    pass.ReadBuffer(hSrc);
    pass.WriteBuffer(hDst);
    pass.ReadBuffer(hGraphBakedVoxels);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c, uiGroupsX, uiGroupsYZ, hSrc, hDst, hGraphBakedVoxels](const WRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        VoxelBakeConstants* constants = WRenderContext::GetConstantBufferData<VoxelBakeConstants>(m_hVoxelBakeConstants);
        *constants = c;
        pRC->BindShader(m_hVoxelCleanupShader);
        WBindGroupBuilder& bg = pRC->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedVoxelDistIn",  ctx.ResolveBuffer(hSrc));
        bg.BindBuffer("BakedVoxelDistOut", ctx.ResolveBuffer(hDst));
        bg.BindBuffer("BakedVoxels",       ctx.ResolveBuffer(hGraphBakedVoxels));
        bg.BindBuffer("VoxelBakeConstants", m_hVoxelBakeConstants);
        pRC->Dispatch(uiGroupsX, uiGroupsYZ, uiGroupsYZ).AssertSuccess(); });

    WMath::Swap(hGraphCleanupSrc, hGraphCleanupDst);
  }

  // After cleanup, hGraphCleanupSrc holds the most recent output.
  const auto hGraphFinalVoxelDist = hGraphCleanupSrc;

  // Clear mesh counts before surface nets atomic-adds.
  {
    auto pass = graph.AddComputePass("VoxelMeshClear");
    pass.WriteBuffer(hGraphGpuMeshCounts);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, hGraphGpuMeshCounts](const WRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        pRC->BindShader(m_hVoxelMeshClearShader);
        pRC->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL).BindBuffer("OutCounts", ctx.ResolveBuffer(hGraphGpuMeshCounts));
        pRC->Dispatch(1, 1, 1).AssertSuccess(); });
  }

  // Surface Nets Pass 1: scatter vertices into compact slots, record remapping.
  const WUInt32 uiPass1Groups = (uiResolution + 1u + 3u) / 4u;
  {
    VoxelBakeConstants c = {};
    c.GridSpacing = vol.m_fVoxelSize;
    c.VoxelResolution = WVec3U32(uiResolution, uiResolution, uiResolution);
    c.BufferSize = WVec3U32(uiPackPitch * 8u, uiBufYZ, uiBufYZ);
    c.NumBorderVoxels = c_uiVoxelBorderVoxels;

    auto pass = graph.AddComputePass("VoxelSurfaceNetsPass1");
    pass.ReadBuffer(hGraphBakedVoxels);
    pass.ReadBuffer(hGraphFinalVoxelDist);
    pass.WriteBuffer(hGraphGpuMeshRemap);
    pass.WriteBuffer(hGraphGpuMeshCompactVerts);
    pass.WriteBuffer(hGraphGpuMeshCounts);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c, uiPass1Groups, hGraphBakedVoxels, hGraphFinalVoxelDist, hGraphGpuMeshRemap, hGraphGpuMeshCompactVerts, hGraphGpuMeshCounts](const WRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        VoxelBakeConstants* constants = WRenderContext::GetConstantBufferData<VoxelBakeConstants>(m_hVoxelBakeConstants);
        *constants = c;
        pRC->BindShader(m_hVoxelSurfaceNetsPass1Shader);
        WBindGroupBuilder& bg = pRC->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedVoxels",        ctx.ResolveBuffer(hGraphBakedVoxels));
        bg.BindBuffer("BakedVoxelDist",     ctx.ResolveBuffer(hGraphFinalVoxelDist));
        bg.BindBuffer("OutRemap",           ctx.ResolveBuffer(hGraphGpuMeshRemap));
        bg.BindBuffer("OutCompactVertices", ctx.ResolveBuffer(hGraphGpuMeshCompactVerts));
        bg.BindBuffer("OutCounts",          ctx.ResolveBuffer(hGraphGpuMeshCounts));
        bg.BindBuffer("VoxelBakeConstants", m_hVoxelBakeConstants);
        pRC->Dispatch(uiPass1Groups, uiPass1Groups, uiPass1Groups).AssertSuccess(); });
  }

  // Surface Nets Pass 2: emit indices for each axis (3 separate passes).
  // Reading OutRemap (now InRemap as SRV) causes a UAV→SRV barrier, which serializes pass 1 writes.
  const WUInt32 uiPass2Groups = (uiResolution + 1u + 3u) / 4u;
  for (WUInt32 axis = 0; axis < 3; ++axis)
  {
    VoxelBakeConstants c = {};
    c.GridSpacing = vol.m_fVoxelSize;
    c.VoxelResolution = WVec3U32(uiResolution, uiResolution, uiResolution);
    c.BufferSize = WVec3U32(uiPackPitch * 8u, uiBufYZ, uiBufYZ);
    c.NumBorderVoxels = c_uiVoxelBorderVoxels;
    c.EdgeAxis = axis;

    auto pass = graph.AddComputePass("VoxelSurfaceNetsPass2");
    pass.ReadBuffer(hGraphFinalVoxelDist);
    pass.ReadBuffer(hGraphGpuMeshRemap);
    pass.WriteBuffer(hGraphGpuMeshIndices);
    pass.WriteBuffer(hGraphGpuMeshCounts);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c, uiPass2Groups, hGraphBakedVoxels, hGraphFinalVoxelDist, hGraphGpuMeshRemap, hGraphGpuMeshIndices, hGraphGpuMeshCounts](const WRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        VoxelBakeConstants* constants = WRenderContext::GetConstantBufferData<VoxelBakeConstants>(m_hVoxelBakeConstants);
        *constants = c;
        pRC->BindShader(m_hVoxelSurfaceNetsPass2Shader);
        WBindGroupBuilder& bg = pRC->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedVoxels",    ctx.ResolveBuffer(hGraphBakedVoxels));
        bg.BindBuffer("BakedVoxelDist", ctx.ResolveBuffer(hGraphFinalVoxelDist));
        bg.BindBuffer("InRemap",        ctx.ResolveBuffer(hGraphGpuMeshRemap));
        bg.BindBuffer("OutIndices",     ctx.ResolveBuffer(hGraphGpuMeshIndices));
        bg.BindBuffer("OutCounts",      ctx.ResolveBuffer(hGraphGpuMeshCounts));
        bg.BindBuffer("VoxelBakeConstants", m_hVoxelBakeConstants);
        pRC->Dispatch(uiPass2Groups, uiPass2Groups, uiPass2Groups).AssertSuccess(); });
  }

  // Fill indirect dispatch args from GPU counts (written by surface nets passes above).
  {
    auto pass = graph.AddComputePass("VoxelFillCompactCopyArgs");
    pass.ReadBuffer(hGraphGpuMeshCounts);
    pass.WriteBuffer(hGraphCompactCopyDispatchArgs);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, hGraphGpuMeshCounts, hGraphCompactCopyDispatchArgs](const WRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        pRC->BindShader(m_hVoxelFillCompactCopyArgsShader);
        WBindGroupBuilder& bg = pRC->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("InCounts",        ctx.ResolveBuffer(hGraphGpuMeshCounts));
        bg.BindBuffer("OutDispatchArgs", ctx.ResolveBuffer(hGraphCompactCopyDispatchArgs));
        pRC->Dispatch(1, 1, 1).AssertSuccess(); });
  }

  // Compact-copy: GPU-driven DispatchIndirect copies only active entries to final render buffers.
  // The dispatch args buffer must be in DrawIndirect state for DispatchIndirect.
  {
    WGALBufferHandle hDispatchArgs = m_hSharedCompactCopyDispatchArgs;

    auto pass = graph.AddComputePass("VoxelCompactCopy");
    pass.ReadBuffer(hGraphCompactCopyDispatchArgs, WGALResourceState::DrawIndirect);
    pass.ReadBuffer(hGraphGpuMeshCompactVerts);
    pass.ReadBuffer(hGraphGpuMeshIndices);
    pass.ReadBuffer(hGraphGpuMeshCounts);
    pass.WriteBuffer(hGraphFinalVertices);
    pass.WriteBuffer(hGraphFinalIndices);
    pass.WriteBuffer(hGraphFinalDrawArgs);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, hDispatchArgs, hGraphGpuMeshCompactVerts, hGraphGpuMeshIndices, hGraphGpuMeshCounts, hGraphFinalVertices, hGraphFinalIndices, hGraphFinalDrawArgs](const WRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        pRC->BindShader(m_hVoxelCompactCopyShader);
        WBindGroupBuilder& bg = pRC->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("InCompactVertices", ctx.ResolveBuffer(hGraphGpuMeshCompactVerts));
        bg.BindBuffer("InIndices",         ctx.ResolveBuffer(hGraphGpuMeshIndices));
        bg.BindBuffer("InCounts",          ctx.ResolveBuffer(hGraphGpuMeshCounts));
        bg.BindBuffer("OutFinalVertices",  ctx.ResolveBuffer(hGraphFinalVertices));
        bg.BindBuffer("OutFinalIndices",   ctx.ResolveBuffer(hGraphFinalIndices));
        bg.BindBuffer("OutFinalDrawArgs",  ctx.ResolveBuffer(hGraphFinalDrawArgs));
        pRC->ApplyContextStates().AssertSuccess();
        ctx.GetCommandEncoder()->DispatchIndirect(hDispatchArgs, 0).AssertSuccess(); });
  }
}


WResult WTerrainSystem::ReadbackVoxelData(WUInt32 uiIndex, WTempArray<VoxelGpuVertex>& out_verts, WDynamicArray<WUInt32>& out_indices, WUInt32& out_uiVertexCount, WUInt32& out_uiPrimitiveCount, WTime timeout)
{
  out_uiVertexCount = 0;
  out_uiPrimitiveCount = 0;

  if (uiIndex >= m_VoxelVolumes.GetCount())
    return W_FAILURE;

  auto& vol = m_VoxelVolumes[uiIndex];
  if (!vol.m_bInUse)
    return W_FAILURE;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  // The mesh scratch is shared and not persistent — always re-bake and read back within one sync graph.
  vol.m_bDirty = true;

  WGALReadbackBufferHelper countRB;
  WGALReadbackBufferHelper vertRB;
  WGALReadbackBufferHelper idxRB;

  auto pGraph = WRenderGraphManager::CreateRenderGraph("VoxelExportReadback", WRenderGraphPhase::PreRender);
  pGraph->Reset();

  UpdateVoxels(uiIndex, *pGraph);

  // Re-import the shared scratch (ImportBuffer dedupes by handle) to add the readback transfer pass.
  auto hGraphCounts = pGraph->ImportBuffer(m_hSharedMeshCounts);
  auto hGraphVerts = pGraph->ImportBuffer(m_hSharedMeshCompactVertices);
  auto hGraphIdx = pGraph->ImportBuffer(m_hSharedMeshIndices);

  const WGALBufferHandle hCounts = m_hSharedMeshCounts;
  const WGALBufferHandle hVerts = m_hSharedMeshCompactVertices;
  const WGALBufferHandle hIdx = m_hSharedMeshIndices;

  auto pass = pGraph->AddTransferPass("VoxelReadback");
  pass.ReadBuffer(hGraphCounts, WGALResourceState::CopySource);
  pass.ReadBuffer(hGraphVerts, WGALResourceState::CopySource);
  pass.ReadBuffer(hGraphIdx, WGALResourceState::CopySource);
  pass.HasSideEffects();
  pass.SetExecuteCallback([hCounts, hVerts, hIdx, &countRB, &vertRB, &idxRB](const WRenderGraphContext& ctx)
    {
      countRB.ReadbackBuffer(*ctx.GetCommandEncoder(), hCounts);
      vertRB.ReadbackBuffer(*ctx.GetCommandEncoder(), hVerts);
      idxRB.ReadbackBuffer(*ctx.GetCommandEncoder(), hIdx); });

  ExecuteGraphSync(*pGraph, pDevice);

  const WTime tDeadline = WTime::Now() + timeout;

  auto PollReadback = [&](WGALReadbackBufferHelper& readback, const char* szName) -> WResult
  {
    while (true)
    {
      const auto result = readback.GetReadbackResult(WTime::MakeFromMilliseconds(2));
      if (result == WGALAsyncResult::Expired)
      {
        WLog::Error("ReadbackVoxelData: {} readback expired for volume {}.", szName, uiIndex);
        return W_FAILURE;
      }
      if (result == WGALAsyncResult::Ready)
        return W_SUCCESS;
      if (WTime::Now() >= tDeadline)
      {
        WLog::Error("ReadbackVoxelData: timed out waiting for {} of volume {}.", szName, uiIndex);
        return W_FAILURE;
      }
    }
  };

  if (PollReadback(countRB, "counts").Failed())
    return W_FAILURE;
  if (PollReadback(vertRB, "vertices").Failed())
    return W_FAILURE;
  if (PollReadback(idxRB, "indices").Failed())
    return W_FAILURE;

  // Pull the counts first; they bound the vertex/index copies below.
  {
    WArrayPtr<const WUInt8> rawMemory;
    auto lock = countRB.LockBuffer(rawMemory);
    if (!lock)
      return W_FAILURE;
    const VoxelMeshCounts* pCounts = reinterpret_cast<const VoxelMeshCounts*>(rawMemory.GetPtr());
    out_uiVertexCount = pCounts->VertexCount;
    out_uiPrimitiveCount = pCounts->PrimitiveCount;
  }

  {
    WArrayPtr<const WUInt8> rawMemory;
    auto lock = vertRB.LockBuffer(rawMemory);
    if (lock)
    {
      out_verts.SetCountUninitialized(out_uiVertexCount);
      WMemoryUtils::Copy(out_verts.GetData(), reinterpret_cast<const VoxelGpuVertex*>(rawMemory.GetPtr()), out_uiVertexCount);
    }
  }

  {
    WArrayPtr<const WUInt8> rawMemory;
    auto lock = idxRB.LockBuffer(rawMemory);
    if (lock)
    {
      const WUInt32 uiIndexCount = out_uiPrimitiveCount * 3;
      out_indices.SetCountUninitialized(uiIndexCount);
      WMemoryUtils::Copy(out_indices.GetData(), reinterpret_cast<const WUInt32*>(rawMemory.GetPtr()), uiIndexCount);
    }
  }

  return W_SUCCESS;
}
