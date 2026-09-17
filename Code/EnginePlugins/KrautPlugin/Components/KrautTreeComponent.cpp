#include <KrautPlugin/KrautPluginPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(WKrautTreeTypeBits, 1)
  W_BITFLAGS_CONSTANTS(WKrautTreeTypeBits::Trunk1, WKrautTreeTypeBits::Trunk2, WKrautTreeTypeBits::Trunk3)
  W_BITFLAGS_CONSTANTS(WKrautTreeTypeBits::MainBranches1, WKrautTreeTypeBits::MainBranches2, WKrautTreeTypeBits::MainBranches3)
  W_BITFLAGS_CONSTANTS(WKrautTreeTypeBits::SubBranches1, WKrautTreeTypeBits::SubBranches2, WKrautTreeTypeBits::SubBranches3)
  W_BITFLAGS_CONSTANTS(WKrautTreeTypeBits::Twigs1, WKrautTreeTypeBits::Twigs2, WKrautTreeTypeBits::Twigs3)
W_END_STATIC_REFLECTED_BITFLAGS

W_BEGIN_COMPONENT_TYPE(WKrautTreeComponent, 3, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("KrautTree", GetKrautGeneratorResource, SetKrautGeneratorResource)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Kraut_Tree"), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("VariationIndex", GetVariationIndex, SetVariationIndex)->AddAttributes(new WDefaultValueAttribute(0xFFFF)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgExtractGeometry, OnMsgExtractGeometry),
    W_MESSAGE_HANDLER(WMsgBuildStaticMesh, OnBuildStaticMesh),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Terrain"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WKrautTreeComponent::WKrautTreeComponent() = default;
WKrautTreeComponent::~WKrautTreeComponent() = default;

void WKrautTreeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hKrautGenerator;
  s << m_uiVariationIndex;
  s << m_uiCustomRandomSeed;
}

void WKrautTreeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  if (uiVersion <= 1)
  {
    s >> m_hKrautTree;
  }
  else
  {
    s >> m_hKrautGenerator;
  }

  s >> m_uiVariationIndex;
  s >> m_uiCustomRandomSeed;

  if (uiVersion == 2)
  {
    WUInt16 m_uiDefaultVariationIndex;
    s >> m_uiDefaultVariationIndex;
  }

  GetWorld()->GetOrCreateComponentManager<WKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
}

WResult WKrautTreeComponent::GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg)
{
  if (!m_hKrautTree.IsValid())
    return W_FAILURE;

  WResourceLock<WKrautTreeResource> pTree(m_hKrautTree, WResourceAcquireMode::PointerOnly);
  if (!pTree.IsValid())
    return W_FAILURE;

  const WBoundingBoxSphere treeBounds = pTree->GetDetails().m_Bounds;
  if (!treeBounds.IsValid())
    return W_FAILURE; // base data not yet generated; re-trigger once available

  bounds = treeBounds;

  // Artificially inflate the bounds so the main camera keeps selecting a decent LOD even when
  // the tree is not directly in view (e.g. for correct shadow LOD selection).
  bounds.m_fSphereRadius *= s_iLocalBoundsScale;
  bounds.m_vBoxHalfExtents *= (float)s_iLocalBoundsScale;

  return W_SUCCESS;
}

