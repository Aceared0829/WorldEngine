#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Input/Declarations.h>
#include <Core/Messages/EventMessageSender.h>
#include <Core/Messages/TriggerMessage.h>

using WInputComponentManager = WComponentManagerSimple<class WInputComponent, WComponentUpdateType::WhenSimulating>;

/// Which types of input events are broadcast
struct W_GAMEENGINE_DLL WInputMessageGranularity
{
  using StorageType = WInt8;

  /// Which types of input events are broadcast
  enum Enum
  {
    PressOnly,           ///< Key pressed events are sent, but nothing else
    PressAndRelease,     ///< Key pressed and key released events are sent
    PressReleaseAndDown, ///< Key pressed and released events are sent, and while a key is down, another message is sent every frame as well

    Default = PressOnly
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WInputMessageGranularity);

/// WInputComponent raises this event when it detects input
struct W_GAMEENGINE_DLL WMsgInputActionTriggered : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgInputActionTriggered, WMessage);

  /// The input action string.
  WHashedString m_sInputAction;

  /// The 'trigger state', depending on the key state and the configuration on the WInputComponent
  WEnum<WTriggerState> m_TriggerState;

  /// For analog keys, how much they are pressed. Typically between 0 and 1.
  float m_fKeyPressValue;

private:
  const char* GetInputAction() const { return m_sInputAction; }
  void SetInputAction(const char* szInputAction) { m_sInputAction.Assign(szInputAction); }
};

/// This component polls all input events from the given input set every frame and broadcasts the information to components on the same game
/// object.
///
/// To deactivate input handling, just deactivate the entire component.
/// To use the input data, add a message handler on another component and handle messages of type WMsgInputActionTriggered.
/// For every input event, one such message is sent every frame.
/// The granularity property defines for which input events (key pressed, released or down) messages are sent.
class W_GAMEENGINE_DLL WInputComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WInputComponent, WComponent, WInputComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // WInputComponent

public:
  WInputComponent();
  ~WInputComponent();

  /// Returns the amount to which szInputAction is active (0 to 1).
  ///
  /// If bOnlyKeyPressed is set to true, only key press events return a non-zero value,
  /// ie key down and key released events are ignored.
  float GetCurrentInputState(const char* szInputAction, bool bOnlyKeyPressed = false) const; // [ scriptable ]

  WString m_sInputSet;                                                                      // [ property ]
  WEnum<WInputMessageGranularity> m_Granularity;                                           // [ property ]
  bool m_bForwardToBlackboard = false;                                                       // [ property ]

protected:
  void Update();

  WEventMessageSender<WMsgInputActionTriggered> m_InputEventSender; // [ event ]
};
