#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Core/Input/Declarations.h>
#include <Foundation/Strings/String.h>
#include <RmlUi/Include/RmlUi/Core.h>

struct WRmlUiInputButtons
{
  using StorageType = WUInt32;

  enum Enum
  {
    None             = 0,

    Mouse0           = W_BIT(0),
    Mouse1           = W_BIT(1),
    Mouse2           = W_BIT(2),
    MouseWheelUp     = W_BIT(3),
    MouseWheelDown   = W_BIT(4),
    Tab              = W_BIT(5),
    Left             = W_BIT(6),
    Up               = W_BIT(7),
    Right            = W_BIT(8),
    Down             = W_BIT(9),
    PageUp           = W_BIT(10),
    PageDown         = W_BIT(11),
    Home             = W_BIT(12),
    End              = W_BIT(13),
    Delete           = W_BIT(14),
    Backspace        = W_BIT(15),
    Return           = W_BIT(16),
    NumpadEnter      = W_BIT(17),
    Escape           = W_BIT(18),
    Alt              = W_BIT(19),
    Ctrl             = W_BIT(20),
    Shift            = W_BIT(21),

    Default          = None,
  };

  struct Bits
  {
    StorageType Mouse0          : 1;
    StorageType Mouse1          : 1;
    StorageType Mouse2          : 1;
    StorageType MouseWheelUp    : 1;
    StorageType MouseWheelDown  : 1;
    StorageType Tab             : 1;
    StorageType Left            : 1;
    StorageType Up              : 1;
    StorageType Right           : 1;
    StorageType Down            : 1;
    StorageType PageUp          : 1;
    StorageType PageDown        : 1;
    StorageType Home            : 1;
    StorageType End             : 1;
    StorageType Delete          : 1;
    StorageType Backspace       : 1;
    StorageType Return          : 1;
    StorageType NumpadEnter     : 1;
    StorageType Escape          : 1;
    StorageType Alt             : 1;
    StorageType Ctrl            : 1;
    StorageType Shift           : 1;
  };

  struct MouseButtonMapping
  {
    Enum uiEzButton;
    WUInt32 uiRmlButton;
    const char* szEzButton;
  };

  struct KeyMapping
  {
    Enum uiEzKey;
    Rml::Input::KeyIdentifier uiRmlKey;
    const char* szEzKey;
  };

  constexpr static MouseButtonMapping s_MouseButtonMappings[] = {
    { WRmlUiInputButtons::Mouse0, 0, WInputSlot_MouseButton0 },
    { WRmlUiInputButtons::Mouse1, 1, WInputSlot_MouseButton1 },
    { WRmlUiInputButtons::Mouse2, 2, WInputSlot_MouseButton2 },
  };

  constexpr static KeyMapping s_KeyMappings[] = {
    { WRmlUiInputButtons::Tab, Rml::Input::KI_TAB, WInputSlot_KeyTab },
    { WRmlUiInputButtons::Left, Rml::Input::KI_LEFT, WInputSlot_KeyLeft },
    { WRmlUiInputButtons::Up, Rml::Input::KI_UP, WInputSlot_KeyUp },
    { WRmlUiInputButtons::Right, Rml::Input::KI_RIGHT, WInputSlot_KeyRight },
    { WRmlUiInputButtons::Down, Rml::Input::KI_DOWN, WInputSlot_KeyDown },
    { WRmlUiInputButtons::PageUp, Rml::Input::KI_PRIOR, WInputSlot_KeyPageUp },
    { WRmlUiInputButtons::PageDown, Rml::Input::KI_NEXT, WInputSlot_KeyPageDown },
    { WRmlUiInputButtons::Home, Rml::Input::KI_HOME, WInputSlot_KeyHome },
    { WRmlUiInputButtons::End, Rml::Input::KI_END, WInputSlot_KeyEnd },
    { WRmlUiInputButtons::Delete, Rml::Input::KI_DELETE, WInputSlot_KeyDelete },
    { WRmlUiInputButtons::Backspace, Rml::Input::KI_BACK, WInputSlot_KeyBackspace },
    { WRmlUiInputButtons::Return, Rml::Input::KI_RETURN, WInputSlot_KeyReturn },
    { WRmlUiInputButtons::NumpadEnter, Rml::Input::KI_NUMPADENTER, WInputSlot_KeyNumpadEnter },
    { WRmlUiInputButtons::Escape, Rml::Input::KI_ESCAPE, WInputSlot_KeyEscape },
  };
};

W_DECLARE_FLAGS_OPERATORS(WRmlUiInputButtons);

struct W_RMLUIPLUGIN_DLL WRmlUiInputSnapshot
{
  WBitflags<WRmlUiInputButtons> m_Buttons;
  WString m_sLastCharacters;

  W_ALWAYS_INLINE bool operator==(const WRmlUiInputSnapshot& rhs) const
  {
    return m_Buttons == rhs.m_Buttons && m_sLastCharacters == rhs.m_sLastCharacters;
  }

  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WRmlUiInputSnapshot&);

  [[nodiscard]] W_ALWAYS_INLINE static WRmlUiInputSnapshot MakeEmpty() { return WRmlUiInputSnapshot(); }
  [[nodiscard]] static WRmlUiInputSnapshot MakeFromCurrentInput();
};

struct W_RMLUIPLUGIN_DLL WRmlUiInputProvider
{
  bool Update(WRmlUiInputSnapshot input);

  WKeyState::Enum GetButtonState(WRmlUiInputButtons::Enum button) const;

  W_ALWAYS_INLINE bool IsButtonDown(WRmlUiInputButtons::Enum button) const
  {
    return m_Buttons.IsSet(button);
  }

  W_ALWAYS_INLINE bool IsAnyButtonDown() const
  {
    return m_Buttons.IsAnyFlagSet() || !m_sLastCharacters.IsEmpty();
  }

  W_ALWAYS_INLINE bool HasAnyInput() const
  {
    return IsAnyButtonDown() || m_PrevButtons.IsAnyFlagSet();
  }

  WString m_sLastCharacters;
  WBitflags<WRmlUiInputButtons> m_Buttons, m_PrevButtons;
};
