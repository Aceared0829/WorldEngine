#include <RendererCore/RendererCorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Decals/Implementation/DecalManager.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/DynamicTextureAtlas.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Utils/CoreRenderProfile.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>
#include <Shaders/Common/LightData.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalManager)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderWorld"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WDecalManager::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WDecalManager::OnEngineShutdown();
  }
W_END_SUBSYSTEM_DECLARATION;
// clang-format on

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
WCVarBool cvar_RenderingDecalsShowAtlasTexture("Rendering.Decals.ShowAtlasTexture", false, WCVarFlags::Default, "Display the dynamic decal atlas texture");
#endif

/// NOTE: The default values for these are defined in WCoreRenderProfileConfig
///       but they can also be overwritten in custom game states at startup.
W_RENDERERCORE_DLL WCVarInt cvar_RenderingDecalsDynamicAtlasSize("Rendering.Decals.DynamicAtlasSize", 3072, WCVarFlags::RequiresDelayedSync, "The size of the dynamic decal atlas texture.");

constexpr WUInt32 s_uiMaxDecalSize = 1024;

static WUInt32 s_uiLastConfigModification = 0;

//////////////////////////////////////////////////////////////////////////

WPerDecalAtlasData MakeAtlasData(const WRectU16& rect, const WVec2& vTextureSize)
{
  WVec2 scale, offset;
  scale.x = (float)rect.width / vTextureSize.x * 0.5f;
  scale.y = (float)rect.height / vTextureSize.y * 0.5f;
  offset.x = (float)rect.x / vTextureSize.x + scale.x;
  offset.y = (float)rect.y / vTextureSize.y + scale.y;

  WPerDecalAtlasData res;
  res.scale = WShaderUtils::Float2ToRG16F(scale);
  res.offset = WShaderUtils::Float2ToRG16F(offset);
  return res;
}

struct DecalInfo
{
  WUInt32 m_uiRefCount = 0;

  WUInt8 m_uiGeneration = 1;
  WUInt16 m_uiAtlasDataOffset = WSmallInvalidIndex; // Also used as index into s_pData->m_DecalInfos
  WDynamicTextureAtlas::AllocationId m_atlasAllocationId;

  float m_fMaxScreenSpaceSize = 0.0f;

  WTexture2DResourceHandle m_hTexture;
  WMaterialResourceHandle m_hMaterial;
  WUInt16 m_uiMaxWidth = 0;
  WUInt16 m_uiMaxHeight = 0;
  float m_fUpdateInterval = WTime::MakeFromHours(3600).AsFloatInSeconds();

  WTime m_NextUpdateTime = WTime::MakeFromHours(-1);
  WTime m_WorldTime;

  WHashedString m_sName;

  W_ALWAYS_INLINE bool IsStatic() const { return m_hTexture.IsValid(); }
  W_ALWAYS_INLINE bool IsDynamic() const { return !IsStatic(); }

  float CalculateScore(WTime now) const
  {
    const float fClampedScreenSpaceSize = WMath::Clamp(m_fMaxScreenSpaceSize, 0.0f, 10.0f);
    const float fTimeSinceLastUpdate = (now - m_NextUpdateTime).AsFloatInSeconds();

    return fClampedScreenSpaceSize + (fTimeSinceLastUpdate * fTimeSinceLastUpdate);
  }

  WVec2U32 CalculateScaledSize() const
  {
    const float fScreenSpaceSize = WMath::Saturate(WMath::Pow(m_fMaxScreenSpaceSize, 1.2f));
    const WUInt32 uiWidth = WMath::Min(static_cast<WUInt32>(m_uiMaxWidth * fScreenSpaceSize), s_uiMaxDecalSize);
    const WUInt32 uiHeight = WMath::Min(static_cast<WUInt32>(m_uiMaxHeight * fScreenSpaceSize), s_uiMaxDecalSize);

    const WUInt32 uiWidthAlign = uiWidth < 64 ? 16 : 64;
    const WUInt32 uiHeightAlign = uiHeight < 64 ? 16 : 64;
    return WVec2U32(WMemoryUtils::AlignSize(uiWidth, uiWidthAlign), WMemoryUtils::AlignSize(uiHeight, uiHeightAlign));
  }

