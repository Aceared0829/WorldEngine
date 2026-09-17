#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Tasks/PlaceProbesTask.h>
#include <BakingPlugin/Tasks/SkyVisibilityTask.h>
#include <BakingPlugin/Tracer/TracerEmbree.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/BakedProbes/BakedProbesComponent.h>
#include <RendererCore/BakedProbes/BakedProbesVolumeComponent.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

WResult WBakingScene::Extract()
{
  m_Volumes.Clear();
  m_MeshObjects.Clear();
  m_BoundingBox = WBoundingBox::MakeInvalid();
  m_bIsBaked = false;

  const WWorld* pWorld = WWorld::GetWorld(m_uiWorldIndex);
  if (pWorld == nullptr)
  {
    return W_FAILURE;
  }

  const WWorld& world = *pWorld;
  W_LOCK(world.GetReadMarker());

  // settings
  {
    auto pManager = world.GetComponentManager<WBakedProbesComponentManager>();
    auto pComponent = pManager->GetSingletonComponent();

    m_Settings = pComponent->m_Settings;
  }

  // volumes
  {
    if (auto pManager = world.GetComponentManager<WBakedProbesVolumeComponentManager>())
    {
      for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
      {
        if (it->IsActiveAndInitialized())
        {
          WSimdTransform scaledTransform = it->GetOwner()->GetGlobalTransformSimd();
          scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(WSimdConversion::ToVec3(it->GetExtents())) * 0.5f;

          auto& volume = m_Volumes.ExpandAndGetRef();
          volume.m_GlobalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();

          WBoundingBoxSphere globalBounds = it->GetOwner()->GetGlobalBounds();
          if (globalBounds.IsValid())
          {
            m_BoundingBox.ExpandToInclude(globalBounds.GetBox());
          }
        }
      }
    }

    if (m_Volumes.IsEmpty())
    {
      WLog::Error("No Baked Probes Volume found");
      return W_FAILURE;
    }
  }

  WBoundingBox queryBox = m_BoundingBox;
  queryBox.Grow(WVec3(m_Settings.m_fMaxRayDistance));

  WTagSet excludeTags;
  excludeTags.SetByName("Editor");

  WSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = WDefaultSpatialDataCategories::RenderStatic.GetBitmask();
  queryParams.m_pExcludeTags = &excludeTags;

  WMsgExtractGeometry msg;
  msg.m_Mode = WWorldGeoExtractionUtil::ExtractionMode::RenderMesh;
  msg.m_pMeshObjects = &m_MeshObjects;

  world.GetSpatialSystem()->FindObjectsInBox(queryBox, queryParams,
    [&](WGameObject* pObject)
    {
      pObject->SendMessage(msg);

      return WVisitorExecution::Continue;
    });

  return W_SUCCESS;
}

WResult WBakingScene::Bake(const WStringView& sOutputPath, WProgress& progress)
{
  W_ASSERT_DEV(!WThreadUtils::IsMainThread(), "BakeScene must be executed on a worker thread");

  if (m_pTracer == nullptr)
  {
    m_pTracer = W_DEFAULT_NEW(WTracerEmbree);
  }

  WProgressRange pgRange("Baking Scene", 2, true, &progress);
  pgRange.SetStepWeighting(0, 0.95f);
  pgRange.SetStepWeighting(1, 0.05f);

  if (!pgRange.BeginNextStep("Building Scene"))
    return W_FAILURE;

  W_SUCCEED_OR_RETURN(m_pTracer->BuildScene(*this));

  WBakingInternal::PlaceProbesTask placeProbesTask(m_Settings, m_BoundingBox, m_Volumes);
  placeProbesTask.Execute();

  WBakingInternal::SkyVisibilityTask skyVisibilityTask(m_Settings, *m_pTracer, placeProbesTask.GetProbePositions());
  skyVisibilityTask.Execute();

  if (!pgRange.BeginNextStep("Writing Result"))
    return W_FAILURE;

  WStringBuilder sFullOutputPath = sOutputPath;
  sFullOutputPath.Append("_Global.WProbeTreeSector");

  WFileWriter file;
  W_SUCCEED_OR_RETURN(file.Open(sFullOutputPath));

  WAssetFileHeader header;
  header.SetFileHashAndVersion(1, 1);
  W_SUCCEED_OR_RETURN(header.Write(file));

  WProbeTreeSectorResourceDescriptor desc;
  desc.m_vGridOrigin = placeProbesTask.GetGridOrigin();
  desc.m_vProbeSpacing = m_Settings.m_vProbeSpacing;
  desc.m_vProbeCount = placeProbesTask.GetProbeCount();
  desc.m_ProbePositions = placeProbesTask.GetProbePositions();
  desc.m_SkyVisibility = skyVisibilityTask.GetSkyVisibility();

  W_SUCCEED_OR_RETURN(desc.Serialize(file));

  m_bIsBaked = true;

  return W_SUCCESS;
}

