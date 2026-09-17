#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Gameplay/PowerConnectorComponent.h>

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WEventMsgSetPowerInput);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEventMsgSetPowerInput, 1, WRTTIDefaultAllocator<WEventMsgSetPowerInput>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("PrevValue", m_uiPrevValue),
    W_MEMBER_PROPERTY("NewValue", m_uiNewValue),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WPowerConnectorComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Output", GetOutput, SetOutput),
    W_ACCESSOR_PROPERTY("Buddy", DummyGetter, SetBuddyReference)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_ACCESSOR_PROPERTY("ConnectedTo", DummyGetter, SetConnectedToReference)->AddAttributes(new WGameObjectReferenceAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgSensorDetectedObjectsChanged, OnMsgSensorDetectedObjectsChanged),
    W_MESSAGE_HANDLER(WMsgObjectGrabbed, OnMsgObjectGrabbed),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(IsConnected),
    W_SCRIPT_FUNCTION_PROPERTY(IsAttached),
    W_SCRIPT_FUNCTION_PROPERTY(Detach),
    // W_SCRIPT_FUNCTION_PROPERTY(Attach, In, "Object"), // not supported (yet)
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

void WPowerConnectorComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hBuddy);
  inout_stream.WriteGameObjectHandle(m_hConnectedTo);

  s << m_uiOutput;
}

void WPowerConnectorComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  m_hBuddy = inout_stream.ReadGameObjectHandle();
  m_hConnectedTo = inout_stream.ReadGameObjectHandle();

  s >> m_uiOutput;
}

void WPowerConnectorComponent::ConnectToSocket(WGameObjectHandle hSocket)
{
  if (IsConnected())
    return;

  if (GetOwner()->GetWorld()->GetClock().GetAccumulatedTime() - m_DetachTime < WTime::MakeFromSeconds(1))
  {
    // recently detached -> wait a bit before allowing to attach again
    return;
  }

  Attach(hSocket);
}

void WPowerConnectorComponent::SetOutput(WUInt16 value)
{
  if (m_uiOutput == value)
    return;

  m_uiOutput = value;

  OutputChanged(m_uiOutput);
}

void WPowerConnectorComponent::SetInput(WUInt16 value)
{
  if (m_uiInput == value)
    return;

  InputChanged(m_uiInput, value);
  m_uiInput = value;
}

void WPowerConnectorComponent::SetBuddyReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetBuddy(resolver(szReference, GetHandle(), "Buddy"));
}

void WPowerConnectorComponent::SetBuddy(WGameObjectHandle hNewBuddy)
{
  if (m_hBuddy == hNewBuddy)
    return;

  if (!IsActiveAndInitialized())
  {
    m_hBuddy = hNewBuddy;
    return;
  }

  WGameObjectHandle hPrevBuddy = m_hBuddy;
  m_hBuddy = {};

  WGameObject* pBuddy;
  if (GetOwner()->GetWorld()->TryGetObject(hPrevBuddy, pBuddy))
  {
    WPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetOutput(0);
      pConnector->SetBuddy({});
    }
  }

  m_hBuddy = hNewBuddy;

  if (GetOwner()->GetWorld()->TryGetObject(hNewBuddy, pBuddy))
  {
    WPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetBuddy(GetOwner()->GetHandle());
      pConnector->SetOutput(m_uiInput);
    }
  }
}

void WPowerConnectorComponent::SetConnectedToReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetConnectedTo(resolver(szReference, GetHandle(), "ConnectedTo"));
}

void WPowerConnectorComponent::SetConnectedTo(WGameObjectHandle hNewConnectedTo)
{
  if (m_hConnectedTo == hNewConnectedTo)
    return;

  if (!IsActiveAndInitialized())
  {
    m_hConnectedTo = hNewConnectedTo;
    return;
  }

  WGameObjectHandle hPrevConnectedTo = m_hConnectedTo;
  m_hConnectedTo = {};

  WGameObject* pConnectedTo;
  if (GetOwner()->GetWorld()->TryGetObject(hPrevConnectedTo, pConnectedTo))
  {
    WPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetInput(0);
      pConnector->SetConnectedTo({});
    }
  }

  m_hConnectedTo = hNewConnectedTo;

  if (GetOwner()->GetWorld()->TryGetObject(hNewConnectedTo, pConnectedTo))
  {
    WPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetConnectedTo(GetOwner()->GetHandle());
      pConnector->SetInput(m_uiOutput);
    }
  }

  if (hNewConnectedTo.IsInvalidated() && IsAttached())
  {
    // make sure that if we get disconnected, we also clean up our detachment state
    Detach();
  }
}

bool WPowerConnectorComponent::IsConnected() const
{
  // since connectors automatically disconnect themselves from their peers upon destruction, this should be sufficient (no need to check object for existence)
  return !m_hConnectedTo.IsInvalidated();
}

