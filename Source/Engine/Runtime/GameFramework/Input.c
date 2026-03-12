#include "Input.h"
#include "Platform/Platform.h"

bool InputIsMouseCaptured() {
  return PWindowIsMouseCaptured();
}

void InputSetMouseCaptured(bool bCapture) {
  PWindowSetMouseCaptured(bCapture);
}
