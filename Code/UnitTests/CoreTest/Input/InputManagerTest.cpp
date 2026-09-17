#include <CoreTest/CoreTestPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Memory/MemoryUtils.h>

W_CREATE_SIMPLE_TEST_GROUP(Input);

static bool operator==(const WInputActionConfig& lhs, const WInputActionConfig& rhs)
{
  if (lhs.m_bApplyTimeScaling != rhs.m_bApplyTimeScaling)
    return false;
  if (lhs.m_fFilteredPriority != rhs.m_fFilteredPriority)
    return false;
  if (lhs.m_fFilterXMaxValue != rhs.m_fFilterXMaxValue)
    return false;
  if (lhs.m_fFilterXMinValue != rhs.m_fFilterXMinValue)
    return false;
  if (lhs.m_fFilterYMaxValue != rhs.m_fFilterYMaxValue)
    return false;
  if (lhs.m_fFilterYMinValue != rhs.m_fFilterYMinValue)
    return false;

  if (lhs.m_OnEnterArea != rhs.m_OnEnterArea)
    return false;
  if (lhs.m_OnLeaveArea != rhs.m_OnLeaveArea)
    return false;

  for (int i = 0; i < WInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    if (lhs.m_sInputSlotTrigger[i] != rhs.m_sInputSlotTrigger[i])
      return false;
    if (lhs.m_fInputSlotScale[i] != rhs.m_fInputSlotScale[i])
      return false;
    if (lhs.m_sFilterByInputSlotX[i] != rhs.m_sFilterByInputSlotX[i])
      return false;
    if (lhs.m_sFilterByInputSlotY[i] != rhs.m_sFilterByInputSlotY[i])
      return false;
  }

  return true;
}

class WTestInputDevide : public WInputDevice
{
public:
  void ActivateAll()
  {
    m_InputSlotValues["testdevice_button"] = 0.1f;
    m_InputSlotValues["testdevice_stick"] = 0.2f;
    m_InputSlotValues["testdevice_wheel"] = 0.3f;
    m_InputSlotValues["testdevice_touchpoint"] = 0.4f;
    m_sLastCharacters.Append('\42');
  }

private:
  void InitializeDevice() override {}
  void UpdateInputSlotValues() override {}
  void RegisterInputSlots() override
  {
    RegisterInputSlot("testdevice_button", "", WInputSlotFlags::IsButton);
    RegisterInputSlot("testdevice_stick", "", WInputSlotFlags::IsAnalogStick);
    RegisterInputSlot("testdevice_wheel", "", WInputSlotFlags::IsMouseWheel);
    RegisterInputSlot("testdevice_touchpoint", "", WInputSlotFlags::IsTouchPoint);
  }

  void ResetInputSlotValues() override { m_InputSlotValues.Clear(); }
};

static bool operator!=(const WInputActionConfig& lhs, const WInputActionConfig& rhs)
{
  return !(lhs == rhs);
}