bool WPowerConnectorComponent::IsAttached() const
{
  return !m_hAttachPoint.IsInvalidated();
}

void WPowerConnectorComponent::OnDeactivated()
{
  Detach();
  SetBuddy({});

  SUPER::OnDeactivated();
}

void WPowerConnectorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  WGameObjectHandle hAlreadyConnectedTo = m_hConnectedTo;
  m_hConnectedTo.Invalidate();

  if (!hAlreadyConnectedTo.IsInvalidated())
  {
    Attach(hAlreadyConnectedTo);
  }

  if (m_uiInput != 0)
  {
    InputChanged(0, m_uiInput);
  }

  if (m_uiOutput != 0)
  {
    OutputChanged(m_uiOutput);
  }
}

void WPowerConnectorComponent::OnMsgSensorDetectedObjectsChanged(WMsgSensorDetectedObjectsChanged& msg)
{
  if (!msg.m_DetectedObjects.IsEmpty())
  {
    ConnectToSocket(msg.m_DetectedObjects[0]);
  }
}

void WPowerConnectorComponent::OnMsgObjectGrabbed(WMsgObjectGrabbed& msg)
{
  if (msg.m_bGotGrabbed)
  {
    Detach();

    m_hGrabbedBy = msg.m_hGrabbedBy;

    if (WGameObject* pSensor = GetOwner()->FindChildByName("ActiveWhenGrabbed"))
    {
      pSensor->SetActiveFlag(true);
    }
  }
  else
  {
    m_hGrabbedBy.Invalidate();

    if (WGameObject* pSensor = GetOwner()->FindChildByName("ActiveWhenGrabbed"))
    {
      pSensor->SetActiveFlag(false);
    }
  }
}

void WPowerConnectorComponent::Attach(WGameObjectHandle hSocket)
{
  WWorld* pWorld = GetOwner()->GetWorld();

  WGameObject* pSocket;
  if (!pWorld->TryGetObject(hSocket, pSocket))
    return;

  WPowerConnectorComponent* pConnector;
  if (pSocket->TryGetComponentOfBaseType(pConnector))
  {
    // don't connect to an already connected object
    if (pConnector->IsConnected())
      return;
  }

  const WTransform tSocket = pSocket->GetGlobalTransform();

  WGameObjectDesc go;
  go.m_hParent = hSocket;

  WGameObject* pAttach;
  m_hAttachPoint = pWorld->CreateObject(go, pAttach);

  WPhysicsWorldModuleInterface* pPhysicsWorldModule = GetWorld()->GetOrCreateModule<WPhysicsWorldModuleInterface>();

  WPhysicsWorldModuleInterface::FixedJointConfig cfg;
  cfg.m_hActorA = {};
  cfg.m_hActorB = GetOwner()->GetHandle();
  cfg.m_LocalFrameA = tSocket;
  pPhysicsWorldModule->AddFixedJointComponent(pAttach, cfg);

  SetConnectedTo(hSocket);

  if (!m_hGrabbedBy.IsInvalidated())
  {
    WGameObject* pGrab;
    if (pWorld->TryGetObject(m_hGrabbedBy, pGrab))
    {
      WMsgReleaseObjectGrab msg;
      msg.m_hGrabbedObjectToRelease = GetOwner()->GetHandle();
      pGrab->SendMessage(msg);
    }
  }
}

void WPowerConnectorComponent::Detach()
{
  if (IsConnected())
  {
    m_DetachTime = GetOwner()->GetWorld()->GetClock().GetAccumulatedTime();

    SetConnectedTo({});
  }

  if (!m_hAttachPoint.IsInvalidated())
  {
    GetOwner()->GetWorld()->DeleteObjectDelayed(m_hAttachPoint, false);
    m_hAttachPoint.Invalidate();
  }
}

void WPowerConnectorComponent::InputChanged(WUInt16 uiPrevInput, WUInt16 uiInput)
{
  if (!IsActiveAndSimulating())
    return;

  WEventMsgSetPowerInput msg;
  msg.m_uiPrevValue = uiPrevInput;
  msg.m_uiNewValue = uiInput;

  GetOwner()->PostEventMessage(msg, this, WTime());

  if (m_hBuddy.IsInvalidated())
    return;

  WGameObject* pBuddy;
  if (GetOwner()->GetWorld()->TryGetObject(m_hBuddy, pBuddy))
  {
    WPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetOutput(uiInput);
    }
  }
}

void WPowerConnectorComponent::OutputChanged(WUInt16 uiOutput)
{
  if (!IsActiveAndSimulating())
    return;

  if (m_hConnectedTo.IsInvalidated())
    return;

  WGameObject* pConnectedTo;
  if (GetOwner()->GetWorld()->TryGetObject(m_hConnectedTo, pConnectedTo))
  {
    WPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetInput(uiOutput);
    }
  }
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Gameplay_Implementation_PowerConnectorComponent);
