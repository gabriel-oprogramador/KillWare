#include "GameModule.h"
#include "Platform/Platform.h"

void GameQuit() {
  PWindowClose();
}

void GameSetFullscreen(bool bFullscreen) {
  PWindowSetFullscreen(bFullscreen);
}

void GameToggleFullscreen() {
  bool bFullscreen = PWindowIsFullscreen();
  PWindowSetFullscreen(!bFullscreen);
}

bool GameIsFullscreen() {
  return PWindowIsFullscreen();
}
