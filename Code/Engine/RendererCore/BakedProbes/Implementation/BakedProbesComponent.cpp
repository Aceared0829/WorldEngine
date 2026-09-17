#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/BakedProbes/BakedProbesComponent.h>
#include <RendererCore/BakedProbes/BakedProbesWorldModule.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

struct WBakedProbesComponent::RenderDebugViewTask : public WTask
{
  RenderDebugViewTask()
  {
    ConfigureTask("BakingDebugView", WTaskNesting::Never);
  }

  virtual void Execute() override
  {
    W_ASSERT_DEV(m_PixelData.GetCount() == m_uiWidth * m_uiHeight, "Pixel data must be pre-allocated");

    WProgress progress;
    progress.m_Events.AddEventHandler([this](const WProgressEvent& e)
      {
      if (e.m_Type != WProgressEvent::Type::CancelClicked)
      {
        if (HasBeenCanceled())
        {
          e.m_pProgressbar->UserClickedCancel();
        }
        m_bHasNewData = true;
      } });

    if (m_pBakingInterface->RenderDebugView(*m_pWorld, m_InverseViewProjection, m_uiWidth, m_uiHeight, m_PixelData, progress).Succeeded())
    {
      m_bHasNewData = true;
    }
  }

  WBakingInterface* m_pBakingInterface = nullptr;

  const WWorld* m_pWorld = nullptr;
  WMat4 m_InverseViewProjection = WMat4::MakeIdentity();
  WUInt32 m_uiWidth = 0;
  WUInt32 m_uiHeight = 0;
  WDynamicArray<WColorGammaUB> m_PixelData;

  bool m_bHasNewData = false;
};

//////////////////////////////////////////////////////////////////////////


WBakedProbesComponentManager::WBakedProbesComponentManager(WWorld* pWorld)
  : WSettingsComponentManager<WBakedProbesComponent>(pWorld)
{
}

WBakedProbesComponentManager::~WBakedProbesComponentManager() = default;

void WBakedProbesComponentManager::Initialize()
{
  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WBakedProbesComponentManager::RenderDebug, this);

    this->RegisterUpdateFunction(desc);
  }

  CreateDebugResources();
}

void WBakedProbesComponentManager::RenderDebug(const WWorldModule::UpdateContext& updateContext)
{
  if (WBakedProbesComponent* pComponent = GetSingletonComponent())
  {
    if (pComponent->GetShowDebugOverlay())
    {
      pComponent->RenderDebugOverlay();
    }
  }
}

