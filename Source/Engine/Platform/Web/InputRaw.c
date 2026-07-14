#ifdef PLATFORM_WEB
#include <emscripten/dom_pk_codes.h>
#include <emscripten/html5.h>
#include <emscripten.h>

#include "Platform/Platform.h"
#include "Platform/Events.h"
#include "Platform/Log.h"

#define DOM_BUTTON_LEFT     0
#define DOM_BUTTON_MIDDLE   1
#define DOM_BUTTON_RIGHT    2
#define DOM_BUTTON_FORWARD  6
#define DOM_BUTTON_BACKWARD 7

static void InputUpdateKeyMap();
static cstring InputDOMKeyToStr(cstring DomKey);
static PKey InputDomButtonToPKey(uint32 Button);
static PKey InputDOMKeyCodeToPKey(DOM_PK_CODE_TYPE DomKeyCode);
static uint32 InputEKeyCodeToDomKeyCode(EKeyCode KeyCode);

static bool KeybordInputCallback(int Type, const EmscriptenKeyboardEvent* Event, void* UserData);
static bool MouseInputCallback(int Type, const EmscriptenMouseEvent* Event, void* UserData);
static bool MouseWheelCallback(int Type, const EmscriptenWheelEvent* Event, void* UserData);

static struct {
  PKey keyMap[KEY_MAX];
} SApiState;

bool ApiInputRawInit(cstring CanvasID) {
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, true, &KeybordInputCallback);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, true, &KeybordInputCallback);
  emscripten_set_mousedown_callback(CanvasID, NULL, true, &MouseInputCallback);
  emscripten_set_mouseup_callback(CanvasID, NULL, true, &MouseInputCallback);
  emscripten_set_mousemove_callback(CanvasID, NULL, true, &MouseInputCallback);
  emscripten_set_wheel_callback(CanvasID, NULL, true, &MouseWheelCallback);
  return true;
}

static bool KeybordInputCallback(int Type, const EmscriptenKeyboardEvent* Event, void* UserData) {
  bool bPressed = (Type == EMSCRIPTEN_EVENT_KEYDOWN);
  DOM_PK_CODE_TYPE pkCode = emscripten_compute_dom_pk_code(Event->code);
  PKey pKey = InputDOMKeyCodeToPKey(pkCode);
  if(pKey.keyCode == KEY_UNKNOWN) {
    return false;
  }
  PEvent pInputKey = {PE_INPUT_KEY};
  pInputKey.inputKey.keyCode = pKey.keyCode;
  pInputKey.inputKey.bState = bPressed;
  PEventPush(pInputKey);
  return true;
}