  WUInt64 GetKey() const
  {
    if (m_hTexture.IsValid())
      return GetKey(m_hTexture);

    if (m_hMaterial.IsValid())
      return GetKey(m_hMaterial);

    W_REPORT_FAILURE("Invalid decal");
    return 0;
  }

  W_FORCE_INLINE void SetUpdateInterval(WTime updateInterval)
  {
    m_fUpdateInterval = WMath::Min(m_fUpdateInterval, updateInterval.AsFloatInSeconds());
    m_NextUpdateTime = WMath::Min(m_NextUpdateTime, WTime::Now() + WTime::MakeFromSeconds(m_fUpdateInterval));
  }

  W_FORCE_INLINE void MarkUsage(float fScreenSpaceSize, const WView* pReferenceView)
  {
    m_fMaxScreenSpaceSize = WMath::Max(m_fMaxScreenSpaceSize, fScreenSpaceSize);

    if (pReferenceView != nullptr && pReferenceView->GetWorld() != nullptr)
    {
      const bool bIsMainView = pReferenceView->GetCameraUsageHint() == WCameraUsageHint::MainView || pReferenceView->GetCameraUsageHint() == WCameraUsageHint::EditorView;
      if (bIsMainView || m_WorldTime.IsZero())
      {
        m_WorldTime = pReferenceView->GetWorld()->GetClock().GetAccumulatedTime();
      }
    }

    if (m_sName.IsEmpty())
    {
      if (m_hTexture.IsValid())
      {
        WResourceLock<WTexture2DResource> pTexture(m_hTexture, WResourceAcquireMode::AllowLoadingFallback);
        if (pTexture.GetAcquireResult() != WResourceAcquireResult::Final || pTexture->GetNumQualityLevelsLoadable() > 0)
          return;

        m_uiMaxWidth = WMath::Min(pTexture->GetWidth(), s_uiMaxDecalSize);
        m_uiMaxHeight = WMath::Min(pTexture->GetHeight(), s_uiMaxDecalSize);
        m_sName.Assign(GetNameFromResource(*pTexture.GetPointer()));
      }
      else
      {
        W_ASSERT_DEV(m_hMaterial.IsValid(), "DecalInfo must have either a texture or a material assigned.");

        WResourceLock<WMaterialResource> pMaterial(m_hMaterial, WResourceAcquireMode::AllowLoadingFallback);
        if (pMaterial.GetAcquireResult() != WResourceAcquireResult::Final)
          return;

        m_sName.Assign(DecalInfo::GetNameFromResource(*pMaterial.GetPointer()));
      }
    }
  }

  W_FORCE_INLINE void ResetAfterUpdate()
  {
    m_fMaxScreenSpaceSize = 0.0f;

    m_NextUpdateTime = WTime::Now() + WTime::MakeFromSeconds(m_fUpdateInterval);
  }

  W_ALWAYS_INLINE static WUInt64 GetKey(const WTexture2DResourceHandle& hTexture)
  {
    return hTexture.GetResourceIDHash();
  }

  W_ALWAYS_INLINE static WUInt64 GetKey(const WMaterialResourceHandle& hMaterial)
  {
    return hMaterial.GetResourceIDHash();
  }

  W_ALWAYS_INLINE static WStringView GetNameFromResource(const WResource& resource)
  {
    if (resource.GetResourceDescription().IsEmpty() == false)
    {
      return resource.GetResourceDescription().GetFileName();
    }
    else
    {
      return resource.GetResourceID();
    }
  }
};

struct SortedDecal
{
  W_DECLARE_POD_TYPE();

