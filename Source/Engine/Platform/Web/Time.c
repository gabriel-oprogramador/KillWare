#ifdef PLATFORM_WEB

#include "emscripten.h"

double PGetTime() {
  return emscripten_get_now() / 1000.0;
}

void PWait(double Seconds) {
  emscripten_sleep((int)(Seconds * 1000.0));
}

#endif  // PLATFORM_WEB
