#ifdef PLATFORM_LINUX
#include <X11/extensions/XInput2.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/X.h>

#include "Platform/Platform.h"
#include "Platform/Events.h"

#define XBUTTON_MOUSE_LEFT         Button1
#define XBUTTON_MOUSE_MIDDLE       Button2
#define XBUTTON_MOUSE_RIGHT        Button3
#define XBUTTON_MOUSE_SCROLL_UP    4
#define XBUTTON_MOUSE_SCROLL_DOWN  5
#define XBUTTON_MOUSE_SCROLL_LEFT  6
#define XBUTTON_MOUSE_SCROLL_RIGHT 7

static void InputX2InitScrollAxes(Display* dpy, int32 deviceId);
static cstring InputXKeyCodeToStr(KeyCode XKeyCode);
static PKey InputXButtonToPKey(uint32 XButton);
static PKey InputXKeyCodeToPKey(KeyCode XKeyCode);
static uint32 InputEKeyCodeToXKeyCode(EKeyCode KeyCode);

static struct {
  PKey keyMap[KEY_MAX];
  Display* display;
  XkbStateRec xkbStateRec;
  int32 mouseRelX;
  int32 mouseRelY;
} SApiState;

bool ApiXInput2Init(Display* Display) {
  SApiState.display = Display;
  Window root = DefaultRootWindow(Display);
  unsigned char rawMask[XIMaskLen(XI_LASTEVENT)];
  memset(rawMask, 0, sizeof(rawMask));
  XIEventMask evRawMask;
  evRawMask.deviceid = XIAllDevices;
  evRawMask.mask_len = sizeof(rawMask);
  evRawMask.mask = rawMask;
  XISetMask(rawMask, XI_RawMotion);
  XISelectEvents(Display, root, &evRawMask, 1);

  int32 ndevices = 0;
  XIDeviceInfo* info = XIQueryDevice(Display, XIAllDevices, &ndevices);
  for(int c = 0; c < ndevices; c++) {
    XIDeviceInfo* dev = &info[c];
    if(dev->use == XIMasterPointer) {
      InputX2InitScrollAxes(Display, dev->deviceid);
      break;
    }
  }
  XIFreeDeviceInfo(info);
  return true;
}

void ApiXInput2UpdateKeyMap() {
  XkbGetState(SApiState.display, XkbUseCoreKbd, &SApiState.xkbStateRec);
  for(uint32 c = 0; c < KEY_MAX; c++) {
    KeyCode xKey = InputEKeyCodeToXKeyCode(c);
    PKey pKey = InputXKeyCodeToPKey(xKey);
    if(pKey.keyCode == KEY_UNKNOWN) {
      continue;
    }
    cstring name = InputXKeyCodeToStr(xKey);
    if(name) {
      pKey.layoutName = name;
    }
    SApiState.keyMap[pKey.keyCode] = pKey;
  }
  SApiState.keyMap[KEY_MOUSE_LEFT] = InputXButtonToPKey(XBUTTON_MOUSE_LEFT);
  SApiState.keyMap[KEY_MOUSE_MIDDLE] = InputXButtonToPKey(XBUTTON_MOUSE_MIDDLE);
  SApiState.keyMap[KEY_MOUSE_RIGHT] = InputXButtonToPKey(XBUTTON_MOUSE_RIGHT);
  PEvent pEvent = {PE_INPUT_KEY_MAP};
  pEvent.keymap.map = SApiState.keyMap;
  pEvent.keymap.count = KEY_MAX;
  PEventPush(pEvent);
}

void ApiXInput2PollCoreEvent(XEvent* Event) {
  switch(Event->type) {
    case KeyPress:
    case KeyRelease: {
      PKey pKey = InputXKeyCodeToPKey(Event->xkey.keycode);
      if(pKey.keyCode == KEY_UNKNOWN) {
        break;
      }
      PEvent pKeyEvent = {PE_INPUT_KEY};
      pKeyEvent.inputKey.keyCode = pKey.keyCode;
      pKeyEvent.inputKey.bState = (Event->type == KeyPress);
      PEventPush(pKeyEvent);
      break;
    }
    case ButtonPress:
    case ButtonRelease: {
      PKey pKey = InputXButtonToPKey(Event->xbutton.button);
      if(pKey.keyCode != KEY_UNKNOWN) {
        PEvent pButtonEvent = {PE_INPUT_KEY};
        pButtonEvent.inputKey.keyCode = pKey.keyCode;
        pButtonEvent.inputKey.bState = (Event->type == ButtonPress);
        PEventPush(pButtonEvent);
        break;
      }
      int32 scrollX = 0;
      int32 scrollY = 0;
      switch(Event->xbutton.button) {
        case XBUTTON_MOUSE_SCROLL_UP: scrollY = 1; break;
        case XBUTTON_MOUSE_SCROLL_DOWN: scrollY = -1; break;
        case XBUTTON_MOUSE_SCROLL_LEFT: scrollX = 1; break;
        case XBUTTON_MOUSE_SCROLL_RIGHT: scrollX = -1; break;
        default: break;
      }
      if(Event->type == ButtonPress && (scrollX != 0 || scrollY != 0)) {
        PEvent pScrollEvent = {PE_INPUT_MOUSE_SCROLL};
        pScrollEvent.mouseScroll.scrollX = (float)scrollX;
        pScrollEvent.mouseScroll.scrollY = (float)scrollY;
        PEventPush(pScrollEvent);
        break;
      }
    }
  }
}

