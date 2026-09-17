#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/SensorComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgSensorDetectedObjectsChanged);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgSensorDetectedObjectsChanged, 1, WRTTIDefaultAllocator<WMsgSensorDetectedObjectsChanged>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY_READ_ONLY("DetectedObjects", m_DetectedObjects),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_BEGIN_ABSTRACT_COMPONENT_TYPE(WSensorComponent, 2)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("UpdateRate", WUpdateRate, m_UpdateRate),
    W_ACCESSOR_PROPERTY("SpatialCategory", GetSpatialCategory, SetSpatialCategory)->AddAttributes(new WDynamicStringEnumAttribute("SpatialDataCategoryEnum")),
    W_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new WTagSetWidgetAttribute("Default")),
    W_SET_MEMBER_PROPERTY("ExcludeTags", m_ExcludeTags)->AddAttributes(new WTagSetWidgetAttribute("Default")),
    W_MEMBER_PROPERTY("TestVisibility", m_bTestVisibility)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new WDefaultValueAttribute(WColorScheme::LightUI(WColorScheme::Orange))),
  }
  W_END_PROPERTIES;


  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(GetDetectedObjectsCount),
    W_SCRIPT_FUNCTION_PROPERTY(GetDetectedObject, In, "uiIndex"),
  }
  W_END_FUNCTIONS;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("AI/Sensors"),
  }
  W_END_ATTRIBUTES;
}
W_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

WSensorComponent::WSensorComponent() = default;
WSensorComponent::~WSensorComponent() = default;

void WSensorComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sSpatialCategory;
  s << m_bTestVisibility;
  s << m_uiCollisionLayer;
  s << m_UpdateRate;
  s << m_bShowDebugInfo;
  s << m_Color;
  m_IncludeTags.Save(s);
  m_ExcludeTags.Save(s);
}

void WSensorComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sSpatialCategory;
  s >> m_bTestVisibility;
  s >> m_uiCollisionLayer;
  s >> m_UpdateRate;
  s >> m_bShowDebugInfo;
  s >> m_Color;
  if (uiVersion >= 2)
  {
    m_IncludeTags.Load(s, WTagRegistry::GetGlobalRegistry());
    m_ExcludeTags.Load(s, WTagRegistry::GetGlobalRegistry());
  }
}

void WSensorComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateSpatialCategory();
  UpdateScheduling();
  UpdateDebugInfo();
}

void WSensorComponent::OnDeactivated()
{
  auto pModule = GetWorld()->GetOrCreateModule<WSensorWorldModule>();
  pModule->RemoveComponentToSchedule(this);
  pModule->RemoveComponentForDebugRendering(this);

  SUPER::OnDeactivated();
}

void WSensorComponent::SetSpatialCategory(const char* szCategory)
{
  m_sSpatialCategory.Assign(szCategory);

  if (IsActiveAndInitialized())
  {
    UpdateSpatialCategory();
  }
}

const char* WSensorComponent::GetSpatialCategory() const
{
  return m_sSpatialCategory;
}

void WSensorComponent::SetUpdateRate(const WEnum<WUpdateRate>& updateRate)
{
  if (m_UpdateRate == updateRate)
    return;

  m_UpdateRate = updateRate;

  if (IsActiveAndInitialized())
  {
    UpdateScheduling();
  }
}

const WEnum<WUpdateRate>& WSensorComponent::GetUpdateRate() const
{
  return m_UpdateRate;
}

void WSensorComponent::SetShowDebugInfo(bool bShow)
{
  if (m_bShowDebugInfo == bShow)
    return;

  m_bShowDebugInfo = bShow;

  if (IsActiveAndInitialized())
  {
    UpdateDebugInfo();
  }
}

bool WSensorComponent::GetShowDebugInfo() const
{
  return m_bShowDebugInfo;
}

void WSensorComponent::SetColor(WColorGammaUB color)
{
  m_Color = color;
}

WColorGammaUB WSensorComponent::GetColor() const
{
  return m_Color;
}