void WKrautTreeComponent::SetVariationIndex(WUInt16 uiIndex)
{
  if (m_uiVariationIndex == uiIndex)
    return;

  m_uiVariationIndex = uiIndex;

  if (IsActiveAndInitialized() && m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<WKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

WUInt16 WKrautTreeComponent::GetVariationIndex() const
{
  return m_uiVariationIndex;
}

void WKrautTreeComponent::SetCustomRandomSeed(WUInt16 uiSeed)
{
  if (m_uiCustomRandomSeed == uiSeed)
    return;

  m_uiCustomRandomSeed = uiSeed;

  if (IsActiveAndInitialized() && m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<WKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

WUInt16 WKrautTreeComponent::GetCustomRandomSeed() const
{
  return m_uiCustomRandomSeed;
}

void WKrautTreeComponent::SetKrautGeneratorResource(const WKrautGeneratorResourceHandle& hTree)
{
  if (m_hKrautGenerator == hTree)
    return;

  m_hKrautGenerator = hTree;

  if (IsActiveAndInitialized())
  {
    GetWorld()->GetOrCreateComponentManager<WKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void WKrautTreeComponent::OnActivated()
{
  SUPER::OnActivated();

  m_hKrautTree.Invalidate();
  m_vWindSpringPos.SetZero();
  m_vWindSpringVel.SetZero();

  if (m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<WKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void WKrautTreeComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hKrautTree.IsValid() || !m_hKrautGenerator.IsValid())
    return;

  WResourceLock<WKrautTreeResource> pTree(m_hKrautTree, WResourceAcquireMode::PointerOnly);
  if (!pTree.IsValid())
    return;

  const WGameObject* pOwner = GetOwner();
  const WTransform tOwner = pOwner->GetGlobalTransform();
  const float fGlobalUniformScale = pOwner->GetGlobalScalingSimd().HorizontalSum<3>() * WSimdFloat(1.0f / 3.0f);

  const WVec3 vLodCamPos = msg.m_pView->GetLodCamera()->GetPosition();
  const bool bIsShadowView = msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow;
  const float fDistanceSQR = (tOwner.m_vPosition - vLodCamPos).GetLengthSquared();

  const WUInt8 uiMaxLods = static_cast<WUInt8>(pTree->GetTreeLODs().GetCount());

  // Determine which single LOD to render
  WUInt8 uiActiveLod = uiMaxLods; // sentinel: no LOD selected yet

  if (m_iLodOverride >= 0 && m_iLodOverride < uiMaxLods)
  {
    uiActiveLod = static_cast<WUInt8>(m_iLodOverride);
  }
  else
  {
    // Skip LOD0 (full-detail) in distance-based selection; it is only shown via override.
    // LODs 1..N are the runtime LODs.
    for (WUInt8 uiCurLod = 1; uiCurLod < uiMaxLods; ++uiCurLod)
    {
      const auto& lodData = pTree->GetTreeLODs()[uiCurLod];
      const float fMinDistSQR = WMath::Square(fGlobalUniformScale * lodData.m_fMinLodDistance);
      const float fMaxDistSQR = WMath::Square(fGlobalUniformScale * lodData.m_fMaxLodDistance);

      if (fDistanceSQR >= fMinDistSQR && fDistanceSQR < fMaxDistSQR)
      {
        uiActiveLod = uiCurLod;
        break;
      }
    }

    if (uiActiveLod == uiMaxLods)
      return; // beyond all LOD distances: don't render
  }

  // Request the LOD mesh — returns true if ready, false if still generating.
  {
    WResourceLock<WKrautGeneratorResource> pGenerator(m_hKrautGenerator, WResourceAcquireMode::PointerOnly);
    if (!pGenerator.IsValid())
      return;

    if (!pGenerator->RequestLodMesh(m_hKrautTree, m_uiCurrentSeed, uiActiveLod, m_bForceGenerateImmediate))
    {
      // In runtime, fall back to a coarser LOD (higher index) that may already be ready,
      // rather than skipping rendering entirely for this frame.
      bool bFoundFallback = false;
      for (WUInt8 uiFallbackLod = uiActiveLod + 1; uiFallbackLod < uiMaxLods; ++uiFallbackLod)
      {
        if (pGenerator->RequestLodMesh(m_hKrautTree, m_uiCurrentSeed, uiFallbackLod, false))
        {
          uiActiveLod = uiFallbackLod;
          bFoundFallback = true;
          break;
        }
      }

      if (!bFoundFallback)
        return; // no LOD ready yet; skip this frame
    }
  }

  m_iLastRenderedLod = static_cast<WInt8>(uiActiveLod);

  const auto& lodData = pTree->GetTreeLODs()[uiActiveLod];
  if (!lodData.m_hMesh.IsValid())
    return;

  if (bIsShadowView && lodData.m_LodType != WKrautLodType::Mesh)
    return;

  // ignore scale, the shader expects the wind strength in the global 0-20 m/sec range
  const WVec3 vLocalWind = pOwner->GetGlobalRotation().GetInverse() * m_vWindSpringPos;

  const bool bDynamic = true;
  const WColor color = WColor(vLocalWind.x, vLocalWind.y, vLocalWind.z, vLocalWind.GetLength());
  const WVec4 customData = pTree->GetDetails().m_vLeafCenter.GetAsVec4(0.0f);
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, tOwner, m_InstanceDataOffset, GetUniqueIdForRendering(), color, customData);

  WResourceLock<WMeshResource> pMesh(lodData.m_hMesh, WResourceAcquireMode::AllowLoadingFallback);
  WArrayPtr<const WMeshResourceDescriptor::SubMesh> subMeshes = pMesh->GetSubMeshes();
  WArrayPtr<const WKrautTreeResourceDescriptor::MaterialData> materials = lodData.m_Materials;

  for (WUInt32 subMeshIdx = 0; subMeshIdx < subMeshes.GetCount(); ++subMeshIdx)
  {
    const WUInt32 uiMaterialIndex = subMeshes[subMeshIdx].m_uiMaterialIndex;

    if (uiMaterialIndex < materials.GetCount())
    {
      const auto& matInfo = materials[uiMaterialIndex];

      if (matInfo.m_BranchType != WKrautBranchType::None && m_bHideFrondsAndLeafs &&
          (matInfo.m_MaterialType == WKrautMaterialType::Frond || matInfo.m_MaterialType == WKrautMaterialType::Leaf))
      {
        continue;
      }
    }

    WMaterialResourceHandle hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    WMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(pOwner);
    pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
    pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, lodData.m_hMesh, uiMaterialIndex, subMeshIdx);

    msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::LitOpaque, WRenderData::Caching::Never);
  }
}

WResult WKrautTreeComponent::CreateGeometry(WGeometry& geo, WWorldGeoExtractionUtil::ExtractionMode mode) const
{
  if (GetOwner()->IsDynamic())
    return W_FAILURE;

  // EnsureTreeIsGenerated(); // not const

  if (!m_hKrautTree.IsValid())
    return W_FAILURE;

  WResourceLock<WKrautTreeResource> pTree(m_hKrautTree, WResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTree.GetAcquireResult() != WResourceAcquireResult::Final)
    return W_FAILURE;

  const auto& details = pTree->GetDetails();

  if (mode == WWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
  {
    // TODO: support to load the actual tree mesh and return it
  }
  // else
  {
    const float fHeightScale = GetOwner()->GetGlobalScalingSimd().z();
    const float fMaxScale = GetOwner()->GetGlobalScalingSimd().HorizontalMax<3>();

    if (details.m_fStaticColliderRadius * fMaxScale <= 0.0f)
      return W_FAILURE;

    const float fTreeHeight = (details.m_Bounds.m_vCenter.z + details.m_Bounds.m_vBoxHalfExtents.z) * 0.9f;

    if (fHeightScale * fTreeHeight <= 0.0f)
      return W_FAILURE;

    // using a cone or even a cylinder with a thinner top results in the character controller getting stuck while sliding along the geometry
    // TODO: instead of triangle geometry it would maybe be better to use actual physics capsules

    // due to 'transform' this will already include the tree scale
    geo.AddCylinderOnePiece(details.m_fStaticColliderRadius, details.m_fStaticColliderRadius, fTreeHeight, 0.0f, 8);

    geo.TriangulatePolygons();
  }

  return W_SUCCESS;
}

void WKrautTreeComponent::EnsureTreeIsGenerated()
{
  if (!m_hKrautGenerator.IsValid())
    return;

  WResourceLock<WKrautGeneratorResource> pResource(m_hKrautGenerator, WResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pResource.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  auto pDesc = pResource->GetDescriptor();
  if (pDesc == nullptr)
    return;

  // Compute the seed for this component instance
  WUInt32 uiSeed;
  if (m_uiCustomRandomSeed != 0xFFFF)
  {
    uiSeed = m_uiCustomRandomSeed;
  }
  else
  {
    const WUInt16 uiIndex = (m_uiVariationIndex != 0xFFFF) ? m_uiVariationIndex : static_cast<WUInt16>(GetOwner()->GetStableRandomSeed() & 0xFFFF);
    if (pDesc->m_GoodRandomSeeds.IsEmpty())
      uiSeed = uiIndex;
    else
      uiSeed = pDesc->m_GoodRandomSeeds[uiIndex % pDesc->m_GoodRandomSeeds.GetCount()];
  }

  const WKrautTreeResourceHandle hNewTree = pResource->GetOrCreateTreeResource(uiSeed);

  if (m_hKrautTree == hNewTree)
  {
    m_uiCurrentSeed = uiSeed;
    return;
  }

  // A new tree resource is needed (generator content changed or first-time setup).
  // If we were already rendering a specific LOD, wait until that LOD is ready in the new
  // tree before switching, so the old tree keeps rendering without flickering.
  if (m_iLastRenderedLod >= 0)
  {
    const WUInt32 uiRequiredLod = static_cast<WUInt32>(m_iLastRenderedLod);

    // Kick off (or force) generation of the required LOD on the new tree resource.
    pResource->RequestLodMesh(hNewTree, uiSeed, uiRequiredLod, m_bForceGenerateImmediate);

    if (!m_bForceGenerateImmediate)
    {
      // Check whether the LOD is ready yet; if not, retry next frame so the old tree keeps rendering.
      WResourceLock<WKrautTreeResource> pNewTree(hNewTree, WResourceAcquireMode::PointerOnly);
      if (pNewTree.IsValid() && uiRequiredLod < pNewTree->GetTreeLODs().GetCount() &&
          pNewTree->GetLodState(uiRequiredLod) != WKrautLodState::Ready)
      {
        GetWorld()->GetOrCreateComponentManager<WKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
        return;
      }
    }
    // When m_bForceGenerateImmediate, RequestLodMesh generated synchronously — no need to retry.
  }
  else if (m_bForceGenerateImmediate)
  {
    // No previously rendered LOD to wait for. Generate base data + the coarsest runtime LOD
    // synchronously so that bounds are valid within this same frame. This makes first-frame
    // rendering deterministic, which is required for image comparison tests.
    constexpr WUInt32 uiCoarsestRuntimeLod = 1;
    pResource->RequestLodMesh(hNewTree, uiSeed, uiCoarsestRuntimeLod, true);
  }

  m_uiCurrentSeed = uiSeed;
  m_hKrautTree = hNewTree;
  TriggerLocalBoundsUpdate();
}

void WKrautTreeComponent::ComputeWind()
{
  if (!IsActiveAndSimulating() || GetOwner()->GetVisibilityState() == WVisibilityState::Invisible)
    return;

  const WWindWorldModuleInterface* pWindInterface = GetWorld()->GetModuleReadOnly<WWindWorldModuleInterface>();

  if (!pWindInterface)
    return;

  auto pOwnder = GetOwner();

  const WVec3 vOwnerPos = pOwnder->GetGlobalPosition();
  const WVec3 vSampleWindPos = vOwnerPos + WVec3(0, 0, 2);
  const WVec3 vWindForce = pWindInterface->GetWindAt(vSampleWindPos);

  const float realTimeStep = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  // springy wind force
  {
    const float fOverallStrength = 4.0f;

    const float fSpringConstant = 1.0f;
    const float fSpringDamping = 0.5f;
    const float fTreeMass = 1.0f;

    const WVec3 vSpringForce = -(fSpringConstant * m_vWindSpringPos + fSpringDamping * m_vWindSpringVel);

    const WVec3 vTotalForce = vWindForce + vSpringForce;

    // F = mass*acc
    // acc = F / mass
    const WVec3 vTreeAcceleration = vTotalForce / fTreeMass;

    m_vWindSpringVel += vTreeAcceleration * realTimeStep * fOverallStrength;
    m_vWindSpringPos += m_vWindSpringVel * realTimeStep * fOverallStrength;
  }

  // debug draw wind vectors
  if (false)
  {
    const WVec3 offset = GetOwner()->GetGlobalPosition() + WVec3(2, 0, 1);

    WTempHybridArray<WDebugRendererLine, 2> lines;

    // actual wind
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = offset;
      l.m_end = offset + vWindForce;
      l.m_startColor = WColor::BlueViolet;
      l.m_endColor = WColor::PowderBlue;
    }

    // springy wind
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = offset;
      l.m_end = offset + m_vWindSpringPos;
      l.m_startColor = WColor::BlueViolet;
      l.m_endColor = WColor::MediumVioletRed;
    }

    // springy wind 2
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = offset;
      l.m_end = offset + m_vWindSpringPos;
      l.m_startColor = WColor::LightGoldenRodYellow;
      l.m_endColor = WColor::MediumVioletRed;
    }

    WDebugRenderer::DrawLines(GetWorld(), lines, WColor::White);

    WStringBuilder tmp;
    tmp.SetFormat("Wind: {}m/s", m_vWindSpringPos.GetLength());

    WDebugRenderer::Draw3DText(GetWorld(), tmp, GetOwner()->GetGlobalPosition() + WVec3(0, 0, 1), WColor::DeepSkyBlue);
  }
}

void WKrautTreeComponent::OnMsgExtractGeometry(WMsgExtractGeometry& ref_msg) const
{
  WStringBuilder sResourceName;
  sResourceName.SetFormat("KrautTreeCpu:{}", m_hKrautGenerator.GetResourceID());

  WCpuMeshResourceHandle hMesh = WResourceManager::GetExistingResource<WCpuMeshResource>(sResourceName);
  if (!hMesh.IsValid())
  {
    WGeometry geo;
    if (CreateGeometry(geo, ref_msg.m_Mode).Failed())
      return;

    WMeshResourceDescriptor desc;

    desc.MeshBufferDesc().AddCommonStreams();
    desc.MeshBufferDesc().AllocateStreamsFromGeometry(geo, WGALPrimitiveTopology::Triangles);

    desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

    desc.ComputeBounds();

    hMesh = WResourceManager::GetOrCreateResource<WCpuMeshResource>(sResourceName, std::move(desc), sResourceName);
  }

  ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), hMesh);
}

