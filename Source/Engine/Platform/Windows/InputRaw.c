#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <windowsx.h>

#include "Platform/Platform.h"
#include "Platform/Log.h"
#include "Platform/Events.h"

#define MOUSE_LEFT_CODE   0b0000  // 0x0 D0
#define MOUSE_MIDDLE_CODE 0b0001  // 0x1 D1
#define MOUSE_RIGHT_CODE  0b0010  // 0x2 D2

typedef struct {
  uint8 scanCode;
  bool isExtended;
} PKeyScan;

static struct {
  PKey keyMap[KEY_MAX];
} SApiState;

static PKeyScan InternalEKeyCodeToPKeyScan(EKeyCode KeyCode);
static cstring InternalVkCodeToStr(uint32 VkCode);
static PKey InternalMouseCodeToPKey(uint32 MouseCode);
static PKey InternalScancodeToPKey(uint32 Scancode, bool bIsExtended);

bool ApiRawInputInit(HWND hWindow) {
#define MAX_DEVICES 2
  RAWINPUTDEVICE rid[MAX_DEVICES];
  // Keybord
  rid[0].usUsagePage = 0x01;  // Generic Desktop Controls
  rid[0].usUsage = 0x06;      // Keybord
  rid[0].dwFlags = RIDEV_DEVNOTIFY;
  rid[0].hwndTarget = hWindow;

  // Mouse
  rid[1].usUsagePage = 0x01;  // Generic Desktop Controls
  rid[1].usUsage = 0x02;      // Mouse
  rid[1].dwFlags = RIDEV_DEVNOTIFY;
  rid[1].hwndTarget = hWindow;

  if(!RegisterRawInputDevices(rid, MAX_DEVICES, sizeof(rid[0]))) {
    DWORD err = GetLastError();
    GT_FATAL("API:WIN32 RawInput failed, error = %lu", err);
    return false;
  }
  return true;
}

void ApiRawInputUpdateKeyMap() {
  HKL keyboardLayout = GetKeyboardLayout(0);
  for(uint32 c = 0; c < KEY_MAX; c++) {
    PKeyScan keyScan = InternalEKeyCodeToPKeyScan(c);
    PKey pKey = InternalScancodeToPKey(keyScan.scanCode, keyScan.isExtended);
    if(pKey.keyCode == KEY_UNKNOWN) {
      continue;
    }
    if(keyScan.isExtended) {
      keyScan.scanCode |= 0xE000;
    }
    UINT vk = MapVirtualKeyExA(keyScan.scanCode, MAPVK_VSC_TO_VK_EX, keyboardLayout);
    cstring name = InternalVkCodeToStr(vk);
    if(name) {
      pKey.layoutName = name;
    }
    SApiState.keyMap[pKey.keyCode] = pKey;
  }
  SApiState.keyMap[KEY_MOUSE_LEFT] = InternalMouseCodeToPKey(MOUSE_LEFT_CODE);
  SApiState.keyMap[KEY_MOUSE_MIDDLE] = InternalMouseCodeToPKey(MOUSE_MIDDLE_CODE);
  SApiState.keyMap[KEY_MOUSE_RIGHT] = InternalMouseCodeToPKey(MOUSE_RIGHT_CODE);

  PEvent pEvent = {PE_INPUT_KEY_MAP};
  pEvent.keymap.map = SApiState.keyMap;
  pEvent.keymap.count = KEY_MAX;
  PEventPush(pEvent);
}

