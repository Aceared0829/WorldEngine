#include <Core/CorePCH.h>

#if W_ENABLED(W_SUPPORTS_GLFW)

#  include <Core/Platform/GLFW/InputDevice_GLFW.h>
#  include <GLFW/glfw3.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInputDeviceMouseKeyboard_GLFW, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  const char* ConvertGLFWKeyToEngineName(int key)
  {
    switch (key)
    {
      case GLFW_KEY_LEFT:
        return WInputSlot_KeyLeft;
      case GLFW_KEY_RIGHT:
        return WInputSlot_KeyRight;
      case GLFW_KEY_UP:
        return WInputSlot_KeyUp;
      case GLFW_KEY_DOWN:
        return WInputSlot_KeyDown;
      case GLFW_KEY_ESCAPE:
        return WInputSlot_KeyEscape;
      case GLFW_KEY_SPACE:
        return WInputSlot_KeySpace;
      case GLFW_KEY_BACKSPACE:
        return WInputSlot_KeyBackspace;
      case GLFW_KEY_ENTER:
        return WInputSlot_KeyReturn;
      case GLFW_KEY_TAB:
        return WInputSlot_KeyTab;
      case GLFW_KEY_LEFT_SHIFT:
        return WInputSlot_KeyLeftShift;
      case GLFW_KEY_RIGHT_SHIFT:
        return WInputSlot_KeyRightShift;
      case GLFW_KEY_LEFT_CONTROL:
        return WInputSlot_KeyLeftCtrl;
      case GLFW_KEY_RIGHT_CONTROL:
        return WInputSlot_KeyRightCtrl;
      case GLFW_KEY_LEFT_ALT:
        return WInputSlot_KeyLeftAlt;
      case GLFW_KEY_RIGHT_ALT:
        return WInputSlot_KeyRightAlt;
      case GLFW_KEY_LEFT_SUPER:
        return WInputSlot_KeyLeftWin;
      case GLFW_KEY_RIGHT_SUPER:
        return WInputSlot_KeyRightWin;
      case GLFW_KEY_MENU:
        return WInputSlot_KeyApps;
      case GLFW_KEY_LEFT_BRACKET:
        return WInputSlot_KeyBracketOpen;
      case GLFW_KEY_RIGHT_BRACKET:
        return WInputSlot_KeyBracketClose;
      case GLFW_KEY_SEMICOLON:
        return WInputSlot_KeySemicolon;
      case GLFW_KEY_APOSTROPHE:
        return WInputSlot_KeyApostrophe;
      case GLFW_KEY_SLASH:
        return WInputSlot_KeySlash;
      case GLFW_KEY_EQUAL:
        return WInputSlot_KeyEquals;
      case GLFW_KEY_GRAVE_ACCENT:
        return WInputSlot_KeyTilde;
      case GLFW_KEY_MINUS:
        return WInputSlot_KeyHyphen;
      case GLFW_KEY_COMMA:
        return WInputSlot_KeyComma;
      case GLFW_KEY_PERIOD:
        return WInputSlot_KeyPeriod;
      case GLFW_KEY_BACKSLASH:
        return WInputSlot_KeyBackslash;
      case GLFW_KEY_WORLD_1:
        return WInputSlot_KeyPipe;
      case GLFW_KEY_1:
        return WInputSlot_Key1;
      case GLFW_KEY_2:
        return WInputSlot_Key2;
      case GLFW_KEY_3:
        return WInputSlot_Key3;
      case GLFW_KEY_4:
        return WInputSlot_Key4;
      case GLFW_KEY_5:
        return WInputSlot_Key5;
      case GLFW_KEY_6:
        return WInputSlot_Key6;
      case GLFW_KEY_7:
        return WInputSlot_Key7;
      case GLFW_KEY_8:
        return WInputSlot_Key8;
      case GLFW_KEY_9:
        return WInputSlot_Key9;
      case GLFW_KEY_0:
        return WInputSlot_Key0;
      case GLFW_KEY_KP_1:
        return WInputSlot_KeyNumpad1;
      case GLFW_KEY_KP_2:
        return WInputSlot_KeyNumpad2;
      case GLFW_KEY_KP_3:
        return WInputSlot_KeyNumpad3;
      case GLFW_KEY_KP_4:
        return WInputSlot_KeyNumpad4;
      case GLFW_KEY_KP_5:
        return WInputSlot_KeyNumpad5;
      case GLFW_KEY_KP_6:
        return WInputSlot_KeyNumpad6;
      case GLFW_KEY_KP_7:
        return WInputSlot_KeyNumpad7;
      case GLFW_KEY_KP_8:
        return WInputSlot_KeyNumpad8;
      case GLFW_KEY_KP_9:
        return WInputSlot_KeyNumpad9;
      case GLFW_KEY_KP_0:
        return WInputSlot_KeyNumpad0;
      case GLFW_KEY_A:
        return WInputSlot_KeyA;
      case GLFW_KEY_B:
        return WInputSlot_KeyB;
      case GLFW_KEY_C:
        return WInputSlot_KeyC;
      case GLFW_KEY_D:
        return WInputSlot_KeyD;
      case GLFW_KEY_E:
        return WInputSlot_KeyE;
      case GLFW_KEY_F:
        return WInputSlot_KeyF;
      case GLFW_KEY_G:
        return WInputSlot_KeyG;
      case GLFW_KEY_H:
        return WInputSlot_KeyH;
      case GLFW_KEY_I:
        return WInputSlot_KeyI;
      case GLFW_KEY_J:
        return WInputSlot_KeyJ;
      case GLFW_KEY_K:
        return WInputSlot_KeyK;
      case GLFW_KEY_L:
        return WInputSlot_KeyL;
      case GLFW_KEY_M:
        return WInputSlot_KeyM;
      case GLFW_KEY_N:
        return WInputSlot_KeyN;
      case GLFW_KEY_O:
        return WInputSlot_KeyO;
      case GLFW_KEY_P:
        return WInputSlot_KeyP;
      case GLFW_KEY_Q:
        return WInputSlot_KeyQ;
      case GLFW_KEY_R:
        return WInputSlot_KeyR;
      case GLFW_KEY_S:
        return WInputSlot_KeyS;
      case GLFW_KEY_T:
        return WInputSlot_KeyT;
      case GLFW_KEY_U:
        return WInputSlot_KeyU;
      case GLFW_KEY_V:
        return WInputSlot_KeyV;
      case GLFW_KEY_W:
        return WInputSlot_KeyW;
      case GLFW_KEY_X:
        return WInputSlot_KeyX;
      case GLFW_KEY_Y:
        return WInputSlot_KeyY;
      case GLFW_KEY_Z:
        return WInputSlot_KeyZ;
      case GLFW_KEY_F1:
        return WInputSlot_KeyF1;
      case GLFW_KEY_F2:
        return WInputSlot_KeyF2;
      case GLFW_KEY_F3:
        return WInputSlot_KeyF3;
      case GLFW_KEY_F4:
        return WInputSlot_KeyF4;
      case GLFW_KEY_F5:
        return WInputSlot_KeyF5;
      case GLFW_KEY_F6:
        return WInputSlot_KeyF6;
      case GLFW_KEY_F7:
        return WInputSlot_KeyF7;
      case GLFW_KEY_F8:
        return WInputSlot_KeyF8;
      case GLFW_KEY_F9:
        return WInputSlot_KeyF9;
      case GLFW_KEY_F10:
        return WInputSlot_KeyF10;
      case GLFW_KEY_F11:
        return WInputSlot_KeyF11;
      case GLFW_KEY_F12:
        return WInputSlot_KeyF12;
      case GLFW_KEY_HOME:
        return WInputSlot_KeyHome;
      case GLFW_KEY_END:
        return WInputSlot_KeyEnd;
      case GLFW_KEY_DELETE:
        return WInputSlot_KeyDelete;
      case GLFW_KEY_INSERT:
        return WInputSlot_KeyInsert;
      case GLFW_KEY_PAGE_UP:
        return WInputSlot_KeyPageUp;
      case GLFW_KEY_PAGE_DOWN:
        return WInputSlot_KeyPageDown;
      case GLFW_KEY_NUM_LOCK:
        return WInputSlot_KeyNumLock;
      case GLFW_KEY_KP_ADD:
        return WInputSlot_KeyNumpadPlus;
      case GLFW_KEY_KP_SUBTRACT:
        return WInputSlot_KeyNumpadMinus;
      case GLFW_KEY_KP_MULTIPLY:
        return WInputSlot_KeyNumpadStar;
      case GLFW_KEY_KP_DIVIDE:
        return WInputSlot_KeyNumpadSlash;
      case GLFW_KEY_KP_DECIMAL:
        return WInputSlot_KeyNumpadPeriod;
      case GLFW_KEY_KP_ENTER:
        return WInputSlot_KeyNumpadEnter;
      case GLFW_KEY_CAPS_LOCK:
        return WInputSlot_KeyCapsLock;
      case GLFW_KEY_PRINT_SCREEN:
        return WInputSlot_KeyPrint;
      case GLFW_KEY_SCROLL_LOCK:
        return WInputSlot_KeyScroll;
      case GLFW_KEY_PAUSE:
        return WInputSlot_KeyPause;
      // TODO WInputSlot_KeyPrevTrack
      // TODO WInputSlot_KeyNextTrack
      // TODO WInputSlot_KeyPlayPause
      // TODO WInputSlot_KeyStop
      // TODO WInputSlot_KeyVolumeUp
      // TODO WInputSlot_KeyVolumeDown
      // TODO WInputSlot_KeyMute
      default:
        return nullptr;
    }
  }
} // namespace