void ApiXInput2PollRawEvent(XEvent* Event) {
  if(!XGetEventData(SApiState.display, &Event->xcookie)) {
    return;
  }
  XIRawEvent* raw = (XIRawEvent*)Event->xcookie.data;
  if(raw->evtype == XI_RawMotion) {
    double* values = raw->raw_values;
    int32 idx = 0;
    double deltaX = 0.0;
    double deltaY = 0.0;
    int32 max = raw->valuators.mask_len * 8;
    for(int32 c = 0; c < max; c++) {
      if(XIMaskIsSet(raw->valuators.mask, c)) {
        double v = values[idx++];
        if(c == SApiState.mouseRelX) {
          deltaX = v;
        }
        if(c == SApiState.mouseRelY) {
          deltaY = v;
        }
      }
    }
    if((deltaX || deltaY) && PWindowIsMouseCaptured()) {
      PEvent pEventDelta = {PE_INPUT_MOUSE_DELTA};
      pEventDelta.mouseDelta.deltaX = (float)deltaX;
      pEventDelta.mouseDelta.deltaY = (float)deltaY;
      PEventPush(pEventDelta);
    }
  }
  XFreeEventData(SApiState.display, &Event->xcookie);
}

static void InputX2InitScrollAxes(Display* Dpy, int32 DeviceId) {
  SApiState.mouseRelX = -1;
  SApiState.mouseRelY = -1;

  int32 ndevices = 0;
  XIDeviceInfo* info = XIQueryDevice(Dpy, DeviceId, &ndevices);
  for(int32 i = 0; i < ndevices; i++) {
    XIDeviceInfo* dev = &info[i];
    for(int32 j = 0; j < dev->num_classes; j++) {
      XIAnyClassInfo* cls = dev->classes[j];
      if(cls->type == XIValuatorClass) {
        XIValuatorClassInfo* valuatorInfo = (XIValuatorClassInfo*)cls;
        char* name = XGetAtomName(Dpy, valuatorInfo->label);
        if(!name) {
          continue;
        }
        if(strcmp(name, "Rel X") == 0) {
          SApiState.mouseRelX = valuatorInfo->number;
        }
        if(strcmp(name, "Rel Y") == 0) {
          SApiState.mouseRelY = valuatorInfo->number;
        }
        XFree(name);
      }
    }
  }
  XIFreeDeviceInfo(info);
}

static cstring InputXKeyCodeToStr(KeyCode XKeyCode) {
  KeySym sym = NoSymbol;
  if(XKeyCode >= 24 && XKeyCode <= 58) {
    for(uint32 level = 0; level < 4; level++) {
      sym = XkbKeycodeToKeysym(SApiState.display, XKeyCode, SApiState.xkbStateRec.group, level);
      if(sym == NoSymbol) {
        sym = XkbKeycodeToKeysym(SApiState.display, XKeyCode, 0, level);
      }
      if(sym != NoSymbol) {
        break;
      }
    }
    if(sym == NoSymbol) {
      return NULL;
    }
    switch(sym) {
      case XK_a:
      case XK_A: return "A";
      case XK_b:
      case XK_B: return "B";
      case XK_c:
      case XK_C: return "C";
      case XK_d:
      case XK_D: return "D";
      case XK_e:
      case XK_E: return "E";
      case XK_f:
      case XK_F: return "F";
      case XK_g:
      case XK_G: return "G";
      case XK_h:
      case XK_H: return "H";
      case XK_i:
      case XK_I: return "I";
      case XK_j:
      case XK_J: return "J";
      case XK_k:
      case XK_K: return "K";
      case XK_l:
      case XK_L: return "L";
      case XK_m:
      case XK_M: return "M";
      case XK_n:
      case XK_N: return "N";
      case XK_o:
      case XK_O: return "O";
      case XK_p:
      case XK_P: return "P";
      case XK_q:
      case XK_Q: return "Q";
      case XK_r:
      case XK_R: return "R";
      case XK_s:
      case XK_S: return "S";
      case XK_t:
      case XK_T: return "T";
      case XK_u:
      case XK_U: return "U";
      case XK_v:
      case XK_V: return "V";
      case XK_w:
      case XK_W: return "W";
      case XK_x:
      case XK_X: return "X";
      case XK_y:
      case XK_Y: return "Y";
      case XK_z:
      case XK_Z: return "Z";
    }
  }
  return NULL;
}

