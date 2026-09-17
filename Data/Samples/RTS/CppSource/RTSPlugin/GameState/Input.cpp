#include <RTSPlugin/RTSPluginPCH.h>

#include <RTSPlugin/GameState/RTSGameState.h>

void RTSGameState::ConfigureMainWindowInputDevices(WWindow* pWindow)
{
  SUPER::ConfigureMainWindowInputDevices(pWindow);

  if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard*>(pWindow->GetInputDevice()))
  {
    // pInput->SetClipMouseCursor(WMouseCursorClipMode::NoClip);
    // The hardware cursor is hidden automatically while a custom cursor is set,
    // see RTSGameState::UpdateMouseCursor() and WMouseCursorRenderer.
    pInput->SetMouseSpeed(WVec2(0.002f));
  }
}

void RTSGameState::ConfigureInputActions()
{
  // do NOT call the base implementation, because we don't want the default setup
  // instead, go to WGameApplication directly and only set up what we want
  // SUPER::ConfigureInputActions();

  if (auto pApp = WGameApplication::GetGameApplicationInstance())
  {
    WBitflags<WGameApplicationInputFlags> flags = WGameApplicationInputFlags::All;
    flags.Remove(WGameApplicationInputFlags::Dev_EscapeToClose); // remove the "ESC to quit" functionality
    pApp->RegisterGameApplicationInputActions(flags);
  }


  WInputActionConfig cfg;

  // Mouse Input
  {
    cfg.m_sInputSlotTrigger[0] = WInputSlot_MousePositionX;
    WInputManager::SetInputActionConfig("Game", "MousePosX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_MousePositionY;
    WInputManager::SetInputActionConfig("Game", "MousePosY", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseButton0;
    WInputManager::SetInputActionConfig("Game", "MouseLeftClick", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseButton1;
    WInputManager::SetInputActionConfig("Game", "MouseRightClick", cfg, true);
  }

  // Default Camera Navigation
  {
    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseWheelUp;
    WInputManager::SetInputActionConfig("Game", "CamZoomIn", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseWheelDown;
    WInputManager::SetInputActionConfig("Game", "CamZoomOut", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMovePosX;
    WInputManager::SetInputActionConfig("Game", "CamMovePosX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMoveNegX;
    WInputManager::SetInputActionConfig("Game", "CamMoveNegX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMovePosY;
    WInputManager::SetInputActionConfig("Game", "CamMovePosY", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMoveNegY;
    WInputManager::SetInputActionConfig("Game", "CamMoveNegY", cfg, true);
  }
}

void RTSGameState::UpdateMousePosition()
{
  WView* pView = nullptr;
  if (!WRenderWorld::TryGetView(m_hMainView, pView))
    return;

  const auto vp = pView->GetViewport();

  float valueX, valueY;
  WInputManager::GetInputActionState("Game", "MousePosX", &valueX);
  WInputManager::GetInputActionState("Game", "MousePosY", &valueY);
  m_MouseInputState.m_LeftClickState = WInputManager::GetInputActionState("Game", "MouseLeftClick");
  m_MouseInputState.m_RightClickState = WInputManager::GetInputActionState("Game", "MouseRightClick");

  m_MouseInputState.m_MousePos.x = (WUInt32)(valueX * vp.width);
  m_MouseInputState.m_MousePos.y = (WUInt32)(valueY * vp.height);

  if (m_MouseInputState.m_LeftClickState == WKeyState::Pressed)
  {
    m_MouseInputState.m_MousePosLeftClick = m_MouseInputState.m_MousePos;
    m_MouseInputState.m_bLeftMouseMoved = false;
  }

  if (m_MouseInputState.m_RightClickState == WKeyState::Pressed)
  {
    m_MouseInputState.m_MousePosRightClick = m_MouseInputState.m_MousePos;
    m_MouseInputState.m_bRightMouseMoved = false;
  }

  m_MouseInputState.m_bLeftMouseMoved = m_MouseInputState.m_bLeftMouseMoved || RtsMouseInputState::HasMouseMoved(m_MouseInputState.m_MousePosLeftClick, m_MouseInputState.m_MousePos);
  m_MouseInputState.m_bRightMouseMoved = m_MouseInputState.m_bRightMouseMoved || RtsMouseInputState::HasMouseMoved(m_MouseInputState.m_MousePosRightClick, m_MouseInputState.m_MousePos);

  ComputePickingRay().IgnoreResult();
}

void RTSGameState::ProcessInput()
{
  SUPER::ProcessInput();

  W_LOCK(m_pMainWorld->GetWriteMarker());

  UpdateMousePosition();
  UpdateMouseCursor();

  if (m_pActiveGameMode)
  {
    m_pActiveGameMode->ProcessInput(m_MouseInputState);

    m_pActiveGameMode->AfterProcessInput();
  }
}
