#ifdef PLATFORM_WEB
#include <emscripten.h>
#include <emscripten/html5.h>
#include <unistd.h>

#include "Platform/Platform.h"
#include "Platform/Events.h"
#include "Platform/ApiGL.h"
#include "Platform/Log.h"

#define CANVAS_ID         "#canvas"
#define TEXT_INPUT_ID     "#hidden-text"
#define GAME_CONTAINER_ID "#game-container"

static bool PointerlockCallback(int Type, const EmscriptenPointerlockChangeEvent* Event, void* UserData);
static bool FullscreenCallback(int Type, const EmscriptenFullscreenChangeEvent* Event, void* UserData);
static bool ResizeCallback(int Type, const EmscriptenUiEvent* Event, void* UserData);

static void InternalShowMouse(bool bShow);

extern uint32 PlatformInitialize(ETargetPlatform TargetPlatform, ETargetRenderer TargetRenderer, const char** Args);
extern void PlatformTerminate();
extern void PlatformUpdate();

// InputRaw.c
extern bool ApiInputRawInit(cstring CanvasID);

typedef struct PWindow {
  uint32 width;
  uint32 height;
  cstring title;
  bool bFocused;
  bool bFullscreen;
  bool bNeedsResize;
  bool bShouldClose;
  bool bMouseCaptured;
} PWindow;

static struct {
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
  EmscriptenWebGLContextAttributes attribs;
  PWindow* mainWindow;
  bool bMouseCaptured;
} SApiState;

static void MainLoop() {
  if(SApiState.mainWindow->bNeedsResize) {
    SApiState.mainWindow->bNeedsResize = false;
    PEvent pEvent = {PE_WINDOW_RESIZE};
    pEvent.windowResize.width = SApiState.mainWindow->width;
    pEvent.windowResize.height = SApiState.mainWindow->height;
    PEventPush(pEvent);
  }
  PlatformUpdate();
}

int main(int argc, const char** argv) {
  ApiInputRawInit(CANVAS_ID);
  emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, &PointerlockCallback);
  emscripten_set_fullscreenchange_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, &FullscreenCallback);
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, &ResizeCallback);

  emscripten_webgl_init_context_attributes(&SApiState.attribs);
  SApiState.attribs.majorVersion = 2;  // 2
  SApiState.attribs.minorVersion = 0;  // 0
  SApiState.attribs.depth = true;
  SApiState.attribs.alpha = false;
  SApiState.attribs.stencil = true;
  SApiState.attribs.antialias = true;
  SApiState.attribs.premultipliedAlpha = false;
  SApiState.context = emscripten_webgl_create_context(CANVAS_ID, &SApiState.attribs);
  if(SApiState.context == 0) {
    return false;
  }
  emscripten_webgl_make_context_current(SApiState.context);
  GT_INFO("API:EMS Context => WebGL:%d.%d", 2, 0);

  PlatformInitialize(TARGET_PLATFORM_WEB, TARGET_RENDERER_GL_ES_30, argv);

  emscripten_set_main_loop(&MainLoop, 0, true);
  emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
  return 0;
}

void PWindowInit(uint32 Width, uint32 Height, cstring Title) {
  EM_ASM_({ document.title = UTF8ToString($0); }, STR(GAME_NAME));
  uint32 width = EM_ASM_INT({ return window.innerWidth; });
  uint32 height = EM_ASM_INT({ return window.innerHeight; });
  emscripten_set_element_css_size(CANVAS_ID, width, height);
  emscripten_set_canvas_element_size(CANVAS_ID, width, height);

  SApiState.mainWindow = (PWindow*)PMemAlloc(sizeof(PWindow));
  SApiState.mainWindow->width = width;
  SApiState.mainWindow->height = height;
  SApiState.mainWindow->title = Title;
  SApiState.mainWindow->bFocused = false;
  SApiState.mainWindow->bFullscreen = false;
  SApiState.mainWindow->bNeedsResize = true;
  SApiState.mainWindow->bShouldClose = false;
}

void PWindowClose() {
}

bool PWindowIsVsync() {
}

void PWindowSetVsync(bool bEnable) {
}

bool PWindowIsMouseCaptured() {
  return SApiState.mainWindow->bMouseCaptured;
}

void PWindowSetMouseCaptured(bool bCapture) {
  if(SApiState.mainWindow->bMouseCaptured == bCapture) {
    return;
  }
  SApiState.mainWindow->bMouseCaptured = bCapture;
  if(bCapture) {
    emscripten_request_pointerlock(CANVAS_ID, false);
  } else {
    emscripten_exit_pointerlock();
  }
}

bool PWindowIsFullscreen() {
  return SApiState.mainWindow->bFullscreen;
}

