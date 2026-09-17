#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameComponentsPlugin/Gameplay/AreaDamageComponent.h>
#include <GameEngine/Messages/DamageMessage.h>
#include <GameEngine/Physics/ImpulseType.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WAreaDamageComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("OnCreation", m_bTriggerOnCreation)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("Damage", m_fDamage)->AddAttributes(new WDefaultValueAttribute(10.0f)),
    W_MEMBER_PROPERTY("ImpulseType", m_uiImpulseType)->AddAttributes(new WDynamicEnumAttribute("PhysicsImpulseType")),
    W_MEMBER_PROPERTY("Impulse", m_fImpulse)->AddAttributes(new WDefaultValueAttribute(100.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(ApplyAreaDamage),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
    new WSphereVisualizerAttribute("Radius", WColor::OrangeRed),
    new WSphereManipulatorAttribute("Radius"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static WPhysicsOverlapResultArray g_OverlapResults;

WAreaDamageComponent::WAreaDamageComponent() = default;
WAreaDamageComponent::~WAreaDamageComponent() = default;

void WAreaDamageComponent::ApplyAreaDamage()
{
  if (!IsActiveAndSimulating())
    return;

  W_PROFILE_SCOPE("ApplyAreaDamage");

  WPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<WPhysicsWorldModuleInterface>();

  if (pPhysicsInterface == nullptr)
    return;

  const WVec3 vOwnPosition = GetOwner()->GetGlobalPosition();

  WPhysicsQueryParameters query(m_uiCollisionLayer);
  query.m_ShapeTypes.Remove(WPhysicsShapeType::Static | WPhysicsShapeType::Trigger);

  pPhysicsInterface->QueryShapesInSphere(g_OverlapResults, m_fRadius, vOwnPosition, query);

  const float fInvRadius = 1.0f / m_fRadius;

  for (const auto& hit : g_OverlapResults.m_Results)
  {
    if (!hit.m_hActorObject.IsInvalidated())
    {
      WGameObject* pObject = nullptr;
      if (GetWorld()->TryGetObject(hit.m_hActorObject, pObject))
      {
        const WVec3 vTargetPos = hit.m_vCenterPosition;
        const WVec3 vDistToTarget = vTargetPos - vOwnPosition;
        WVec3 vDirToTarget = vDistToTarget;
        const float fDistance = vDirToTarget.GetLength();

        if (fDistance >= 0.01f)
        {
          // if the direction is valid (non-zero), just normalize it
          vDirToTarget /= fDistance;
        }
        else
        {
          // otherwise, if we are so close, that the distance is zero, pick a random direction away from it
          vDirToTarget = WVec3::MakeRandomDirection(GetWorld()->GetRandomNumberGenerator());
        }

        // linearly scale damage and impulse down by distance
        const float fScale = 1.0f - WMath::Min(fDistance * fInvRadius, 1.0f);

        // apply a physical impulse
        if (m_uiImpulseType >= WImpulseTypeConfig::FirstValidKey || (m_uiImpulseType == WImpulseTypeConfig::CustomValueKey && m_fImpulse != 0.0f))
        {
          WMsgPhysicsAddImpulse msg;
          msg.m_uiImpulseType = m_uiImpulseType;
          msg.m_vGlobalPosition = vTargetPos;
          msg.m_vImpulse = vDirToTarget * fScale;
          msg.m_uiObjectFilterID = hit.m_uiObjectFilterID;
          msg.m_pInternalPhysicsShape = hit.m_pInternalPhysicsShape;
          msg.m_pInternalPhysicsActor = hit.m_pInternalPhysicsActor;

          if (m_uiImpulseType == WImpulseTypeConfig::CustomValueKey)
          {
            msg.m_vImpulse *= m_fImpulse;
          }

          pObject->SendMessage(msg);
        }

        // apply damage
        if (m_fDamage != 0.0f)
        {
          WMsgDamage msg;
          msg.m_fDamage = static_cast<double>(m_fDamage) * static_cast<double>(fScale);
          msg.m_vImpactDirection = vDirToTarget;
          msg.m_vGlobalPosition = vOwnPosition + vDistToTarget * 0.9f; // rough guess for a position where to apply the damage

          WGameObject* pShape = nullptr;
          if (GetWorld()->TryGetObject(hit.m_hShapeObject, pShape))
          {
            msg.m_sHitObjectName = pShape->GetName();
          }
          else
          {
            msg.m_sHitObjectName = pObject->GetName();
          }

          // delay the damage a little bit for nicer chain reactions
          pObject->PostEventMessage(msg, this, WTime::MakeFromMilliseconds(200));
        }
      }
    }
  }
}

void WAreaDamageComponent::OnSimulationStarted()
{
  if (m_bTriggerOnCreation)
  {
    ApplyAreaDamage();
  }
}

void WAreaDamageComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bTriggerOnCreation;
  s << m_fRadius;
  s << m_uiCollisionLayer;
  s << m_fDamage;
  s << m_uiImpulseType;
  s << m_fImpulse;
}

void WAreaDamageComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bTriggerOnCreation;
  s >> m_fRadius;
  s >> m_uiCollisionLayer;
  s >> m_fDamage;

  if (uiVersion >= 2)
  {
    s >> m_uiImpulseType;
  }

  s >> m_fImpulse;
}

//////////////////////////////////////////////////////////////////////////

WAreaDamageComponentManager::WAreaDamageComponentManager(WWorld* pWorld)
  : SUPER(pWorld)

{
}

void WAreaDamageComponentManager::Initialize()
{
  SUPER::Initialize();

  m_pPhysicsInterface = GetWorld()->GetOrCreateModule<WPhysicsWorldModuleInterface>();
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Gameplay_Implementation_AreaDamageComponent);
