#pragma once
#include "Core/CoreTypes.h"
#include "Core/Defines.h"

#define MAX_PLATFORM_EVENTS 1024

typedef enum {
  PE_EVENT_UNKNOW,        //
  PE_WINDOW_FOCUS,        //
  PE_WINDOW_RESIZE,       //
  PE_INPUT_KEY,           //
  PE_INPUT_KEY_MAP,       //
  PE_INPUT_MOUSE_POS,     //
  PE_INPUT_MOUSE_DELTA,   //
  PE_INPUT_MOUSE_SCROLL,  //
} EPlatformEventType;

typedef struct {
  EPlatformEventType eventType;
  union {
    struct {
      bool bFocused;
    } windowFocus;
    struct {
      uint32 width;
      uint32 height;
    } windowResize;
    struct {
      EKeyCode keyCode;
      bool bState;
    } inputKey;
    struct {
      int32 posX;
      int32 posY;
    } mousePos;
    struct {
      float deltaX;
      float deltaY;
    } mouseDelta;
    struct {
      float scrollX;
      float scrollY;
    } mouseScroll;
  };
} PEvent;

GT_EXTERN_C_BEGIN

ENGINE_API bool PEventPush(PEvent Event);
ENGINE_API bool PEventNext(PEvent* OutEvent);

GT_EXTERN_C_END
