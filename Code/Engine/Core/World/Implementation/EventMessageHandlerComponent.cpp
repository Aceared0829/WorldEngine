#include <Core/CorePCH.h>

#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

namespace
{
  static WStaticArray<WDynamicArray<WComponentHandle>*, 64> s_GlobalEventHandlerPerWorld;

  static void RegisterGlobalEventHandler(WComponent* pComponent)
  {
    const WUInt32 uiWorldIndex = pComponent->GetWorld()->GetIndex();
    s_GlobalEventHandlerPerWorld.EnsureCount(uiWorldIndex + 1);

    auto globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex];
    if (globalEventHandler == nullptr)
    {
      globalEventHandler = W_NEW(WStaticsAllocatorWrapper::GetAllocator(), WDynamicArray<WComponentHandle>);

      s_GlobalEventHandlerPerWorld[uiWorldIndex] = globalEventHandler;
    }

    globalEventHandler->PushBack(pComponent->GetHandle());
  }

  static void DeregisterGlobalEventHandler(WComponent* pComponent)
  {
    WUInt32 uiWorldIndex = pComponent->GetWorld()->GetIndex();
    auto globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex];
    W_ASSERT_DEV(globalEventHandler != nullptr, "Implementation error.");

    globalEventHandler->RemoveAndSwap(pComponent->GetHandle());
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WEventMessageHandlerComponent, 3)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("HandleGlobalEvents", GetGlobalEventHandlerMode, SetGlobalEventHandlerMode),
    W_ACCESSOR_PROPERTY("PassThroughUnhandledEvents", GetPassThroughUnhandledEvents, SetPassThroughUnhandledEvents),
  }
  W_END_PROPERTIES;
}
W_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

WEventMessageHandlerComponent::WEventMessageHandlerComponent() = default;
WEventMessageHandlerComponent::~WEventMessageHandlerComponent() = default;

void WEventMessageHandlerComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  // version 2
  s << m_bIsGlobalEventHandler;

  // version 3
  s << m_bPassThroughUnhandledEvents;
}

void WEventMessageHandlerComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  if (uiVersion >= 2)
  {
    bool bGlobalEH;
    s >> bGlobalEH;

    SetGlobalEventHandlerMode(bGlobalEH);
  }

  if (uiVersion >= 3)
  {
    s >> m_bPassThroughUnhandledEvents;
  }
}

void WEventMessageHandlerComponent::Deinitialize()
{
  SetGlobalEventHandlerMode(false);

  SUPER::Deinitialize();
}

void WEventMessageHandlerComponent::SetDebugOutput(bool bEnable)
{
  m_bDebugOutput = bEnable;
}

bool WEventMessageHandlerComponent::GetDebugOutput() const
{
  return m_bDebugOutput;
}

void WEventMessageHandlerComponent::SetGlobalEventHandlerMode(bool bEnable)
{
  if (m_bIsGlobalEventHandler == bEnable)
    return;

  m_bIsGlobalEventHandler = bEnable;

  if (bEnable)
  {
    RegisterGlobalEventHandler(this);
  }
  else
  {
    DeregisterGlobalEventHandler(this);
  }
}

void WEventMessageHandlerComponent::SetPassThroughUnhandledEvents(bool bPassThrough)
{
  m_bPassThroughUnhandledEvents = bPassThrough;
}

// static
WArrayPtr<WComponentHandle> WEventMessageHandlerComponent::GetAllGlobalEventHandler(const WWorld* pWorld)
{
  WUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex < s_GlobalEventHandlerPerWorld.GetCount())
  {
    if (auto globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex])
    {
      return globalEventHandler->GetArrayPtr();
    }
  }

  return WArrayPtr<WComponentHandle>();
}


void WEventMessageHandlerComponent::ClearGlobalEventHandlersForWorld(const WWorld* pWorld)
{
  WUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex < s_GlobalEventHandlerPerWorld.GetCount())
  {
    s_GlobalEventHandlerPerWorld[uiWorldIndex]->Clear();
  }
}

W_STATICLINK_FILE(Core, Core_World_Implementation_EventMessageHandlerComponent);