  WUInt32 m_uiIndex;
  float m_fScore;

  WVec2U32 m_vNewSize;

  bool operator<(const SortedDecal& other) const
  {
    if (m_fScore != other.m_fScore)
      return m_fScore > other.m_fScore; // Descending order

    return m_uiIndex < other.m_uiIndex;
  }
};

struct DecalUpdateInfo
{
  WMaterialResourceHandle m_hMaterial;
  WRectU16 m_TargetRect;
  WTime m_WorldTime;
};

struct WDecalManager::Data
{
  Data()
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(WPerDecalAtlasData);
    desc.m_uiTotalSize = desc.m_uiStructSize * 64;
    desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hAtlasDataBuffer = WGALDevice::GetDefaultDevice()->CreateDynamicBuffer(desc, "Decal Atlas Data");
  }

  ~Data()
  {
    WGALDevice::GetDefaultDevice()->DestroyDynamicBuffer(m_hAtlasDataBuffer);
  }

  void EnsureResourceCreated()
  {
    W_LOCK(m_Mutex);

    if (m_RuntimeAtlas.IsInitialized())
      return;

    // use the current CVar values to initialize the values
    WUInt32 uiAtlasSize = cvar_RenderingDecalsDynamicAtlasSize;

    // if the platform profile has changed, use it to reset the defaults
    const auto& platformProfile = WGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile();
    if (s_uiLastConfigModification != platformProfile.GetLastModificationCounter())
    {
      s_uiLastConfigModification = platformProfile.GetLastModificationCounter();

      const auto* pConfig = platformProfile.GetTypeConfig<WCoreRenderProfileConfig>();
      uiAtlasSize = pConfig->m_uiRuntimeDecalAtlasTextureSize;
    }

    // if the CVars were modified recently (e.g. during game startup), use those values to override the default
    if (cvar_RenderingDecalsDynamicAtlasSize.HasDelayedSyncValueChanged())
      uiAtlasSize = cvar_RenderingDecalsDynamicAtlasSize.GetValue(WCVarValue::DelayedSync);

    // make sure the values are valid
    uiAtlasSize = WMath::Clamp(static_cast<WUInt32>(WMath::RoundToMultiple(uiAtlasSize, 512.0)), 512u, 8192u);

    // write back the clamped values, so that everyone sees the valid values
    cvar_RenderingDecalsDynamicAtlasSize = uiAtlasSize;
    cvar_RenderingDecalsDynamicAtlasSize.SetToDelayedSyncValue();

    WGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(uiAtlasSize, uiAtlasSize, WGALResourceFormat::RGBAUByteNormalized);

    m_RuntimeAtlas.Initialize(desc).AssertSuccess("Failed to initialize runtime decal atlas");

    {
      m_hSimpleCopyMaterial = WResourceManager::LoadResource<WMaterialResource>("{ c542b3fa-0c24-4bac-97b7-e481f66f18f1 }"); // DecalCopy.WMaterialAsset
      WResourceLock<WMaterialResource> pMaterial(m_hSimpleCopyMaterial, WResourceAcquireMode::BlockTillLoaded);
      WShaderResourceHandle hShader = pMaterial->GetCurrentDesc().m_hShader;
      WResourceLock<WShaderResource> pShader(hShader, WResourceAcquireMode::BlockTillLoaded);
    }

    const char* szBufferResourceName = "DecalPlaneMeshBuffer";
    m_hPlaneMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>(szBufferResourceName);
    if (!m_hPlaneMeshBuffer.IsValid())
    {
      WGeometry geom;
      geom.AddRect(WVec2(2.0f));

      WMeshBufferResourceDescriptor desc;
      desc.AddStream(WMeshVertexStreamType::Position);
      desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

      m_hPlaneMeshBuffer = WResourceManager::GetOrCreateResource<WMeshBufferResource>(szBufferResourceName, std::move(desc));
    }
  }

  WMutex m_Mutex;
  WDynamicTextureAtlas m_RuntimeAtlas;
  WGALDynamicBufferHandle m_hAtlasDataBuffer;

  WMaterialResourceHandle m_hSimpleCopyMaterial;
  WMeshBufferResourceHandle m_hPlaneMeshBuffer;

  WDynamicArray<DecalInfo> m_DecalInfos;
  WHashTable<WUInt64, WUInt32> m_DecalKeyToInfoIndex;

  WDynamicArray<SortedDecal> m_SortedDecals;
  WDynamicArray<DecalUpdateInfo> m_DecalsToUpdate[2];

  WDecalAtlasResourceHandle m_hBakedAtlas;

  WSharedPtr<WRenderGraph> m_pRenderGraph;
};