void PWindowSetFullscreen(bool bFullscreen) {
  EMSCRIPTEN_RESULT result;
  if(bFullscreen) {
    result = emscripten_request_fullscreen(GAME_CONTAINER_ID, 1);
  } else {
    result = emscripten_exit_fullscreen();
  }
  if(result == EMSCRIPTEN_RESULT_SUCCESS) {
    SApiState.mainWindow->bFullscreen = bFullscreen;
  } else {
    GT_ERROR("Not Set Fullscreen:%d", result);
  }
}

void PWindowGetMousePos(int32* OutPosX, int32* OutPosY) {
}

void PWindowSetMousePos(int32 PosX, int PosY) {
}

// Emscripten Callbacks //====================================================================================//
static bool PointerlockCallback(int Type, const EmscriptenPointerlockChangeEvent* Event, void* UserData) {
  SApiState.bMouseCaptured = Event->isActive;
  InternalShowMouse(!Event->isActive);
  return true;
}

static bool FullscreenCallback(int Type, const EmscriptenFullscreenChangeEvent* Event, void* UserData) {
  SApiState.mainWindow->bFullscreen = Event->isFullscreen;
  return true;
}

static bool ResizeCallback(int Type, const EmscriptenUiEvent* Event, void* UserData) {
  PWindow* win = SApiState.mainWindow;
  win->width = EM_ASM_INT({ return window.innerWidth; });
  win->height = EM_ASM_INT({ return window.innerHeight; });
  emscripten_set_element_css_size(CANVAS_ID, win->width, win->height);
  emscripten_set_canvas_element_size(CANVAS_ID, win->width, win->height);
  PEvent pEvent = {PE_WINDOW_RESIZE};
  pEvent.windowResize.width = win->width;
  pEvent.windowResize.height = win->height;
  PEventPush(pEvent);
  return true;
}

// Internal Functions //======================================================================================//
static void InternalShowMouse(bool bShow) {
  EM_ASM(
      {
        var canvas = document.getElementById('canvas');
        if(canvas) {
          canvas.style.cursor = $0 ? 'default' : 'none';
        }
      },
      bShow);
}

// Log //=====================================================================================================//
extern void PlatformLog(ELogLevel Level, cstring FuncName, cstring Context, cstring Format, ...) {
  cstring logTag = "";
  cstring cssColor = "color: white;";
  char textBuffer[GT_LOG_BUFFER] = "";
  char finalBuffer[GT_LOG_BUFFER] = "";
  uint64 offset = 0;
  va_list args;

  if(!Format) {
    fprintf(stderr, "Log Format invalid\n");
    return;
  }

  // Define tag e cor CSS
  switch(Level) {
    case LOG_INFO:
      logTag = "[LOG INFO]";
      cssColor = "color: white;";
      break;
    case LOG_ALERT:
      logTag = "[LOG ALERT]";
      cssColor = "color: yellow;";
      break;
    case LOG_SUCCESS:
      logTag = "[LOG SUCCESS]";
      cssColor = "color: green;";
      break;
    case LOG_WARNING:
      logTag = "[LOG WARNING]";
      cssColor = "color: yellow;";
      break;
    case LOG_ERROR:
      logTag = "[LOG ERROR]";
      cssColor = "color: red;";
      break;
    case LOG_FATAL:
      logTag = "[LOG FATAL]";
      cssColor = "color: darkred; font-weight: bold;";
      break;
  }

  va_start(args, Format);
  vsnprintf(textBuffer, sizeof(textBuffer), Format, args);
  va_end(args);

  if(Level == LOG_INFO) {
    snprintf(finalBuffer, sizeof(finalBuffer), "%s => %s", logTag, textBuffer);
  } else if(Level == LOG_ALERT) {
    if(FuncName) {
      snprintf(finalBuffer, sizeof(finalBuffer), "%s %s() => %s", logTag, FuncName, textBuffer);
    } else {
      snprintf(finalBuffer, sizeof(finalBuffer), "%s => %s", logTag, textBuffer);
    }
  } else {
    if(FuncName) {
      offset = snprintf(finalBuffer, sizeof(finalBuffer), "%s %s() => ", logTag, FuncName);
    } else {
      offset = snprintf(finalBuffer, sizeof(finalBuffer), "%s => ", logTag);
    }

    if(Context) {
      snprintf(finalBuffer + offset, sizeof(finalBuffer) - offset, "%s -> %s", textBuffer, Context);
    } else {
      snprintf(finalBuffer + offset, sizeof(finalBuffer) - offset, "%s", textBuffer);
    }
  }

  EM_ASM_(
      {
        var msg = UTF8ToString($0);
        var style = UTF8ToString($1);
        console.log("%c%s", style, msg);
      },
      finalBuffer,
      cssColor);
}

#endif  // PLATFORM_WEB