W_CREATE_SIMPLE_TEST(Input, InputManager)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "SetInputSlotDisplayName / GetInputSlotDisplayName")
  {
    WInputManager::SetInputSlotDisplayName("test_slot_1", "Test Slot 1 Name");
    WInputManager::SetInputSlotDisplayName("test_slot_2", "Test Slot 2 Name");
    WInputManager::SetInputSlotDisplayName("test_slot_3", "Test Slot 3 Name");
    WInputManager::SetInputSlotDisplayName("test_slot_4", "Test Slot 4 Name");

    W_TEST_STRING(WInputManager::GetInputSlotDisplayName("test_slot_1"), "Test Slot 1 Name");
    W_TEST_STRING(WInputManager::GetInputSlotDisplayName("test_slot_2"), "Test Slot 2 Name");
    W_TEST_STRING(WInputManager::GetInputSlotDisplayName("test_slot_3"), "Test Slot 3 Name");
    W_TEST_STRING(WInputManager::GetInputSlotDisplayName("test_slot_4"), "Test Slot 4 Name");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetInputSlotDeadZone / GetInputSlotDisplayName")
  {
    WInputManager::SetInputSlotDeadZone("test_slot_1", 0.1f);
    WInputManager::SetInputSlotDeadZone("test_slot_2", 0.2f);
    WInputManager::SetInputSlotDeadZone("test_slot_3", 0.3f);
    WInputManager::SetInputSlotDeadZone("test_slot_4", 0.4f);

    W_TEST_FLOAT(WInputManager::GetInputSlotDeadZone("test_slot_1"), 0.1f, 0.0f);
    W_TEST_FLOAT(WInputManager::GetInputSlotDeadZone("test_slot_2"), 0.2f, 0.0f);
    W_TEST_FLOAT(WInputManager::GetInputSlotDeadZone("test_slot_3"), 0.3f, 0.0f);
    W_TEST_FLOAT(WInputManager::GetInputSlotDeadZone("test_slot_4"), 0.4f, 0.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetInputActionConfig / GetInputActionConfig")
  {
    WInputActionConfig iac1, iac2;
    iac1.m_bApplyTimeScaling = true;
    iac1.m_fFilteredPriority = 23.0f;
    iac1.m_fInputSlotScale[0] = 2.0f;
    iac1.m_fInputSlotScale[1] = 3.0f;
    iac1.m_fInputSlotScale[2] = 4.0f;
    iac1.m_sInputSlotTrigger[0] = WInputSlot_Key0;
    iac1.m_sInputSlotTrigger[1] = WInputSlot_Key1;
    iac1.m_sInputSlotTrigger[2] = WInputSlot_Key2;

    iac2.m_bApplyTimeScaling = false;
    iac2.m_fFilteredPriority = 42.0f;
    iac2.m_fInputSlotScale[0] = 4.0f;
    iac2.m_fInputSlotScale[1] = 5.0f;
    iac2.m_fInputSlotScale[2] = 6.0f;
    iac2.m_sInputSlotTrigger[0] = WInputSlot_Key3;
    iac2.m_sInputSlotTrigger[1] = WInputSlot_Key4;
    iac2.m_sInputSlotTrigger[2] = WInputSlot_Key5;

    WInputManager::SetInputActionConfig("test_inputset", "test_action_1", iac1, true);
    WInputManager::SetInputActionConfig("test_inputset", "test_action_2", iac2, true);

    W_TEST_BOOL(WInputManager::GetInputActionConfig("test_inputset", "test_action_1") == iac1);
    W_TEST_BOOL(WInputManager::GetInputActionConfig("test_inputset", "test_action_2") == iac2);

    WInputManager::SetInputActionConfig("test_inputset", "test_action_3", iac1, false);
    WInputManager::SetInputActionConfig("test_inputset", "test_action_4", iac2, false);

    W_TEST_BOOL(WInputManager::GetInputActionConfig("test_inputset", "test_action_1") == iac1);
    W_TEST_BOOL(WInputManager::GetInputActionConfig("test_inputset", "test_action_2") == iac2);
    W_TEST_BOOL(WInputManager::GetInputActionConfig("test_inputset", "test_action_3") == iac1);
    W_TEST_BOOL(WInputManager::GetInputActionConfig("test_inputset", "test_action_4") == iac2);

    WInputManager::SetInputActionConfig("test_inputset", "test_action_3", iac1, true);
    WInputManager::SetInputActionConfig("test_inputset", "test_action_4", iac2, true);

    W_TEST_BOOL(WInputManager::GetInputActionConfig("test_inputset", "test_action_1") != iac1);
    W_TEST_BOOL(WInputManager::GetInputActionConfig("test_inputset", "test_action_2") != iac2);
    W_TEST_BOOL(WInputManager::GetInputActionConfig("test_inputset", "test_action_3") == iac1);
    W_TEST_BOOL(WInputManager::GetInputActionConfig("test_inputset", "test_action_4") == iac2);


    WInputManager::RemoveInputAction("test_inputset", "test_action_1");
    WInputManager::RemoveInputAction("test_inputset", "test_action_2");
    WInputManager::RemoveInputAction("test_inputset", "test_action_3");
    WInputManager::RemoveInputAction("test_inputset", "test_action_4");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Input Slot State Changes / Dead Zones")
  {
    float f = 0;
    WInputManager::InjectInputSlotValue("test_slot_1", 0.0f);
    WInputManager::SetInputSlotDeadZone("test_slot_1", 0.25f);

    // just check the first state
    W_TEST_BOOL(WInputManager::GetInputSlotState("test_slot_1", &f) == WKeyState::Up);
    W_TEST_FLOAT(f, 0.0f, 0);

    // value is not yet propagated
    WInputManager::InjectInputSlotValue("test_slot_1", 1.0f);
    W_TEST_BOOL(WInputManager::GetInputSlotState("test_slot_1", &f) == WKeyState::Up);
    W_TEST_FLOAT(f, 0.0f, 0);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));
    W_TEST_BOOL(WInputManager::GetInputSlotState("test_slot_1", &f) == WKeyState::Pressed);
    W_TEST_FLOAT(f, 1.0f, 0);

    WInputManager::InjectInputSlotValue("test_slot_1", 0.5f);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));
    W_TEST_BOOL(WInputManager::GetInputSlotState("test_slot_1", &f) == WKeyState::Down);
    W_TEST_FLOAT(f, 0.5f, 0);

    WInputManager::InjectInputSlotValue("test_slot_1", 0.3f);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));
    W_TEST_BOOL(WInputManager::GetInputSlotState("test_slot_1", &f) == WKeyState::Down);
    W_TEST_FLOAT(f, 0.3f, 0);

    // below dead zone value
    WInputManager::InjectInputSlotValue("test_slot_1", 0.2f);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));
    W_TEST_BOOL(WInputManager::GetInputSlotState("test_slot_1", &f) == WKeyState::Released);
    W_TEST_FLOAT(f, 0.0f, 0);

    WInputManager::InjectInputSlotValue("test_slot_1", 0.5f);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));
    W_TEST_BOOL(WInputManager::GetInputSlotState("test_slot_1", &f) == WKeyState::Pressed);
    W_TEST_FLOAT(f, 0.5f, 0);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));
    W_TEST_BOOL(WInputManager::GetInputSlotState("test_slot_1", &f) == WKeyState::Released);
    W_TEST_FLOAT(f, 0.0f, 0);

    WInputManager::InjectInputSlotValue("test_slot_1", 0.2f);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));
    W_TEST_BOOL(WInputManager::GetInputSlotState("test_slot_1", &f) == WKeyState::Up);
    W_TEST_FLOAT(f, 0.0f, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetActionDisplayName / GetActionDisplayName")
  {
    WInputManager::SetActionDisplayName("test_action_1", "Test Action 1 Name");
    WInputManager::SetActionDisplayName("test_action_2", "Test Action 2 Name");
    WInputManager::SetActionDisplayName("test_action_3", "Test Action 3 Name");
    WInputManager::SetActionDisplayName("test_action_4", "Test Action 4 Name");

    W_TEST_STRING(WInputManager::GetActionDisplayName("test_action_0"), "test_action_0");
    W_TEST_STRING(WInputManager::GetActionDisplayName("test_action_1"), "Test Action 1 Name");
    W_TEST_STRING(WInputManager::GetActionDisplayName("test_action_2"), "Test Action 2 Name");
    W_TEST_STRING(WInputManager::GetActionDisplayName("test_action_3"), "Test Action 3 Name");
    W_TEST_STRING(WInputManager::GetActionDisplayName("test_action_4"), "Test Action 4 Name");
    W_TEST_STRING(WInputManager::GetActionDisplayName("test_action_5"), "test_action_5");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Input Sets")
  {
    WInputActionConfig iac;
    WInputManager::SetInputActionConfig("test_inputset", "test_action_1", iac, true);
    WInputManager::SetInputActionConfig("test_inputset2", "test_action_2", iac, true);

    WDynamicArray<WString> InputSetNames;
    WInputManager::GetAllInputSets(InputSetNames);

    W_TEST_INT(InputSetNames.GetCount(), 2);

    W_TEST_STRING(InputSetNames[0].GetData(), "test_inputset");
    W_TEST_STRING(InputSetNames[1].GetData(), "test_inputset2");

    WInputManager::RemoveInputAction("test_inputset", "test_action_1");
    WInputManager::RemoveInputAction("test_inputset2", "test_action_2");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAllInputActions / RemoveInputAction")
  {
    WTempHybridArray<WString, 24> InputActions;

    WInputManager::GetAllInputActions("test_inputset_3", InputActions);

    W_TEST_BOOL(InputActions.IsEmpty());

    WInputActionConfig iac;
    WInputManager::SetInputActionConfig("test_inputset_3", "test_action_1", iac, true);
    WInputManager::SetInputActionConfig("test_inputset_3", "test_action_2", iac, true);
    WInputManager::SetInputActionConfig("test_inputset_3", "test_action_3", iac, true);

    WInputManager::GetAllInputActions("test_inputset_3", InputActions);

    W_TEST_INT(InputActions.GetCount(), 3);

    W_TEST_STRING(InputActions[0].GetData(), "test_action_1");
    W_TEST_STRING(InputActions[1].GetData(), "test_action_2");
    W_TEST_STRING(InputActions[2].GetData(), "test_action_3");


    WInputManager::RemoveInputAction("test_inputset_3", "test_action_2");

    WInputManager::GetAllInputActions("test_inputset_3", InputActions);

    W_TEST_INT(InputActions.GetCount(), 2);

    W_TEST_STRING(InputActions[0].GetData(), "test_action_1");
    W_TEST_STRING(InputActions[1].GetData(), "test_action_3");

    WInputManager::RemoveInputAction("test_inputset_3", "test_action_1");
    WInputManager::RemoveInputAction("test_inputset_3", "test_action_3");

    WInputManager::GetAllInputActions("test_inputset_3", InputActions);

    W_TEST_BOOL(InputActions.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Input Action State Changes")
  {
    WInputActionConfig iac;
    iac.m_bApplyTimeScaling = false;
    iac.m_sInputSlotTrigger[0] = "test_input_slot_1";
    iac.m_sInputSlotTrigger[1] = "test_input_slot_2";
    iac.m_sInputSlotTrigger[2] = "test_input_slot_3";

    // bind the three slots to this action
    WInputManager::SetInputActionConfig("test_inputset", "test_action", iac, true);

    // bind the same three slots to another action
    WInputManager::SetInputActionConfig("test_inputset", "test_action_2", iac, false);

    // the first slot to trigger the action is bound to it, the other slots can now trigger other actions
    // but not this one anymore
    WInputManager::InjectInputSlotValue("test_input_slot_2", 1.0f);

    float f = 0;
    WInt8 iSlot = 0;
    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == WKeyState::Up);
    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == WKeyState::Up);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == WKeyState::Pressed);
    W_TEST_INT(iSlot, 1);
    W_TEST_FLOAT(f, 1.0f, 0.0f);

    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == WKeyState::Pressed);
    W_TEST_INT(iSlot, 1);
    W_TEST_FLOAT(f, 1.0f, 0.0f);

    // inject all three input slots
    WInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    WInputManager::InjectInputSlotValue("test_input_slot_2", 1.0f);
    WInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == WKeyState::Down);
    W_TEST_INT(iSlot, 1); // still the same slot that 'triggered' the action
    W_TEST_FLOAT(f, 1.0f, 0.0f);

    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == WKeyState::Down);
    W_TEST_INT(iSlot, 1); // still the same slot that 'triggered' the action
    W_TEST_FLOAT(f, 1.0f, 0.0f);

    WInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    WInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == WKeyState::Released);
    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == WKeyState::Released);

    WInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    WInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == WKeyState::Up);
    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == WKeyState::Up);

    WInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == WKeyState::Pressed);
    W_TEST_INT(iSlot, 2);
    W_TEST_FLOAT(f, 1.0f, 0.0f);

    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == WKeyState::Pressed);
    W_TEST_INT(iSlot, 2);
    W_TEST_FLOAT(f, 1.0f, 0.0f);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == WKeyState::Released);
    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == WKeyState::Released);

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == WKeyState::Up);
    W_TEST_BOOL(WInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == WKeyState::Up);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetPressedInputSlot")
  {
    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    WStringView sSlot = WInputManager::GetPressedInputSlot(WInputSlotFlags::None, WInputSlotFlags::None);
    W_TEST_BOOL(sSlot.IsEmpty());

    WInputManager::InjectInputSlotValue("test_slot", 1.0f);

    sSlot = WInputManager::GetPressedInputSlot(WInputSlotFlags::None, WInputSlotFlags::None);
    W_TEST_STRING(sSlot, "");

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    sSlot = WInputManager::GetPressedInputSlot(WInputSlotFlags::None, WInputSlotFlags::None);
    W_TEST_STRING(sSlot, "test_slot");


    {
      WTestInputDevide dev;
      dev.ActivateAll();

      WInputManager::InjectInputSlotValue("test_slot", 1.0f);

      WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

      sSlot = WInputManager::GetPressedInputSlot(WInputSlotFlags::IsButton, WInputSlotFlags::None);
      W_TEST_STRING(sSlot, "testdevice_button");

      sSlot = WInputManager::GetPressedInputSlot(WInputSlotFlags::IsAnalogStick, WInputSlotFlags::None);
      W_TEST_STRING(sSlot, "testdevice_stick");

      sSlot = WInputManager::GetPressedInputSlot(WInputSlotFlags::IsMouseWheel, WInputSlotFlags::None);
      W_TEST_STRING(sSlot, "testdevice_wheel");

      sSlot = WInputManager::GetPressedInputSlot(WInputSlotFlags::IsTouchPoint, WInputSlotFlags::None);
      W_TEST_STRING(sSlot, "testdevice_touchpoint");

      WInputManager::InjectInputSlotValue("test_slot", 1.0f);

      WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

      sSlot = WInputManager::GetPressedInputSlot(WInputSlotFlags::IsButton, WInputSlotFlags::None);
      W_TEST_STRING(sSlot, "");

      sSlot = WInputManager::GetPressedInputSlot(WInputSlotFlags::IsAnalogStick, WInputSlotFlags::None);
      W_TEST_STRING(sSlot, "");

      sSlot = WInputManager::GetPressedInputSlot(WInputSlotFlags::IsMouseWheel, WInputSlotFlags::None);
      W_TEST_STRING(sSlot, "");

      sSlot = WInputManager::GetPressedInputSlot(WInputSlotFlags::IsTouchPoint, WInputSlotFlags::None);
      W_TEST_STRING(sSlot, "");
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "LastCharacter")
  {
    WTestInputDevide dev;
    dev.ActivateAll();

    W_TEST_BOOL(WInputManager::RetrieveLastCharacters(true).IsEmpty());

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    W_TEST_STRING(WInputManager::RetrieveLastCharacters(false), "\42");
    W_TEST_STRING(WInputManager::RetrieveLastCharacters(true), "\42");
    W_TEST_BOOL(WInputManager::RetrieveLastCharacters(true).IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Time Scaling")
  {
    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    WInputActionConfig iac;
    iac.m_bApplyTimeScaling = true;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    WInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    WTestInputDevide dev;
    dev.ActivateAll();

    float fVal;

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));
    WInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    W_TEST_FLOAT(fVal, 0.1f * (1.0 / 60.0), 0.0001f); // testdevice_button has a value of 0.1f

    dev.ActivateAll();

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 30.0));
    WInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    W_TEST_FLOAT(fVal, 0.1f * (1.0 / 30.0), 0.0001f);


    iac.m_bApplyTimeScaling = false;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    WInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    dev.ActivateAll();

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));
    WInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    W_TEST_FLOAT(fVal, 0.1f, 0.0001f); // testdevice_button has a value of 0.1f

    dev.ActivateAll();

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 30.0));
    WInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    W_TEST_FLOAT(fVal, 0.1f, 0.0001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetInputSlotFlags")
  {
    WTestInputDevide dev;
    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 30.0));

    W_TEST_BOOL(WInputManager::GetInputSlotFlags("testdevice_button") == WInputSlotFlags::IsButton);
    W_TEST_BOOL(WInputManager::GetInputSlotFlags("testdevice_stick") == WInputSlotFlags::IsAnalogStick);
    W_TEST_BOOL(WInputManager::GetInputSlotFlags("testdevice_wheel") == WInputSlotFlags::IsMouseWheel);
    W_TEST_BOOL(WInputManager::GetInputSlotFlags("testdevice_touchpoint") == WInputSlotFlags::IsTouchPoint);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ClearInputMapping")
  {
    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    WInputActionConfig iac;
    iac.m_bApplyTimeScaling = true;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    WInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    WTestInputDevide dev;
    dev.ActivateAll();

    float fVal;

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));
    WInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    W_TEST_FLOAT(fVal, 0.1f * (1.0 / 60.0), 0.0001f); // testdevice_button has a value of 0.1f

    // clear the button from the action
    WInputManager::ClearInputMapping("test_inputset", "testdevice_button");

    dev.ActivateAll();

    WInputManager::Update(WTime::MakeFromSeconds(1.0 / 60.0));

    // should not receive input anymore
    WInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    W_TEST_FLOAT(fVal, 0.0f, 0.0001f);
  }
}