WInputDeviceMouseKeyboard_GLFW::WInputDeviceMouseKeyboard_GLFW(GLFWwindow* windowHandle)
  : m_pWindow(windowHandle)
{
}

WInputDeviceMouseKeyboard_GLFW::~WInputDeviceMouseKeyboard_GLFW()
{
}

void WInputDeviceMouseKeyboard_GLFW::ApplyShowMouseCursor(bool bShow, bool bCustomCursorActive)
{
  int iMode = GLFW_CURSOR_NORMAL;

  if (!bShow)
  {
    // GLFW_CURSOR_DISABLED not only hides the cursor, it also captures it and switches to unbounded
    // relative movement, which is what an application that hides the cursor for mouse-look wants.
    // A custom ('software') cursor however still needs absolute in-window mouse positions to be
    // rendered at the right place, so there the cursor may only be hidden.
    iMode = bCustomCursorActive ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED;
  }

  glfwSetInputMode(m_pWindow, GLFW_CURSOR, iMode);
}

WUInt32 WInputDeviceMouseKeyboard_GLFW::GetHardwareCursorSize() const
{
  // GLFW can't report the cursor size, so this is the common default of 32 pixels at 100% scaling,
  // adjusted for the monitor's DPI. It does not pick up a custom cursor size that the user configured.
  float fScaleX = 1.0f;
  float fScaleY = 1.0f; // not used, GLFW requires both to be passed in

  if (m_pWindow != nullptr)
  {
    glfwGetWindowContentScale(m_pWindow, &fScaleX, &fScaleY);
  }

  return (WUInt32)(32.0f * WMath::Max(fScaleX, 1.0f));
}

