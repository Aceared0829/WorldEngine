#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Physics/ClothSheetComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(WClothSheetFlags, 1)
  W_ENUM_CONSTANT(WClothSheetFlags::FixedCornerTopLeft),
  W_ENUM_CONSTANT(WClothSheetFlags::FixedCornerTopRight),
  W_ENUM_CONSTANT(WClothSheetFlags::FixedCornerBottomRight),
  W_ENUM_CONSTANT(WClothSheetFlags::FixedCornerBottomLeft),
  W_ENUM_CONSTANT(WClothSheetFlags::FixedEdgeTop),
  W_ENUM_CONSTANT(WClothSheetFlags::FixedEdgeRight),
  W_ENUM_CONSTANT(WClothSheetFlags::FixedEdgeBottom),
  W_ENUM_CONSTANT(WClothSheetFlags::FixedEdgeLeft),
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_COMPONENT_TYPE(WClothSheetComponent, 1, WComponentMode::Static)
  {
    W_BEGIN_PROPERTIES
    {
      W_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new WDefaultValueAttribute(WVec2(0.5f, 0.5f))),
      W_ACCESSOR_PROPERTY("Slack", GetSlack, SetSlack)->AddAttributes(new WDefaultValueAttribute(WVec2(0.0f, 0.0f))),
      W_ACCESSOR_PROPERTY("Segments", GetSegments, SetSegments)->AddAttributes(new WDefaultValueAttribute(WVec2U32(7, 7)), new WClampValueAttribute(WVec2U32(1, 1), WVec2U32(31, 31))),
      W_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
      W_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 10.0f)),
      W_BITFLAGS_ACCESSOR_PROPERTY("Flags", WClothSheetFlags, GetFlags, SetFlags),
      W_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material"), new WRequiredAttribute()),
      W_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new WDefaultValueAttribute(WColor::White)),
    }
    W_END_PROPERTIES;
    W_BEGIN_ATTRIBUTES
    {
      new WCategoryAttribute("Effects"),
    }
    W_END_ATTRIBUTES;
    W_BEGIN_MESSAGEHANDLERS
    {
      W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    }
    W_END_MESSAGEHANDLERS;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WClothSheetComponent::WClothSheetComponent() = default;
WClothSheetComponent::~WClothSheetComponent() = default;

void WClothSheetComponent::SetSize(WVec2 vVal)
{
  m_vSize = vVal;
  SetupCloth();
}

void WClothSheetComponent::SetSlack(WVec2 vVal)
{
  m_vSlack = vVal;
  SetupCloth();
}

void WClothSheetComponent::SetSegments(WVec2U32 vVal)
{
  m_vSegments = vVal;
  SetupCloth();
}

void WClothSheetComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vSize;
  s << m_vSegments;
  s << m_vSlack;
  s << m_fWindInfluence;
  s << m_fDamping;
  s << m_Flags;
  s << m_hMaterial;
  s << m_Color;
}

void WClothSheetComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_vSize;
  s >> m_vSegments;
  s >> m_vSlack;
  s >> m_fWindInfluence;
  s >> m_fDamping;
  s >> m_Flags;
  s >> m_hMaterial;
  s >> m_Color;
}

void WClothSheetComponent::OnActivated()
{
  SUPER::OnActivated();

  SetupCloth();
}

void WClothSheetComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  SetupCloth();
}

void WClothSheetComponent::OnDeactivated()
{
  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  m_Simulator.m_Nodes.Clear();

  SUPER::OnDeactivated();
}

WResult WClothSheetComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  if (m_Bbox.IsValid())
  {
    ref_bounds.ExpandToInclude(WBoundingBoxSphere::MakeFromBox(m_Bbox));
  }
  else
  {
    WBoundingBox box = WBoundingBox::MakeInvalid();
    box.ExpandToInclude(WVec3::MakeZero());
    box.ExpandToInclude(WVec3(m_vSize.x, 0, -0.1f));
    box.ExpandToInclude(WVec3(0, m_vSize.y, +0.1f));
    box.ExpandToInclude(WVec3(m_vSize.x, m_vSize.y, 0));

    ref_bounds.ExpandToInclude(WBoundingBoxSphere::MakeFromBox(box));
  }

  return W_SUCCESS;
}

void WClothSheetComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hDynamicMeshBuffer.IsValid())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color);

  WCustomMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WCustomMeshRenderData>(GetOwner());
  {
    pRenderData->m_uiNumInstances = 1;
    pRenderData->m_DataOffsets.m_uiInstance = m_InstanceDataOffset.m_uiOffset;
    pRenderData->m_hInstanceDataBuffer = hInstanceDataBuffer;
    pRenderData->m_fSortingDepthOffset = 0.0f;

    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_hDynamicMeshBuffer = m_hDynamicMeshBuffer;
    pRenderData->m_uiFirstPrimitive = 0;
    pRenderData->m_uiNumPrimitives = m_vSegments.x * m_vSegments.y * 2;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    pRenderData->m_FallbackGlobalBBox = GetOwner()->GetGlobalBounds().GetBox();
#endif

    pRenderData->FillSortingKey();
  }

  WRenderData::Category category = WDefaultRenderDataCategories::LitOpaque;
  bool bDontCacheYet = true;

  if (m_hMaterial.IsValid())
  {
    WResourceLock<WMaterialResource> pMaterial(m_hMaterial, WResourceAcquireMode::AllowLoadingFallback);
    category = pMaterial->GetRenderDataCategory();
    bDontCacheYet = pMaterial.GetAcquireResult() == WResourceAcquireResult::LoadingFallback;
  }

  msg.AddRenderData(pRenderData, category, bDontCacheYet ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic);
}