//////////////////////////////////////////////////////////////////////////

WDecalManager::Data* WDecalManager::s_pData = nullptr;

// static
WDecalId WDecalManager::GetOrCreateRuntimeDecal(const WTexture2DResourceHandle& hTexture)
{
  s_pData->EnsureResourceCreated();

  const WUInt64 uiKey = DecalInfo::GetKey(hTexture);

  W_LOCK(s_pData->m_Mutex);

  bool bExisted = false;
  WUInt32& uiIndex = s_pData->m_DecalKeyToInfoIndex.FindOrAdd(uiKey, &bExisted);
  if (!bExisted)
  {
    auto pAtlasDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(s_pData->m_hAtlasDataBuffer);
    uiIndex = pAtlasDataBuffer->Allocate(0, 1, WGALDynamicBuffer::AllocateFlags::ZeroFill);
    W_ASSERT_DEV(uiIndex < WSmallInvalidIndex, "Too many decals");

    s_pData->m_DecalInfos.EnsureCount(uiIndex + 1);

    auto& decalInfo = s_pData->m_DecalInfos[uiIndex];
    decalInfo.m_uiAtlasDataOffset = static_cast<WUInt16>(uiIndex);
    decalInfo.m_hTexture = hTexture;
    decalInfo.SetUpdateInterval(WTime::MakeFromSeconds(0.1));

    WStringBuilder decalMaterialName;
    decalMaterialName.AppendFormat("DecalMaterial_{0}", hTexture.GetResourceID());

    decalInfo.m_hMaterial = WResourceManager::GetExistingResource<WMaterialResource>(decalMaterialName);
    if (!decalInfo.m_hMaterial.IsValid())
    {
      WMaterialResourceDescriptor desc;
      desc.m_hBaseMaterial = s_pData->m_hSimpleCopyMaterial;
      desc.m_Texture2DBindings.PushBack({WMakeHashedString("BaseTexture"), hTexture});

      decalInfo.m_hMaterial = WResourceManager::CreateResource<WMaterialResource>(decalMaterialName, std::move(desc));
    }
  }

  auto& decalInfo = s_pData->m_DecalInfos[uiIndex];
  W_ASSERT_DEBUG(decalInfo.m_uiAtlasDataOffset == uiIndex, "Implementation error");
  ++decalInfo.m_uiRefCount;

  return WDecalId(uiIndex, decalInfo.m_uiGeneration);
}