void WBakedProbesComponentManager::CreateDebugResources()
{
  if (!m_hDebugSphere.IsValid())
  {
    WGeometry geom;
    geom.AddStackedSphere(0.3f, 32, 16);

    const char* szBufferResourceName = "IrradianceProbeDebugSphereBuffer";
    WMeshBufferResourceHandle hMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>(szBufferResourceName);
    if (!hMeshBuffer.IsValid())
    {
      WMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

      hMeshBuffer = WResourceManager::GetOrCreateResource<WMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
    }

    const char* szMeshResourceName = "IrradianceProbeDebugSphere";
    m_hDebugSphere = WResourceManager::GetExistingResource<WMeshResource>(szMeshResourceName);
    if (!m_hDebugSphere.IsValid())
    {
      WMeshResourceDescriptor desc;
      desc.UseExistingMeshBuffer(hMeshBuffer);
      desc.AddSubMesh(geom.CalculateTriangleCount(), 0, 0);
      desc.ComputeBounds();

      m_hDebugSphere = WResourceManager::GetOrCreateResource<WMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
    }
  }

  if (!m_hDebugMaterial.IsValid())
  {
    m_hDebugMaterial = WResourceManager::LoadResource<WMaterialResource>(
      "{ 4d15c716-a8e9-43d4-9424-43174403fb94 }"); // IrradianceProbeVisualization.WMaterialAsset
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WBakedProbesComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Settings", m_Settings),
    W_ACCESSOR_PROPERTY("ShowDebugOverlay", GetShowDebugOverlay, SetShowDebugOverlay)->AddAttributes(new WGroupAttribute("Debug")),
    W_ACCESSOR_PROPERTY("ShowDebugProbes", GetShowDebugProbes, SetShowDebugProbes),
    W_ACCESSOR_PROPERTY("UseTestPosition", GetUseTestPosition, SetUseTestPosition),
    W_ACCESSOR_PROPERTY("TestPosition", GetTestPosition, SetTestPosition)
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_FUNCTION_PROPERTY(OnObjectCreated),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Lighting/Baking"),
    new WLongOpAttribute("WLongOpProxy_BakeScene"),
    new WTransformManipulatorAttribute("TestPosition"),
    new WInDevelopmentAttribute(WInDevelopmentAttribute::Phase::Beta),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WBakedProbesComponent::WBakedProbesComponent() = default;
WBakedProbesComponent::~WBakedProbesComponent() = default;

void WBakedProbesComponent::OnActivated()
{
  auto pModule = GetWorld()->GetOrCreateModule<WBakedProbesWorldModule>();
  pModule->SetProbeTreeResourcePrefix(m_sProbeTreeResourcePrefix);

  GetOwner()->UpdateLocalBounds();

  SUPER::OnActivated();
}

void WBakedProbesComponent::OnDeactivated()
{
  if (m_pRenderDebugViewTask != nullptr)
  {
    WTaskSystem::CancelTask(m_pRenderDebugViewTask).IgnoreResult();
  }

  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void WBakedProbesComponent::SetShowDebugOverlay(bool bShow)
{
  m_bShowDebugOverlay = bShow;

  if (bShow && m_pRenderDebugViewTask == nullptr)
  {
    m_pRenderDebugViewTask = W_DEFAULT_NEW(RenderDebugViewTask);
  }
}

void WBakedProbesComponent::SetShowDebugProbes(bool bShow)
{
  if (m_bShowDebugProbes != bShow)
  {
    m_bShowDebugProbes = bShow;

    if (IsActiveAndInitialized())
    {
      WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
    }
  }
}

void WBakedProbesComponent::SetUseTestPosition(bool bUse)
{
  if (m_bUseTestPosition != bUse)
  {
    m_bUseTestPosition = bUse;

    if (IsActiveAndInitialized())
    {
      WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
    }
  }
}

void WBakedProbesComponent::SetTestPosition(const WVec3& vPos)
{
  m_vTestPosition = vPos;

  if (IsActiveAndInitialized())
  {
    WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void WBakedProbesComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg)
{
  ref_msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? WDefaultSpatialDataCategories::RenderDynamic : WDefaultSpatialDataCategories::RenderStatic);
}

void WBakedProbesComponent::OnExtractRenderData(WMsgExtractRenderData& ref_msg) const
{
  if (!m_bShowDebugProbes)
    return;

  // Don't trigger probe rendering in shadow or reflection views.
  if (ref_msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow ||
      ref_msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Reflection)
    return;

  auto pModule = GetWorld()->GetModule<WBakedProbesWorldModule>();
  if (!pModule->HasProbeData())
    return;

  const WGameObject* pOwner = GetOwner();
  const bool bDynamic = true;
  const WUInt32 uiUniqueID = WRenderComponent::GetUniqueIdForRendering(*this);
  const WUInt32 uiMaxNumProbes = 1024;

  WGALDynamicBufferHandle hInstanceDataBuffer;
  auto instanceData = ref_msg.m_pRenderDataManager->GetOrCreateInstanceData(this, bDynamic, hInstanceDataBuffer, m_InstanceDataOffset, uiMaxNumProbes);
  WUInt32 uiNumProbes = 0;

  auto addProbeRenderData = [&](WPerInstanceData& out_instanceData, const WVec3& vPosition, WCompressedSkyVisibility skyVisibility)
  {
    WTransform transform = WTransform::MakeIdentity();
    transform.m_vPosition = vPosition;

    WColor encodedSkyVisibility = WColor::Black;
    encodedSkyVisibility.r = *reinterpret_cast<const float*>(&skyVisibility);

    WRenderDataManager::FillPerInstanceData(out_instanceData, pOwner, WTransform::Make(vPosition), uiUniqueID, encodedSkyVisibility);
  };

  if (m_bUseTestPosition)
  {
    WBakedProbesWorldModule::ProbeIndexData indexData;
    if (pModule->GetProbeIndexData(m_vTestPosition, WVec3::MakeAxisZ(), indexData).Failed())
      return;

    if (true)
    {
      WResourceLock<WProbeTreeSectorResource> pProbeTree(pModule->m_hProbeTree, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pProbeTree.GetAcquireResult() != WResourceAcquireResult::Final)
        return;

      for (WUInt32 i = 0; i < W_ARRAY_SIZE(indexData.m_probeIndices); ++i)
      {
        WVec3 pos = pProbeTree->GetProbePositions()[indexData.m_probeIndices[i]];
        WDebugRenderer::DrawCross(ref_msg.m_pView->GetHandle(), pos, 0.5f, WColor::Yellow);

        pos.z += 0.5f;
        WDebugRenderer::Draw3DText(ref_msg.m_pView->GetHandle(), WFmt("Weight: {}", indexData.m_probeWeights[i]), pos, WColor::Yellow);
      }
    }

    WCompressedSkyVisibility skyVisibility = WBakingUtils::CompressSkyVisibility(pModule->GetSkyVisibility(indexData));

    addProbeRenderData(instanceData[0], m_vTestPosition, skyVisibility);
    uiNumProbes = 1;
  }
  else
  {
    WResourceLock<WProbeTreeSectorResource> pProbeTree(pModule->m_hProbeTree, WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pProbeTree.GetAcquireResult() != WResourceAcquireResult::Final)
      return;

    auto probePositions = pProbeTree->GetProbePositions();
    auto skyVisibility = pProbeTree->GetSkyVisibility();

    uiNumProbes = WMath::Min(probePositions.GetCount(), uiMaxNumProbes);
    for (WUInt32 uiProbeIndex = 0; uiProbeIndex < uiNumProbes; ++uiProbeIndex)
    {
      addProbeRenderData(instanceData[uiProbeIndex], probePositions[uiProbeIndex], skyVisibility[uiProbeIndex]);
    }
  }

  auto pManager = static_cast<const WBakedProbesComponentManager*>(GetOwningManager());

  WMeshRenderData* pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(pOwner);
  pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, pManager->m_hDebugMaterial, pManager->m_hDebugSphere, 0, 0, uiNumProbes);

  ref_msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::SimpleOpaque, m_bUseTestPosition ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic);
}

void WBakedProbesComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  if (m_Settings.Serialize(s).Failed())
    return;

  s << m_sProbeTreeResourcePrefix;
  s << m_bShowDebugOverlay;
  s << m_bShowDebugProbes;
  s << m_bUseTestPosition;
  s << m_vTestPosition;
}

void WBakedProbesComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  if (m_Settings.Deserialize(s).Failed())
    return;

  s >> m_sProbeTreeResourcePrefix;
  s >> m_bShowDebugOverlay;
  s >> m_bShowDebugProbes;
  s >> m_bUseTestPosition;
  s >> m_vTestPosition;
}

void WBakedProbesComponent::RenderDebugOverlay()
{
  WView* pView = WRenderWorld::GetViewByUsageHint(WCameraUsageHint::MainView, WCameraUsageHint::EditorView);
  if (pView == nullptr)
    return;

  WBakingInterface* pBakingInterface = WSingletonRegistry::GetSingletonInstance<WBakingInterface>();
  if (pBakingInterface == nullptr)
  {
    WDebugRenderer::Draw2DText(pView->GetHandle(), "Baking Plugin not loaded", WVec2I32(10, 10), WColor::OrangeRed);
    return;
  }

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  WRectFloat viewport = pView->GetViewport();
  WUInt32 uiWidth = static_cast<WUInt32>(WMath::Ceil(viewport.width / 3.0f));
  WUInt32 uiHeight = static_cast<WUInt32>(WMath::Ceil(viewport.height / 3.0f));

  WMat4 inverseViewProjection = pView->GetInverseViewProjectionMatrix(WCameraEye::Left);

  if (m_pRenderDebugViewTask->m_InverseViewProjection != inverseViewProjection ||
      m_pRenderDebugViewTask->m_uiWidth != uiWidth || m_pRenderDebugViewTask->m_uiHeight != uiHeight)
  {
    WTaskSystem::CancelTask(m_pRenderDebugViewTask).IgnoreResult();

    m_pRenderDebugViewTask->m_pBakingInterface = pBakingInterface;
    m_pRenderDebugViewTask->m_pWorld = GetWorld();
    m_pRenderDebugViewTask->m_InverseViewProjection = inverseViewProjection;
    m_pRenderDebugViewTask->m_uiWidth = uiWidth;
    m_pRenderDebugViewTask->m_uiHeight = uiHeight;
    m_pRenderDebugViewTask->m_PixelData.SetCount(uiWidth * uiHeight, WColor::Red);
    m_pRenderDebugViewTask->m_bHasNewData = false;

    WTaskSystem::StartSingleTask(m_pRenderDebugViewTask, WTaskPriority::LongRunning);
  }

  WUInt32 uiTextureWidth = 0;
  WUInt32 uiTextureHeight = 0;
  if (const WGALTexture* pTexture = pDevice->GetTexture(m_hDebugViewTexture))
  {
    uiTextureWidth = pTexture->GetDescription().m_uiWidth;
    uiTextureHeight = pTexture->GetDescription().m_uiHeight;
  }

  if (uiTextureWidth != uiWidth || uiTextureHeight != uiHeight)
  {
    pDevice->DestroyTexture(m_hDebugViewTexture);

    WGALTextureCreationDescription desc;
    desc.m_uiWidth = uiWidth;
    desc.m_uiHeight = uiHeight;
    desc.m_Format = WGALResourceFormat::RGBAUByteNormalizedsRGB;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hDebugViewTexture = pDevice->CreateTexture(desc);
  }

  auto& task = m_pRenderDebugViewTask;
  if (task != nullptr && task->m_bHasNewData)
  {
    task->m_bHasNewData = false;

    WGALSystemMemoryDescription sourceData;
    sourceData.m_pData = task->m_PixelData.GetByteArrayPtr();
    sourceData.m_uiRowPitch = task->m_uiWidth * sizeof(WColorGammaUB);

    pDevice->UpdateTextureForNextFrame(m_hDebugViewTexture, sourceData);
  }

  WRectFloat rectInPixel = WRectFloat(10.0f, 10.0f, static_cast<float>(uiWidth), static_cast<float>(uiHeight));

  WDebugRenderer::Draw2DRectangle(pView->GetHandle(), rectInPixel, 0.0f, WColor::White, m_hDebugViewTexture);
}

void WBakedProbesComponent::OnObjectCreated(const WAbstractObjectNode& node)
{
  WStringBuilder sPrefix;
  sPrefix.SetFormat(":project/AssetCache/Generated/{0}", node.GetGuid());

  m_sProbeTreeResourcePrefix.Assign(sPrefix);
}


W_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesComponent);