static bool MouseInputCallback(int Type, const EmscriptenMouseEvent* Event, void* UserData) {
  const float MOUSE_SENSITIVITY = 0.2f;
  switch(Type) {
    case EMSCRIPTEN_EVENT_MOUSEUP:
    case EMSCRIPTEN_EVENT_MOUSEDOWN: {
      PEvent pButton = {PE_INPUT_KEY};
      PKey pKey = InputDomButtonToPKey(Event->button);
      if(pKey.keyCode == KEY_UNKNOWN) {
        return false;
      }
      pButton.inputKey.keyCode = pKey.keyCode;
      pButton.inputKey.bState = (Type == EMSCRIPTEN_EVENT_MOUSEDOWN);
      PEventPush(pButton);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE: {
      if(PWindowIsMouseCaptured()) {
        PEvent pDelta = {PE_INPUT_MOUSE_DELTA};
        pDelta.mouseDelta.deltaX = Event->movementX * MOUSE_SENSITIVITY;
        pDelta.mouseDelta.deltaY = Event->movementY * MOUSE_SENSITIVITY;
        PEventPush(pDelta);
      } else {
        PEvent pPos = {PE_INPUT_MOUSE_POS};
        pPos.mousePos.posX = Event->targetX;
        pPos.mousePos.posY = Event->targetY;
        PEventPush(pPos);
      }
      break;
    }
  }
  return false;
}

static bool MouseWheelCallback(int Type, const EmscriptenWheelEvent* Event, void* UserData) {
  PEvent pWheel = {PE_INPUT_MOUSE_SCROLL};
  pWheel.mouseScroll.scrollX = Event->deltaX * -1 / 120.f;
  pWheel.mouseScroll.scrollY = Event->deltaY * -1 / 120.f;
  PEventPush(pWheel);
  return true;
}

static void InputUpdateKeyMap() {
  for(uint32 c = 0; c < KEY_MAX; c++) {
    uint32 pk = InputEKeyCodeToDomKeyCode(c);
    if(pk == DOM_PK_UNKNOWN) {
      continue;
    }
    PKey key = InputDOMKeyCodeToPKey(pk);
    if(key.keyCode != KEY_UNKNOWN) {
      SApiState.keyMap[key.keyCode] = key;
    }
  }
  SApiState.keyMap[KEY_MOUSE_LEFT] = InputDomButtonToPKey(DOM_BUTTON_LEFT);
  SApiState.keyMap[KEY_MOUSE_MIDDLE] = InputDomButtonToPKey(DOM_BUTTON_MIDDLE);
  SApiState.keyMap[KEY_MOUSE_RIGHT] = InputDomButtonToPKey(DOM_BUTTON_RIGHT);
}

static cstring InputDOMKeyToStr(cstring DomKey) {
  if(!DomKey || DomKey[0] == '\0') {
    return NULL;
  }
  if(DomKey[1] == '\0') {
    char c = DomKey[0];
    if(c >= 'a' && c <= 'z') {
      static char upper[2] = {0};
      upper[0] = c - 32;
      return upper;
    }
    if((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
      static char result[2] = {0};
      result[0] = c;
      return result;
    }
    if(c == ' ') {
      return NULL;
    }
  }
  return NULL;
}

static PKey InputDomButtonToPKey(uint32 Button) {
  PKey key = {.keyCode = KEY_UNKNOWN, "Unknown", "Unknown"};
  switch(Button) {
    case DOM_BUTTON_LEFT: return (PKey){KEY_MOUSE_LEFT, "Left Mouse", "Left Mouse", true};
    case DOM_BUTTON_MIDDLE: return (PKey){KEY_MOUSE_MIDDLE, "Middle Mouse", "Middle Mouse", true};
    case DOM_BUTTON_RIGHT: return (PKey){KEY_MOUSE_RIGHT, "Right Mouse", "Right Mouse", true};
    default: return key;
  }
}

static PKey InputDOMKeyCodeToPKey(DOM_PK_CODE_TYPE DomKeyCode) {
  PKey key = {KEY_UNKNOWN, "Unknown", "Unknown"};
  switch(DomKeyCode) {
    case DOM_PK_0: key = (PKey){KEY_ZERO, "0", "0", false}; break;
    case DOM_PK_1: key = (PKey){KEY_ONE, "1", "1", false}; break;
    case DOM_PK_2: key = (PKey){KEY_TWO, "2", "2", false}; break;
    case DOM_PK_3: key = (PKey){KEY_THREE, "3", "3", false}; break;
    case DOM_PK_4: key = (PKey){KEY_FOUR, "4", "4", false}; break;
    case DOM_PK_5: key = (PKey){KEY_FIVE, "5", "5", false}; break;
    case DOM_PK_6: key = (PKey){KEY_SIX, "6", "6", false}; break;
    case DOM_PK_7: key = (PKey){KEY_SEVEN, "7", "7", false}; break;
    case DOM_PK_8: key = (PKey){KEY_EIGHT, "8", "8", false}; break;
    case DOM_PK_9: key = (PKey){KEY_NINE, "9", "9", false}; break;
    case DOM_PK_A: key = (PKey){KEY_A, "A", "A", false}; break;
    case DOM_PK_B: key = (PKey){KEY_B, "B", "B", false}; break;
    case DOM_PK_C: key = (PKey){KEY_C, "C", "C", false}; break;
    case DOM_PK_D: key = (PKey){KEY_D, "D", "D", false}; break;
    case DOM_PK_E: key = (PKey){KEY_E, "E", "E", false}; break;
    case DOM_PK_F: key = (PKey){KEY_F, "F", "F", false}; break;
    case DOM_PK_G: key = (PKey){KEY_G, "G", "G", false}; break;
    case DOM_PK_H: key = (PKey){KEY_H, "H", "H", false}; break;
    case DOM_PK_I: key = (PKey){KEY_I, "I", "I", false}; break;
    case DOM_PK_J: key = (PKey){KEY_J, "J", "J", false}; break;
    case DOM_PK_K: key = (PKey){KEY_K, "K", "K", false}; break;
    case DOM_PK_L: key = (PKey){KEY_L, "L", "L", false}; break;
    case DOM_PK_M: key = (PKey){KEY_M, "M", "M", false}; break;
    case DOM_PK_N: key = (PKey){KEY_N, "N", "N", false}; break;
    case DOM_PK_O: key = (PKey){KEY_O, "O", "O", false}; break;
    case DOM_PK_P: key = (PKey){KEY_P, "P", "P", false}; break;
    case DOM_PK_Q: key = (PKey){KEY_Q, "Q", "Q", false}; break;
    case DOM_PK_R: key = (PKey){KEY_R, "R", "R", false}; break;
    case DOM_PK_S: key = (PKey){KEY_S, "S", "S", false}; break;
    case DOM_PK_T: key = (PKey){KEY_T, "T", "T", false}; break;
    case DOM_PK_U: key = (PKey){KEY_U, "U", "U", false}; break;
    case DOM_PK_V: key = (PKey){KEY_V, "V", "V", false}; break;
    case DOM_PK_W: key = (PKey){KEY_W, "W", "W", false}; break;
    case DOM_PK_X: key = (PKey){KEY_X, "X", "X", false}; break;
    case DOM_PK_Y: key = (PKey){KEY_Y, "Y", "Y", false}; break;
    case DOM_PK_Z: key = (PKey){KEY_Z, "Z", "Z", false}; break;
    case DOM_PK_ESCAPE: key = (PKey){KEY_ESCAPE, "Escape", "Escape", true}; break;
    case DOM_PK_TAB: key = (PKey){KEY_TAB, "Tab", "Tab", true}; break;
    case DOM_PK_BACKSPACE: key = (PKey){KEY_BACKSPACE, "Backspace", "Backspace", true}; break;
    case DOM_PK_ENTER: key = (PKey){KEY_ENTER, "Enter", "Enter", true}; break;
    case DOM_PK_SPACE: key = (PKey){KEY_SPACE, "Space", "Space", false}; break;
    case DOM_PK_ARROW_UP: key = (PKey){KEY_UP, "Up Arrow", "Up Arrow", true}; break;
    case DOM_PK_ARROW_DOWN: key = (PKey){KEY_DOWN, "Down Arrow", "Down Arrow", true}; break;
    case DOM_PK_ARROW_LEFT: key = (PKey){KEY_LEFT, "Left Arrow", "Left Arrow", true}; break;
    case DOM_PK_ARROW_RIGHT: key = (PKey){KEY_RIGHT, "Right Arrow", "Right Arrow", true}; break;
    case DOM_PK_INSERT: key = (PKey){KEY_INSERT, "Insert", "Insert", true}; break;
    case DOM_PK_DELETE: key = (PKey){KEY_DELETE, "Delete", "Delete", true}; break;
    case DOM_PK_HOME: key = (PKey){KEY_HOME, "Home", "Home", true}; break;
    case DOM_PK_END: key = (PKey){KEY_END, "End", "End", true}; break;
    case DOM_PK_PAGE_UP: key = (PKey){KEY_PAGE_UP, "Page Up", "Page Up", true}; break;
    case DOM_PK_PAGE_DOWN: key = (PKey){KEY_PAGE_DOWN, "Page Down", "Page Down", true}; break;
    case DOM_PK_CAPS_LOCK: key = (PKey){KEY_CAPS_LOCK, "Caps Lock", "Caps Lock", true}; break;
    case DOM_PK_SCROLL_LOCK: key = (PKey){KEY_SCROLL_LOCK, "Scroll Lock", "Scroll Lock", true}; break;
    case DOM_PK_NUM_LOCK: key = (PKey){KEY_NUM_LOCK, "Num Lock", "Num Lock", true}; break;
    case DOM_PK_F1: key = (PKey){KEY_F1, "F1", "F1", true}; break;
    case DOM_PK_F2: key = (PKey){KEY_F2, "F2", "F2", true}; break;
    case DOM_PK_F3: key = (PKey){KEY_F3, "F3", "F3", true}; break;
    case DOM_PK_F4: key = (PKey){KEY_F4, "F4", "F4", true}; break;
    case DOM_PK_F5: key = (PKey){KEY_F5, "F5", "F5", true}; break;
    case DOM_PK_F6: key = (PKey){KEY_F6, "F6", "F6", true}; break;
    case DOM_PK_F7: key = (PKey){KEY_F7, "F7", "F7", true}; break;
    case DOM_PK_F8: key = (PKey){KEY_F8, "F8", "F8", true}; break;
    case DOM_PK_F9: key = (PKey){KEY_F9, "F9", "F9", true}; break;
    case DOM_PK_F10: key = (PKey){KEY_F10, "F10", "F10", true}; break;
    case DOM_PK_F11: key = (PKey){KEY_F11, "F11", "F11", true}; break;
    case DOM_PK_F12: key = (PKey){KEY_F12, "F12", "F12", true}; break;
    case DOM_PK_SHIFT_LEFT: key = (PKey){KEY_LEFT_SHIFT, "Left Shift", "Left Shift", true}; break;
    case DOM_PK_SHIFT_RIGHT: key = (PKey){KEY_RIGHT_SHIFT, "Right Shift", "Right Shift", true}; break;
    case DOM_PK_CONTROL_LEFT: key = (PKey){KEY_LEFT_CONTROL, "Left Control", "Left Control", true}; break;
    case DOM_PK_CONTROL_RIGHT: key = (PKey){KEY_RIGHT_CONTROL, "Right Control", "Right Control", true}; break;
    case DOM_PK_ALT_LEFT: key = (PKey){KEY_LEFT_ALT, "Left Alt", "Left Alt", true}; break;
    case DOM_PK_ALT_RIGHT: key = (PKey){KEY_RIGHT_ALT, "Right Alt", "Right Alt", true}; break;
    case DOM_PK_OS_LEFT: key = (PKey){KEY_LEFT_SUPER, "Left Super", "Left Super", true}; break;
    case DOM_PK_OS_RIGHT: key = (PKey){KEY_RIGHT_SUPER, "Right Super", "Right Super", true}; break;
    case DOM_PK_CONTEXT_MENU: key = (PKey){KEY_KB_MENU, "Menu", "Menu", true}; break;
    case DOM_PK_NUMPAD_0: key = (PKey){KEY_KP_0, "Numpad 0", "Numpad 0", false}; break;
    case DOM_PK_NUMPAD_1: key = (PKey){KEY_KP_1, "Numpad 1", "Numpad 1", false}; break;
    case DOM_PK_NUMPAD_2: key = (PKey){KEY_KP_2, "Numpad 2", "Numpad 2", false}; break;
    case DOM_PK_NUMPAD_3: key = (PKey){KEY_KP_3, "Numpad 3", "Numpad 3", false}; break;
    case DOM_PK_NUMPAD_4: key = (PKey){KEY_KP_4, "Numpad 4", "Numpad 4", false}; break;
    case DOM_PK_NUMPAD_5: key = (PKey){KEY_KP_5, "Numpad 5", "Numpad 5", false}; break;
    case DOM_PK_NUMPAD_6: key = (PKey){KEY_KP_6, "Numpad 6", "Numpad 6", false}; break;
    case DOM_PK_NUMPAD_7: key = (PKey){KEY_KP_7, "Numpad 7", "Numpad 7", false}; break;
    case DOM_PK_NUMPAD_8: key = (PKey){KEY_KP_8, "Numpad 8", "Numpad 8", false}; break;
    case DOM_PK_NUMPAD_9: key = (PKey){KEY_KP_9, "Numpad 9", "Numpad 9", false}; break;
    case DOM_PK_NUMPAD_DECIMAL: key = (PKey){KEY_KP_DECIMAL, "Numpad .", "Numpad .", false}; break;
    case DOM_PK_NUMPAD_DIVIDE: key = (PKey){KEY_KP_DIVIDE, "Numpad /", "Numpad /", false}; break;
    case DOM_PK_NUMPAD_MULTIPLY: key = (PKey){KEY_KP_MULTIPLY, "Numpad *", "Numpad *", false}; break;
    case DOM_PK_NUMPAD_SUBTRACT: key = (PKey){KEY_KP_SUBTRACT, "Numpad -", "Numpad -", false}; break;
    case DOM_PK_NUMPAD_ADD: key = (PKey){KEY_KP_ADD, "Numpad +", "Numpad +", false}; break;
    case DOM_PK_NUMPAD_ENTER: key = (PKey){KEY_KP_ENTER, "Numpad Enter", "Numpad Enter", true}; break;
    case DOM_PK_NUMPAD_EQUAL: key = (PKey){KEY_KP_EQUAL, "Numpad =", "Numpad =", false}; break;
    default: break;
  }
  return key;
}

static uint32 InputEKeyCodeToDomKeyCode(EKeyCode KeyCode) {
  switch(KeyCode) {
    case KEY_ZERO: return DOM_PK_0;
    case KEY_ONE: return DOM_PK_1;
    case KEY_TWO: return DOM_PK_2;
    case KEY_THREE: return DOM_PK_3;
    case KEY_FOUR: return DOM_PK_4;
    case KEY_FIVE: return DOM_PK_5;
    case KEY_SIX: return DOM_PK_6;
    case KEY_SEVEN: return DOM_PK_7;
    case KEY_EIGHT: return DOM_PK_8;
    case KEY_NINE: return DOM_PK_9;
    case KEY_A: return DOM_PK_A;
    case KEY_B: return DOM_PK_B;
    case KEY_C: return DOM_PK_C;
    case KEY_D: return DOM_PK_D;
    case KEY_E: return DOM_PK_E;
    case KEY_F: return DOM_PK_F;
    case KEY_G: return DOM_PK_G;
    case KEY_H: return DOM_PK_H;
    case KEY_I: return DOM_PK_I;
    case KEY_J: return DOM_PK_J;
    case KEY_K: return DOM_PK_K;
    case KEY_L: return DOM_PK_L;
    case KEY_M: return DOM_PK_M;
    case KEY_N: return DOM_PK_N;
    case KEY_O: return DOM_PK_O;
    case KEY_P: return DOM_PK_P;
    case KEY_Q: return DOM_PK_Q;
    case KEY_R: return DOM_PK_R;
    case KEY_S: return DOM_PK_S;
    case KEY_T: return DOM_PK_T;
    case KEY_U: return DOM_PK_U;
    case KEY_V: return DOM_PK_V;
    case KEY_W: return DOM_PK_W;
    case KEY_X: return DOM_PK_X;
    case KEY_Y: return DOM_PK_Y;
    case KEY_Z: return DOM_PK_Z;
    case KEY_ESCAPE: return DOM_PK_ESCAPE;
    case KEY_TAB: return DOM_PK_TAB;
    case KEY_BACKSPACE: return DOM_PK_BACKSPACE;
    case KEY_ENTER: return DOM_PK_ENTER;
    case KEY_SPACE: return DOM_PK_SPACE;
    case KEY_UP: return DOM_PK_ARROW_UP;
    case KEY_DOWN: return DOM_PK_ARROW_DOWN;
    case KEY_LEFT: return DOM_PK_ARROW_LEFT;
    case KEY_RIGHT: return DOM_PK_ARROW_RIGHT;
    case KEY_HOME: return DOM_PK_HOME;
    case KEY_END: return DOM_PK_END;
    case KEY_PAGE_UP: return DOM_PK_PAGE_UP;
    case KEY_PAGE_DOWN: return DOM_PK_PAGE_DOWN;
    case KEY_INSERT: return DOM_PK_INSERT;
    case KEY_DELETE: return DOM_PK_DELETE;
    case KEY_CAPS_LOCK: return DOM_PK_CAPS_LOCK;
    case KEY_SCROLL_LOCK: return DOM_PK_SCROLL_LOCK;
    case KEY_NUM_LOCK: return DOM_PK_NUM_LOCK;
    case KEY_PRINT_SCREEN: return DOM_PK_PRINT_SCREEN;
    case KEY_F1: return DOM_PK_F1;
    case KEY_F2: return DOM_PK_F2;
    case KEY_F3: return DOM_PK_F3;
    case KEY_F4: return DOM_PK_F4;
    case KEY_F5: return DOM_PK_F5;
    case KEY_F6: return DOM_PK_F6;
    case KEY_F7: return DOM_PK_F7;
    case KEY_F8: return DOM_PK_F8;
    case KEY_F9: return DOM_PK_F9;
    case KEY_F10: return DOM_PK_F10;
    case KEY_F11: return DOM_PK_F11;
    case KEY_F12: return DOM_PK_F12;
    case KEY_LEFT_SHIFT: return DOM_PK_SHIFT_LEFT;
    case KEY_RIGHT_SHIFT: return DOM_PK_SHIFT_RIGHT;
    case KEY_LEFT_CONTROL: return DOM_PK_CONTROL_LEFT;
    case KEY_RIGHT_CONTROL: return DOM_PK_CONTROL_RIGHT;
    case KEY_LEFT_ALT: return DOM_PK_ALT_LEFT;
    case KEY_RIGHT_ALT: return DOM_PK_ALT_RIGHT;
    case KEY_LEFT_SUPER: return DOM_PK_OS_LEFT;
    case KEY_RIGHT_SUPER: return DOM_PK_OS_RIGHT;
    case KEY_KB_MENU: return DOM_PK_CONTEXT_MENU;
    case KEY_KP_0: return DOM_PK_NUMPAD_0;
    case KEY_KP_1: return DOM_PK_NUMPAD_1;
    case KEY_KP_2: return DOM_PK_NUMPAD_2;
    case KEY_KP_3: return DOM_PK_NUMPAD_3;
    case KEY_KP_4: return DOM_PK_NUMPAD_4;
    case KEY_KP_5: return DOM_PK_NUMPAD_5;
    case KEY_KP_6: return DOM_PK_NUMPAD_6;
    case KEY_KP_7: return DOM_PK_NUMPAD_7;
    case KEY_KP_8: return DOM_PK_NUMPAD_8;
    case KEY_KP_9: return DOM_PK_NUMPAD_9;
    case KEY_KP_DECIMAL: return DOM_PK_NUMPAD_DECIMAL;
    case KEY_KP_DIVIDE: return DOM_PK_NUMPAD_DIVIDE;
    case KEY_KP_MULTIPLY: return DOM_PK_NUMPAD_MULTIPLY;
    case KEY_KP_SUBTRACT: return DOM_PK_NUMPAD_SUBTRACT;
    case KEY_KP_ADD: return DOM_PK_NUMPAD_ADD;
    case KEY_KP_ENTER: return DOM_PK_NUMPAD_ENTER;
    case KEY_KP_EQUAL: return DOM_PK_NUMPAD_EQUAL;
    default: return DOM_PK_UNKNOWN;
  }
}

#endif  // PLATFORM_WEB