void ApiRawInputPollEvent(HRAWINPUT RawInput) {
  static struct {
    char* data;
    uint64 size;
  } inputData = {NULL, 0};
  static struct {
    USHORT flagDown;
    USHORT flagUp;
    int code;
  } buttons[] = {
      {RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_LEFT_BUTTON_UP, MOUSE_LEFT_CODE},       //
      {RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_UP, MOUSE_RIGHT_CODE},    //
      {RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_UP, MOUSE_MIDDLE_CODE}  //
  };

  UINT dwSize = 0;
  GetRawInputData(RawInput, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
  if(inputData.size < dwSize) {
    inputData.data = PMemRealloc(inputData.data, dwSize);
    inputData.size = dwSize;
  }
  if(!GetRawInputData(RawInput, RID_INPUT, inputData.data, &dwSize, sizeof(RAWINPUTHEADER))) {
    return;
  }
  RAWINPUT* raw = (RAWINPUT*)inputData.data;
  if(raw->header.dwType == RIM_TYPEKEYBOARD) {
    RAWKEYBOARD kb = raw->data.keyboard;
    bool bIsExtended = (kb.Flags & RI_KEY_E0) != 0;
    bool bIsE1 = (kb.Flags & RI_KEY_E1) != 0;
    bool isKeyUp = (kb.Flags & RI_KEY_BREAK) != 0;
    if(bIsE1) {
      return;
    }
    PKey pKey = InternalScancodeToPKey(kb.MakeCode, bIsExtended);
    if(pKey.keyCode == KEY_UNKNOWN) {
      return;
    }
    PEvent pKeyEvent;
    pKeyEvent.eventType = PE_INPUT_KEY;
    pKeyEvent.inputKey.keyCode = pKey.keyCode;
    pKeyEvent.inputKey.bState = !isKeyUp;
    PEventPush(pKeyEvent);
  } else if(raw->header.dwType == RIM_TYPEMOUSE) {
    RAWMOUSE mouse = raw->data.mouse;
    if((mouse.usFlags & MOUSE_MOVE_RELATIVE) && PWindowIsMouseCaptured()) {
      PEvent pMouseDetaEvent = {PE_INPUT_MOUSE_DELTA};
      pMouseDetaEvent.mouseDelta.deltaX = mouse.lLastX;
      pMouseDetaEvent.mouseDelta.deltaY = mouse.lLastY;
      PEventPush(pMouseDetaEvent);
    }
    if(mouse.usButtonFlags & RI_MOUSE_HWHEEL) {
      SHORT wheelDelta = (SHORT)mouse.usButtonData;
      PEvent pMouseWheel = {PE_INPUT_MOUSE_SCROLL};
      pMouseWheel.mouseScroll.scrollX = (float)wheelDelta / 120.f;
      pMouseWheel.mouseScroll.scrollY = 0.f;
      PEventPush(pMouseWheel);
    }
    if(mouse.usButtonFlags & RI_MOUSE_WHEEL) {
      SHORT wheelDelta = (SHORT)mouse.usButtonData;
      PEvent pMouseWheel = {PE_INPUT_MOUSE_SCROLL};
      pMouseWheel.mouseScroll.scrollX = 0.f;
      pMouseWheel.mouseScroll.scrollY = (float)wheelDelta / 120.f;
      PEventPush(pMouseWheel);
    }
    for(int32 i = 0; i < 3; ++i) {
      PKey pKey = InternalMouseCodeToPKey(buttons[i].code);
      int32 bPressed = -1;
      if(mouse.usButtonFlags & buttons[i].flagDown) {
        bPressed = 1;
      }
      if(mouse.usButtonFlags & buttons[i].flagUp) {
        bPressed = 0;
      }
      if(bPressed != -1 && pKey.keyCode != KEY_UNKNOWN) {
        PEvent pEvent = {PE_INPUT_KEY};
        pEvent.inputKey.keyCode = pKey.keyCode;
        pEvent.inputKey.bState = bPressed;
        PEventPush(pEvent);
      }
    }
  }
}

static PKeyScan InternalEKeyCodeToPKeyScan(EKeyCode KeyCode) {
  switch(KeyCode) {
    case KEY_ZERO: return (PKeyScan){0x0B, false};
    case KEY_ONE: return (PKeyScan){0x02, false};
    case KEY_TWO: return (PKeyScan){0x03, false};
    case KEY_THREE: return (PKeyScan){0x04, false};
    case KEY_FOUR: return (PKeyScan){0x05, false};
    case KEY_FIVE: return (PKeyScan){0x06, false};
    case KEY_SIX: return (PKeyScan){0x07, false};
    case KEY_SEVEN: return (PKeyScan){0x08, false};
    case KEY_EIGHT: return (PKeyScan){0x09, false};
    case KEY_NINE: return (PKeyScan){0x0A, false};
    case KEY_A: return (PKeyScan){0x1E, false};
    case KEY_B: return (PKeyScan){0x30, false};
    case KEY_C: return (PKeyScan){0x2E, false};
    case KEY_D: return (PKeyScan){0x20, false};
    case KEY_E: return (PKeyScan){0x12, false};
    case KEY_F: return (PKeyScan){0x21, false};
    case KEY_G: return (PKeyScan){0x22, false};
    case KEY_H: return (PKeyScan){0x23, false};
    case KEY_I: return (PKeyScan){0x17, false};
    case KEY_J: return (PKeyScan){0x24, false};
    case KEY_K: return (PKeyScan){0x25, false};
    case KEY_L: return (PKeyScan){0x26, false};
    case KEY_M: return (PKeyScan){0x32, false};
    case KEY_N: return (PKeyScan){0x31, false};
    case KEY_O: return (PKeyScan){0x18, false};
    case KEY_P: return (PKeyScan){0x19, false};
    case KEY_Q: return (PKeyScan){0x10, false};
    case KEY_R: return (PKeyScan){0x13, false};
    case KEY_S: return (PKeyScan){0x1F, false};
    case KEY_T: return (PKeyScan){0x14, false};
    case KEY_U: return (PKeyScan){0x16, false};
    case KEY_V: return (PKeyScan){0x2F, false};
    case KEY_W: return (PKeyScan){0x11, false};
    case KEY_X: return (PKeyScan){0x2D, false};
    case KEY_Y: return (PKeyScan){0x15, false};
    case KEY_Z: return (PKeyScan){0x2C, false};
    case KEY_ESCAPE: return (PKeyScan){0x01, false};
    case KEY_TAB: return (PKeyScan){0x0F, false};
    case KEY_BACKSPACE: return (PKeyScan){0x0E, false};
    case KEY_ENTER: return (PKeyScan){0x1C, false};
    case KEY_SPACE: return (PKeyScan){0x39, false};
    case KEY_F1: return (PKeyScan){0x3B, false};
    case KEY_F2: return (PKeyScan){0x3C, false};
    case KEY_F3: return (PKeyScan){0x3D, false};
    case KEY_F4: return (PKeyScan){0x3E, false};
    case KEY_F5: return (PKeyScan){0x3F, false};
    case KEY_F6: return (PKeyScan){0x40, false};
    case KEY_F7: return (PKeyScan){0x41, false};
    case KEY_F8: return (PKeyScan){0x42, false};
    case KEY_F9: return (PKeyScan){0x43, false};
    case KEY_F10: return (PKeyScan){0x44, false};
    case KEY_F11: return (PKeyScan){0x57, false};
    case KEY_F12: return (PKeyScan){0x58, false};
    case KEY_LEFT_SHIFT: return (PKeyScan){0x2A, false};
    case KEY_RIGHT_SHIFT: return (PKeyScan){0x36, false};
    case KEY_LEFT_CONTROL: return (PKeyScan){0x1D, false};
    case KEY_RIGHT_CONTROL: return (PKeyScan){0x1D, true};
    case KEY_LEFT_ALT: return (PKeyScan){0x38, false};
    case KEY_RIGHT_ALT: return (PKeyScan){0x38, true};
    case KEY_LEFT_SUPER: return (PKeyScan){0x5B, true};
    case KEY_RIGHT_SUPER: return (PKeyScan){0x5C, true};
    case KEY_KB_MENU: return (PKeyScan){0x5D, true};
    case KEY_CAPS_LOCK: return (PKeyScan){0x3A, false};
    case KEY_NUM_LOCK: return (PKeyScan){0x45, false};
    case KEY_SCROLL_LOCK: return (PKeyScan){0x46, false};
    case KEY_UP: return (PKeyScan){0x48, true};
    case KEY_DOWN: return (PKeyScan){0x50, true};
    case KEY_LEFT: return (PKeyScan){0x4B, true};
    case KEY_RIGHT: return (PKeyScan){0x4D, true};
    case KEY_HOME: return (PKeyScan){0x47, true};
    case KEY_END: return (PKeyScan){0x4F, true};
    case KEY_PAGE_UP: return (PKeyScan){0x49, true};
    case KEY_PAGE_DOWN: return (PKeyScan){0x51, true};
    case KEY_INSERT: return (PKeyScan){0x52, true};
    case KEY_DELETE: return (PKeyScan){0x53, true};
    case KEY_KP_0: return (PKeyScan){0x52, false};
    case KEY_KP_1: return (PKeyScan){0x4F, false};
    case KEY_KP_2: return (PKeyScan){0x50, false};
    case KEY_KP_3: return (PKeyScan){0x51, false};
    case KEY_KP_4: return (PKeyScan){0x4B, false};
    case KEY_KP_5: return (PKeyScan){0x4C, false};
    case KEY_KP_6: return (PKeyScan){0x4D, false};
    case KEY_KP_7: return (PKeyScan){0x47, false};
    case KEY_KP_8: return (PKeyScan){0x48, false};
    case KEY_KP_9: return (PKeyScan){0x49, false};
    case KEY_KP_DECIMAL: return (PKeyScan){0x53, false};
    case KEY_KP_DIVIDE: return (PKeyScan){0x35, true};
    case KEY_KP_MULTIPLY: return (PKeyScan){0x37, false};
    case KEY_KP_SUBTRACT: return (PKeyScan){0x4A, false};
    case KEY_KP_ADD: return (PKeyScan){0x4E, false};
    case KEY_KP_ENTER: return (PKeyScan){0x1C, true};
    case KEY_KP_EQUAL: return (PKeyScan){0x0D, false};
    default: return (PKeyScan){0, false};
  }
}

static cstring InternalVkCodeToStr(uint32 VkCode) {
  switch(VkCode) {
    case 'A': return "A";
    case 'B': return "B";
    case 'C': return "C";
    case 'D': return "D";
    case 'E': return "E";
    case 'F': return "F";
    case 'G': return "G";
    case 'H': return "H";
    case 'I': return "I";
    case 'J': return "J";
    case 'K': return "K";
    case 'L': return "L";
    case 'M': return "M";
    case 'N': return "N";
    case 'O': return "O";
    case 'P': return "P";
    case 'Q': return "Q";
    case 'R': return "R";
    case 'S': return "S";
    case 'T': return "T";
    case 'U': return "U";
    case 'V': return "V";
    case 'W': return "W";
    case 'X': return "X";
    case 'Y': return "Y";
    case 'Z': return "Z";
    default: return NULL;
  }
}

static PKey InternalMouseCodeToPKey(uint32 MouseCode) {
  switch(MouseCode) {
    case MOUSE_LEFT_CODE: return (PKey){KEY_MOUSE_LEFT, "Mouse Left", "Mouse Left"};
    case MOUSE_MIDDLE_CODE: return (PKey){KEY_MOUSE_MIDDLE, "Mouse Middler", "Mouse Middler"};
    case MOUSE_RIGHT_CODE: return (PKey){KEY_MOUSE_RIGHT, "Mouse Right", "Mouse Right"};
    default: return (PKey){KEY_UNKNOWN, "Unknown", "Unknown"};
  }
}

static PKey InternalScancodeToPKey(uint32 Scancode, bool bIsExtended) {
  if(bIsExtended) {
    switch(Scancode) {
      case 0x47: return (PKey){KEY_HOME, "Home", "Home"};
      case 0x48: return (PKey){KEY_UP, "Up Arrow", "Up Arrow"};
      case 0x49: return (PKey){KEY_PAGE_UP, "Page Up", "Page Up"};
      case 0x4B: return (PKey){KEY_LEFT, "Left Arrow", "Left Arrow"};
      case 0x4D: return (PKey){KEY_RIGHT, "Right Arrow", "Right Arrow"};
      case 0x4F: return (PKey){KEY_END, "End", "End"};
      case 0x50: return (PKey){KEY_DOWN, "Down Arrow", "Down Arrow"};
      case 0x51: return (PKey){KEY_PAGE_DOWN, "Page Down", "Page Down"};
      case 0x52: return (PKey){KEY_INSERT, "Insert", "Insert"};
      case 0x53: return (PKey){KEY_DELETE, "Delete", "Delete"};
      case 0x1C: return (PKey){KEY_KP_ENTER, "Numpad Enter", "Numpad Enter"};
      case 0x1D: return (PKey){KEY_RIGHT_CONTROL, "Right Control", "Right Control"};
      case 0x38: return (PKey){KEY_RIGHT_ALT, "Right Alt", "Right Alt"};
      case 0x5B: return (PKey){KEY_LEFT_SUPER, "Left Super", "Left Super"};
      case 0x5C: return (PKey){KEY_RIGHT_SUPER, "Right Super", "Right Super"};
      case 0x5D: return (PKey){KEY_KB_MENU, "Menu", "Menu"};
      case 0x37: return (PKey){KEY_PRINT_SCREEN, "Print Screen", "Print Screen"};
    }
    return (PKey){KEY_UNKNOWN, "Unknown", "Unknown"};
  }

  switch(Scancode) {
    case 0x1E: return (PKey){KEY_A, "A", "A"};
    case 0x30: return (PKey){KEY_B, "B", "B"};
    case 0x2E: return (PKey){KEY_C, "C", "C"};
    case 0x20: return (PKey){KEY_D, "D", "D"};
    case 0x12: return (PKey){KEY_E, "E", "E"};
    case 0x21: return (PKey){KEY_F, "F", "F"};
    case 0x22: return (PKey){KEY_G, "G", "G"};
    case 0x23: return (PKey){KEY_H, "H", "H"};
    case 0x17: return (PKey){KEY_I, "I", "I"};
    case 0x24: return (PKey){KEY_J, "J", "J"};
    case 0x25: return (PKey){KEY_K, "K", "K"};
    case 0x26: return (PKey){KEY_L, "L", "L"};
    case 0x32: return (PKey){KEY_M, "M", "M"};
    case 0x31: return (PKey){KEY_N, "N", "N"};
    case 0x18: return (PKey){KEY_O, "O", "O"};
    case 0x19: return (PKey){KEY_P, "P", "P"};
    case 0x10: return (PKey){KEY_Q, "Q", "Q"};
    case 0x13: return (PKey){KEY_R, "R", "R"};
    case 0x1F: return (PKey){KEY_S, "S", "S"};
    case 0x14: return (PKey){KEY_T, "T", "T"};
    case 0x16: return (PKey){KEY_U, "U", "U"};
    case 0x2F: return (PKey){KEY_V, "V", "V"};
    case 0x11: return (PKey){KEY_W, "W", "W"};
    case 0x2D: return (PKey){KEY_X, "X", "X"};
    case 0x15: return (PKey){KEY_Y, "Y", "Y"};
    case 0x2C: return (PKey){KEY_Z, "Z", "Z"};
    case 0x0B: return (PKey){KEY_ZERO, "0", "0"};
    case 0x02: return (PKey){KEY_ONE, "1", "1"};
    case 0x03: return (PKey){KEY_TWO, "2", "2"};
    case 0x04: return (PKey){KEY_THREE, "3", "3"};
    case 0x05: return (PKey){KEY_FOUR, "4", "4"};
    case 0x06: return (PKey){KEY_FIVE, "5", "5"};
    case 0x07: return (PKey){KEY_SIX, "6", "6"};
    case 0x08: return (PKey){KEY_SEVEN, "7", "7"};
    case 0x09: return (PKey){KEY_EIGHT, "8", "8"};
    case 0x0A: return (PKey){KEY_NINE, "9", "9"};
    case 0x3B: return (PKey){KEY_F1, "F1", "F1"};
    case 0x3C: return (PKey){KEY_F2, "F2", "F2"};
    case 0x3D: return (PKey){KEY_F3, "F3", "F3"};
    case 0x3E: return (PKey){KEY_F4, "F4", "F4"};
    case 0x3F: return (PKey){KEY_F5, "F5", "F5"};
    case 0x40: return (PKey){KEY_F6, "F6", "F6"};
    case 0x41: return (PKey){KEY_F7, "F7", "F7"};
    case 0x42: return (PKey){KEY_F8, "F8", "F8"};
    case 0x43: return (PKey){KEY_F9, "F9", "F9"};
    case 0x44: return (PKey){KEY_F10, "F10", "F10"};
    case 0x57: return (PKey){KEY_F11, "F11", "F11"};
    case 0x58: return (PKey){KEY_F12, "F12", "F12"};
    case 0x2A: return (PKey){KEY_LEFT_SHIFT, "Left Shift", "Left Shift"};
    case 0x36: return (PKey){KEY_RIGHT_SHIFT, "Right Shift", "Right Shift"};
    case 0x1D: return (PKey){KEY_LEFT_CONTROL, "Left Control", "Left Control"};
    case 0x38: return (PKey){KEY_LEFT_ALT, "Left Alt", "Left Alt"};
    case 0x01: return (PKey){KEY_ESCAPE, "Escape", "Escape"};
    case 0x0F: return (PKey){KEY_TAB, "Tab", "Tab"};
    case 0x0E: return (PKey){KEY_BACKSPACE, "Backspace", "Backspace"};
    case 0x1C: return (PKey){KEY_ENTER, "Enter", "Enter"};
    case 0x39: return (PKey){KEY_SPACE, "Space", "Space"};
    case 0x3A: return (PKey){KEY_CAPS_LOCK, "Caps Lock", "Caps Lock"};
    case 0x45: return (PKey){KEY_NUM_LOCK, "Num Lock", "Num Lock"};
    case 0x46: return (PKey){KEY_SCROLL_LOCK, "Scroll Lock", "Scroll Lock"};
    case 0x52: return (PKey){KEY_KP_0, "Numpad 0", "Numpad 0"};
    case 0x4F: return (PKey){KEY_KP_1, "Numpad 1", "Numpad 1"};
    case 0x50: return (PKey){KEY_KP_2, "Numpad 2", "Numpad 2"};
    case 0x51: return (PKey){KEY_KP_3, "Numpad 3", "Numpad 3"};
    case 0x4B: return (PKey){KEY_KP_4, "Numpad 4", "Numpad 4"};
    case 0x4C: return (PKey){KEY_KP_5, "Numpad 5", "Numpad 5"};
    case 0x4D: return (PKey){KEY_KP_6, "Numpad 6", "Numpad 6"};
    case 0x47: return (PKey){KEY_KP_7, "Numpad 7", "Numpad 7"};
    case 0x48: return (PKey){KEY_KP_8, "Numpad 8", "Numpad 8"};
    case 0x49: return (PKey){KEY_KP_9, "Numpad 9", "Numpad 9"};
    case 0x4A: return (PKey){KEY_KP_SUBTRACT, "Numpad -", "Numpad -"};
    case 0x4E: return (PKey){KEY_KP_ADD, "Numpad +", "Numpad +"};
    case 0x37: return (PKey){KEY_KP_MULTIPLY, "Numpad *", "Numpad *"};
    case 0x53: return (PKey){KEY_KP_DECIMAL, "Numpad .", "Numpad ."};
    case 0x0D: return (PKey){KEY_KP_EQUAL, "Numpad =", "Numpad ="};
  }
  return (PKey){KEY_UNKNOWN, "Unknown", "Unknown"};
}

#endif  // PLATFORM_WINDOWS
