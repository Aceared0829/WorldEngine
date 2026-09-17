#include <RendererCore/RendererCorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Graphics/Camera.h>
#include <Core/Utils/Blackboard.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/DynamicTextureAtlas.h>
#include <RendererCore/Utils/CoreRenderProfile.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <Shaders/Common/LightData.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ShadowPool)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderWorld"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WShadowPool::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WShadowPool::OnEngineShutdown();
  }
W_END_SUBSYSTEM_DECLARATION;
// clang-format on

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
WCVarBool cvar_RenderingShadowsShowAtlasTexture("Rendering.Shadows.ShowAtlasTexture", false, WCVarFlags::Default, "Display the shadow atlas texture");
WCVarBool cvar_RenderingShadowsVisCascadeBounds("Rendering.Shadows.VisCascadeBounds", false, WCVarFlags::Default, "Visualizes the bounding volumes of shadow cascades");
#endif

WCVarFloat cvar_RenderingShadowsScaleMappingExponent("Rendering.Shadows.ScaleMappingExponent", 1.5f, WCVarFlags::Default, "Determines how fast the shadow map size is reduced with screen space size");

/// NOTE: The default values for these are defined in WCoreRenderProfileConfig
///       but they can also be overwritten in custom game states at startup.
W_RENDERERCORE_DLL WCVarInt cvar_RenderingShadowsAtlasSize("Rendering.Shadows.AtlasSize", 4096, WCVarFlags::RequiresDelayedSync, "The size of the shadow atlas texture.");
W_RENDERERCORE_DLL WCVarInt cvar_RenderingShadowsMaxShadowMapSize("Rendering.Shadows.MaxShadowMapSize", 1024, WCVarFlags::RequiresDelayedSync, "The max shadow map size used.");
W_RENDERERCORE_DLL WCVarInt cvar_RenderingShadowsMinShadowMapSize("Rendering.Shadows.MinShadowMapSize", 64, WCVarFlags::RequiresDelayedSync, "The min shadow map size used.");

static WUInt32 s_uiLastConfigModification = 0;
static float s_fMinRelativeShadowMapSize = 0.0f;

struct ShadowView
{
  WViewHandle m_hView;
  WCamera m_Camera;
  WCamera m_CullingCamera;
};

struct ShadowData
{
  WHybridArray<WViewHandle, 6> m_Views;
  WUInt32 m_uiType;
  float m_fShadowMapScale;
  float m_fPenumbraSize;
  float m_fSlopeBias;
  float m_fConstantBias;
  float m_fFadeOutStart;
  float m_fMinRange;
  float m_fActualRange;
  WUInt32 m_uiPackedDataOffset; // in 16 bytes steps
};

struct LightAndRefView
{
  W_DECLARE_POD_TYPE();

  const WLightComponent* m_pLight;
  const WView* m_pReferenceView;
};

struct SortedShadowData
{
  W_DECLARE_POD_TYPE();

  WUInt32 m_uiIndex;
  float m_fShadowMapScale;

  W_ALWAYS_INLINE bool operator<(const SortedShadowData& other) const
  {
    if (m_fShadowMapScale > other.m_fShadowMapScale) // we want to sort descending (higher scale first)
      return true;

    return m_uiIndex < other.m_uiIndex;
  }
};

static WDynamicArray<SortedShadowData> s_SortedShadowData;

static float ShadowMapScaleFromScreenSpaceSize(float fScreenSpaceSize)
{
  const float fClampedShadowMapScale = WMath::Clamp(WMath::Pow(fScreenSpaceSize * 0.5f, cvar_RenderingShadowsScaleMappingExponent), s_fMinRelativeShadowMapSize, 1.0f);
  return fClampedShadowMapScale;
}

static float AddSafeBorder(WAngle fov, float fPenumbraSize)
{
  float fHalfHeight = WMath::Tan(fov * 0.5f);
  float fNewFov = WMath::ATan(fHalfHeight + fPenumbraSize).GetDegree() * 2.0f;
  return fNewFov;
}

WTagSet s_ExcludeTagsWhiteList;

static void CopyExcludeTagsOnWhiteList(const WTagSet& referenceTags, WTagSet& out_targetTags)
{
  out_targetTags.Clear();
  out_targetTags.SetByName("EditorHidden");

  for (auto& tag : referenceTags)
  {
    if (s_ExcludeTagsWhiteList.IsSet(tag))
    {
      out_targetTags.Set(tag);
    }
  }
}

// must not be in anonymous namespace
template <>
struct WHashHelper<LightAndRefView>
{
  W_ALWAYS_INLINE static WUInt32 Hash(LightAndRefView value) { return WHashingUtils::xxHash32(&value.m_pLight, sizeof(LightAndRefView)); }

  W_ALWAYS_INLINE static bool Equal(const LightAndRefView& a, const LightAndRefView& b)
  {
    return a.m_pLight == b.m_pLight && a.m_pReferenceView == b.m_pReferenceView;
  }
};

//////////////////////////////////////////////////////////////////////////

struct WShadowPool::Data
{
  Data() { Clear(); }

  ~Data()
  {
    for (auto& shadowView : m_ShadowViews)
    {
      WRenderWorld::DeleteView(shadowView.m_hView);
    }

    m_TextureAtlas.Deinitialize();

    WGALDevice::GetDefaultDevice()->DestroyBuffer(m_hShadowDataBuffer);
  }

  enum
  {
    MAX_SHADOW_DATA = 1024
  };