bool WSensorComponent::RunSensorCheck(WPhysicsWorldModuleInterface* pPhysicsWorldModule, WDynamicArray<WGameObject*>& out_objectsInSensorVolume, WDynamicArray<WGameObjectHandle>& ref_detectedObjects, bool bPostChangeMsg) const
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  m_LastOccludedObjectPositions.Clear();
#endif

  m_bHadUpdate = true;
  out_objectsInSensorVolume.Clear();

  GetObjectsInSensorVolume(out_objectsInSensorVolume);
  const WGameObject* pSensorOwner = GetOwner();

  ref_detectedObjects.Clear();

  if (m_bTestVisibility && pPhysicsWorldModule)
  {
    const WVec3 rayStart = pSensorOwner->GetGlobalPosition();
    for (auto pObject : out_objectsInSensorVolume)
    {
      const WVec3 rayEnd = pObject->GetGlobalPosition();
      WVec3 rayDir = rayEnd - rayStart;
      const float fDistance = rayDir.GetLengthAndNormalize();

      WPhysicsCastResult hitResult;
      WPhysicsQueryParameters params(m_uiCollisionLayer);
      params.m_bIgnoreInitialOverlap = true;
      params.m_ShapeTypes = WPhysicsShapeType::Default;

      // TODO: probably best to expose the WPhysicsShapeType bitflags on the component
      params.m_ShapeTypes.Remove(WPhysicsShapeType::Rope);
      params.m_ShapeTypes.Remove(WPhysicsShapeType::Ragdoll);
      params.m_ShapeTypes.Remove(WPhysicsShapeType::Trigger);
      params.m_ShapeTypes.Remove(WPhysicsShapeType::Query);
      params.m_ShapeTypes.Remove(WPhysicsShapeType::Character);

      if (pPhysicsWorldModule->Raycast(hitResult, rayStart, rayDir, fDistance, params))
      {
        // hit something in between -> not visible
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
        m_LastOccludedObjectPositions.PushBack(rayEnd);
#endif

        continue;
      }

      ref_detectedObjects.PushBack(pObject->GetHandle());
    }
  }
  else
  {
    for (auto pObject : out_objectsInSensorVolume)
    {
      ref_detectedObjects.PushBack(pObject->GetHandle());
    }
  }

  ref_detectedObjects.Sort();
  if (ref_detectedObjects == m_LastDetectedObjects)
    return false;

  ref_detectedObjects.Swap(m_LastDetectedObjects);

  if (bPostChangeMsg)
  {
    WMsgSensorDetectedObjectsChanged msg;
    msg.m_DetectedObjects = m_LastDetectedObjects;
    pSensorOwner->PostEventMessage(msg, this, WTime::MakeZero(), WObjectMsgQueueType::PostAsync);
  }

  return true;
}

void WSensorComponent::UpdateSpatialCategory()
{
  if (!m_sSpatialCategory.IsEmpty())
  {
    m_SpatialCategory = WSpatialData::RegisterCategory(m_sSpatialCategory, WSpatialData::Flags::None);
  }
  else
  {
    m_SpatialCategory = WInvalidSpatialDataCategory;
  }
}

void WSensorComponent::UpdateScheduling()
{
  auto pModule = GetWorld()->GetOrCreateModule<WSensorWorldModule>();

  if (m_UpdateRate == WUpdateRate::Never)
    pModule->RemoveComponentToSchedule(this);
  else
    pModule->AddComponentToSchedule(this, m_UpdateRate);
}

void WSensorComponent::UpdateDebugInfo()
{
  auto pModule = GetWorld()->GetOrCreateModule<WSensorWorldModule>();
  if (IsActiveAndInitialized() && m_bShowDebugInfo)
  {
    pModule->AddComponentForDebugRendering(this);
  }
  else
  {
    pModule->RemoveComponentForDebugRendering(this);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSensorSphereComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.0f, WVariant())),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WSphereManipulatorAttribute("Radius"),
    new WSphereVisualizerAttribute("Radius", WColor::White, "Color"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WSensorSphereComponent::WSensorSphereComponent() = default;
WSensorSphereComponent::~WSensorSphereComponent() = default;

void WSensorSphereComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
}

void WSensorSphereComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
}

void WSensorSphereComponent::GetObjectsInSensorVolume(WDynamicArray<WGameObject*>& out_objects) const
{
  const WGameObject* pOwner = GetOwner();

  const float scale = pOwner->GetGlobalTransformSimd().GetMaxScale();
  const WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(pOwner->GetGlobalPosition(), m_fRadius * scale);

  WSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();
  params.m_pIncludeTags = &m_IncludeTags;
  params.m_pExcludeTags = &m_ExcludeTags;

  WSimdMat4f toLocalSpace = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  WSimdFloat radiusSquared = m_fRadius * m_fRadius;

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](WGameObject* pObject)
    {
    WSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const bool bInRadius = localSpacePos.GetLengthSquared<3>() <= radiusSquared;

    if (bInRadius)
    {
      out_objects.PushBack(pObject);
    }

    return WVisitorExecution::Continue; });
}