void WKrautTreeComponent::OnBuildStaticMesh(WMsgBuildStaticMesh& ref_msg) const
{
  WGeometry geo;
  if (CreateGeometry(geo, WWorldGeoExtractionUtil::ExtractionMode::CollisionMesh).Failed())
    return;

  auto& desc = *ref_msg.m_pStaticMeshDescription;
  auto& subMesh = ref_msg.m_pStaticMeshDescription->m_SubMeshes.ExpandAndGetRef();

  {
    WResourceLock<WKrautTreeResource> pTree(m_hKrautTree, WResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pTree.GetAcquireResult() != WResourceAcquireResult::Final)
      return;

    const auto& details = pTree->GetDetails();

    if (!details.m_sSurfaceResource.IsEmpty())
    {
      const WUInt32 uiSurfIdx = desc.m_Surfaces.IndexOf(details.m_sSurfaceResource);
      if (uiSurfIdx == WInvalidIndex)
      {
        subMesh.m_uiSurfaceIndex = static_cast<WUInt16>(desc.m_Surfaces.GetCount());
        desc.m_Surfaces.PushBack(details.m_sSurfaceResource);
      }
      else
      {
        subMesh.m_uiSurfaceIndex = static_cast<WUInt16>(uiSurfIdx);
      }
    }
  }

  const WTransform transform = GetOwner()->GetGlobalTransform();

  subMesh.m_uiFirstTriangle = desc.m_Triangles.GetCount();
  subMesh.m_uiNumTriangles = geo.GetPolygons().GetCount();

  const WUInt32 uiFirstVertex = desc.m_Vertices.GetCount();

  for (const auto& vtx : geo.GetVertices())
  {
    desc.m_Vertices.ExpandAndGetRef() = transform.TransformPosition(vtx.m_vPosition);
  }

  for (const auto& tri : geo.GetPolygons())
  {
    auto& t = desc.m_Triangles.ExpandAndGetRef();
    t.m_uiVertexIndices[0] = uiFirstVertex + tri.m_Vertices[0];
    t.m_uiVertexIndices[1] = uiFirstVertex + tri.m_Vertices[1];
    t.m_uiVertexIndices[2] = uiFirstVertex + tri.m_Vertices[2];
  }
}