  void CreateShadowAtlasTexture()
  {
    if (m_TextureAtlas.IsInitialized() == false)
    {
      // use the current CVar values to initialize the values
      WUInt32 uiAtlas = cvar_RenderingShadowsAtlasSize;
      WUInt32 uiMax = cvar_RenderingShadowsMaxShadowMapSize;
      WUInt32 uiMin = cvar_RenderingShadowsMinShadowMapSize;

      // if the platform profile has changed, use it to reset the defaults
      if (s_uiLastConfigModification != WGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetLastModificationCounter())
      {
        s_uiLastConfigModification = WGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetLastModificationCounter();

        const auto* pConfig = WGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<WCoreRenderProfileConfig>();

        uiAtlas = pConfig->m_uiShadowAtlasTextureSize;
        uiMax = pConfig->m_uiMaxShadowMapSize;
        uiMin = pConfig->m_uiMinShadowMapSize;
      }

      // if the CVars were modified recently (e.g. during game startup), use those values to override the default
      if (cvar_RenderingShadowsAtlasSize.HasDelayedSyncValueChanged())
        uiAtlas = cvar_RenderingShadowsAtlasSize.GetValue(WCVarValue::DelayedSync);

      if (cvar_RenderingShadowsMaxShadowMapSize.HasDelayedSyncValueChanged())
        uiMax = cvar_RenderingShadowsMaxShadowMapSize.GetValue(WCVarValue::DelayedSync);

      if (cvar_RenderingShadowsMinShadowMapSize.HasDelayedSyncValueChanged())
        uiMin = cvar_RenderingShadowsMinShadowMapSize.GetValue(WCVarValue::DelayedSync);

      // make sure the values are valid
      uiMax = WMath::Clamp(WMath::PowerOfTwo_Floor(uiMax), 64u, 2048u);
      uiMin = WMath::Clamp(WMath::PowerOfTwo_Floor(uiMin), 8u, 512u);

      uiMax = WMath::Max(uiMin, uiMax);
      uiMin = WMath::Min(uiMin, uiMax);

      uiAtlas = WMath::Clamp(static_cast<WUInt32>(WMath::RoundToMultiple(double(uiAtlas), uiMax)), uiMax, 8192u);

      // write back the clamped values, so that everyone sees the valid values
      cvar_RenderingShadowsAtlasSize = uiAtlas;
      cvar_RenderingShadowsMaxShadowMapSize = uiMax;
      cvar_RenderingShadowsMinShadowMapSize = uiMin;

      // apply the new values
      cvar_RenderingShadowsAtlasSize.SetToDelayedSyncValue();
      cvar_RenderingShadowsMaxShadowMapSize.SetToDelayedSyncValue();
      cvar_RenderingShadowsMinShadowMapSize.SetToDelayedSyncValue();

      WGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(cvar_RenderingShadowsAtlasSize, cvar_RenderingShadowsAtlasSize, WGALResourceFormat::D16);

      m_TextureAtlas.Initialize(desc).AssertSuccess("Failed to initialize shadow atlas");

      s_fMinRelativeShadowMapSize = (cvar_RenderingShadowsMinShadowMapSize - 1.0f) / cvar_RenderingShadowsMaxShadowMapSize;
    }
  }

  void CreateShadowDataBuffer()
  {
    if (m_hShadowDataBuffer.IsInvalidated())
    {
      WGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(WVec4);
      desc.m_uiTotalSize = desc.m_uiStructSize * MAX_SHADOW_DATA;
      desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hShadowDataBuffer = WGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    }
  }

  WViewHandle CreateShadowView()
  {
    CreateShadowAtlasTexture();
    CreateShadowDataBuffer();

    WView* pView = nullptr;
    WViewHandle hView = WRenderWorld::CreateView("Unknown", pView);

    pView->SetCameraUsageHint(WCameraUsageHint::Shadow);

    WGALRenderTargets renderTargets;
    renderTargets.m_hDSTarget = m_TextureAtlas.GetTexture();
    pView->SetRenderTargets(renderTargets);

    W_ASSERT_DEV(m_ShadowViewsMutex.IsLocked(), "m_ShadowViewsMutex must be locked at this point.");
    m_ShadowViewsMutex.Unlock(); // if the resource gets loaded in the call below, his could lead to a deadlock

    // ShadowMapRenderPipeline.WRenderPipelineAsset
    pView->SetRenderPipelineResource(WResourceManager::LoadResource<WRenderPipelineResource>("{ 4f4d9f16-3d47-4c67-b821-a778f11dcaf5 }"));

    m_ShadowViewsMutex.Lock();

    // Set viewport size to something valid, this will be changed to the proper location in the atlas texture in OnEndExtraction before
    // rendering.
    pView->SetViewport(WRectFloat(0.0f, 0.0f, 1024.0f, 1024.0f));

    pView->m_IncludeTags.SetByName("CastShadow");
    pView->m_ExcludeTags.SetByName("EditorHidden");

    pView->SetBlackboard(WBlackboard::Create("ShadowViewBlackboard"));

    return hView;
  }

  ShadowView& GetShadowView(WView*& out_pView)
  {
    W_LOCK(m_ShadowViewsMutex);

    if (m_uiUsedViews == m_ShadowViews.GetCount())
    {
      m_ShadowViews.ExpandAndGetRef().m_hView = CreateShadowView();
    }

    auto& shadowView = m_ShadowViews[m_uiUsedViews];
    if (WRenderWorld::TryGetView(shadowView.m_hView, out_pView))
    {
      out_pView->SetCamera(&shadowView.m_Camera);
      out_pView->SetCullingCamera(nullptr);
      out_pView->SetLodCamera(nullptr);
    }

    m_uiUsedViews++;
    return shadowView;
  }

