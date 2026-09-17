#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiCanvas3DInteractionExampleComponent.h>
#include <RmlUiPlugin/Components/RmlUiCanvas3DComponent.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Core/Input/InputManager.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WRmlUiCanvas3DInteractionExampleComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new WDefaultValueAttribute(2.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Input/RmlUi"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on


WRmlUiCanvas3DInteractionExampleComponent::WRmlUiCanvas3DInteractionExampleComponent() = default;
WRmlUiCanvas3DInteractionExampleComponent::~WRmlUiCanvas3DInteractionExampleComponent() = default;

void WRmlUiCanvas3DInteractionExampleComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_uiCollisionLayer;
  s << m_fMaxDistance;
}

void WRmlUiCanvas3DInteractionExampleComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_fMaxDistance;
}

void WRmlUiCanvas3DInteractionExampleComponent::OnSimulationStarted()
{
  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<WPhysicsWorldModuleInterface>();
}

void WRmlUiCanvas3DInteractionExampleComponent::Interact(WRmlUiInputSnapshot input)
{
  if (!m_pPhysicsWorldModule)
  {
    return;
  }

  WVec3 vRayOrigin = GetOwner()->GetGlobalPosition();
  WVec3 vRayDir = GetOwner()->GetGlobalDirForwards().GetNormalized();

  WPhysicsCastResult hit;

  WPhysicsQueryParameters queryParams{m_uiCollisionLayer};
  queryParams.m_bIgnoreInitialOverlap = true;
  queryParams.m_ShapeTypes = WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic | WPhysicsShapeType::Trigger;

  if (!m_pPhysicsWorldModule->Raycast(hit, vRayOrigin, vRayDir, m_fMaxDistance, queryParams))
  {
    return;
  }
  
  WGameObject* pGameObject = nullptr;
  if (!GetWorld()->TryGetObject(hit.m_hShapeObject, pGameObject))
  {
    WLog::Dev("WRmlUiCanvas3DInteractionComponent: failed to acquire a game object from raycast result");
    return;
  }

  WRmlUiCanvas3DComponent* pCanvas = nullptr;
  if (!pGameObject->TryGetComponentOfBaseType(pCanvas))
  {
    return;
  }

  pCanvas->RaycastInput(vRayOrigin, vRayDir, input);
}

void WRmlUiCanvas3DInteractionExampleComponent::Update()
{
  WRmlUiInputSnapshot input{};
  if (WInputManager::GetInputSlotState(WInputSlot_MouseButton1) == WKeyState::Down)
    input.m_Buttons |= WRmlUiInputButtons::Mouse0;
  if (WInputManager::GetInputSlotState(WInputSlot_MouseWheelUp) == WKeyState::Pressed)
    input.m_Buttons |= WRmlUiInputButtons::MouseWheelUp;
  if (WInputManager::GetInputSlotState(WInputSlot_MouseWheelDown) == WKeyState::Pressed)
    input.m_Buttons |= WRmlUiInputButtons::MouseWheelDown;
  input.m_sLastCharacters = WInputManager::RetrieveLastCharacters(false);

  Interact(input);
}

W_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas3DInteractionExampleComponent);