void WClothSheetComponent::SetFlags(WBitflags<WClothSheetFlags> flags)
{
  m_Flags = flags;
  SetupCloth();
}

void WClothSheetComponent::Update()
{
  if (m_Simulator.m_Nodes.IsEmpty() || GetOwner()->GetVisibilityState() == WVisibilityState::Invisible)
    return;

  {
    WVec3 acc = -GetOwner()->GetLinearVelocity();

    if (const WPhysicsWorldModuleInterface* pModule = GetWorld()->GetModuleReadOnly<WPhysicsWorldModuleInterface>())
    {
      acc += pModule->GetGravity();
    }
    else
    {
      acc += WVec3(0, 0, -9.81f);
    }

    if (m_fWindInfluence > 0.0f)
    {
      if (const WWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<WWindWorldModuleInterface>())
      {
        WVec3 ropeDir(0, 0, 1);

        // take the position of the center cloth node to sample the wind
        const WVec3 vSampleWindPos = GetOwner()->GetGlobalTransform().TransformPosition(WSimdConversion::ToVec3(m_Simulator.m_Nodes[m_Simulator.m_uiWidth * (m_Simulator.m_uiHeight / 2) + m_Simulator.m_uiWidth / 2].m_vPosition));

        const WVec3 vWind = pWind->GetWindAt(vSampleWindPos) * m_fWindInfluence;

        acc += vWind;
        acc += pWind->ComputeWindFlutter(vWind, ropeDir, 0.5f, GetOwner()->GetStableRandomSeed());
      }
    }

    // rotate the acceleration vector into the local simulation space
    acc = GetOwner()->GetGlobalRotation().GetInverse() * acc;

    if (m_Simulator.m_vAcceleration != acc)
    {
      m_Simulator.m_vAcceleration = acc;
      m_uiSleepCounter = 0;
    }
  }

  if (m_uiSleepCounter <= 10)
  {
    m_Simulator.m_fDampingFactor = WMath::Lerp(1.0f, 0.97f, m_fDamping);

    m_Simulator.SimulateCloth(GetWorld()->GetClock().GetTimeDiff());

    auto prevBbox = m_Bbox;
    m_Bbox.ExpandToInclude(WSimdConversion::ToVec3(m_Simulator.m_Nodes[0].m_vPosition));
    m_Bbox.ExpandToInclude(WSimdConversion::ToVec3(m_Simulator.m_Nodes[m_Simulator.m_uiWidth - 1].m_vPosition));
    m_Bbox.ExpandToInclude(WSimdConversion::ToVec3(m_Simulator.m_Nodes[((m_Simulator.m_uiHeight - 1) * m_Simulator.m_uiWidth)].m_vPosition));
    m_Bbox.ExpandToInclude(WSimdConversion::ToVec3(m_Simulator.m_Nodes.PeekBack().m_vPosition));

    if (prevBbox != m_Bbox)
    {
      SetUserFlag(0, true); // flag 0 => requires local bounds update

      // can't call this here in the async phase
      // TriggerLocalBoundsUpdate();
    }

    UpdateClothMesh();

    ++m_uiCheckEquilibriumCounter;
    if (m_uiCheckEquilibriumCounter > 64)
    {
      m_uiCheckEquilibriumCounter = 0;

      if (m_Simulator.HasEquilibrium(0.01f))
      {
        ++m_uiSleepCounter;
      }
      else
      {
        m_uiSleepCounter = 0;
      }
    }
  }
}

void WClothSheetComponent::UpdateClothMesh()
{
  WResourceLock<WDynamicMeshBufferResource> pDynamicMeshBuffer(m_hDynamicMeshBuffer, WResourceAcquireMode::BlockTillLoaded);

  auto nodes = m_Simulator.m_Nodes.GetArrayPtr();
  auto positions = pDynamicMeshBuffer->AccessPositionData();

  const WVec2U32 vNumVertices = m_vSegments + WVec2U32(1);

  WUInt32 vidx = 0;
  for (WUInt32 y = 0; y < vNumVertices.y; ++y)
  {
    for (WUInt32 x = 0; x < vNumVertices.x; ++x, ++vidx)
    {
      positions[vidx] = WSimdConversion::ToVec3(nodes[vidx].m_vPosition);
    }
  }

  WDynamicMeshBufferResource::CalculateGridNormalAndTangents(pDynamicMeshBuffer.GetPointerNonConst(), vNumVertices);
}