  bool GetDataForExtraction(const WLightComponent* pLight, const WView* pReferenceView, float fShadowMapScale, WUInt32 uiPackedDataSizeInBytes, ShadowData*& out_pData)
  {
    W_LOCK(m_ShadowDataMutex);

    LightAndRefView key = {pLight, pReferenceView};

    WUInt32 uiDataIndex = WInvalidIndex;
    if (m_LightToShadowDataTable.TryGetValue(key, uiDataIndex))
    {
      out_pData = &m_ShadowData[uiDataIndex];
      out_pData->m_fShadowMapScale = WMath::Max(out_pData->m_fShadowMapScale, fShadowMapScale);
      return true;
    }

    m_ShadowData.EnsureCount(m_uiUsedShadowData + 1);

    out_pData = &m_ShadowData[m_uiUsedShadowData];
    out_pData->m_fShadowMapScale = fShadowMapScale;
    out_pData->m_fPenumbraSize = pLight->GetPenumbraSize();
    out_pData->m_fSlopeBias = pLight->GetSlopeBias() * 100.0f;       // map from user friendly range to real range
    out_pData->m_fConstantBias = pLight->GetConstantBias() / 100.0f; // map from user friendly range to real range
    out_pData->m_fFadeOutStart = 1.0f;
    out_pData->m_fMinRange = 1.0f;
    out_pData->m_fActualRange = 1.0f;
    out_pData->m_uiPackedDataOffset = m_uiUsedPackedShadowData;

    m_LightToShadowDataTable.Insert(key, m_uiUsedShadowData);

    ++m_uiUsedShadowData;
    m_uiUsedPackedShadowData += uiPackedDataSizeInBytes / sizeof(WVec4);

    return false;
  }

  void Clear()
  {
    m_uiUsedViews = 0;
    m_uiUsedShadowData = 0;

    m_LightToShadowDataTable.Clear();

    m_uiUsedPackedShadowData = 0;
  }

  WMutex m_ShadowViewsMutex;
  WDeque<ShadowView> m_ShadowViews;
  WUInt32 m_uiUsedViews = 0;

  WMutex m_ShadowDataMutex;
  WDeque<ShadowData> m_ShadowData;
  WUInt32 m_uiUsedShadowData = 0;
  WHashTable<LightAndRefView, WUInt32> m_LightToShadowDataTable;

  WDynamicArray<WVec4, WAlignedAllocatorWrapper> m_PackedShadowData[2];
  WUInt32 m_uiUsedPackedShadowData = 0; // in 16 bytes steps (sizeof(WVec4))

  WDynamicTextureAtlas m_TextureAtlas;
  WGALBufferHandle m_hShadowDataBuffer;

  WSharedPtr<WRenderGraph> m_pRenderGraph;
};

//////////////////////////////////////////////////////////////////////////

WShadowPool::Data* WShadowPool::s_pData = nullptr;