WDecalId WDecalManager::GetOrCreateRuntimeDecal(const WMaterialResourceHandle& hMaterial, WUInt32 uiResolution, WTime updateInterval)
{
  s_pData->EnsureResourceCreated();

  const WUInt64 uiKey = DecalInfo::GetKey(hMaterial);

  W_LOCK(s_pData->m_Mutex);

  bool bExisted = false;
  WUInt32& uiIndex = s_pData->m_DecalKeyToInfoIndex.FindOrAdd(uiKey, &bExisted);
  if (!bExisted)
  {
    auto pAtlasDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(s_pData->m_hAtlasDataBuffer);
    uiIndex = pAtlasDataBuffer->Allocate(0, 1, WGALDynamicBuffer::AllocateFlags::ZeroFill);
    W_ASSERT_DEV(uiIndex < WSmallInvalidIndex, "Too many decals");

    s_pData->m_DecalInfos.EnsureCount(uiIndex + 1);

    auto& decalInfo = s_pData->m_DecalInfos[uiIndex];
    decalInfo.m_uiAtlasDataOffset = static_cast<WUInt16>(uiIndex);
    decalInfo.m_hMaterial = hMaterial;
  }

  auto& decalInfo = s_pData->m_DecalInfos[uiIndex];
  W_ASSERT_DEBUG(decalInfo.m_uiAtlasDataOffset == uiIndex, "Implementation error");
  decalInfo.SetUpdateInterval(updateInterval);
  ++decalInfo.m_uiRefCount;

  decalInfo.m_uiMaxWidth = WMath::Clamp<WUInt16>(decalInfo.m_uiMaxWidth, uiResolution, s_uiMaxDecalSize);
  decalInfo.m_uiMaxHeight = decalInfo.m_uiMaxWidth;

  return WDecalId(uiIndex, decalInfo.m_uiGeneration);
}

// static
void WDecalManager::DeleteRuntimeDecal(WDecalId& ref_decalId)
{
  if (ref_decalId.IsInvalidated())
    return;

  W_SCOPE_EXIT(ref_decalId.Invalidate());

  W_LOCK(s_pData->m_Mutex);

  auto& decalInfo = s_pData->m_DecalInfos[ref_decalId.m_InstanceIndex];
  W_ASSERT_DEV(decalInfo.m_uiGeneration == ref_decalId.m_Generation, "Invalid decal id");

  --decalInfo.m_uiRefCount;
  if (decalInfo.m_uiRefCount > 0)
    return;

  if (decalInfo.m_atlasAllocationId.IsInvalidated() == false)
  {
    s_pData->m_RuntimeAtlas.Deallocate(decalInfo.m_atlasAllocationId);
  }

  auto pAtlasDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(s_pData->m_hAtlasDataBuffer);
  pAtlasDataBuffer->Deallocate(decalInfo.m_uiAtlasDataOffset);

  W_VERIFY(s_pData->m_DecalKeyToInfoIndex.Remove(decalInfo.GetKey()), "Implemenation error");

  const WUInt8 generation = decalInfo.m_uiGeneration;
  decalInfo = {}; // Reset the decal info

  decalInfo.m_uiGeneration = generation + 1;
  if (decalInfo.m_uiGeneration == 0)
    decalInfo.m_uiGeneration = 1;
}

// static
void WDecalManager::MarkRuntimeDecalAsUsed(WDecalId decalId, float fScreenSpaceSize, const WView* pReferenceView)
{
  if (decalId.IsInvalidated())
    return;

  W_LOCK(s_pData->m_Mutex);

  auto& decalInfo = s_pData->m_DecalInfos[decalId.m_InstanceIndex];
  W_ASSERT_DEV(decalInfo.m_uiGeneration == decalId.m_Generation && decalInfo.m_uiRefCount > 0, "Invalid decal");

  decalInfo.MarkUsage(fScreenSpaceSize, pReferenceView);
}

WDecalAtlasResourceHandle WDecalManager::GetBakedDecalAtlas()
{
  if (s_pData->m_hBakedAtlas.IsValid() == false)
  {
    s_pData->m_hBakedAtlas = WResourceManager::LoadResource<WDecalAtlasResource>("{ ProjectDecalAtlas }");
  }

  return s_pData->m_hBakedAtlas;
}

WGALTextureHandle WDecalManager::GetRuntimeDecalAtlasTexture()
{
  if (s_pData->m_RuntimeAtlas.IsInitialized())
  {
    return s_pData->m_RuntimeAtlas.GetTexture();
  }

  return WGALTextureHandle();
}