void WClothSheetComponent::SetupCloth()
{
  m_Bbox = WBoundingBox::MakeInvalid();

  if (IsActiveAndSimulating())
  {
    m_uiSleepCounter = 0;

    m_Simulator.m_uiWidth = static_cast<WUInt8>(m_vSegments.x + 1);
    m_Simulator.m_uiHeight = static_cast<WUInt8>(m_vSegments.y + 1);
    m_Simulator.m_vAcceleration.Set(0, 0, -10);
    m_Simulator.m_vSegmentLength = m_vSize.CompMul(WVec2(1.0f) + m_vSlack);
    m_Simulator.m_vSegmentLength.x /= (float)m_vSegments.x;
    m_Simulator.m_vSegmentLength.y /= (float)m_vSegments.y;
    m_Simulator.m_Nodes.Clear();
    m_Simulator.m_Nodes.SetCount(m_Simulator.m_uiWidth * m_Simulator.m_uiHeight);

    const WVec3 dirX = WVec3(1, 0, 0);
    const WVec3 dirY = WVec3(0, 1, 0);

    WVec2 dist = m_vSize;
    dist.x /= (float)m_vSegments.x;
    dist.y /= (float)m_vSegments.y;

    for (WUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
    {
      for (WUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const WUInt32 idx = (y * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_vPosition = WSimdConversion::ToVec3(x * dist.x * dirX + y * dist.y * dirY);
        m_Simulator.m_Nodes[idx].m_vPreviousPosition = m_Simulator.m_Nodes[idx].m_vPosition;
      }
    }

    if (m_Flags.IsSet(WClothSheetFlags::FixedCornerTopLeft))
      m_Simulator.m_Nodes[0].m_bFixed = true;

    if (m_Flags.IsSet(WClothSheetFlags::FixedCornerTopRight))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth - 1].m_bFixed = true;

    if (m_Flags.IsSet(WClothSheetFlags::FixedCornerBottomRight))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth * m_Simulator.m_uiHeight - 1].m_bFixed = true;

    if (m_Flags.IsSet(WClothSheetFlags::FixedCornerBottomLeft))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth * (m_Simulator.m_uiHeight - 1)].m_bFixed = true;

    if (m_Flags.IsSet(WClothSheetFlags::FixedEdgeTop))
    {
      for (WUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const WUInt32 idx = (0 * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(WClothSheetFlags::FixedEdgeRight))
    {
      for (WUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
      {
        const WUInt32 idx = (y * m_Simulator.m_uiWidth) + (m_Simulator.m_uiWidth - 1);

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(WClothSheetFlags::FixedEdgeBottom))
    {
      for (WUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const WUInt32 idx = ((m_Simulator.m_uiHeight - 1) * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(WClothSheetFlags::FixedEdgeLeft))
    {
      for (WUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
      {
        const WUInt32 idx = (y * m_Simulator.m_uiWidth) + 0;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }
  }

  if (IsActiveAndInitialized() && m_vSize.x > 0 && m_vSize.y > 0 && m_vSegments.x > 0 && m_vSegments.y > 0)
  {
    WStringBuilder sResourceName;
    sResourceName.SetFormat("ClothSheet_{}_{}x{}_{}x{}", WArgP(this), m_vSize.x, m_vSize.y, m_vSegments.x, m_vSegments.y);

    m_hDynamicMeshBuffer = WResourceManager::GetExistingResource<WDynamicMeshBufferResource>(sResourceName);

    if (!m_hDynamicMeshBuffer.IsValid())
    {
      WDynamicMeshBufferResourceDescriptor desc;
      desc.m_uiMaxVertices = (m_vSegments.x + 1) * (m_vSegments.y + 1);
      desc.m_IndexType = WGALIndexType::UShort;
      desc.m_uiMaxPrimitives = m_vSegments.x * m_vSegments.y * 2;

      m_hDynamicMeshBuffer = WResourceManager::GetOrCreateResource<WDynamicMeshBufferResource>(sResourceName, std::move(desc));
    }

    WResourceLock<WDynamicMeshBufferResource> pDynamicMeshBuffer(m_hDynamicMeshBuffer, WResourceAcquireMode::BlockTillLoaded);
    WDynamicMeshBufferResource::CreateGridXY(pDynamicMeshBuffer.GetPointerNonConst(), m_vSize, m_vSegments + WVec2U32(1));
  }

  TriggerLocalBoundsUpdate();
}

//////////////////////////////////////////////////////////////////////////

WClothSheetComponentManager::WClothSheetComponentManager(WWorld* pWorld)
  : WComponentManager(pWorld)
{
}

WClothSheetComponentManager::~WClothSheetComponentManager() = default;

void WClothSheetComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WClothSheetComponentManager::Update, this);
    desc.m_Phase = WWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_uiAsyncPhaseBatchSize = 2;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WClothSheetComponentManager::UpdateBounds, this);
    desc.m_Phase = WWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void WClothSheetComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update();
    }
  }
}

void WClothSheetComponentManager::UpdateBounds(const WWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUserFlag(0))
    {
      it->TriggerLocalBoundsUpdate();

      // reset update bounds flag
      it->SetUserFlag(0, false);
    }
  }
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Physics_Implementation_ClothSheetComponent);