// static
WUInt32 WShadowPool::AddDirectionalLight(const WDirectionalLightComponent* pDirLight, const WView* pReferenceView)
{
  W_ASSERT_DEBUG(pDirLight->GetCastShadows(), "Implementation error");

  // No shadows in orthographic views
  if (pReferenceView->GetCullingCamera()->IsOrthographic())
  {
    return WInvalidIndex;
  }

  float fMaxReferenceSize = WMath::Max(pReferenceView->GetViewport().width, pReferenceView->GetViewport().height);
  float fShadowMapScale = WMath::Clamp(fMaxReferenceSize / cvar_RenderingShadowsMaxShadowMapSize, s_fMinRelativeShadowMapSize, 10.0f);

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pDirLight, pReferenceView, fShadowMapScale, sizeof(WDirShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  WUInt32 uiNumCascades = WMath::Min(pDirLight->GetNumCascades(), 4u);
  const WCamera* pReferenceCamera = pReferenceView->GetCullingCamera();

  pData->m_uiType = LIGHT_TYPE_DIR;
  pData->m_fFadeOutStart = pDirLight->GetFadeOutStart();
  pData->m_fMinRange = pDirLight->GetMinShadowRange();
  pData->m_Views.SetCount(uiNumCascades);

  // determine cascade ranges
  float fNearPlane = pReferenceCamera->GetNearPlane();
  float fShadowRange = pDirLight->GetMinShadowRange();
  float fSplitModeWeight = pDirLight->GetSplitModeWeight();

  float fCascadeRanges[4];
  for (WUInt32 i = 0; i < uiNumCascades; ++i)
  {
    float f = float(i + 1) / uiNumCascades;
    float logDistance = fNearPlane * WMath::Pow(fShadowRange / fNearPlane, f);
    float linearDistance = fNearPlane + (fShadowRange - fNearPlane) * f;
    fCascadeRanges[i] = WMath::Lerp(linearDistance, logDistance, fSplitModeWeight);
  }

  const WStringView viewNames[4] = {"-C0"_wsv, "-C1"_wsv, "-C2"_wsv, "-C3"_wsv};
  WStringBuilder tmp;

  const WGameObject* pOwner = pDirLight->GetOwner();
  const WVec3 vLightDirForwards = pOwner->GetGlobalDirForwards();
  const WVec3 vLightDirUp = pOwner->GetGlobalDirUp();

  float fAspectRatio = pReferenceView->GetViewport().width / pReferenceView->GetViewport().height;

  float fCascadeStart = 0.0f;
  float fCascadeEnd = 0.0f;
  float fTanFovX = WMath::Tan(pReferenceCamera->GetFovX(fAspectRatio) * 0.5f);
  float fTanFovY = WMath::Tan(pReferenceCamera->GetFovY(fAspectRatio) * 0.5f);
  WVec3 corner = WVec3(fTanFovX, fTanFovY, 1.0f);

  float fNearPlaneOffset = pDirLight->GetNearPlaneOffset();

  for (WUInt32 i = 0; i < uiNumCascades; ++i)
  {
    WView* pView = nullptr;
    ShadowView& shadowView = s_pData->GetShadowView(pView);
    pData->m_Views[i] = shadowView.m_hView;

    // Setup view
    {
      if (pOwner->GetName().IsEmpty())
      {
        tmp.Set("Dir", viewNames[i]);
      }
      else
      {
        tmp.Set(pOwner->GetName(), viewNames[i]);
      }

      pView->SetName(tmp);

      pView->SetWorld(const_cast<WWorld*>(pDirLight->GetWorld()));
      pView->SetCullingCamera(&shadowView.m_CullingCamera);
      pView->SetLodCamera(pReferenceCamera);
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("ShadowDepth.RenderTransparentObjects"), pDirLight->GetTransparentShadows());
      CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
    }

    // Setup camera
    {
      fCascadeStart = fCascadeEnd;

      // Make sure that the last cascade always covers the whole range so effects like e.g. particles can get away with
      // sampling only the last cascade.
      if (i == uiNumCascades - 1)
      {
        fCascadeStart = 0.0f;
      }

      fCascadeEnd = fCascadeRanges[i];

      WVec3 startCorner = corner * fCascadeStart;
      WVec3 endCorner = corner * fCascadeEnd;
      pData->m_fActualRange = endCorner.GetLength();

      // Find the enclosing sphere for the frustum:
      // The sphere center must be on the view's center ray and should be equally far away from the corner points.
      // x = distance from camera origin to sphere center
      // d1^2 = sc.x^2 + sc.y^2 + (x - sc.z)^2
      // d2^2 = ec.x^2 + ec.y^2 + (x - ec.z)^2
      // d1 == d2 and solve for x:
      float x = (endCorner.Dot(endCorner) - startCorner.Dot(startCorner)) / (2.0f * (endCorner.z - startCorner.z));
      x = WMath::Min(x, fCascadeEnd);

      WVec3 center = pReferenceCamera->GetPosition() + pReferenceCamera->GetDirForwards() * x;

      // prevent too large values
      // sometimes this can happen when imported data is badly scaled and thus way too large
      // then adding dirForwards result in no change and we run into other asserts later
      center.x = WMath::Clamp(center.x, -1000000.0f, +1000000.0f);
      center.y = WMath::Clamp(center.y, -1000000.0f, +1000000.0f);
      center.z = WMath::Clamp(center.z, -1000000.0f, +1000000.0f);

      endCorner.z -= x;
      const float radius = endCorner.GetLength();

      const float fCameraToCenterDistance = radius + fNearPlaneOffset;
      const WVec3 shadowCameraPos = center - vLightDirForwards * fCameraToCenterDistance;
      const float fFarPlane = radius + fCameraToCenterDistance;

      WCamera& camera = shadowView.m_Camera;
      camera.LookAt(shadowCameraPos, center, vLightDirUp);
      camera.SetCameraMode(WCameraMode::OrthoFixedWidth, radius * 2.0f, 0.0f, fFarPlane);

      // stabilize
      const WMat4 worldToLightMatrix = pView->GetViewMatrix(WCameraEye::Left);
      const float texelInWorld = (2.0f * radius) / cvar_RenderingShadowsMaxShadowMapSize;
      WVec3 offset = worldToLightMatrix.TransformPosition(WVec3::MakeZero());
      offset.x -= WMath::Floor(offset.x / texelInWorld) * texelInWorld;
      offset.y -= WMath::Floor(offset.y / texelInWorld) * texelInWorld;

      camera.MoveLocally(0.0f, offset.x, offset.y);

      // culling camera with pulled back near plane
      WCamera& cullingCamera = shadowView.m_CullingCamera;
      cullingCamera = camera;
      cullingCamera.SetCameraMode(WCameraMode::OrthoFixedWidth, radius * 2.0f, -pReferenceCamera->GetFarPlane(), fFarPlane);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      if (cvar_RenderingShadowsVisCascadeBounds)
      {
        WDebugRenderer::DrawLineSphere(pReferenceView->GetHandle(), WBoundingSphere::MakeFromCenterAndRadius(center, radius), WColorScheme::LightUI(WColorScheme::Orange));

        WDebugRenderer::DrawLineSphere(pReferenceView->GetHandle(), WBoundingSphere::MakeFromCenterAndRadius(WVec3(0, 0, fCameraToCenterDistance), radius), WColorScheme::LightUI(WColorScheme::Red), WTransform::MakeFromMat4(camera.GetViewMatrix().GetInverse()));
      }
#endif
    }

    WRenderWorld::AddViewDependency(*pReferenceView, GetShadowAtlasTexture(), WGALResourceState::ShaderResource);
    WRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

// static
WUInt32 WShadowPool::AddPointLight(const WPointLightComponent* pPointLight, float fScreenSpaceSize, const WView* pReferenceView)
{
  W_ASSERT_DEBUG(pPointLight->GetCastShadows(), "Implementation error");

  // point lights use a lot of atlas space thus we half the scale
  const float fShadowMapScale = ShadowMapScaleFromScreenSpaceSize(fScreenSpaceSize) * 0.5f;
  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pPointLight, nullptr, fShadowMapScale, sizeof(WPointShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_POINT;
  pData->m_Views.SetCount(6);

  WVec3 faceDirs[6] = {
    WVec3(1.0f, 0.0f, 0.0f),
    WVec3(-1.0f, 0.0f, 0.0f),
    WVec3(0.0f, 1.0f, 0.0f),
    WVec3(0.0f, -1.0f, 0.0f),
    WVec3(0.0f, 0.0f, 1.0f),
    WVec3(0.0f, 0.0f, -1.0f),
  };

  const WStringView viewNames[6] = {
    "+X"_wsv,
    "-X"_wsv,
    "+Y"_wsv,
    "-Y"_wsv,
    "+Z"_wsv,
    "-Z"_wsv,
  };

  const WGameObject* pOwner = pPointLight->GetOwner();
  WVec3 vPosition = pOwner->GetGlobalPosition();
  WVec3 vUp = WVec3(0.0f, 0.0f, 1.0f);

  float fPenumbraSize = WMath::Max(pPointLight->GetPenumbraSize(), (0.5f / cvar_RenderingShadowsMinShadowMapSize)); // at least one texel for hardware pcf
  float fFov = AddSafeBorder(WAngle::MakeFromDegree(90.0f), fPenumbraSize);

  ///\todo expose somewhere
  float fNearPlane = 0.1f;
  float fFarPlane = pPointLight->GetEffectiveRange();

  WStringBuilder tmp;

  for (WUInt32 i = 0; i < 6; ++i)
  {
    WView* pView = nullptr;
    ShadowView& shadowView = s_pData->GetShadowView(pView);
    pData->m_Views[i] = shadowView.m_hView;


    // Setup view
    {
      if (pOwner->GetName().IsEmpty())
      {
        tmp.Set("Point", viewNames[i]);
      }
      else
      {
        tmp.Set(pOwner->GetName(), viewNames[i]);
      }

      pView->SetName(tmp);

      pView->SetWorld(const_cast<WWorld*>(pPointLight->GetWorld()));
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("ShadowDepth.RenderTransparentObjects"), pPointLight->GetTransparentShadows());
      CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
    }

    // Setup camera
    {
      WVec3 vForward = faceDirs[i];

      WCamera& camera = shadowView.m_Camera;
      camera.LookAt(vPosition, vPosition + vForward, vUp);
      camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, fFov, fNearPlane, fFarPlane);
    }

    WRenderWorld::AddViewDependency(*pReferenceView, GetShadowAtlasTexture(), WGALResourceState::ShaderResource);
    WRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

// static
WUInt32 WShadowPool::AddSpotLight(const WSpotLightComponent* pSpotLight, float fScreenSpaceSize, const WView* pReferenceView)
{
  W_ASSERT_DEBUG(pSpotLight->GetCastShadows(), "Implementation error");

  const float fShadowMapScale = ShadowMapScaleFromScreenSpaceSize(fScreenSpaceSize);
  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pSpotLight, nullptr, fShadowMapScale, sizeof(WSpotShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_SPOT;
  pData->m_Views.SetCount(1);

  WView* pView = nullptr;
  ShadowView& shadowView = s_pData->GetShadowView(pView);
  pData->m_Views[0] = shadowView.m_hView;

  const WGameObject* pOwner = pSpotLight->GetOwner();

  // Setup view
  {
    if (pOwner->GetName().IsEmpty())
    {
      pView->SetName("Spot"_wsv);
    }
    else
    {
      pView->SetName(pOwner->GetName());
    }

    pView->SetWorld(const_cast<WWorld*>(pSpotLight->GetWorld()));
    pView->GetBlackboard()->SetEntryValue(WMakeHashedString("ShadowDepth.RenderTransparentObjects"), pSpotLight->GetTransparentShadows());
    CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
  }

  // Setup camera
  {
    const WGameObject* pOwner = pSpotLight->GetOwner();
    WVec3 vPosition = pOwner->GetGlobalPosition();
    WVec3 vForward = pOwner->GetGlobalDirForwards();
    WVec3 vUp = pOwner->GetGlobalDirUp();

    float fFov = AddSafeBorder(pSpotLight->GetOuterSpotAngle(), pSpotLight->GetPenumbraSize());
    float fNearPlane = 0.1f; ///\todo expose somewhere
    float fFarPlane = pSpotLight->GetEffectiveRange();

    WCamera& camera = shadowView.m_Camera;
    camera.LookAt(vPosition, vPosition + vForward, vUp);
    camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, fFov, fNearPlane, fFarPlane);
  }
  WRenderWorld::AddViewDependency(*pReferenceView, GetShadowAtlasTexture(), WGALResourceState::ShaderResource);
  WRenderWorld::AddViewToRender(shadowView.m_hView);

  return pData->m_uiPackedDataOffset;
}

// static
WGALTextureHandle WShadowPool::GetShadowAtlasTexture()
{
  return s_pData->m_TextureAtlas.GetTexture();
}

// static
WGALBufferHandle WShadowPool::GetShadowDataBuffer()
{
  return s_pData->m_hShadowDataBuffer;
}

// static
void WShadowPool::AddExcludeTagToWhiteList(const WTag& tag)
{
  s_ExcludeTagsWhiteList.Set(tag);
}

// static
void WShadowPool::OnEngineStartup()
{
  s_pData = W_DEFAULT_NEW(WShadowPool::Data);

  WRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  WRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void WShadowPool::OnEngineShutdown()
{
  WRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  WRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  W_DEFAULT_DELETE(s_pData);
}

// static
void WShadowPool::OnExtractionEvent(const WRenderWorldExtractionEvent& e)
{
  if (e.m_Type != WRenderWorldExtractionEvent::Type::EndExtraction)
    return;

  W_PROFILE_SCOPE("Shadow Pool Update");

  WUInt32 uiDataIndex = WRenderWorld::GetDataIndexForExtraction();
  auto& packedShadowData = s_pData->m_PackedShadowData[uiDataIndex];
  packedShadowData.SetCountUninitialized(s_pData->m_uiUsedPackedShadowData);

  if (s_pData->m_uiUsedShadowData == 0)
    return;

  // Sort by shadow map scale
  s_SortedShadowData.Clear();

  for (WUInt32 uiShadowDataIndex = 0; uiShadowDataIndex < s_pData->m_uiUsedShadowData; ++uiShadowDataIndex)
  {
    auto& shadowData = s_pData->m_ShadowData[uiShadowDataIndex];

    auto& sorted = s_SortedShadowData.ExpandAndGetRef();
    sorted.m_uiIndex = uiShadowDataIndex;
    sorted.m_fShadowMapScale = shadowData.m_fShadowMapScale;
  }

  s_SortedShadowData.Sort();

  // Prepare atlas
  s_pData->m_TextureAtlas.Clear();

  float fAtlasInvWidth = 1.0f / cvar_RenderingShadowsAtlasSize;
  float fAtlasInvHeight = 1.0f / cvar_RenderingShadowsAtlasSize;

  for (auto& sorted : s_SortedShadowData)
  {
    WUInt32 uiShadowDataIndex = sorted.m_uiIndex;
    auto& shadowData = s_pData->m_ShadowData[uiShadowDataIndex];

    WUInt32 uiMaxShadowMapSize = cvar_RenderingShadowsMaxShadowMapSize;
    const WUInt32 uiShadowMapSize = WMath::PowerOfTwo_Ceil((WUInt32)(uiMaxShadowMapSize * WMath::Saturate(shadowData.m_fShadowMapScale)));

    WTempHybridArray<WView*, 8> shadowViews;
    WTempHybridArray<WRectU16, 8> atlasRects;

    // Fill atlas
    for (WUInt32 uiViewIndex = 0; uiViewIndex < shadowData.m_Views.GetCount(); ++uiViewIndex)
    {
      WView* pShadowView = nullptr;
      WRenderWorld::TryGetView(shadowData.m_Views[uiViewIndex], pShadowView);
      shadowViews.PushBack(pShadowView);

      W_ASSERT_DEV(pShadowView != nullptr, "Implementation error");

      WRectU16 atlasRect = WRectU16::MakeZero();
      auto allocationId = s_pData->m_TextureAtlas.Allocate(uiShadowMapSize, uiShadowMapSize, pShadowView->GetName(), &atlasRect);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      if (allocationId.IsInvalidated())
      {
        static WUInt8 s_uiWarnCounter = 0;
        if (s_uiWarnCounter == 0)
        {
          WLog::Warning("Shadow Pool is full. Not enough space for a {0}x{0} shadow map. The light will have no shadow.", uiShadowMapSize);
        }

        ++s_uiWarnCounter; // rely on integer overflow to reach 0 again
      }
#endif

      atlasRects.PushBack(atlasRect);

      pShadowView->SetViewport(WRectFloat((float)atlasRect.x, (float)atlasRect.y, (float)atlasRect.width, (float)atlasRect.height));
    }

    // Fill shadow data
    if (shadowData.m_uiType == LIGHT_TYPE_DIR)
    {
      WUInt32 uiNumCascades = shadowData.m_Views.GetCount();

      WUInt32 uiMatrixIndex = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowData.m_uiPackedDataOffset, 0);
      WMat4& worldToLightMatrix = *reinterpret_cast<WMat4*>(&packedShadowData[uiMatrixIndex]);

      worldToLightMatrix = shadowViews[0]->GetViewProjectionMatrix(WCameraEye::Left);

      for (WUInt32 uiViewIndex = 0; uiViewIndex < uiNumCascades; ++uiViewIndex)
      {
        if (uiViewIndex >= 1)
        {
          WMat4 cascadeToWorldMatrix = shadowViews[uiViewIndex]->GetInverseViewProjectionMatrix(WCameraEye::Left);
          WVec3 cascadeCorner = cascadeToWorldMatrix.TransformPosition(WVec3(0.0f));
          cascadeCorner = worldToLightMatrix.TransformPosition(cascadeCorner);

          WVec3 otherCorner = cascadeToWorldMatrix.TransformPosition(WVec3(1.0f));
          otherCorner = worldToLightMatrix.TransformPosition(otherCorner);

          WUInt32 uiCascadeScaleIndex = GET_CASCADE_SCALE_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex - 1);
          WUInt32 uiCascadeOffsetIndex = GET_CASCADE_OFFSET_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex - 1);

          WVec4& cascadeScale = packedShadowData[uiCascadeScaleIndex];
          WVec4& cascadeOffset = packedShadowData[uiCascadeOffsetIndex];

          cascadeScale = WVec3(1.0f).CompDiv(otherCorner - cascadeCorner).GetAsVec4(1.0f);
          cascadeOffset = cascadeCorner.GetAsVec4(0.0f).CompMul(-cascadeScale);
        }

        WUInt32 uiAtlasScaleOffsetIndex = GET_ATLAS_SCALE_OFFSET_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex);
        WVec4& atlasScaleOffset = packedShadowData[uiAtlasScaleOffsetIndex];

        const WRectU16& atlasRect = atlasRects[uiViewIndex];
        if (atlasRect.HasNonZeroArea())
        {
          WVec2 scale = WVec2(atlasRect.width * fAtlasInvWidth, atlasRect.height * fAtlasInvHeight);
          WVec2 offset = WVec2(atlasRect.x * fAtlasInvWidth, atlasRect.y * fAtlasInvHeight);

          // combine with tex scale offset
          atlasScaleOffset.x = scale.x * 0.5f;
          atlasScaleOffset.y = scale.y * -0.5f;
          atlasScaleOffset.z = offset.x + scale.x * 0.5f;
          atlasScaleOffset.w = offset.y + scale.y * 0.5f;
        }
        else
        {
          atlasScaleOffset.Set(1.0f, 1.0f, 0.0f, 0.0f);
        }
      }

      const WCamera* pFirstCascadeCamera = shadowViews[0]->GetCamera();
      const WCamera* pLastCascadeCamera = shadowViews[uiNumCascades - 1]->GetCamera();

      float cascadeSize = pFirstCascadeCamera->GetFovOrDim();
      float texelSize = 1.0f / uiShadowMapSize;
      float penumbraSize = WMath::Max(shadowData.m_fPenumbraSize / cascadeSize, texelSize);
      float goodPenumbraSize = 8.0f / uiShadowMapSize;
      float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

      // params
      {
        // tweak values to keep the default values consistent with spot and point lights
        float slopeBias = shadowData.m_fSlopeBias * WMath::Max(penumbraSize, goodPenumbraSize);
        float constantBias = shadowData.m_fConstantBias * 0.2f;
        WUInt32 uilastCascadeIndex = uiNumCascades - 1;

        WUInt32 uiParamsIndex = GET_SHADOW_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
        WVec4& shadowParams = packedShadowData[uiParamsIndex];
        shadowParams.x = slopeBias;
        shadowParams.y = constantBias;
        shadowParams.z = penumbraSize * relativeShadowSize;
        shadowParams.w = *reinterpret_cast<float*>(&uilastCascadeIndex);
      }

      // params2
      {
        float ditherMultiplier = 0.2f / cascadeSize;
        float zRange = cascadeSize / pFirstCascadeCamera->GetFarPlane();

        float actualPenumbraSize = shadowData.m_fPenumbraSize / pLastCascadeCamera->GetFovOrDim();
        float penumbraSizeIncrement = WMath::Max(goodPenumbraSize - actualPenumbraSize, 0.0f) / shadowData.m_fMinRange;

        WUInt32 uiParams2Index = GET_SHADOW_PARAMS2_INDEX(shadowData.m_uiPackedDataOffset);
        WVec4& shadowParams2 = packedShadowData[uiParams2Index];
        shadowParams2.x = 1.0f - WMath::Max(penumbraSize, goodPenumbraSize);
        shadowParams2.y = ditherMultiplier;
        shadowParams2.z = ditherMultiplier * zRange;
        shadowParams2.w = penumbraSizeIncrement * relativeShadowSize;
      }

      // fadeout
      {
        float fadeOutRange = 1.0f - shadowData.m_fFadeOutStart;
        float xyScale = -1.0f / fadeOutRange;
        float xyOffset = -xyScale;
        WUInt32 xyScaleOffset = WShaderUtils::PackFloat16intoUint(xyScale, xyOffset);

        float zFadeOutRange = fadeOutRange * pLastCascadeCamera->GetFovOrDim() / pLastCascadeCamera->GetFarPlane();
        float zScale = -1.0f / zFadeOutRange;
        float zOffset = -zScale;
        WUInt32 zScaleOffset = WShaderUtils::PackFloat16intoUint(zScale, zOffset);

        float distanceFadeOutRange = fadeOutRange * shadowData.m_fActualRange;
        float distanceScale = -1.0f / distanceFadeOutRange;
        float distanceOffset = -distanceScale * shadowData.m_fActualRange;

        WUInt32 uiFadeOutIndex = GET_FADE_OUT_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
        WVec4& fadeOutParams = packedShadowData[uiFadeOutIndex];
        fadeOutParams.x = *reinterpret_cast<float*>(&xyScaleOffset);
        fadeOutParams.y = *reinterpret_cast<float*>(&zScaleOffset);
        fadeOutParams.z = distanceScale;
        fadeOutParams.w = distanceOffset;
      }
    }
    else // spot or point light
    {
      WMat4 texMatrix;
      texMatrix.SetIdentity();
      texMatrix.SetDiagonal(WVec4(0.5f, -0.5f, 1.0f, 1.0f));
      texMatrix.SetTranslationVector(WVec3(0.5f, 0.5f, 0.0f));

      WAngle fov = WAngle::MakeZero();
      float fRange = 0.0f;

      for (WUInt32 uiViewIndex = 0; uiViewIndex < shadowData.m_Views.GetCount(); ++uiViewIndex)
      {
        WView* pShadowView = shadowViews[uiViewIndex];
        W_ASSERT_DEV(pShadowView != nullptr, "Implementation error");

        WUInt32 uiMatrixIndex = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex);
        WMat4& worldToLightMatrix = *reinterpret_cast<WMat4*>(&packedShadowData[uiMatrixIndex]);

        const WRectU16& atlasRect = atlasRects[uiViewIndex];
        if (atlasRect.HasNonZeroArea())
        {
          WVec2 scale = WVec2(atlasRect.width * fAtlasInvWidth, atlasRect.height * fAtlasInvHeight);
          WVec2 offset = WVec2(atlasRect.x * fAtlasInvWidth, atlasRect.y * fAtlasInvHeight);

          WMat4 atlasMatrix;
          atlasMatrix.SetIdentity();
          atlasMatrix.SetDiagonal(WVec4(scale.x, scale.y, 1.0f, 1.0f));
          atlasMatrix.SetTranslationVector(offset.GetAsVec3(0.0f));

          fov = pShadowView->GetCamera()->GetFovY(1.0f);
          fRange = pShadowView->GetCamera()->GetFarPlane();
          const WMat4& viewProjection = pShadowView->GetViewProjectionMatrix(WCameraEye::Left);

          worldToLightMatrix = atlasMatrix * texMatrix * viewProjection;
        }
        else
        {
          worldToLightMatrix.SetIdentity();
        }
      }

      const float screenHeight = WMath::Tan(fov * 0.5f) * 20.0f; // screen height in world-space at 10m distance
      const float texelSize = 1.0f / uiShadowMapSize;
      const float penumbraSize = WMath::Max(shadowData.m_fPenumbraSize / screenHeight, texelSize);
      const float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

      // empirical tweak factors
      const float fovFactor = 0.15f * WMath::Pow(5.5f, fov.GetRadian());
      const float rangeFactor = WMath::Max(0.018f * fRange + 0.0098f * fRange * fRange, 0.1f);
      const float slopeBias = shadowData.m_fSlopeBias * penumbraSize * fovFactor * rangeFactor;
      const float constantBias = shadowData.m_fConstantBias * cvar_RenderingShadowsMaxShadowMapSize / uiShadowMapSize;

      WUInt32 uiParamsIndex = GET_SHADOW_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
      WVec4& shadowParams = packedShadowData[uiParamsIndex];
      shadowParams.x = slopeBias;
      shadowParams.y = constantBias;
      shadowParams.z = penumbraSize * relativeShadowSize;
      shadowParams.w = 0.0f;
    }
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingShadowsShowAtlasTexture)
  {
    WDebugRendererContext debugContext(WWorld::GetWorld(0));
    float viewWidth = 1920;
    float viewHeight = 1080;

    if (const WView* pView = WRenderWorld::GetViewByUsageHint(WCameraUsageHint::MainView, WCameraUsageHint::EditorView))
    {
      debugContext = WDebugRendererContext(pView->GetHandle());
      viewWidth = pView->GetViewport().width;
      viewHeight = pView->GetViewport().height;
    }

    s_pData->m_TextureAtlas.DebugDraw(debugContext, viewWidth, viewHeight);
  }
