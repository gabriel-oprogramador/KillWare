#pragma once
#include "Core/Defines.h"
#include "Core/EngineTypes.h"

GT_EXTERN_C FGT GEngine;

GT_EXTERN_C_BEGIN

static inline bool InputIsKeyPressed(EKeyCode KeyCode) {
  FInputState* curr = &GEngine.input.currentState;
  FInputState* prev = &GEngine.input.previousState;
  return (curr->keys[KeyCode] && !prev->keys[KeyCode]);
}
static inline bool InputIsKeyReleased(EKeyCode KeyCode) {
  FInputState* curr = &GEngine.input.currentState;
  FInputState* prev = &GEngine.input.previousState;
  return (!curr->keys[KeyCode] && prev->keys[KeyCode]);
}
static inline bool InputIsKeyRepeat(EKeyCode KeyCode) {
  FInputState* curr = &GEngine.input.currentState;
  FInputState* prev = &GEngine.input.previousState;
  return (curr->keys[KeyCode] && prev->keys[KeyCode]);
}

static inline FVector2 InputGetMousePos() {
  return GEngine.input.currentState.mousePos;
}

static inline FVector2 InputGetMouseDelta() {
  return GEngine.input.currentState.mouseDelta;
}

static inline FVector2 InputGetMouseScroll() {
  return GEngine.input.currentState.mouseScroll;
}

ENGINE_API bool InputIsMouseCaptured();
ENGINE_API void InputSetMouseCaptured(bool bCapture);

GT_EXTERN_C_END