WGALBufferHandle WDecalManager::GetDecalAtlasDataBufferForRendering()
{
  auto pAtlasDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(s_pData->m_hAtlasDataBuffer);

  return pAtlasDataBuffer->GetBufferForRendering();
}

// static
void WDecalManager::OnEngineStartup()
{
  s_pData = W_DEFAULT_NEW(WDecalManager::Data);

  WRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  WRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void WDecalManager::OnEngineShutdown()
{
  WRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  WRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  W_DEFAULT_DELETE(s_pData);
}

// static
void WDecalManager::OnExtractionEvent(const WRenderWorldExtractionEvent& e)
{
  if (e.m_Type == WRenderWorldExtractionEvent::Type::BeginExtraction)
  {
    if (s_pData->m_RuntimeAtlas.IsInitialized() &&
        (cvar_RenderingDecalsDynamicAtlasSize.HasDelayedSyncValueChanged() ||
          s_uiLastConfigModification != WGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetLastModificationCounter()))
    {
      W_LOCK(s_pData->m_Mutex);

      s_pData->m_RuntimeAtlas.Deinitialize();

      for (auto& decalInfo : s_pData->m_DecalInfos)
      {
        decalInfo.m_atlasAllocationId.Invalidate();
        decalInfo.m_NextUpdateTime = WTime::MakeFromHours(-1);
      }

      s_pData->EnsureResourceCreated();
    }
  }

  if (e.m_Type != WRenderWorldExtractionEvent::Type::EndExtraction)
    return;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingDecalsShowAtlasTexture)
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

    s_pData->m_RuntimeAtlas.DebugDraw(debugContext, viewWidth, viewHeight);
  }
#endif

  auto pAtlasDataBuffer = WGALDevice::GetDefaultDevice()->GetDynamicBuffer(s_pData->m_hAtlasDataBuffer);

  W_LOCK(s_pData->m_Mutex);

  {
    const WTime now = WTime::Now();

    for (auto& decalInfo : s_pData->m_DecalInfos)
    {
      if (decalInfo.m_uiAtlasDataOffset == WSmallInvalidIndex || decalInfo.m_sName.IsEmpty())
        continue;

      if (decalInfo.m_NextUpdateTime > now)
        continue;

      const WRectU16 currentRect = s_pData->m_RuntimeAtlas.GetAllocationRect(decalInfo.m_atlasAllocationId);
      const WVec2U32 newSize = decalInfo.CalculateScaledSize();
      const bool bSizeChanged = currentRect.width != newSize.x || currentRect.height != newSize.y;
      if (bSizeChanged)
      {
        s_pData->m_RuntimeAtlas.Deallocate(decalInfo.m_atlasAllocationId);
      }

      if ((decalInfo.IsDynamic() || bSizeChanged) && newSize.x > 0 && newSize.y > 0)
      {
        auto& sortedDecal = s_pData->m_SortedDecals.ExpandAndGetRef();
        sortedDecal.m_uiIndex = decalInfo.m_uiAtlasDataOffset;
        sortedDecal.m_fScore = decalInfo.CalculateScore(now);
        sortedDecal.m_vNewSize = newSize;
      }

      decalInfo.ResetAfterUpdate();
    }
  }


  s_pData->m_SortedDecals.Sort();

  const WVec2 vAtlasSize = WVec2(float(cvar_RenderingDecalsDynamicAtlasSize));
  auto& decalsToUpdate = s_pData->m_DecalsToUpdate[WRenderWorld::GetDataIndexForExtraction()];

  for (auto& decalToUpdate : s_pData->m_SortedDecals)
  {
    auto& decalInfo = s_pData->m_DecalInfos[decalToUpdate.m_uiIndex];

    if (decalInfo.m_atlasAllocationId.IsInvalidated())
    {
      WRectU16 rect;
      decalInfo.m_atlasAllocationId = s_pData->m_RuntimeAtlas.Allocate(decalToUpdate.m_vNewSize.x, decalToUpdate.m_vNewSize.y, decalInfo.m_sName, &rect);

      auto data = pAtlasDataBuffer->MapForWriting<WPerDecalAtlasData>(decalInfo.m_uiAtlasDataOffset);
      data[0] = MakeAtlasData(rect, vAtlasSize);
    }

    auto& updateInfo = decalsToUpdate.ExpandAndGetRef();
    updateInfo.m_hMaterial = decalInfo.m_hMaterial;
    updateInfo.m_TargetRect = s_pData->m_RuntimeAtlas.GetAllocationRect(decalInfo.m_atlasAllocationId);
    updateInfo.m_WorldTime = decalInfo.m_WorldTime;

    decalInfo.m_WorldTime = WTime::MakeZero();
  }

  s_pData->m_SortedDecals.Clear();

  pAtlasDataBuffer->UploadChangesForNextFrame();
}

