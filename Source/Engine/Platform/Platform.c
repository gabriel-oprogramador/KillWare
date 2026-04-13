#include "Platform.h"

extern uint32 EngineInitialize(ETargetPlatform TargetPlatform, ETargetRenderer TargetRenderer, const char** Args);
extern void EngineTerminate();
extern void EngineUpdate();

uint32 PlatformInitialize(ETargetPlatform TargetPlatform, ETargetRenderer TargetRenderer, const char** Args) {
  PWindowInit(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, STR(GAME_NAME));
  return EngineInitialize(TargetPlatform, TargetRenderer, NULL);
}

void PlatformTerminate() {
  EngineTerminate();
}

void PlatformUpdate() {
  EngineUpdate();
}