void WSensorSphereComponent::DebugDrawSensorShape() const
{
  const WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fRadius);
  WDebugRenderer::DrawLineSphere(GetWorld(), sphere, m_bHadUpdate ? WColor(m_Color) : WColor(m_Color).GetDarker(1.5f), GetOwner()->GetGlobalTransform());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSensorCylinderComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.0f, WVariant())),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WCylinderVisualizerAttribute(WBasisAxis::PositiveZ, "Height", "Radius", WColor::White, "Color"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WSensorCylinderComponent::WSensorCylinderComponent() = default;
WSensorCylinderComponent::~WSensorCylinderComponent() = default;

void WSensorCylinderComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fHeight;
}

void WSensorCylinderComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fHeight;
}

void WSensorCylinderComponent::GetObjectsInSensorVolume(WDynamicArray<WGameObject*>& out_objects) const
{
  const WGameObject* pOwner = GetOwner();

  const WVec3 scale = pOwner->GetGlobalScaling().Abs();
  const float xyScale = WMath::Max(scale.x, scale.y);

  const float sphereRadius = WVec2(m_fRadius * xyScale, m_fHeight * 0.5f * scale.z).GetLength();
  const WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(pOwner->GetGlobalPosition(), sphereRadius);

  WSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();
  params.m_pIncludeTags = &m_IncludeTags;
  params.m_pExcludeTags = &m_ExcludeTags;

  WSimdMat4f toLocalSpace = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  WSimdFloat radiusSquared = m_fRadius * m_fRadius;
  WSimdFloat halfHeight = m_fHeight * 0.5f;

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](WGameObject* pObject)
    {
    WSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const bool bInRadius = localSpacePos.GetLengthSquared<2>() <= radiusSquared;
    const bool bInHeight = localSpacePos.Abs().z() <= halfHeight;

    if (bInRadius && bInHeight)
    {
      out_objects.PushBack(pObject);
    }

    return WVisitorExecution::Continue; });
}

void WSensorCylinderComponent::DebugDrawSensorShape() const
{
  WTransform pt = GetOwner()->GetGlobalTransform();

  WQuat r = WQuat::MakeFromAxisAndAngle(WVec3(0, 1, 0), WAngle::MakeFromDegree(-90.0f));
  WTransform t = WTransform(WVec3(0, 0, -0.5f * m_fHeight * pt.m_vScale.z), r, WVec3(pt.m_vScale.z, pt.m_vScale.y, pt.m_vScale.x));

  pt.m_vScale.Set(1);
  t = pt * t;

  WDebugRenderer::DrawCylinder(GetWorld(), m_fRadius, m_fRadius, m_fHeight, WColor::MakeZero(), m_bHadUpdate ? WColor(m_Color) : WColor(m_Color).GetDarker(1.5f), t);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSensorConeComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("NearDistance", m_fNearDistance)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("FarDistance", m_fFarDistance)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(90.0f)), new WClampValueAttribute(0.0f, WAngle::MakeFromDegree(180.0f))),
  }
  W_END_PROPERTIES;
}
W_END_COMPONENT_TYPE
// clang-format on

WSensorConeComponent::WSensorConeComponent() = default;
WSensorConeComponent::~WSensorConeComponent() = default;

void WSensorConeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fNearDistance;
  s << m_fFarDistance;
  s << m_Angle;
}

void WSensorConeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fNearDistance;
  s >> m_fFarDistance;
  s >> m_Angle;
}