// static
void WDecalManager::OnRenderEvent(const WRenderWorldRenderEvent& e)
{
  if (e.m_Type != WRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_RuntimeAtlas.IsInitialized() == false || s_pData->m_hPlaneMeshBuffer.IsValid() == false)
    return;

  auto& decalsToUpdate = s_pData->m_DecalsToUpdate[WRenderWorld::GetDataIndexForRendering()];
  if (decalsToUpdate.IsEmpty())
    return;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  auto rtv = pDevice->GetDefaultRenderTargetView(s_pData->m_RuntimeAtlas.GetTexture());
  if (rtv.IsInvalidated())
    return;

  if (s_pData->m_pRenderGraph == nullptr)
    s_pData->m_pRenderGraph = WRenderGraphManager::CreateRenderGraph("DecalManager", WRenderGraphPhase::PreRender);

  s_pData->m_pRenderGraph->Reset();

  WRenderGraphTextureHandle hAtlas = s_pData->m_pRenderGraph->ImportTexture(s_pData->m_RuntimeAtlas.GetTexture());
  {
    auto pass = s_pData->m_pRenderGraph->AddGraphicsPass("Decal Atlas");
    pass.AddColorTarget(hAtlas, {}, WGALRenderTargetLoadOp::Load, WGALRenderTargetStoreOp::Store);
    pass.HasSideEffects();
    pass.SetExecuteCallback(
      [](const WRenderGraphContext& ctx)
      {
        auto& decalsToUpdate = s_pData->m_DecalsToUpdate[WRenderWorld::GetDataIndexForRendering()];
        if (decalsToUpdate.IsEmpty())
          return;

        auto* pRenderContext = ctx.GetRenderContext();
        auto* pCommandEncoder = ctx.GetCommandEncoder();

        const bool bAllowAsyncShaderLoading = pRenderContext->GetAllowAsyncShaderLoading();
        pRenderContext->SetAllowAsyncShaderLoading(false);
        W_SCOPE_EXIT(pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

        pRenderContext->BindMeshBuffer(s_pData->m_hPlaneMeshBuffer);

        for (WUInt32 i = 0; i < decalsToUpdate.GetCount(); ++i)
        {
          auto& updateInfo = decalsToUpdate[i];
          WRectFloat viewport = WRectFloat(updateInfo.m_TargetRect.x, updateInfo.m_TargetRect.y, updateInfo.m_TargetRect.width, updateInfo.m_TargetRect.height);

          pCommandEncoder->SetViewport(viewport);

          pRenderContext->SetGlobalAndWorldTimeConstants(updateInfo.m_WorldTime);
          pRenderContext->BindMaterial(updateInfo.m_hMaterial);

          pRenderContext->DrawMeshBuffer().AssertSuccess();
        }

        decalsToUpdate.Clear();
      });
  }
  WRenderGraphManager::EnqueueRenderGraph(s_pData->m_pRenderGraph);
}

W_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalManager);