static PKey InputXButtonToPKey(uint32 XButton) {
  PKey key = {.keyCode = KEY_UNKNOWN, "Unknown", "Unknown"};
  switch(XButton) {
    case XBUTTON_MOUSE_LEFT: return (PKey){KEY_MOUSE_LEFT, "Left Mouse", "Left Mouse", true};
    case XBUTTON_MOUSE_MIDDLE: return (PKey){KEY_MOUSE_MIDDLE, "Middle Mouse", "Middle Mouse", true};
    case XBUTTON_MOUSE_RIGHT: return (PKey){KEY_MOUSE_RIGHT, "Right Mouse", "Right Mouse", true};
    default: return key;
  }
}

static PKey InputXKeyCodeToPKey(KeyCode XKeyCode) {
  PKey key = {KEY_UNKNOWN, "Unknown", "Unknown", false};
  switch(XKeyCode) {
    case 10: key = (PKey){KEY_ZERO, "0", "0", false}; break;
    case 11: key = (PKey){KEY_ONE, "1", "1", false}; break;
    case 12: key = (PKey){KEY_TWO, "2", "2", false}; break;
    case 13: key = (PKey){KEY_THREE, "3", "3", false}; break;
    case 14: key = (PKey){KEY_FOUR, "4", "4", false}; break;
    case 15: key = (PKey){KEY_FIVE, "5", "5", false}; break;
    case 16: key = (PKey){KEY_SIX, "6", "6", false}; break;
    case 17: key = (PKey){KEY_SEVEN, "7", "7", false}; break;
    case 18: key = (PKey){KEY_EIGHT, "8", "8", false}; break;
    case 19: key = (PKey){KEY_NINE, "9", "9", false}; break;
    case 38: key = (PKey){KEY_A, "A", "A", false}; break;
    case 56: key = (PKey){KEY_B, "B", "B", false}; break;
    case 54: key = (PKey){KEY_C, "C", "C", false}; break;
    case 40: key = (PKey){KEY_D, "D", "D", false}; break;
    case 26: key = (PKey){KEY_E, "E", "E", false}; break;
    case 41: key = (PKey){KEY_F, "F", "F", false}; break;
    case 42: key = (PKey){KEY_G, "G", "G", false}; break;
    case 43: key = (PKey){KEY_H, "H", "H", false}; break;
    case 31: key = (PKey){KEY_I, "I", "I", false}; break;
    case 44: key = (PKey){KEY_J, "J", "J", false}; break;
    case 45: key = (PKey){KEY_K, "K", "K", false}; break;
    case 46: key = (PKey){KEY_L, "L", "L", false}; break;
    case 58: key = (PKey){KEY_M, "M", "M", false}; break;
    case 57: key = (PKey){KEY_N, "N", "N", false}; break;
    case 32: key = (PKey){KEY_O, "O", "O", false}; break;
    case 33: key = (PKey){KEY_P, "P", "P", false}; break;
    case 24: key = (PKey){KEY_Q, "Q", "Q", false}; break;
    case 27: key = (PKey){KEY_R, "R", "R", false}; break;
    case 39: key = (PKey){KEY_S, "S", "S", false}; break;
    case 28: key = (PKey){KEY_T, "T", "T", false}; break;
    case 30: key = (PKey){KEY_U, "U", "U", false}; break;
    case 55: key = (PKey){KEY_V, "V", "V", false}; break;
    case 25: key = (PKey){KEY_W, "W", "W", false}; break;
    case 53: key = (PKey){KEY_X, "X", "X", false}; break;
    case 29: key = (PKey){KEY_Y, "Y", "Y", false}; break;
    case 52: key = (PKey){KEY_Z, "Z", "Z", false}; break;
    case 9: key = (PKey){KEY_ESCAPE, "Escape", "Escape", true}; break;
    case 23: key = (PKey){KEY_TAB, "Tab", "Tab", true}; break;
    case 22: key = (PKey){KEY_BACKSPACE, "Backspace", "Backspace", true}; break;
    case 36: key = (PKey){KEY_ENTER, "Enter", "Enter", true}; break;
    case 65: key = (PKey){KEY_SPACE, "Space", "Space", false}; break;
    case 111: key = (PKey){KEY_UP, "Up Arrow", "Up Arrow", true}; break;
    case 116: key = (PKey){KEY_DOWN, "Down Arrow", "Down Arrow", true}; break;
    case 113: key = (PKey){KEY_LEFT, "Left Arrow", "Left Arrow", true}; break;
    case 114: key = (PKey){KEY_RIGHT, "Right Arrow", "Right Arrow", true}; break;
    case 118: key = (PKey){KEY_INSERT, "Insert", "Insert", true}; break;
    case 119: key = (PKey){KEY_DELETE, "Delete", "Delete", true}; break;
    case 110: key = (PKey){KEY_HOME, "Home", "Home", true}; break;
    case 115: key = (PKey){KEY_END, "End", "End", true}; break;
    case 112: key = (PKey){KEY_PAGE_UP, "Page Up", "Page Up", true}; break;
    case 117: key = (PKey){KEY_PAGE_DOWN, "Page Down", "Page Down", true}; break;
    case 67: key = (PKey){KEY_F1, "F1", "F1", true}; break;
    case 68: key = (PKey){KEY_F2, "F2", "F2", true}; break;
    case 69: key = (PKey){KEY_F3, "F3", "F3", true}; break;
    case 70: key = (PKey){KEY_F4, "F4", "F4", true}; break;
    case 71: key = (PKey){KEY_F5, "F5", "F5", true}; break;
    case 72: key = (PKey){KEY_F6, "F6", "F6", true}; break;
    case 73: key = (PKey){KEY_F7, "F7", "F7", true}; break;
    case 74: key = (PKey){KEY_F8, "F8", "F8", true}; break;
    case 75: key = (PKey){KEY_F9, "F9", "F9", true}; break;
    case 76: key = (PKey){KEY_F10, "F10", "F10", true}; break;
    case 95: key = (PKey){KEY_F11, "F11", "F11", true}; break;
    case 96: key = (PKey){KEY_F12, "F12", "F12", true}; break;
    case 50: key = (PKey){KEY_LEFT_SHIFT, "Left Shift", "Left Shift", true}; break;
    case 62: key = (PKey){KEY_RIGHT_SHIFT, "Right Shift", "Right Shift", true}; break;
    case 37: key = (PKey){KEY_LEFT_CONTROL, "Left Control", "Left Control", true}; break;
    case 105: key = (PKey){KEY_RIGHT_CONTROL, "Right Control", "Right Control", true}; break;
    case 64: key = (PKey){KEY_LEFT_ALT, "Left Alt", "Left Alt", true}; break;
    case 108: key = (PKey){KEY_RIGHT_ALT, "Right Alt", "Right Alt", true}; break;
    case 133: key = (PKey){KEY_LEFT_SUPER, "Left Super", "Left Super", true}; break;
    case 134: key = (PKey){KEY_RIGHT_SUPER, "Right Super", "Right Super", true}; break;
    case 135: key = (PKey){KEY_KB_MENU, "Menu", "Menu", true}; break;
    case 90: key = (PKey){KEY_KP_0, "Numpad 0", "Numpad 0", false}; break;
    case 87: key = (PKey){KEY_KP_1, "Numpad 1", "Numpad 1", false}; break;
    case 88: key = (PKey){KEY_KP_2, "Numpad 2", "Numpad 2", false}; break;
    case 89: key = (PKey){KEY_KP_3, "Numpad 3", "Numpad 3", false}; break;
    case 83: key = (PKey){KEY_KP_4, "Numpad 4", "Numpad 4", false}; break;
    case 84: key = (PKey){KEY_KP_5, "Numpad 5", "Numpad 5", false}; break;
    case 85: key = (PKey){KEY_KP_6, "Numpad 6", "Numpad 6", false}; break;
    case 79: key = (PKey){KEY_KP_7, "Numpad 7", "Numpad 7", false}; break;
    case 80: key = (PKey){KEY_KP_8, "Numpad 8", "Numpad 8", false}; break;
    case 81: key = (PKey){KEY_KP_9, "Numpad 9", "Numpad 9", false}; break;
    case 91: key = (PKey){KEY_KP_DECIMAL, "Numpad .", "Numpad .", false}; break;
    case 106: key = (PKey){KEY_KP_DIVIDE, "Numpad /", "Numpad /", false}; break;
    case 63: key = (PKey){KEY_KP_MULTIPLY, "Numpad *", "Numpad *", false}; break;
    case 82: key = (PKey){KEY_KP_SUBTRACT, "Numpad -", "Numpad -", false}; break;
    case 86: key = (PKey){KEY_KP_ADD, "Numpad +", "Numpad +", false}; break;
    case 104: key = (PKey){KEY_KP_ENTER, "Numpad Enter", "Numpad Enter", true}; break;
    case 125: key = (PKey){KEY_KP_EQUAL, "Numpad =", "Numpad =", false}; break;
    default: break;
  }
  return key;
}

