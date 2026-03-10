// This file uses C++ only to test compatibility between C and C++.
#include "CoreMinimal.h"
#include "GameModule.h"

void GameStart() {
  GT_INFO("Game Start");
}

void TestInputKey(EKeyCode KeyCode, cstring Name) {
  if(InputIsKeyPressed(KeyCode)) {
    GT_ALERT("Pressed:%s", Name);
  }
  if(InputIsKeyReleased(KeyCode)) {
    GT_ALERT("Released:%s", Name);
  }
  if(InputIsKeyRepeat(KeyCode)) {
    GT_ALERT("Repeat:%s", Name);
  }
}

void GameUpdate(float DeltaTime) {
  if(InputIsKeyPressed(KEY_ESCAPE)) {
    GameQuit();
  }

  if(InputIsKeyPressed(KEY_ENTER)) {
    bool bCap = InputIsMouseCaptured();
    InputSetMouseCaptured(!bCap);
  }

  if(InputIsKeyPressed(KEY_F)) {
    GameToggleFullscreen();
  }

  if(InputIsKeyPressed(KEY_MOUSE_LEFT)) {
    FVector2 pos = InputGetMousePos();
    GT_ALERT("Mouse Pos:(X:%.2f, Y:%.f)", pos.x, pos.y);
  }

  float scroll = InputGetMouseScroll().y;
  if(scroll > 0) {
    GT_ALERT("Scroll Y Forward:%f", scroll);
  } else if(scroll < 0) {
    GT_ALERT("Scroll Y Backward:%f", scroll);
  }
}

void GameStop() {
  GT_INFO("Game Stop");
}