WResult WBakingScene::RenderDebugView(const WMat4& InverseViewProjection, WUInt32 uiWidth, WUInt32 uiHeight, WDynamicArray<WColorGammaUB>& out_Pixels,
  WProgress& progress) const
{
  if (!m_bIsBaked)
    return W_FAILURE;

  const WUInt32 uiNumPixel = uiWidth * uiHeight;
  out_Pixels.SetCountUninitialized(uiNumPixel);

  WHybridArray<WTracerInterface::Ray, 128> rays;
  rays.SetCountUninitialized(128);

  WHybridArray<WTracerInterface::Hit, 128> hits;
  hits.SetCountUninitialized(128);

  WUInt32 uiStartPixel = 0;
  WUInt32 uiPixelPerBatch = rays.GetCount();
  while (uiStartPixel < uiNumPixel)
  {
    uiPixelPerBatch = WMath::Min(uiPixelPerBatch, uiNumPixel - uiStartPixel);

    for (WUInt32 i = 0; i < uiPixelPerBatch; ++i)
    {
      WUInt32 uiPixelIndex = uiStartPixel + i;
      WUInt32 x = uiPixelIndex % uiWidth;
      WUInt32 y = uiPixelIndex / uiWidth;

      auto& ray = rays[i];
      WGraphicsUtils::ConvertScreenPosToWorldPos(InverseViewProjection, 0, 0, uiWidth, uiHeight, WVec3(float(x), float(y), 0), ray.m_vStartPos, &ray.m_vDir).IgnoreResult();
      ray.m_fDistance = 1000.0f;
    }

    m_pTracer->TraceRays(rays, hits);

    for (WUInt32 i = 0; i < uiPixelPerBatch; ++i)
    {
      WUInt32 uiPixelIndex = uiStartPixel + i;

      auto& hit = hits[i];
      if (hit.m_fDistance >= 0.0f)
      {
        WVec3 normal = hit.m_vNormal * 0.5f + WVec3(0.5f);
        out_Pixels[uiPixelIndex] = WColorGammaUB(WMath::ColorFloatToByte(normal.x), WMath::ColorFloatToByte(normal.y), WMath::ColorFloatToByte(normal.z));
      }
      else
      {
        out_Pixels[uiPixelIndex] = WColorGammaUB(0, 0, 0);
      }
    }

    uiStartPixel += uiPixelPerBatch;

    progress.SetCompletion((float)uiStartPixel / uiNumPixel);
    if (progress.WasCanceled())
      break;
  }

  return W_SUCCESS;
}

WBakingScene::WBakingScene() = default;
WBakingScene::~WBakingScene() = default;

//////////////////////////////////////////////////////////////////////////

namespace
{
  static WDynamicArray<WUniquePtr<WBakingScene>, WStaticsAllocatorWrapper> s_BakingScenes;
}

W_IMPLEMENT_SINGLETON(WBaking);

WBaking::WBaking()
  : m_SingletonRegistrar(this)
{
}

void WBaking::Startup()
{
}

void WBaking::Shutdown()
{
  s_BakingScenes.Clear();
}

WBakingScene* WBaking::GetOrCreateScene(const WWorld& world)
{
  const WUInt32 uiWorldIndex = world.GetIndex();

  s_BakingScenes.EnsureCount(uiWorldIndex + 1);
  if (s_BakingScenes[uiWorldIndex] == nullptr)
  {
    auto pScene = W_DEFAULT_NEW(WBakingScene);
    pScene->m_uiWorldIndex = uiWorldIndex;

    s_BakingScenes[uiWorldIndex] = pScene;
  }

  return s_BakingScenes[uiWorldIndex].Borrow();
}

WBakingScene* WBaking::GetScene(const WWorld& world)
{
  const WUInt32 uiWorldIndex = world.GetIndex();

  if (uiWorldIndex < s_BakingScenes.GetCount())
  {
    return s_BakingScenes[uiWorldIndex].Borrow();
  }

  return nullptr;
}

const WBakingScene* WBaking::GetScene(const WWorld& world) const
{
  const WUInt32 uiWorldIndex = world.GetIndex();

  if (uiWorldIndex < s_BakingScenes.GetCount())
  {
    return s_BakingScenes[uiWorldIndex].Borrow();
  }

  return nullptr;
}

WResult WBaking::RenderDebugView(const WWorld& world, const WMat4& InverseViewProjection, WUInt32 uiWidth, WUInt32 uiHeight, WDynamicArray<WColorGammaUB>& out_Pixels, WProgress& progress) const
{
  if (const WBakingScene* pScene = GetScene(world))
  {
    return pScene->RenderDebugView(InverseViewProjection, uiWidth, uiHeight, out_Pixels, progress);
  }

  return W_FAILURE;
}
