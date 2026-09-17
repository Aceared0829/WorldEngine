#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WJoltMsgDisconnectConstraints);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltMsgDisconnectConstraints, 1, WRTTIDefaultAllocator<WJoltMsgDisconnectConstraints>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WJoltActorComponent, 2)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Physics/Jolt/Actors"),
  }
  W_END_ATTRIBUTES;
}
W_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

WJoltActorComponent::WJoltActorComponent() = default;
WJoltActorComponent::~WJoltActorComponent() = default;

void WJoltActorComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_uiCollisionLayer;
}

void WJoltActorComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_uiCollisionLayer;
}

void WJoltActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_uiObjectFilterID == WInvalidIndex)
  {
    // only create a new filter ID, if none has been passed in manually

    WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
    m_uiObjectFilterID = pModule->CreateObjectFilterID();
  }
}

void WJoltActorComponent::OnDeactivated()
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
    else
    {
      pModule->RemoveBodyFromQueue(bodyId);
    }

    pBodies->DestroyBody(bodyId);
    m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
  }

  pModule->DeallocateUserData(m_uiUserDataIndex);
  pModule->DeleteObjectFilterID(m_uiObjectFilterID);

  SUPER::OnDeactivated();
}

void WJoltActorComponent::GatherShapes(WDynamicArray<WJoltSubShape>& shapes, WGameObject* pObject, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial)
{
  WTempHybridArray<WJoltShapeComponent*, 8> shapeComps;
  pObject->TryGetComponentsOfBaseType(shapeComps);

  for (auto pShape : shapeComps)
  {
    if (pShape->IsActive())
    {
      pShape->CreateShapes(shapes, rootTransform, fDensity, pMaterial);
    }
  }

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    // ignore all children that are actors themselves
    const WJoltActorComponent* pActorComponent = nullptr;
    if (itChild->TryGetComponentOfBaseType<WJoltActorComponent>(pActorComponent) && pActorComponent->IsActive())
      continue;

    GatherShapes(shapes, itChild, rootTransform, fDensity, pMaterial);
  }
}

WResult WJoltActorComponent::CreateShape(JPH::BodyCreationSettings* pSettings, float fDensity, const WJoltMaterial* pMaterial)
{
  WTempHybridArray<WJoltSubShape, 16> shapes;
  WTransform towner = GetOwner()->GetGlobalTransform();
  towner.m_vScale.Set(1.0f); // pretend like there is no scaling at the root, so that each shape applies its scale

  CreateShapes(shapes, towner, fDensity, pMaterial);
  GatherShapes(shapes, GetOwner(), towner, fDensity, pMaterial);

  auto cleanShapes = [&]()
  {
    for (auto& s : shapes)
    {
      if (s.m_pShape)
      {
        s.m_pShape->Release();
      }
    }
  };

  W_SCOPE_EXIT(cleanShapes());

  if (shapes.IsEmpty())
    return W_FAILURE;

  if (shapes.GetCount() > 0)
  {
    JPH::StaticCompoundShapeSettings opt;

    for (const auto& shape : shapes)
    {
      auto pShape = shape.m_pShape;

      if (!shape.m_Transform.m_vScale.IsEqual(WVec3(1.0f), 0.01f))
      {
        auto* pScaledShape = new JPH::ScaledShape(pShape, WJoltConversionUtils::ToVec3(shape.m_Transform.m_vScale));
        pShape = pScaledShape;
      }

      opt.AddShape(WJoltConversionUtils::ToVec3(shape.m_Transform.m_vPosition), WJoltConversionUtils::ToQuat(shape.m_Transform.m_qRotation).Normalized(), pShape);
    }

    auto res = opt.Create();
    if (!res.IsValid())
      return W_FAILURE;

    pSettings->SetShape(res.Get());
    return W_SUCCESS;
  }
  else
  {
    JPH::Shape* pShape = shapes[0].m_pShape;

    if (!shapes[0].m_Transform.m_vScale.IsEqual(WVec3(1.0f), 0.01f))
    {
      auto* pScaledShape = new JPH::ScaledShape(pShape, WJoltConversionUtils::ToVec3(shapes[0].m_Transform.m_vScale));
      pShape = pScaledShape;
    }

    if (!shapes[0].m_Transform.m_vPosition.IsZero(0.01f) || shapes[0].m_Transform.m_qRotation != WQuat::MakeIdentity())
    {
      JPH::RotatedTranslatedShapeSettings opt(WJoltConversionUtils::ToVec3(shapes[0].m_Transform.m_vPosition), WJoltConversionUtils::ToQuat(shapes[0].m_Transform.m_qRotation), pShape);

      auto res = opt.Create();
      if (!res.IsValid())
        return W_FAILURE;

      pShape = res.Get();
    }

    pSettings->SetShape(pShape);
    return W_SUCCESS;
  }
}

void WJoltActorComponent::ExtractSubShapeGeometry(const WGameObject* pObject, WMsgExtractGeometry& msg) const
{
  WTempHybridArray<const WJoltShapeComponent*, 8> shapes;
  pObject->TryGetComponentsOfBaseType(shapes);

  for (auto pShape : shapes)
  {
    if (pShape->IsActive())
    {
      pShape->ExtractGeometry(msg);
    }
  }

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    // ignore all children that are actors themselves
    const WJoltActorComponent* pActorComponent;
    if (itChild->TryGetComponentOfBaseType<WJoltActorComponent>(pActorComponent))
      continue;

    ExtractSubShapeGeometry(itChild, msg);
  }
}

const WJoltUserData* WJoltActorComponent::GetUserData() const
{
  const WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();

  return &pModule->GetUserData(m_uiUserDataIndex);
}

void WJoltActorComponent::SetInitialObjectFilterID(WUInt32 uiObjectFilterID)
{
  W_ASSERT_DEBUG(!IsActiveAndSimulating(), "The object filter ID can't be changed after simulation has started.");
  m_uiObjectFilterID = uiObjectFilterID;
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltActorComponent);