#endif

  s_pData->Clear();
}

// static
void WShadowPool::OnRenderEvent(const WRenderWorldRenderEvent& e)
{
  if (e.m_Type != WRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_TextureAtlas.IsInitialized() == false || s_pData->m_hShadowDataBuffer.IsInvalidated())
    return;

  if (cvar_RenderingShadowsAtlasSize.HasDelayedSyncValueChanged() || cvar_RenderingShadowsMinShadowMapSize.HasDelayedSyncValueChanged() || cvar_RenderingShadowsMaxShadowMapSize.HasDelayedSyncValueChanged() || s_uiLastConfigModification != WGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetLastModificationCounter())
  {
    OnEngineShutdown();
    OnEngineStartup();
  }

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  if (s_pData->m_pRenderGraph == nullptr)
    s_pData->m_pRenderGraph = WRenderGraphManager::CreateRenderGraph("ShadowPool", WRenderGraphPhase::PreRender);

  auto rtv = pDevice->GetDefaultRenderTargetView(s_pData->m_TextureAtlas.GetTexture());
  if (rtv.IsInvalidated() == false)
  {
    s_pData->m_pRenderGraph->Reset();

    WRenderGraphTextureHandle hShadowAtlas = s_pData->m_pRenderGraph->ImportTexture(s_pData->m_TextureAtlas.GetTexture());

    {
      auto pass = s_pData->m_pRenderGraph->AddGraphicsPass("Shadow Atlas");
      pass.AddDepthStencilTarget(hShadowAtlas);
      pass.SetClearDepth();
      pass.HasSideEffects();
      pass.SetExecuteCallback([](const WRenderGraphContext& ctx)
        {
        WUInt32 uiDataIndex = WRenderWorld::GetDataIndexForRendering();
        auto& packedShadowData = s_pData->m_PackedShadowData[uiDataIndex];
        if (!packedShadowData.IsEmpty())
        {
          W_PROFILE_SCOPE("Shadow Data Buffer Update");

          ctx.GetCommandEncoder()->UpdateBuffer(s_pData->m_hShadowDataBuffer, 0, packedShadowData.GetByteArrayPtr(), WGALUpdateMode::AheadOfTime);
        } });
    }
    WRenderGraphManager::EnqueueRenderGraph(s_pData->m_pRenderGraph);
  }
}

W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ShadowPool);
