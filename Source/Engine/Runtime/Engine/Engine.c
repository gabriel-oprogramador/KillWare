#include "Engine.h"
#include "Renderer/Render.h"
#include "Platform/Log.h"
#include "Platform/Platform.h"
#include "Platform/Events.h"

FGT GEngine;

extern void GameStart();
extern void GameUpdate(float DeltaTime);
extern void GameStop();
extern void RenderInitialize();
extern void RenderTerminate();
extern void RenderUpdate(float DeltaTime);

static void InternalProcessPlatformEvents(float DeltaTime);

uint32 EngineInitialize(ETargetPlatform TargetPlatform, ETargetRenderer TargetRenderer, const char** Args) {
  GEngine.time.startTime = PGetTime();
  GEngine.time.deltaTime = 0.f;
  GEngine.time.framerate = 0;
  GEngine.info.targetPlatform = TargetPlatform;
  GEngine.info.targetRenderer = TargetRenderer;

  RenderInitialize();
  GT_INFO("Engine Initialized");
  GameStart();
  return 0;
}

void EngineTerminate() {
  GameStop();
  RenderTerminate();
  GT_INFO("Engine Terminated");
}

void EngineUpdate() {
  static double lastTime = 0.f;
  static double smoothFPS = 60.0;
  double alpha = 0.1;
  double current = PGetTime();
  double deltaTime = current - lastTime;
  lastTime = current;

  double fps = 1.0 / deltaTime;
  smoothFPS = alpha * fps + (1.0 - alpha) * smoothFPS;

  GEngine.time.deltaTime = deltaTime;
  GEngine.time.framerate = smoothFPS;
  PMemCopy(&GEngine.input.currentState, &GEngine.input.previousState, sizeof(GEngine.input.previousState));
  GEngine.input.currentState.mouseScroll = (FVector2){0, 0};
  GEngine.input.currentState.mouseDelta = (FVector2){0, 0};

  /*GT_ALERT("FPS:%u, ms:%.4f", (uint32)smoothFPS, deltaTime);*/

  InternalProcessPlatformEvents((float)deltaTime);
  RenderClear();
  GameUpdate((float)deltaTime);
  RenderUpdate((float)deltaTime);
}

static void InternalProcessPlatformEvents(float DeltaTime) {
  PEvent event;
  while(PEventNext(&event)) {
    switch(event.eventType) {
      case PE_EVENT_UNKNOW: {
        // Evento desconhecido, ignora
        break;
      }
      case PE_WINDOW_FOCUS: {
        GT_ALERT("Window Focus: %s", event.windowFocus.bFocused ? "Gain" : "Lost");
        break;
      }
      case PE_WINDOW_RESIZE: {
        FViewport viewport;
        viewport.posX = viewport.posY = 0;
        viewport.width = event.windowResize.width;
        viewport.height = event.windowResize.height;
        GEngine.windowInfo.width = event.windowResize.width;
        GEngine.windowInfo.height = event.windowResize.height;
        GEngine.windowInfo.aspect = (float)viewport.width / (float)viewport.height;
        RenderSetViewport(viewport);
        GT_ALERT("Window Resize: %u x %u", event.windowResize.width, event.windowResize.height);
        break;
      }
      case PE_INPUT_KEY_MAP: {
        // Aqui você poderia atualizar o layout do teclado, se necessário
        /*for(uint32 c = 0; c < event.keymap.count; c++) {  // C = KEY_**/
        /*GT_ALERT("Physic:%s, Layout:%s", event.keymap.map[c].physicalName, event.keymap.map[c].layoutName);*/
        /*}*/
        GT_ALERT("Update Key Mapping");
        break;
      }
      case PE_INPUT_KEY: {
        GEngine.input.currentState.keys[event.inputKey.keyCode] = event.inputKey.bState;
        break;
      }
      case PE_INPUT_MOUSE_POS: {
        GEngine.input.currentState.mousePos.x = event.mousePos.posX;
        GEngine.input.currentState.mousePos.y = event.mousePos.posY;
        break;
      }
      case PE_INPUT_MOUSE_DELTA: {
        GEngine.input.currentState.mouseDelta.x += event.mouseDelta.deltaX;
        GEngine.input.currentState.mouseDelta.y += event.mouseDelta.deltaY;
        break;
      }
      case PE_INPUT_MOUSE_SCROLL: {
        GEngine.input.currentState.mouseScroll.x += event.mouseScroll.scrollX;
        GEngine.input.currentState.mouseScroll.y += event.mouseScroll.scrollY;
        break;
      }
      default: {
        break;
      }
    }
  }
}
