#include <RendererCore/RendererCorePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <RendererCore/BakedProbes/BakedProbesWorldModule.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>

// clang-format off
W_IMPLEMENT_WORLD_MODULE(WBakedProbesWorldModule);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBakedProbesWorldModule, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

WBakedProbesWorldModule::WBakedProbesWorldModule(WWorld* pWorld)
  : WWorldModule(pWorld)
{
}

WBakedProbesWorldModule::~WBakedProbesWorldModule() = default;

void WBakedProbesWorldModule::Initialize()
{
}

void WBakedProbesWorldModule::Deinitialize()
{
}

bool WBakedProbesWorldModule::HasProbeData() const
{
  return m_hProbeTree.IsValid();
}

WResult WBakedProbesWorldModule::GetProbeIndexData(const WVec3& vGlobalPosition, const WVec3& vNormal, ProbeIndexData& out_probeIndexData) const
{
  // TODO: optimize

  if (!HasProbeData())
    return W_FAILURE;

  WResourceLock<WProbeTreeSectorResource> pProbeTree(m_hProbeTree, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pProbeTree.GetAcquireResult() != WResourceAcquireResult::Final)
    return W_FAILURE;

  WSimdVec4f gridSpacePos = WSimdConversion::ToVec3((vGlobalPosition - pProbeTree->GetGridOrigin()).CompDiv(pProbeTree->GetProbeSpacing()));
  gridSpacePos = gridSpacePos.CompMax(WSimdVec4f::MakeZero());

  WSimdVec4f gridSpacePosFloor = gridSpacePos.Floor();
  WSimdVec4f weights = gridSpacePos - gridSpacePosFloor;

  WSimdVec4i maxIndices = WSimdVec4i(pProbeTree->GetProbeCount().x, pProbeTree->GetProbeCount().y, pProbeTree->GetProbeCount().z) - WSimdVec4i(1);
  WSimdVec4i pos0 = WSimdVec4i::Truncate(gridSpacePosFloor).CompMin(maxIndices);
  WSimdVec4i pos1 = (pos0 + WSimdVec4i(1)).CompMin(maxIndices);

  WUInt32 x0 = pos0.x();
  WUInt32 y0 = pos0.y();
  WUInt32 z0 = pos0.z();

  WUInt32 x1 = pos1.x();
  WUInt32 y1 = pos1.y();
  WUInt32 z1 = pos1.z();

  WUInt32 xCount = pProbeTree->GetProbeCount().x;
  WUInt32 xyCount = xCount * pProbeTree->GetProbeCount().y;

  out_probeIndexData.m_probeIndices[0] = z0 * xyCount + y0 * xCount + x0;
  out_probeIndexData.m_probeIndices[1] = z0 * xyCount + y0 * xCount + x1;
  out_probeIndexData.m_probeIndices[2] = z0 * xyCount + y1 * xCount + x0;
  out_probeIndexData.m_probeIndices[3] = z0 * xyCount + y1 * xCount + x1;
  out_probeIndexData.m_probeIndices[4] = z1 * xyCount + y0 * xCount + x0;
  out_probeIndexData.m_probeIndices[5] = z1 * xyCount + y0 * xCount + x1;
  out_probeIndexData.m_probeIndices[6] = z1 * xyCount + y1 * xCount + x0;
  out_probeIndexData.m_probeIndices[7] = z1 * xyCount + y1 * xCount + x1;

  WVec3 w1 = WSimdConversion::ToVec3(weights);
  WVec3 w0 = WVec3(1.0f) - w1;

  // TODO: add geometry factor to weight
  out_probeIndexData.m_probeWeights[0] = w0.x * w0.y * w0.z;
  out_probeIndexData.m_probeWeights[1] = w1.x * w0.y * w0.z;
  out_probeIndexData.m_probeWeights[2] = w0.x * w1.y * w0.z;
  out_probeIndexData.m_probeWeights[3] = w1.x * w1.y * w0.z;
  out_probeIndexData.m_probeWeights[4] = w0.x * w0.y * w1.z;
  out_probeIndexData.m_probeWeights[5] = w1.x * w0.y * w1.z;
  out_probeIndexData.m_probeWeights[6] = w0.x * w1.y * w1.z;
  out_probeIndexData.m_probeWeights[7] = w1.x * w1.y * w1.z;

  float weightSum = 0;
  for (WUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    weightSum += out_probeIndexData.m_probeWeights[i];
  }

  float normalizeFactor = 1.0f / weightSum;
  for (WUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    out_probeIndexData.m_probeWeights[i] *= normalizeFactor;
  }

  return W_SUCCESS;
}

WAmbientCube<float> WBakedProbesWorldModule::GetSkyVisibility(const ProbeIndexData& indexData) const
{
  // TODO: optimize

  WAmbientCube<float> result;

  WResourceLock<WProbeTreeSectorResource> pProbeTree(m_hProbeTree, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pProbeTree.GetAcquireResult() != WResourceAcquireResult::Final)
    return result;

  auto compressedSkyVisibility = pProbeTree->GetSkyVisibility();
  WAmbientCube<float> skyVisibility;

  for (WUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    WBakingUtils::DecompressSkyVisibility(compressedSkyVisibility[indexData.m_probeIndices[i]], skyVisibility);

    for (WUInt32 d = 0; d < WAmbientCubeBasis::NumDirs; ++d)
    {
      result.m_Values[d] += skyVisibility.m_Values[d] * indexData.m_probeWeights[i];
    }
  }

  return result;
}

void WBakedProbesWorldModule::SetProbeTreeResourcePrefix(const WHashedString& prefix)
{
  WStringBuilder sResourcePath;
  sResourcePath.SetFormat("{}_Global.WProbeTreeSector", prefix);

  m_hProbeTree = WResourceManager::LoadResource<WProbeTreeSectorResource>(sResourcePath);
}


W_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesWorldModule);
