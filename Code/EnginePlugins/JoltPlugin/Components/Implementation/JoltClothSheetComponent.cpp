#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>
#include <JoltPlugin/Components/JoltClothSheetComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(WJoltClothSheetFlags, 1)
  W_ENUM_CONSTANT(WJoltClothSheetFlags::FixedCornerTopLeft),
  W_ENUM_CONSTANT(WJoltClothSheetFlags::FixedCornerTopRight),
  W_ENUM_CONSTANT(WJoltClothSheetFlags::FixedCornerBottomRight),
  W_ENUM_CONSTANT(WJoltClothSheetFlags::FixedCornerBottomLeft),
  W_ENUM_CONSTANT(WJoltClothSheetFlags::FixedEdgeTop),
  W_ENUM_CONSTANT(WJoltClothSheetFlags::FixedEdgeRight),
  W_ENUM_CONSTANT(WJoltClothSheetFlags::FixedEdgeBottom),
  W_ENUM_CONSTANT(WJoltClothSheetFlags::FixedEdgeLeft),
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_COMPONENT_TYPE(WJoltClothSheetComponent, 4, WComponentMode::Static)
  {
    W_BEGIN_PROPERTIES
    {
      W_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new WDefaultValueAttribute(WVec2(0.5f, 0.5f))),
      W_ACCESSOR_PROPERTY("Segments", GetSegments, SetSegments)->AddAttributes(new WDefaultValueAttribute(WVec2U32(16, 16)), new WClampValueAttribute(WVec2U32(2, 2), WVec2U32(64, 64))),
      W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
      W_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new WDefaultValueAttribute(0.3f), new WClampValueAttribute(0.0f, 10.0f)),
      W_MEMBER_PROPERTY("GravityFactor", m_fGravityFactor)->AddAttributes(new WDefaultValueAttribute(1.0f)),
      W_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
      W_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new WDefaultValueAttribute(0.05f), new WClampValueAttribute(0.0f, 0.5f)),
      W_BITFLAGS_ACCESSOR_PROPERTY("Flags", WJoltClothSheetFlags, GetFlags, SetFlags),
      W_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material"), new WRequiredAttribute()),
      W_MEMBER_PROPERTY("TextureScale", m_vTextureScale)->AddAttributes(new WDefaultValueAttribute(WVec2(1.0f))),
      W_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new WDefaultValueAttribute(WColor::White)),
    }
    W_END_PROPERTIES;
    W_BEGIN_ATTRIBUTES
    {
      new WCategoryAttribute("Physics/Jolt/Effects"),
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

WJoltClothSheetComponent::WJoltClothSheetComponent() = default;
WJoltClothSheetComponent::~WJoltClothSheetComponent() = default;

void WJoltClothSheetComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vSize;
  s << m_vNumVertices;
  s << m_uiCollisionLayer;
  s << m_fWindInfluence;
  s << m_fGravityFactor;
  s << m_fDamping;
  s << m_Flags;
  s << m_hMaterial;
  s << m_vTextureScale;
  s << m_Color;
  s << m_fThickness;
}

void WJoltClothSheetComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  W_ASSERT_DEBUG(uiVersion >= 4, "Outdated version, please re-transform asset.");
  if (uiVersion < 4)
    return;

  auto& s = inout_stream.GetStream();

  s >> m_vSize;
  s >> m_vNumVertices;
  s >> m_uiCollisionLayer;
  s >> m_fWindInfluence;
  s >> m_fGravityFactor;
  s >> m_fDamping;
  s >> m_Flags;
  s >> m_hMaterial;
  s >> m_vTextureScale;
  s >> m_Color;
  s >> m_fThickness;
}

void WJoltClothSheetComponent::OnActivated()
{
  SUPER::OnActivated();

  SetupCloth();
}

void WJoltClothSheetComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_uiObjectFilterID == WInvalidIndex)
  {
    // only create a new filter ID, if none has been passed in manually

    WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
    m_uiObjectFilterID = pModule->CreateObjectFilterID();
  }

  SetupCloth();

  m_BodyGlobalTransform = GetOwner()->GetGlobalTransform();
}

void WJoltClothSheetComponent::OnDeactivated()
{
  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  RemoveBody();

  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  pModule->DeallocateUserData(m_uiUserDataIndex);

  pModule->DeleteObjectFilterID(m_uiObjectFilterID);

  SUPER::OnDeactivated();
}

