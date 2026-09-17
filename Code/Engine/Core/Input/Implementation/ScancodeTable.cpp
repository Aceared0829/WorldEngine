#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

WStringView WInputManager::ConvertScanCodeToEngineName(WUInt8 uiScanCode, bool bIsExtendedKey)
{
  const WUInt8 uiFinalScanCode = bIsExtendedKey ? (uiScanCode + 128) : uiScanCode;

  switch (uiFinalScanCode)
  {
    case 1:
      return WInputSlot_KeyEscape;
    case 2:
      return WInputSlot_Key1;
    case 3:
      return WInputSlot_Key2;
    case 4:
      return WInputSlot_Key3;
    case 5:
      return WInputSlot_Key4;
    case 6:
      return WInputSlot_Key5;
    case 7:
      return WInputSlot_Key6;
    case 8:
      return WInputSlot_Key7;
    case 9:
      return WInputSlot_Key8;
    case 10:
      return WInputSlot_Key9;
    case 11:
      return WInputSlot_Key0;
    case 12:
      return WInputSlot_KeyHyphen;
    case 13:
      return WInputSlot_KeyEquals;
    case 14:
      return WInputSlot_KeyBackspace;
    case 15:
      return WInputSlot_KeyTab;
    case 16:
      return WInputSlot_KeyQ;
    case 17:
      return WInputSlot_KeyW;
    case 18:
      return WInputSlot_KeyE;
    case 19:
      return WInputSlot_KeyR;
    case 20:
      return WInputSlot_KeyT;
    case 21:
      return WInputSlot_KeyY;
    case 22:
      return WInputSlot_KeyU;
    case 23:
      return WInputSlot_KeyI;
    case 24:
      return WInputSlot_KeyO;
    case 25:
      return WInputSlot_KeyP;
    case 26:
      return WInputSlot_KeyBracketOpen;
    case 27:
      return WInputSlot_KeyBracketClose;
    case 28:
      return WInputSlot_KeyReturn;
    case 29:
      return WInputSlot_KeyLeftCtrl;
    case 30:
      return WInputSlot_KeyA;
    case 31:
      return WInputSlot_KeyS;
    case 32:
      return WInputSlot_KeyD;
    case 33:
      return WInputSlot_KeyF;
    case 34:
      return WInputSlot_KeyG;
    case 35:
      return WInputSlot_KeyH;
    case 36:
      return WInputSlot_KeyJ;
    case 37:
      return WInputSlot_KeyK;
    case 38:
      return WInputSlot_KeyL;
    case 39:
      return WInputSlot_KeySemicolon;
    case 40:
      return WInputSlot_KeyApostrophe;
    case 41:
      return WInputSlot_KeyTilde;
    case 42:
      return WInputSlot_KeyLeftShift;
    case 43:
      return WInputSlot_KeyBackslash;
    case 44:
      return WInputSlot_KeyZ;
    case 45:
      return WInputSlot_KeyX;
    case 46:
      return WInputSlot_KeyC;
    case 47:
      return WInputSlot_KeyV;
    case 48:
      return WInputSlot_KeyB;
    case 49:
      return WInputSlot_KeyN;
    case 50:
      return WInputSlot_KeyM;
    case 51:
      return WInputSlot_KeyComma;
    case 52:
      return WInputSlot_KeyPeriod;
    case 53:
      return WInputSlot_KeySlash;
    case 54:
      return WInputSlot_KeyRightShift;
    case 55:
      return WInputSlot_KeyNumpadStar;
    case 56:
      return WInputSlot_KeyLeftAlt;
    case 57:
      return WInputSlot_KeySpace;
    case 58:
      return WInputSlot_KeyCapsLock;
    case 59:
      return WInputSlot_KeyF1;
    case 60:
      return WInputSlot_KeyF2;
    case 61:
      return WInputSlot_KeyF3;
    case 62:
      return WInputSlot_KeyF4;
    case 63:
      return WInputSlot_KeyF5;
    case 64:
      return WInputSlot_KeyF6;
    case 65:
      return WInputSlot_KeyF7;
    case 66:
      return WInputSlot_KeyF8;
    case 67:
      return WInputSlot_KeyF9;
    case 68:
      return WInputSlot_KeyF10;
    case 69:
      return WInputSlot_KeyNumLock;
    case 70:
      return WInputSlot_KeyScroll;
    case 71:
      return WInputSlot_KeyNumpad7;
    case 72:
      return WInputSlot_KeyNumpad8;
    case 73:
      return WInputSlot_KeyNumpad9;
    case 74:
      return WInputSlot_KeyNumpadMinus;
    case 75:
      return WInputSlot_KeyNumpad4;
    case 76:
      return WInputSlot_KeyNumpad5;
    case 77:
      return WInputSlot_KeyNumpad6;
    case 78:
      return WInputSlot_KeyNumpadPlus;
    case 79:
      return WInputSlot_KeyNumpad1;
    case 80:
      return WInputSlot_KeyNumpad2;
    case 81:
      return WInputSlot_KeyNumpad3;
    case 82:
      return WInputSlot_KeyNumpad0;
    case 83:
      return WInputSlot_KeyNumpadPeriod;


    case 86:
      return WInputSlot_KeyPipe;
    case 87:
      return WInputSlot_KeyF11;
    case 88:
      return WInputSlot_KeyF12;


    case 91:
      return WInputSlot_KeyLeftWin;
    case 92:
      return WInputSlot_KeyRightWin;
    case 93:
      return WInputSlot_KeyApps;



    case 128 + 16:
      return WInputSlot_KeyPrevTrack;
    case 128 + 25:
      return WInputSlot_KeyNextTrack;
    case 128 + 28:
      return WInputSlot_KeyNumpadEnter;
    case 128 + 29:
      return WInputSlot_KeyRightCtrl;
    case 128 + 32:
      return WInputSlot_KeyMute;
    case 128 + 34:
      return WInputSlot_KeyPlayPause;
    case 128 + 36:
      return WInputSlot_KeyStop;
    case 128 + 46:
      return WInputSlot_KeyVolumeDown;
    case 128 + 48:
      return WInputSlot_KeyVolumeUp;
    case 128 + 53:
      return WInputSlot_KeyNumpadSlash;
    case 128 + 55:
      return WInputSlot_KeyPrint;
    case 128 + 56:
      return WInputSlot_KeyRightAlt;
    case 128 + 70:
      return WInputSlot_KeyPause;
    case 128 + 71:
      return WInputSlot_KeyHome;
    case 128 + 72:
      return WInputSlot_KeyUp;
    case 128 + 73:
      return WInputSlot_KeyPageUp;
    case 128 + 75:
      return WInputSlot_KeyLeft;
    case 128 + 77:
      return WInputSlot_KeyRight;
    case 128 + 79:
      return WInputSlot_KeyEnd;
    case 128 + 80:
      return WInputSlot_KeyDown;
    case 128 + 81:
      return WInputSlot_KeyPageDown;
    case 128 + 82:
      return WInputSlot_KeyInsert;
    case 128 + 83:
      return WInputSlot_KeyDelete;

    default:

      // for extended keys fall back to the non-extended name
      if (bIsExtendedKey)
        return ConvertScanCodeToEngineName(uiScanCode, false);

      break;
  }

  return "unknown_key";
}
