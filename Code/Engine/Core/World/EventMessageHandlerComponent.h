#pragma once

#include <Core/World/World.h>

/// Base class for components that want advanced handling of event messages.
///
/// Event messages are messages that are 'broadcast' to indicate something happened on a component,
/// e.g. a trigger that got activated or an animation that finished playing. These messages are 'bubbled up'
/// the object hierarchy to the closest parent object that holds a component that handles this message.
/// Event message handler components can control whether the search for handlers should be continued or
/// can register itself as global event message handlers which will be used if no handler is found in the parent hierarchy.
/// This is typically used for level-logic scripts that want to react to events happening on any object in the world
/// without needing to be attached to a specific object in the hierarchy.
class W_CORE_DLL WEventMessageHandlerComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WEventMessageHandlerComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void Deinitialize() override;


  //////////////////////////////////////////////////////////////////////////
  // WEventMessageHandlerComponent

public:
  /// Keep the constructor private or protected in derived classes, so it cannot be called manually.
  WEventMessageHandlerComponent();
  ~WEventMessageHandlerComponent();

  /// Sets the debug output object flag. The effect is type specific, most components will not do anything different.
  void SetDebugOutput(bool bEnable);

  /// Gets the debug output object flag.
  bool GetDebugOutput() const;

  /// Registers or de-registers this component as a global event handler.
  void SetGlobalEventHandlerMode(bool bEnable); // [ property ]

  /// Returns whether this component is registered as a global event handler.
  bool GetGlobalEventHandlerMode() const { return m_bIsGlobalEventHandler; } // [ property ]

  /// Sets whether unhandled event messages should be passed to parent objects or not.
  void SetPassThroughUnhandledEvents(bool bPassThrough);                               // [ property ]
  bool GetPassThroughUnhandledEvents() const { return m_bPassThroughUnhandledEvents; } // [ property ]

  /// Returns all global event handler for the given world.
  static WArrayPtr<WComponentHandle> GetAllGlobalEventHandler(const WWorld* pWorld);

  static void ClearGlobalEventHandlersForWorld(const WWorld* pWorld);

private:
  bool m_bDebugOutput = false;
  bool m_bIsGlobalEventHandler = false;
  bool m_bPassThroughUnhandledEvents = false;
};