//////////////////////////////////////////////////////////////////////////

void WKrautTreeComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WKrautTreeComponentManager::Update, this);
    desc.m_Phase = WWorldUpdatePhase::PreAsync;

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WKrautTreeComponentManager::UpdateWind, this);
    desc.m_Phase = WWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_uiAsyncPhaseBatchSize = 16;

    RegisterUpdateFunction(desc);
  }

  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WKrautTreeComponentManager::ResourceEventHandler, this));
}

void WKrautTreeComponentManager::Deinitialize()
{
  W_LOCK(m_Mutex);

  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WKrautTreeComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void WKrautTreeComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  WDeque<WComponentHandle> requireUpdate;

  {
    W_LOCK(m_Mutex);
    requireUpdate.Swap(m_RequireUpdate);
  }

  for (const auto& hComp : requireUpdate)
  {
    WKrautTreeComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp) || !pComp->IsActiveAndInitialized())
      continue;

    pComp->EnsureTreeIsGenerated();

    // If the tree resource exists but bounds aren't valid yet (base data task still running),
    // re-enqueue so we trigger a bounds update as soon as the async task completes.
    const WKrautTreeResourceHandle& hTree = pComp->GetKrautTreeResource();
    if (hTree.IsValid())
    {
      WResourceLock<WKrautTreeResource> pTree(hTree, WResourceAcquireMode::PointerOnly);
      if (pTree.IsValid())
      {
        if (!pTree->GetDetails().m_Bounds.IsValid())
          EnqueueUpdate(hComp);              // retry next frame
        else
          pComp->TriggerLocalBoundsUpdate(); // bounds are now ready; ensure they propagate
      }
    }
  }
}

void WKrautTreeComponentManager::UpdateWind(const WWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    WKrautTreeComponent* pComponent = it;
    pComponent->ComputeWind();
  }
}

void WKrautTreeComponentManager::EnqueueUpdate(WComponentHandle hComponent)
{
  W_LOCK(m_Mutex);

  if (m_RequireUpdate.IndexOf(hComponent) != WInvalidIndex)
    return;

  m_RequireUpdate.PushBack(hComponent);
}

void WKrautTreeComponentManager::ResourceEventHandler(const WResourceEvent& e)
{
  if ((e.m_Type == WResourceEvent::Type::ResourceContentUnloading || e.m_Type == WResourceEvent::Type::ResourceContentUpdated) && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<WKrautGeneratorResource>())
  {
    W_LOCK(m_Mutex);

    WKrautGeneratorResourceHandle hResource((WKrautGeneratorResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const WKrautTreeComponent* pComponent = static_cast<WKrautTreeComponent*>(it.Value());

      if (pComponent->GetKrautGeneratorResource() == hResource)
      {
        EnqueueUpdate(pComponent->GetHandle());
      }
    }
  }
}


W_STATICLINK_FILE(KrautPlugin, KrautPlugin_Components_KrautTreeComponent);