void WInputDeviceMouseKeyboard_GLFW::ApplyClipMouseCursor(WMouseCursorClipMode::Enum mode)
{
  W_IGNORE_UNUSED(mode);

  // not implemented on GLFW
}

void WInputDeviceMouseKeyboard_GLFW::InitializeDevice() {}

void WInputDeviceMouseKeyboard_GLFW::RegisterInputSlots()
{
  RegisterInputSlot(WInputSlot_KeyLeft, "Left", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyRight, "Right", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyUp, "Up", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyDown, "Down", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyEscape, "Escape", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeySpace, "Space", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyBackspace, "Backspace", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyReturn, "Return", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyTab, "Tab", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyLeftShift, "Left Shift", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyRightShift, "Right Shift", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyLeftCtrl, "Left Ctrl", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyRightCtrl, "Right Ctrl", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyLeftAlt, "Left Alt", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyRightAlt, "Right Alt", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyLeftWin, "Left Win", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyRightWin, "Right Win", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyBracketOpen, "[", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyBracketClose, "]", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeySemicolon, ";", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyApostrophe, "'", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeySlash, "/", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyEquals, "=", WInputSlotFlags::IsButton);
  // TODO RegisterInputSlot(WInputSlot_KeyTilde, "~", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyHyphen, "-", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyComma, ",", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPeriod, ".", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyBackslash, "\\", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPipe, "|", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_Key1, "1", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key2, "2", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key3, "3", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key4, "4", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key5, "5", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key6, "6", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key7, "7", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key8, "8", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key9, "9", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key0, "0", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyNumpad1, "Numpad 1", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad2, "Numpad 2", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad3, "Numpad 3", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad4, "Numpad 4", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad5, "Numpad 5", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad6, "Numpad 6", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad7, "Numpad 7", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad8, "Numpad 8", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad9, "Numpad 9", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad0, "Numpad 0", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyA, "A", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyB, "B", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyC, "C", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyD, "D", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyE, "E", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF, "F", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyG, "G", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyH, "H", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyI, "I", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyJ, "J", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyK, "K", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyL, "L", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyM, "M", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyN, "N", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyO, "O", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyP, "P", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyQ, "Q", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyR, "R", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyS, "S", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyT, "T", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyU, "U", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyV, "V", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyW, "W", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyX, "X", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyY, "Y", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyZ, "Z", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyF1, "F1", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF2, "F2", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF3, "F3", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF4, "F4", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF5, "F5", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF6, "F6", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF7, "F7", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF8, "F8", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF9, "F9", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF10, "F10", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF11, "F11", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF12, "F12", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyHome, "Home", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyEnd, "End", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyDelete, "Delete", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyInsert, "Insert", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPageUp, "Page Up", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPageDown, "Page Down", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyNumLock, "Numlock", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadPlus, "Numpad +", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadMinus, "Numpad -", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadStar, "Numpad *", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadSlash, "Numpad /", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadPeriod, "Numpad .", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadEnter, "Enter", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyCapsLock, "Capslock", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPrint, "Print", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyScroll, "Scroll", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPause, "Pause", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyApps, "Application", WInputSlotFlags::IsButton);

  /* TODO
  RegisterInputSlot(WInputSlot_KeyPrevTrack, "Previous Track", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNextTrack, "Next Track", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPlayPause, "Play / Pause", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyStop, "Stop", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyVolumeUp, "Volume Up", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyVolumeDown, "Volume Down", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyMute, "Mute", WInputSlotFlags::IsButton);
  */

  RegisterInputSlot(WInputSlot_MousePositionX, "Mouse Position X", WInputSlotFlags::IsMouseAxisPosition);
  RegisterInputSlot(WInputSlot_MousePositionY, "Mouse Position Y", WInputSlotFlags::IsMouseAxisPosition);

  RegisterInputSlot(WInputSlot_MouseMoveNegX, "Mouse Move Left", WInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(WInputSlot_MouseMovePosX, "Mouse Move Right", WInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(WInputSlot_MouseMoveNegY, "Mouse Move Down", WInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(WInputSlot_MouseMovePosY, "Mouse Move Up", WInputSlotFlags::IsMouseAxisMove);

  RegisterInputSlot(WInputSlot_MouseButton0, "Mousebutton 0", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_MouseButton1, "Mousebutton 1", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_MouseButton2, "Mousebutton 2", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_MouseButton3, "Mousebutton 3", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_MouseButton4, "Mousebutton 4", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_MouseWheelUp, "Mousewheel Up", WInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(WInputSlot_MouseWheelDown, "Mousewheel Down", WInputSlotFlags::IsMouseWheel);
}

void WInputDeviceMouseKeyboard_GLFW::ResetInputSlotValues()
{
  m_InputSlotValues[WInputSlot_MouseWheelUp] = 0;
  m_InputSlotValues[WInputSlot_MouseWheelDown] = 0;
  m_InputSlotValues[WInputSlot_MouseMoveNegX] = 0;
  m_InputSlotValues[WInputSlot_MouseMovePosX] = 0;
  m_InputSlotValues[WInputSlot_MouseMoveNegY] = 0;
  m_InputSlotValues[WInputSlot_MouseMovePosY] = 0;
}

void WInputDeviceMouseKeyboard_GLFW::OnKey(int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
  {
    m_sLastCharacters.Append(0x00000008u);
  }

  // TODO: if (key != scancode) -> use scancode, (ConvertScanCodeToEngineName), only if they are the same, use ConvertGLFWKeyToEngineName

  const char* szInputSlotName = ConvertGLFWKeyToEngineName(key);
  if (szInputSlotName)
  {
    m_InputSlotValues[szInputSlotName] = (action == GLFW_RELEASE) ? 0.0f : 1.0f;
  }
  else
  {
    WLog::Warning("Unhandeled glfw keyboard key {} {}", key, (action == GLFW_RELEASE) ? "released" : "pressed");
  }
}

void WInputDeviceMouseKeyboard_GLFW::OnCharacter(unsigned int codepoint)
{
  m_sLastCharacters.Append(codepoint);
}

void WInputDeviceMouseKeyboard_GLFW::OnCursorPosition(double xpos, double ypos)
{
  s_pMouseOver = this;

  int width;
  int height;
  glfwGetWindowSize(m_pWindow, &width, &height);

  m_vLocalMouseCoordinates.x = static_cast<float>(xpos / width);
  m_vLocalMouseCoordinates.y = static_cast<float>(ypos / height);

  m_InputSlotValues[WInputSlot_MousePositionX] = m_vLocalMouseCoordinates.x;
  m_InputSlotValues[WInputSlot_MousePositionY] = m_vLocalMouseCoordinates.y;

  if (m_LastPos.x != WMath::MaxValue<double>())
  {
    const float fMouseScale = 1.0f / 10.0f;
    WVec2d diff = WVec2d(xpos, ypos) - m_LastPos;

    m_InputSlotValues[WInputSlot_MouseMoveNegX] += ((diff.x < 0) ? (float)-diff.x : 0.0f) * GetMouseSpeed().x * fMouseScale;
    m_InputSlotValues[WInputSlot_MouseMovePosX] += ((diff.x > 0) ? (float)diff.x : 0.0f) * GetMouseSpeed().x * fMouseScale;
    m_InputSlotValues[WInputSlot_MouseMoveNegY] += ((diff.y < 0) ? (float)-diff.y : 0.0f) * GetMouseSpeed().y * fMouseScale;
    m_InputSlotValues[WInputSlot_MouseMovePosY] += ((diff.y > 0) ? (float)diff.y : 0.0f) * GetMouseSpeed().y * fMouseScale;
  }
  m_LastPos = WVec2d(xpos, ypos);
}

void WInputDeviceMouseKeyboard_GLFW::OnMouseButton(int button, int action, int mods)
{
  const char* inputSlot = nullptr;
  switch (button)
  {
    case GLFW_MOUSE_BUTTON_1:
      inputSlot = WInputSlot_MouseButton0;
      break;
    case GLFW_MOUSE_BUTTON_2:
      inputSlot = WInputSlot_MouseButton1;
      break;
    case GLFW_MOUSE_BUTTON_3:
      inputSlot = WInputSlot_MouseButton2;
      break;
    case GLFW_MOUSE_BUTTON_4:
      inputSlot = WInputSlot_MouseButton3;
      break;
    case GLFW_MOUSE_BUTTON_5:
      inputSlot = WInputSlot_MouseButton4;
      break;
  }

  if (inputSlot)
  {
    m_InputSlotValues[inputSlot] = (action == GLFW_PRESS) ? 1.0f : 0.0f;
  }
}

void WInputDeviceMouseKeyboard_GLFW::OnScroll(double xoffset, double yoffset)
{
  if (yoffset > 0)
  {
    m_InputSlotValues[WInputSlot_MouseWheelUp] = static_cast<float>(yoffset);
  }
  else
  {
    m_InputSlotValues[WInputSlot_MouseWheelDown] = static_cast<float>(-yoffset);
  }
}

#endif


W_STATICLINK_FILE(Core, Core_Platform_GLFW_InputDevice_GLFW);