void WSensorConeComponent::GetObjectsInSensorVolume(WDynamicArray<WGameObject*>& out_objects) const
{
  const WGameObject* pOwner = GetOwner();

  const float scale = pOwner->GetGlobalTransformSimd().GetMaxScale();
  const WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(pOwner->GetGlobalPosition(), m_fFarDistance * scale);

  WSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();
  params.m_pIncludeTags = &m_IncludeTags;
  params.m_pExcludeTags = &m_ExcludeTags;

  WSimdMat4f toLocalSpace = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  const WSimdFloat nearSquared = m_fNearDistance * m_fNearDistance;
  const WSimdFloat farSquared = m_fFarDistance * m_fFarDistance;
  const WSimdFloat cosAngle = WMath::Cos(m_Angle * 0.5f);

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](WGameObject* pObject)
    {
    WSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const WSimdFloat fDistanceSquared = localSpacePos.GetLengthSquared<3>();
    const bool bInDistance = fDistanceSquared >= nearSquared && fDistanceSquared <= farSquared;

    const WSimdVec4f normalizedPos = localSpacePos * fDistanceSquared.GetInvSqrt();
    const bool bInAngle = normalizedPos.x() >= cosAngle;

    if (bInDistance && bInAngle)
    {
      out_objects.PushBack(pObject);
    }

    return WVisitorExecution::Continue; });
}

void WSensorConeComponent::DebugDrawSensorShape() const
{
  constexpr WUInt32 MIN_SEGMENTS = 3;
  constexpr WUInt32 MAX_SEGMENTS = 16;
  constexpr WUInt32 CIRCLE_SEGMENTS = MAX_SEGMENTS * 2;
  constexpr WUInt32 NUM_LINES = MAX_SEGMENTS * 4 + CIRCLE_SEGMENTS * 2 + 4;

  WDebugRendererLine lines[NUM_LINES];
  WUInt32 curLine = 0;

  const WUInt32 numSegments = WMath::Clamp(static_cast<WUInt32>(m_Angle / WAngle::MakeFromDegree(180) * MAX_SEGMENTS), MIN_SEGMENTS, MAX_SEGMENTS);
  const WAngle stepAngle = m_Angle / static_cast<float>(numSegments);
  const WAngle circleStepAngle = WAngle::MakeFromDegree(360.0f / CIRCLE_SEGMENTS);

  for (WUInt32 i = 0; i < 2; ++i)
  {
    WAngle curAngle = m_Angle * -0.5f;

    WQuat q;
    float fX = WMath::Cos(curAngle);
    float fCircleRadius = WMath::Sin(curAngle);

    if (i == 0)
    {
      q.SetIdentity();
      fX *= m_fNearDistance;
      fCircleRadius *= m_fNearDistance;
    }
    else
    {
      q = WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisX(), WAngle::MakeFromDegree(90));
      fX *= m_fFarDistance;
      fCircleRadius *= m_fFarDistance;
    }

    for (WUInt32 s = 0; s < numSegments; ++s)
    {
      const WAngle nextAngle = curAngle + stepAngle;

      const float fCos1 = WMath::Cos(curAngle);
      const float fCos2 = WMath::Cos(nextAngle);

      const float fSin1 = WMath::Sin(curAngle);
      const float fSin2 = WMath::Sin(nextAngle);

      curAngle = nextAngle;

      const WVec3 p1 = q * WVec3(fCos1, fSin1, 0.0f);
      const WVec3 p2 = q * WVec3(fCos2, fSin2, 0.0f);

      lines[curLine].m_start = p1 * m_fNearDistance;
      lines[curLine].m_end = p2 * m_fNearDistance;
      ++curLine;

      lines[curLine].m_start = p1 * m_fFarDistance;
      lines[curLine].m_end = p2 * m_fFarDistance;
      ++curLine;

      if (s == 0)
      {
        lines[curLine].m_start = p1 * m_fNearDistance;
        lines[curLine].m_end = p1 * m_fFarDistance;
        ++curLine;
      }
      else if (s == numSegments - 1)
      {
        lines[curLine].m_start = p2 * m_fNearDistance;
        lines[curLine].m_end = p2 * m_fFarDistance;
        ++curLine;
      }
    }

    curAngle = WAngle::MakeFromDegree(0.0f);
    for (WUInt32 s = 0; s < CIRCLE_SEGMENTS; ++s)
    {
      const WAngle nextAngle = curAngle + circleStepAngle;

      const float fCos1 = WMath::Cos(curAngle);
      const float fCos2 = WMath::Cos(nextAngle);

      const float fSin1 = WMath::Sin(curAngle);
      const float fSin2 = WMath::Sin(nextAngle);

      curAngle = nextAngle;

      const WVec3 p1 = WVec3(fX, fCos1 * fCircleRadius, fSin1 * fCircleRadius);
      const WVec3 p2 = WVec3(fX, fCos2 * fCircleRadius, fSin2 * fCircleRadius);

      lines[curLine].m_start = p1;
      lines[curLine].m_end = p2;
      ++curLine;
    }
  }

  W_ASSERT_DEV(curLine <= NUM_LINES, "");
  WDebugRenderer::DrawLines(GetWorld(), WMakeArrayPtr(lines, curLine), m_bHadUpdate ? WColor(m_Color) : WColor(m_Color).GetDarker(1.5f), GetOwner()->GetGlobalTransform());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_IMPLEMENT_WORLD_MODULE(WSensorWorldModule);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSensorWorldModule, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSensorWorldModule::WSensorWorldModule(WWorld* pWorld)
  : WWorldModule(pWorld)
{
}

void WSensorWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WSensorWorldModule::UpdateSensors, this);
    desc.m_Phase = WWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WSensorWorldModule::DebugDrawSensors, this);
    desc.m_Phase = WWorldUpdatePhase::PostTransform;

    RegisterUpdateFunction(desc);
  }

  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<WPhysicsWorldModuleInterface>();
}

void WSensorWorldModule::AddComponentToSchedule(WSensorComponent* pComponent, WUpdateRate::Enum updateRate)
{
  W_ASSERT_DEBUG(updateRate != WUpdateRate::Never, "Invalid update rate for scheduling");
  m_Scheduler.AddOrUpdateWork(pComponent->GetHandle(), WUpdateRate::GetInterval(updateRate));
}

void WSensorWorldModule::RemoveComponentToSchedule(WSensorComponent* pComponent)
{
  m_Scheduler.RemoveWork(pComponent->GetHandle());
}

void WSensorWorldModule::AddComponentForDebugRendering(WSensorComponent* pComponent)
{
  WComponentHandle hComponent = pComponent->GetHandle();
  if (m_DebugComponents.Contains(hComponent) == false)
  {
    m_DebugComponents.PushBack(hComponent);
  }
}

void WSensorWorldModule::RemoveComponentForDebugRendering(WSensorComponent* pComponent)
{
  m_DebugComponents.RemoveAndSwap(pComponent->GetHandle());
}

void WSensorWorldModule::UpdateSensors(const WWorldModule::UpdateContext& context)
{
  if (m_pPhysicsWorldModule == nullptr)
    return;

  const WTime deltaTime = GetWorld()->GetClock().GetTimeDiff();
  m_Scheduler.Update(deltaTime, [this](const WComponentHandle& hComponent, WTime deltaTime)
    {
      const WWorld* pWorld = GetWorld();
      const WSensorComponent* pSensorComponent = nullptr;
      W_VERIFY(pWorld->TryGetComponent(hComponent, pSensorComponent), "Invalid component handle");

      pSensorComponent->RunSensorCheck(m_pPhysicsWorldModule, m_ObjectsInSensorVolume, m_DetectedObjects, true);
      //
    });
}

void WSensorWorldModule::DebugDrawSensors(const WWorldModule::UpdateContext& context)
{
  WTempHybridArray<WDebugRendererLine, 256> lines;
  const WWorld* pWorld = GetWorld();

  for (WComponentHandle hComponent : m_DebugComponents)
  {
    lines.Clear();

    const WSensorComponent* pSensorComponent = nullptr;
    W_VERIFY(pWorld->TryGetComponent(hComponent, pSensorComponent), "Invalid component handle");

    pSensorComponent->DebugDrawSensorShape();
    pSensorComponent->m_bHadUpdate = false;

    const WVec3 sensorPos = pSensorComponent->GetOwner()->GetGlobalPosition();
    for (WGameObjectHandle hObject : pSensorComponent->m_LastDetectedObjects)
    {
      const WGameObject* pObject = nullptr;
      if (pWorld->TryGetObject(hObject, pObject) == false)
        continue;

      lines.PushBack({sensorPos, pObject->GetGlobalPosition(), WColor::Lime});
    }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    for (const WVec3& occludedPos : pSensorComponent->m_LastOccludedObjectPositions)
    {
      lines.PushBack({sensorPos, occludedPos, WColor::Red});
    }
#endif

    WDebugRenderer::DrawLines(pWorld, lines, WColor::White);
  }
}


W_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_SensorComponent);
