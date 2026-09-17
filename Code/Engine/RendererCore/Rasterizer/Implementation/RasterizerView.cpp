#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/Rasterizer.h>

WCVarInt cvar_SpatialCullingOcclusionMaxResolution("Spatial.Occlusion.MaxResolution", 512, WCVarFlags::Default, "Max resolution for occlusion buffers.");
WCVarInt cvar_SpatialCullingOcclusionMaxOccluders("Spatial.Occlusion.MaxOccluders", 64, WCVarFlags::Default, "Max number of occluders to rasterize per frame.");

WRasterizerView::WRasterizerView() = default;
WRasterizerView::~WRasterizerView() = default;

void WRasterizerView::SetResolution(WUInt32 uiWidth, WUInt32 uiHeight, float fAspectRatio)
{
  if (m_uiResolutionX != uiWidth || m_uiResolutionY != uiHeight)
  {
    m_uiResolutionX = uiWidth;
    m_uiResolutionY = uiHeight;

    m_pRasterizer = W_DEFAULT_NEW(Rasterizer, uiWidth, uiHeight);
  }

  if (fAspectRatio == 0.0f)
    m_fAspectRation = float(m_uiResolutionX) / float(m_uiResolutionY);
  else
    m_fAspectRation = fAspectRatio;
}

void WRasterizerView::BeginScene()
{
  W_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");

  W_PROFILE_SCOPE("WRasterizerView::BeginScene");

  m_pRasterizer->clear();
  m_bAnyOccludersRasterized = false;
}

void WRasterizerView::ReadBackFrame(WArrayPtr<WColorLinearUB> targetBuffer) const
{
  W_PROFILE_SCOPE("WRasterizerView::ReadBackFrame");

  W_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");
  W_ASSERT_DEV(targetBuffer.GetCount() >= m_uiResolutionX * m_uiResolutionY, "Target buffer is too small.");

  m_pRasterizer->readBackDepth(targetBuffer.GetPtr());
}

void WRasterizerView::EndScene()
{
  if (m_Instances.IsEmpty())
    return;

  W_PROFILE_SCOPE("WRasterizerView::EndScene");

  SortObjectsFrontToBack();

  UpdateViewProjectionMatrix();

  // only rasterize a limited number of the closest objects
  RasterizeObjects(cvar_SpatialCullingOcclusionMaxOccluders);

  m_Instances.Clear();

  m_pRasterizer->setModelViewProjection(m_mViewProjection.m_fElementsCM);
}

void WRasterizerView::RasterizeObjects(WUInt32 uiMaxObjects)
{
#if W_ENABLED(W_RASTERIZER_SUPPORTED)

  W_PROFILE_SCOPE("WRasterizerView::RasterizeObjects");

  for (const Instance& inst : m_Instances)
  {
    ApplyModelViewProjectionMatrix(inst.m_Transform);

    bool bNeedsClipping;
    const Occluder& occluder = inst.m_pObject->m_Occluder;

    if (m_pRasterizer->queryVisibility(occluder.m_boundsMin, occluder.m_boundsMax, bNeedsClipping))
    {
      m_bAnyOccludersRasterized = true;

      if (bNeedsClipping)
      {
        m_pRasterizer->rasterize<true>(occluder);
      }
      else
      {
        m_pRasterizer->rasterize<false>(occluder);
      }

      if (--uiMaxObjects == 0)
        return;
    }
  }
#endif
}

void WRasterizerView::UpdateViewProjectionMatrix()
{
  WMat4 mProjection;
  m_pCamera->GetProjectionMatrix(m_fAspectRation, mProjection, WCameraEye::Left, WClipSpaceDepthRange::ZeroToOne);

  m_mViewProjection = mProjection * m_pCamera->GetViewMatrix();
}