WResult WJoltClothSheetComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  if (m_BSphere.IsValid())
  {
    ref_bounds.ExpandToInclude(WBoundingBoxSphere::MakeFromSphere(m_BSphere));
  }
  else
  {
    WBoundingBox box = WBoundingBox::MakeInvalid();
    box.ExpandToInclude(WVec3::MakeZero());
    box.ExpandToInclude(WVec3(0, 0, -0.1f));
    box.ExpandToInclude(WVec3(m_vSize.x, m_vSize.y, 0.1f));

    ref_bounds.ExpandToInclude(WBoundingBoxSphere::MakeFromBox(box));
  }

  return W_SUCCESS;
}

void WJoltClothSheetComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hDynamicMeshBuffer.IsValid())
    return;

  // Force dynamic instance data buffer since the render data is not cached, so we would trash the static instance data buffer every frame.
  const bool bDynamic = true;
  const WTransform globalTransform = IsActiveAndSimulating() ? m_BodyGlobalTransform : GetOwner()->GetGlobalTransform();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, globalTransform, m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color);

  WCustomMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WCustomMeshRenderData>(GetOwner());
  {
    pRenderData->m_uiNumInstances = 1;
    pRenderData->m_DataOffsets.m_uiInstance = m_InstanceDataOffset.m_uiOffset;
    pRenderData->m_hInstanceDataBuffer = hInstanceDataBuffer;
    pRenderData->m_fSortingDepthOffset = 0.0f;

    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_hDynamicMeshBuffer = m_hDynamicMeshBuffer;
    pRenderData->m_uiFirstPrimitive = 0;
    pRenderData->m_uiNumPrimitives = (m_vNumVertices.x - 1) * (m_vNumVertices.y - 1) * 2;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    pRenderData->m_FallbackGlobalBBox = GetOwner()->GetGlobalBounds().GetBox();
#endif

    pRenderData->FillSortingKey();
  }

  WRenderData::Category category = m_RenderDataCategory;
  if (!category.IsValid())
  {
    category = WDefaultRenderDataCategories::LitOpaque; // use as default fallback

    if (m_hMaterial.IsValid())
    {
      WResourceLock<WMaterialResource> pMaterial(m_hMaterial, WResourceAcquireMode::AllowLoadingFallback);
      category = pMaterial->GetRenderDataCategory();

      if (pMaterial.GetAcquireResult() != WResourceAcquireResult::LoadingFallback)
      {
        // if this is the final result, cache it
        m_RenderDataCategory = category;
      }
    }
  }

  msg.AddRenderData(pRenderData, category, WRenderData::Caching::Never);
}

void WJoltClothSheetComponent::SetSize(WVec2 vVal)
{
  m_vSize = vVal;
  SetupCloth();
}

void WJoltClothSheetComponent::SetSegments(WVec2U32 vVal)
{
  m_vNumVertices = vVal;
  SetupCloth();
}

void WJoltClothSheetComponent::SetFlags(WBitflags<WJoltClothSheetFlags> flags)
{
  m_Flags = flags;
  SetupCloth();
}

void WJoltClothSheetComponent::UpdatePreAsync()
{
  if (GetOwner()->GetVisibilityState(60) == WVisibilityState::Direct)
  {
    // only apply wind to directly visible pieces of cloth
    ApplyWind();
  }
}