static uint32 InputEKeyCodeToXKeyCode(EKeyCode KeyCode) {
  switch(KeyCode) {
    case KEY_ZERO: return 10;
    case KEY_ONE: return 11;
    case KEY_TWO: return 12;
    case KEY_THREE: return 13;
    case KEY_FOUR: return 14;
    case KEY_FIVE: return 15;
    case KEY_SIX: return 16;
    case KEY_SEVEN: return 17;
    case KEY_EIGHT: return 18;
    case KEY_NINE: return 19;
    case KEY_A: return 38;
    case KEY_B: return 56;
    case KEY_C: return 54;
    case KEY_D: return 40;
    case KEY_E: return 26;
    case KEY_F: return 41;
    case KEY_G: return 42;
    case KEY_H: return 43;
    case KEY_I: return 31;
    case KEY_J: return 44;
    case KEY_K: return 45;
    case KEY_L: return 46;
    case KEY_M: return 58;
    case KEY_N: return 57;
    case KEY_O: return 32;
    case KEY_P: return 33;
    case KEY_Q: return 24;
    case KEY_R: return 27;
    case KEY_S: return 39;
    case KEY_T: return 28;
    case KEY_U: return 30;
    case KEY_V: return 55;
    case KEY_W: return 25;
    case KEY_X: return 53;
    case KEY_Y: return 29;
    case KEY_Z: return 52;
    case KEY_ESCAPE: return 9;
    case KEY_TAB: return 23;
    case KEY_BACKSPACE: return 22;
    case KEY_ENTER: return 36;
    case KEY_SPACE: return 65;
    case KEY_UP: return 111;
    case KEY_DOWN: return 116;
    case KEY_LEFT: return 113;
    case KEY_RIGHT: return 114;
    case KEY_INSERT: return 118;
    case KEY_DELETE: return 119;
    case KEY_HOME: return 110;
    case KEY_END: return 115;
    case KEY_PAGE_UP: return 112;
    case KEY_PAGE_DOWN: return 117;
    case KEY_F1: return 67;
    case KEY_F2: return 68;
    case KEY_F3: return 69;
    case KEY_F4: return 70;
    case KEY_F5: return 71;
    case KEY_F6: return 72;
    case KEY_F7: return 73;
    case KEY_F8: return 74;
    case KEY_F9: return 75;
    case KEY_F10: return 76;
    case KEY_F11: return 95;
    case KEY_F12: return 96;
    case KEY_LEFT_SHIFT: return 50;
    case KEY_RIGHT_SHIFT: return 62;
    case KEY_LEFT_CONTROL: return 37;
    case KEY_RIGHT_CONTROL: return 105;
    case KEY_LEFT_ALT: return 64;
    case KEY_RIGHT_ALT: return 108;
    case KEY_LEFT_SUPER: return 133;
    case KEY_RIGHT_SUPER: return 134;
    case KEY_KB_MENU: return 135;
    case KEY_KP_0: return 90;
    case KEY_KP_1: return 87;
    case KEY_KP_2: return 88;
    case KEY_KP_3: return 89;
    case KEY_KP_4: return 83;
    case KEY_KP_5: return 84;
    case KEY_KP_6: return 85;
    case KEY_KP_7: return 79;
    case KEY_KP_8: return 80;
    case KEY_KP_9: return 81;
    case KEY_KP_DECIMAL: return 91;
    case KEY_KP_DIVIDE: return 106;
    case KEY_KP_MULTIPLY: return 63;
    case KEY_KP_SUBTRACT: return 82;
    case KEY_KP_ADD: return 86;
    case KEY_KP_ENTER: return 104;
    case KEY_KP_EQUAL: return 125;
    default: return KEY_UNKNOWN;
  }
}

#endif  // PLATFORM_LINUX