void WRasterizerView::ApplyModelViewProjectionMatrix(const WTransform& modelTransform)
{
  const WMat4 mModel = modelTransform.GetAsMat4();
  const WMat4 mMVP = m_mViewProjection * mModel;

  m_pRasterizer->setModelViewProjection(mMVP.m_fElementsCM);
}

void WRasterizerView::SortObjectsFrontToBack()
{
#if W_ENABLED(W_RASTERIZER_SUPPORTED)
  W_PROFILE_SCOPE("WRasterizerView::SortObjectsFrontToBack");

  const WVec3 camPos = m_pCamera->GetCenterPosition();

  m_Instances.Sort([&](const Instance& i1, const Instance& i2)
    {
      const float d1 = (i1.m_Transform.m_vPosition - camPos).GetLengthSquared();
      const float d2 = (i2.m_Transform.m_vPosition - camPos).GetLengthSquared();

      return d1 < d2; });
#endif
}

bool WRasterizerView::IsVisible(const WSimdBBox& aabb) const
{
#if W_ENABLED(W_RASTERIZER_SUPPORTED)
  if (!m_bAnyOccludersRasterized)
    return true; // assume that people already do frustum culling anyway

  WSimdVec4f vmin = aabb.m_Min;
  WSimdVec4f vmax = aabb.m_Max;

  // WSimdBBox makes no guarantees what's in the W component
  // but the SW rasterizer requires them to be 1
  vmin.SetW(1);
  vmax.SetW(1);

  bool needsClipping = false;
  return m_pRasterizer->queryVisibility(vmin.m_v, vmax.m_v, needsClipping);
#else
  return true;
#endif
}

WRasterizerView* WRasterizerViewPool::GetRasterizerView(WUInt32 uiWidth, WUInt32 uiHeight, float fAspectRatio)
{
  W_PROFILE_SCOPE("WRasterizerViewPool::GetRasterizerView");

  W_LOCK(m_Mutex);

  const float divX = (float)uiWidth / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float divY = (float)uiHeight / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float div = WMath::Max(divX, divY);

  if (div > 1.0)
  {
    uiWidth = (WUInt32)(uiWidth / div);
    uiHeight = (WUInt32)(uiHeight / div);
  }

  uiWidth = WMath::RoundDown(uiWidth, 8);
  uiHeight = WMath::RoundDown(uiHeight, 8);

  uiWidth = WMath::Clamp<WUInt32>(uiWidth, 32u, cvar_SpatialCullingOcclusionMaxResolution);
  uiHeight = WMath::Clamp<WUInt32>(uiHeight, 32u, cvar_SpatialCullingOcclusionMaxResolution);

  for (PoolEntry& entry : m_Entries)
  {
    if (entry.m_bInUse)
      continue;

    if (entry.m_RasterizerView.GetResolutionX() == uiWidth && entry.m_RasterizerView.GetResolutionY() == uiHeight)
    {
      entry.m_bInUse = true;
      entry.m_RasterizerView.SetResolution(uiWidth, uiHeight, fAspectRatio);
      return &entry.m_RasterizerView;
    }
  }

  auto& ne = m_Entries.ExpandAndGetRef();
  ne.m_RasterizerView.SetResolution(uiWidth, uiHeight, fAspectRatio);
  ne.m_bInUse = true;

  return &ne.m_RasterizerView;
}

void WRasterizerViewPool::ReturnRasterizerView(WRasterizerView* pView)
{
  if (pView == nullptr)
    return;

  W_PROFILE_SCOPE("WRasterizerViewPool::ReturnRasterizerView");

  pView->SetCamera(nullptr);

  W_LOCK(m_Mutex);

  for (PoolEntry& entry : m_Entries)
  {
    if (&entry.m_RasterizerView == pView)
    {
      entry.m_bInUse = false;
      return;
    }
  }

  W_ASSERT_NOT_IMPLEMENTED;
}


W_STATICLINK_FILE(RendererCore, RendererCore_Rasterizer_Implementation_RasterizerView);