void WJoltClothSheetComponent::UpdatePostAsync()
{
  const WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  const JPH::BodyLockInterface* pLi = &pSystem->GetBodyLockInterface();

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (bodyId.IsInvalid())
    return;

  // Get the body
  JPH::BodyLockRead lock(*pLi, bodyId);
  if (!lock.SucceededAndIsInBroadPhase())
    return;

  const JPH::Body& body = lock.GetBody();

  {
    WBoundingSphere prevBounds = m_BSphere;

    // TODO: should rather iterate over all active (soft) bodies, than to check this here
    if (!body.IsActive())
      return;

    const JPH::AABox box = body.GetWorldSpaceBounds();

    const WTransform t = GetOwner()->GetGlobalTransform().GetInverse();

    m_BSphere.m_vCenter = t.TransformPosition(WJoltConversionUtils::ToVec3(box.GetCenter()));

    const WVec3 ext = WJoltConversionUtils::ToVec3(box.GetExtent());
    m_BSphere.m_fRadius = WMath::Max(ext.x, ext.y, ext.z);

    if (prevBounds != m_BSphere)
    {
      TriggerLocalBoundsUpdate();
    }
  }

  // Don't update mesh and transform when invisible
  if (GetOwner()->GetVisibilityState() == WVisibilityState::Invisible)
    return;

  {
    const JPH::SoftBodyMotionProperties* pMotion = static_cast<const JPH::SoftBodyMotionProperties*>(body.GetMotionProperties());
    const JPH::Array<JPH::SoftBodyMotionProperties::Vertex>& particles = pMotion->GetVertices();

    WResourceLock<WDynamicMeshBufferResource> pDynamicMeshBuffer(m_hDynamicMeshBuffer, WResourceAcquireMode::BlockTillLoaded);
    auto positions = pDynamicMeshBuffer->AccessPositionData();

    const WVec2U32 vNumVertices = m_vNumVertices;

    WUInt32 vidx = 0;
    for (WUInt32 y = 0; y < vNumVertices.y; ++y)
    {
      for (WUInt32 x = 0; x < vNumVertices.x; ++x, ++vidx)
      {
        positions[vidx] = WJoltConversionUtils::ToVec3(particles[vidx].mPosition);
      }
    }

    WDynamicMeshBufferResource::CalculateGridNormalAndTangents(pDynamicMeshBuffer.GetPointerNonConst(), vNumVertices);
  }

  {
    m_BodyGlobalTransform = GetOwner()->GetGlobalTransform();

    const auto& transformed_shape = body.GetTransformedShape();
    JPH::RMat44 matrix = transformed_shape.GetCenterOfMassTransform();

    m_BodyGlobalTransform.m_vPosition = WJoltConversionUtils::ToVec3(matrix.GetTranslation());
    m_BodyGlobalTransform.m_qRotation = WJoltConversionUtils::ToQuat(matrix.GetRotation().GetQuaternion());
  }
}


void WJoltClothSheetComponent::ApplyWind()
{
  if (m_fWindInfluence <= 0.0f)
    return;

  if (!m_BSphere.IsValid())
    return;

  if (const WWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<WWindWorldModuleInterface>())
  {
    const WVec3 vSamplePos = GetOwner()->GetGlobalTransform().TransformPosition(m_BSphere.m_vCenter);

    const WVec3 vWind = pWind->GetWindAt(vSamplePos) * m_fWindInfluence;

    if (!vWind.IsZero())
    {
      WVec3 windForce = vWind;
      windForce += pWind->ComputeWindFlutter(vWind, vWind.GetOrthogonalVector(), 5.0f, GetOwner()->GetStableRandomSeed());

      JPH::Vec3 windVel = WJoltConversionUtils::ToVec3(windForce);

      WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
      auto* pSystem = pModule->GetJoltSystem();
      const JPH::BodyLockInterface* pLi = &pSystem->GetBodyLockInterface();

      JPH::BodyID bodyId(m_uiJoltBodyID);

      if (bodyId.IsInvalid())
        return;

      // Get write access to the body
      JPH::BodyLockWrite lock(*pLi, bodyId);
      if (!lock.SucceededAndIsInBroadPhase())
        return;

      JPH::Body& body = lock.GetBody();
      JPH::SoftBodyMotionProperties* pMotion = static_cast<JPH::SoftBodyMotionProperties*>(body.GetMotionProperties());

      if (!body.IsActive())
      {
        pSystem->GetBodyInterfaceNoLock().ActivateBody(bodyId);
      }

      JPH::Array<JPH::SoftBodyMotionProperties::Vertex>& particles = pMotion->GetVertices();

      // randomize which vertices get the wind velocity applied,
      // both to save performance and also to introduce a nice ripple effect
      const WUInt32 uiStart = GetWorld()->GetRandomNumberGenerator().UIntInRange(WMath::Min<WUInt32>(16u, (WUInt32)particles.size()));
      const WUInt32 uiStep = GetWorld()->GetRandomNumberGenerator().IntMinMax(16, 16 + 32);

      for (WUInt32 i = uiStart; i < particles.size(); i += uiStep)
      {
        if (particles[i].mInvMass > 0)
        {
          particles[i].mVelocity = windVel;
        }
      }
    }
  }
}

static JPH::Ref<JPH::SoftBodySharedSettings> CreateCloth(WVec2U32 vNumVertices, WVec2 vSpacing, WBitflags<WJoltClothSheetFlags> flags, float fPerVertexMass)
{
  // Create settings
  JPH::SoftBodySharedSettings* settings = new JPH::SoftBodySharedSettings;

  const float fInvVtxMass = 1.0f / fPerVertexMass;

  for (WUInt32 y = 0; y < vNumVertices.y; ++y)
  {
    for (WUInt32 x = 0; x < vNumVertices.x; ++x)
    {
      JPH::SoftBodySharedSettings::Vertex v;
      v.mPosition = JPH::Float3(x * vSpacing.x, y * vSpacing.y, 0.0f);
      v.mInvMass = fInvVtxMass;
      settings->mVertices.push_back(v);
    }
  }

  // Function to get the vertex index of a point on the cloth
  auto GetIdx = [vNumVertices](WUInt32 x, WUInt32 y) -> WUInt32
  {
    return x + y * vNumVertices.x;
  };

  if (flags.IsAnyFlagSet())
  {
    if (flags.IsSet(WJoltClothSheetFlags::FixedCornerTopLeft))
    {
      settings->mVertices[GetIdx(0, 0)].mInvMass = 0.0f;
    }

    if (flags.IsSet(WJoltClothSheetFlags::FixedCornerTopRight))
    {
      settings->mVertices[GetIdx(vNumVertices.x - 1, 0)].mInvMass = 0.0f;
    }

    if (flags.IsSet(WJoltClothSheetFlags::FixedCornerBottomLeft))
    {
      settings->mVertices[GetIdx(0, vNumVertices.y - 1)].mInvMass = 0.0f;
    }

    if (flags.IsSet(WJoltClothSheetFlags::FixedCornerBottomRight))
    {
      settings->mVertices[GetIdx(vNumVertices.x - 1, vNumVertices.y - 1)].mInvMass = 0.0f;
    }

    if (flags.IsSet(WJoltClothSheetFlags::FixedEdgeTop))
    {
      for (WUInt32 x = 0; x < vNumVertices.x; ++x)
      {
        settings->mVertices[GetIdx(x, 0)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(WJoltClothSheetFlags::FixedEdgeBottom))
    {
      for (WUInt32 x = 0; x < vNumVertices.x; ++x)
      {
        settings->mVertices[GetIdx(x, vNumVertices.y - 1)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(WJoltClothSheetFlags::FixedEdgeLeft))
    {
      for (WUInt32 y = 0; y < vNumVertices.y; ++y)
      {
        settings->mVertices[GetIdx(0, y)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(WJoltClothSheetFlags::FixedEdgeRight))
    {
      for (WUInt32 y = 0; y < vNumVertices.y; ++y)
      {
        settings->mVertices[GetIdx(vNumVertices.x - 1, y)].mInvMass = 0.0f;
      }
    }
  }

  // Create edges
  for (WUInt32 y = 0; y < vNumVertices.y; ++y)
  {
    for (WUInt32 x = 0; x < vNumVertices.x; ++x)
    {
      JPH::SoftBodySharedSettings::Edge e;
      e.mCompliance = 0.00001f;
      e.mVertex[0] = GetIdx(x, y);
      if (x < vNumVertices.x - 1)
      {
        e.mVertex[1] = GetIdx(x + 1, y);
        settings->mEdgeConstraints.push_back(e);
      }
      if (y < vNumVertices.y - 1)
      {
        e.mVertex[1] = GetIdx(x, y + 1);
        settings->mEdgeConstraints.push_back(e);
      }
      if (x < vNumVertices.x - 1 && y < vNumVertices.y - 1)
      {
        e.mVertex[1] = GetIdx(x + 1, y + 1);
        settings->mEdgeConstraints.push_back(e);

        e.mVertex[0] = GetIdx(x + 1, y);
        e.mVertex[1] = GetIdx(x, y + 1);
        settings->mEdgeConstraints.push_back(e);
      }
    }
  }

  settings->CalculateEdgeLengths();

  // Create faces
  for (WUInt32 y = 0; y < vNumVertices.y - 1; ++y)
  {
    for (WUInt32 x = 0; x < vNumVertices.x - 1; ++x)
    {
      JPH::SoftBodySharedSettings::Face f;
      f.mVertex[0] = GetIdx(x, y);
      f.mVertex[1] = GetIdx(x, y + 1);
      f.mVertex[2] = GetIdx(x + 1, y + 1);
      settings->AddFace(f);

      f.mVertex[1] = GetIdx(x + 1, y + 1);
      f.mVertex[2] = GetIdx(x + 1, y);
      settings->AddFace(f);
    }
  }

  settings->Optimize();

  return settings;
}


void WJoltClothSheetComponent::SetupCloth()
{
  m_BSphere = WBoundingSphere::MakeInvalid();

  if (IsActiveAndSimulating())
  {
    RemoveBody();

    float fPerVertexMass = 1.0f; // default value

    WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    JPH::Ref<JPH::SoftBodySharedSettings> settings = CreateCloth(m_vNumVertices, m_vSize.CompDiv(WVec2(static_cast<float>(m_vNumVertices.x - 1), static_cast<float>(m_vNumVertices.y - 1))), m_Flags, fPerVertexMass);

    WTransform t = GetOwner()->GetGlobalTransform();

    WJoltUserData* pUserData = nullptr;
    m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
    pUserData->Init(this);

    JPH::SoftBodyCreationSettings cloth(settings, WJoltConversionUtils::ToVec3(t.m_vPosition), WJoltConversionUtils::ToQuat(t.m_qRotation), WJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, WJoltBroadphaseLayer::Cloth));

    cloth.mVertexRadius = m_fThickness;
    cloth.mPressure = 0.0f;
    cloth.mLinearDamping = m_fDamping;
    cloth.mGravityFactor = m_fGravityFactor;
    cloth.mUserData = reinterpret_cast<WUInt64>(pUserData);
    cloth.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
    // cloth.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // the group filter is only needed for objects constrained via joints

    auto pBody = pBodies->CreateSoftBody(cloth);

    m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

    pModule->QueueBodyToAdd(pBody, true);
  }

  if (IsActiveAndInitialized() && m_vSize.x > 0 && m_vSize.y > 0 && m_vNumVertices.x > 1 && m_vNumVertices.y > 1)
  {
    WStringBuilder sResourceName;
    sResourceName.SetFormat("JoltClothSheet_{}_{}x{}_{}x{}_{}x{}", WArgP(this), m_vSize.x, m_vSize.y, m_vNumVertices.x, m_vNumVertices.y, m_vTextureScale.x, m_vTextureScale.y);

    m_hDynamicMeshBuffer = WResourceManager::GetExistingResource<WDynamicMeshBufferResource>(sResourceName);

    if (!m_hDynamicMeshBuffer.IsValid())
    {
      WDynamicMeshBufferResourceDescriptor desc;
      desc.m_uiMaxVertices = m_vNumVertices.x * m_vNumVertices.y;
      desc.m_IndexType = WGALIndexType::UShort;
      desc.m_uiMaxPrimitives = (m_vNumVertices.x - 1) * (m_vNumVertices.y - 1) * 2;

      m_hDynamicMeshBuffer = WResourceManager::GetOrCreateResource<WDynamicMeshBufferResource>(sResourceName, std::move(desc));
    }

    WResourceLock<WDynamicMeshBufferResource> pDynamicMeshBuffer(m_hDynamicMeshBuffer, WResourceAcquireMode::BlockTillLoaded);
    WDynamicMeshBufferResource::CreateGridXY(pDynamicMeshBuffer.GetPointerNonConst(), m_vSize, m_vNumVertices);
  }

  TriggerLocalBoundsUpdate();
}

void WJoltClothSheetComponent::RemoveBody()
{
  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (!bodyId.IsInvalid())
  {
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    if (pBodies->IsAdded(bodyId))
    {
      pBodies->RemoveBody(bodyId);
    }

    pBodies->DestroyBody(bodyId);
    m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
  }

  // TODO: currently not yet needed
  // pModule->DeallocateUserData(m_uiUserDataIndex);
  // pModule->DeleteObjectFilterID(m_uiObjectFilterID);
}

//////////////////////////////////////////////////////////////////////////

WJoltClothSheetComponentManager::WJoltClothSheetComponentManager(WWorld* pWorld)
  : WComponentManager(pWorld)
{
}

WJoltClothSheetComponentManager::~WJoltClothSheetComponentManager() = default;

void WJoltClothSheetComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WJoltClothSheetComponentManager::UpdatePreAsync, this);
    desc.m_Phase = WWorldUpdatePhase::PreAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WJoltClothSheetComponentManager::UpdatePostAsync, this);
    desc.m_Phase = WWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void WJoltClothSheetComponentManager::UpdatePreAsync(const WWorldModule::UpdateContext& context)
{
  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  if (pModule == nullptr || pModule->GetJoltUpdateCounter() == m_uiLastJoltUpdateCounter)
  {
    // skip cloth updates, when there was no Jolt update yet
    return;
  }

  m_uiLastJoltUpdateCounter = pModule->GetJoltUpdateCounter();

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->UpdatePreAsync();
    }
  }
}

void WJoltClothSheetComponentManager::UpdatePostAsync(const WWorldModule::UpdateContext& context)
{
  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  if (pModule == nullptr || pModule->GetJoltUpdateCounter() == m_uiLastJoltUpdateCounter)
  {
    // skip cloth updates, when there was no Jolt update yet
    return;
  }

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->UpdatePostAsync();
    }
  }
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltClothSheetComponent);
