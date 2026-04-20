#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <windowsx.h>

#include "Platform/Platform.h"
#include "Platform/Log.h"
#include "Platform/Events.h"
#include "GL/glcorearb.h"
#include "GL/wglext.h"

#define WINDOW_CLOSE_CODE WM_USER + 1
#define IDI_APP_ICON      100

extern bool ApiWindowsLoadGLFunctions();
extern uint32 PlatformInitialize(ETargetPlatform TargetPlatform, ETargetRenderer TargetRenderer, const char** Args);
extern void PlatformTerminate();
extern void PlatformUpdate();

// InputRaw.c
extern bool ApiRawInputInit(HWND hWindow);
extern void ApiRawInputUpdateKeyMap();
extern void ApiRawInputPollEvent(HRAWINPUT RawInput);

typedef struct {
  HDC hDevice;
  HWND hWindow;
  HGLRC hContext;
  cstring title;
  uint32 width;
  uint32 height;
  int32 lastMousePosX;
  int32 lastMousePosY;
  int32 style;
  bool bShow;
  bool bFullscreen;
  bool bShouldClose;
  bool bMouseCaptured;
  bool bNeedsResize;
  WINDOWPLACEMENT placement;
} PWindow;

typedef struct {
  char userDataPath[GT_MAX_FULLPATH];
  PWindow* mainWindow;
  int32 consoleDefaultAttribute;
  HANDLE hConsole;
  bool bMouseCaptured;
  bool bVsync;
  PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
} FPlatformState;

// Internal Functions //=====================================================================================//
static PWindow* InternalCreateWindow(uint32 Width, uint32 Height, cstring Title);
static void InternalDestroyWindow(PWindow* Self);
static void InternalUpdateWindow(PWindow* Self);
static LRESULT InternalWinProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
static void InternalCaptureMouse(PWindow* Window, bool bCapture);

static FPlatformState SPS;  // Static Platform State

GT_AUTO_EXEC(InitConsole) {
  SPS.hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  GetConsoleScreenBufferInfo(SPS.hConsole, &consoleInfo);
  SPS.consoleDefaultAttribute = consoleInfo.wAttributes;
}

// Entrypoint //=============================================================================================//
int32 NativeMain(int argc, const char** argv) {
  {
    char tempBuff[GT_MAX_PATH] = "";
    GetEnvironmentVariableA("LOCALAPPDATA", tempBuff, sizeof(SPS.userDataPath));
    snprintf(SPS.userDataPath, sizeof(SPS.userDataPath), "%s\\%s", tempBuff, STR(GAME_NAME));
  }

  if(PlatformInitialize(TARGET_PLATFORM_WINDOWS, TARGET_RENDERER_GL_CORE_33, argv)) {
    return 0;
  }

  while(!SPS.mainWindow->bShouldClose) {
    InternalUpdateWindow(SPS.mainWindow);
    PlatformUpdate();
    SwapBuffers(SPS.mainWindow->hDevice);
  }
  PlatformTerminate();
  InternalDestroyWindow(SPS.mainWindow);
  return 0;
}

#ifdef DEVELOPMENT_MODE
int main(int argc, const char** argv) {
  char buffer[GT_MAX_PATH];
  snprintf(buffer, sizeof(buffer), "%s", STR(PROJECT_PATH));
  SetCurrentDirectory(buffer);
  return NativeMain(argc, argv);
}
#elif SHIPPING_MODE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
  char buffer[GT_MAX_PATH];
  GetModuleFileNameA(NULL, buffer, GT_MAX_PATH);
  char* lastSlash = strrchr(buffer, '\\');
  if(lastSlash) {
    *lastSlash = '\0';
    SetCurrentDirectory(buffer);
  }
  return NativeMain(0, NULL);
}
#endif

cstring PGetUserDataPath() {
  return SPS.userDataPath;
}

// Window Api //=============================================================================================//
void PWindowInit(uint32 Width, uint32 Height, cstring Title) {
  const uint32 glMajor = 3;
  const uint32 glMinor = 3;
  const BYTE colorBits = 32;
  const BYTE depthBits = 24;
  const BYTE stencilBits = 8;

  PWindow* legacyWindow = InternalCreateWindow(0, 0, "LegacyWindow");
  HDC legacyDC = legacyWindow->hDevice;

  PIXELFORMATDESCRIPTOR pfd;
  PMemSet(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = colorBits;
  pfd.cDepthBits = depthBits;
  pfd.cStencilBits = stencilBits;
  pfd.iLayerType = PFD_MAIN_PLANE;

  int32 legacyPF = ChoosePixelFormat(legacyDC, &pfd);
  if(!legacyPF) {
    GT_FATAL("API:WIN32 Not Choose Pixel Format");
    InternalDestroyWindow(legacyWindow);
    return;
  }

  if(!SetPixelFormat(legacyDC, legacyPF, &pfd)) {
    GT_FATAL("API:WIN32 Not Set Pixel Format");
    InternalDestroyWindow(legacyWindow);
    return;
  }

  HGLRC legacyContext = wglCreateContext(legacyDC);
  if(legacyContext == NULL) {
    GT_FATAL("API:WIN32 Not Create Context");
    InternalDestroyWindow(legacyWindow);
    return;
  }

  if(!wglMakeCurrent(legacyDC, legacyContext)) {
    GT_FATAL("API:WIN32 Not make current context");
    InternalDestroyWindow(legacyWindow);
    return;
  }

  PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
  PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
  GT_ASSERT(wglChoosePixelFormatARB && wglCreateContextAttribsARB);

  wglMakeCurrent(0, 0);
  wglDeleteContext(legacyContext);
  InternalDestroyWindow(legacyWindow);

  // clang-format off
  int32 pixelFormatAttribs[] = {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
    WGL_COLOR_BITS_ARB, colorBits,
    WGL_STENCIL_BITS_ARB, stencilBits,
    WGL_DEPTH_BITS_ARB, depthBits,
    0
  };


uint32 glFlags = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
#ifdef DEVELOPMENT_MODE
  glFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif// DEVELOPMENT_MODE

  int32 contextAttribs[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, glMajor,
    WGL_CONTEXT_MINOR_VERSION_ARB, glMinor,
    WGL_CONTEXT_FLAGS_ARB, glFlags,
    0
  };
  // clang-format on

  PWindow* pWindow = InternalCreateWindow(Width, Height, Title);
  if(pWindow == NULL) {
    GT_FATAL("API:WIN32 Failed to create window");
    return;
  }
  ShowWindow(pWindow->hWindow, SW_SHOWNORMAL);
  UpdateWindow(pWindow->hWindow);

  int32 pixelFormat = 0;
  uint32 numPixelFormat = 0;
  wglChoosePixelFormatARB(pWindow->hDevice, pixelFormatAttribs, NULL, 1, &pixelFormat, (UINT*)&numPixelFormat);
  if(numPixelFormat <= 0) {
    GT_FATAL("API:WIN32 Not valid modern Pixel Format");
    InternalDestroyWindow(pWindow);
    return;
  }

  if(!SetPixelFormat(pWindow->hDevice, pixelFormat, &pfd)) {
    GT_FATAL("API:WIN32 Not set modern Pixel Format");
    InternalDestroyWindow(pWindow);
    return;
  }

  pWindow->hContext = wglCreateContextAttribsARB(pWindow->hDevice, NULL, contextAttribs);
  if(pWindow->hContext == NULL) {
    GT_FATAL("API:WIN32 Not create modern context");
    InternalDestroyWindow(pWindow);
    return;
  }

  wglMakeCurrent(pWindow->hDevice, pWindow->hContext);
  PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

  ApiWindowsLoadGLFunctions();
  ApiRawInputUpdateKeyMap();

  GT_INFO("API:WGL  OpenGL context created: %u.%u Core Profile", glMajor, glMinor);
  GT_INFO("API:WIN32 Window created:'%s' (%ux%u)", Title, Width, Height);
  SPS.mainWindow = pWindow;
  SPS.wglSwapIntervalEXT = wglSwapIntervalEXT;

  PWindowSetVsync(SPS.bVsync);
}

void PWindowClose() {
  SPS.mainWindow->bShouldClose = true;
}

bool PWindowIsVsync() {
  return SPS.bVsync;
}

void PWindowSetVsync(bool bEnable) {
  if(SPS.wglSwapIntervalEXT) {
    SPS.wglSwapIntervalEXT((bEnable) ? 1 : 0);
  }
  SPS.bVsync = bEnable;
}

bool PWindowIsMouseCaptured() {
  return SPS.mainWindow->bMouseCaptured;
}

void PWindowSetMouseCaptured(bool bCapture) {
  PWindow* win = SPS.mainWindow;
  if(win->bMouseCaptured == bCapture) {
    return;
  }
  win->bMouseCaptured = bCapture;
  if(bCapture) {
    PWindowGetMousePos(&win->lastMousePosX, &win->lastMousePosY);
  }
  InternalCaptureMouse(win, bCapture);
}

bool PWindowIsFullscreen() {
  return SPS.mainWindow->bFullscreen;
}

void PWindowSetFullscreen(bool bFullscreen) {
  PWindow* win = SPS.mainWindow;
  if(win->bFullscreen == bFullscreen) {
    return;
  }
  bool bCapture = PWindowIsMouseCaptured();
  PWindowSetMouseCaptured(false);
  win->bFullscreen = bFullscreen;
  if(bFullscreen) {
    GetWindowPlacement(win->hWindow, &win->placement);
    win->style = GetWindowLongA(win->hWindow, GWL_STYLE);
    LONG_PTR fullStyle = win->style & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    GetMonitorInfoA(MonitorFromWindow(win->hWindow, MONITOR_DEFAULTTONEAREST), &mi);
    int32 width = mi.rcMonitor.right - mi.rcMonitor.left;
    int32 height = mi.rcMonitor.bottom - mi.rcMonitor.top;
    int32 posX = mi.rcMonitor.left;
    int32 posY = mi.rcMonitor.top;
    SetWindowLongPtrA(win->hWindow, GWL_STYLE, fullStyle);
    SetWindowPos(win->hWindow, HWND_TOP, posX, posY, width, height, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  } else {
    SetWindowLongPtrA(win->hWindow, GWL_STYLE, win->style);
    SetWindowPlacement(win->hWindow, &win->placement);
  }
  PWindowSetMouseCaptured(bCapture);
}

void PWindowGetMousePos(int32* OutPosX, int32* OutPosY) {
  PWindow* win = SPS.mainWindow;
  POINT point = {};
  GetCursorPos(&point);
  ScreenToClient(win->hWindow, &point);
  if(OutPosX) {
    *OutPosX = point.x;
  }
  if(OutPosY) {
    *OutPosY = point.y;
  }
}

void PWindowSetMousePos(int32 PosX, int PosY) {
  PWindow* win = SPS.mainWindow;
  POINT point = {PosX, PosY};
  ClientToScreen(win->hWindow, &point);
  SetCursorPos(point.x, point.y);
}

// Internal Functions //=====================================================================================//
static void InternalUpdateWindow(PWindow* Self) {
  GT_ASSERT(Self);
  MSG msg = {};
  while(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
    if(msg.message == WINDOW_CLOSE_CODE) {
      Self->bShouldClose = true;
    }
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }

  if(Self->bNeedsResize) {
    Self->bNeedsResize = false;
    PEvent pResize = {PE_WINDOW_RESIZE};
    pResize.windowResize.width = Self->width;
    pResize.windowResize.height = Self->height;
    PEventPush(pResize);
  }
}

static LRESULT InternalWinProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
  PWindow* self = NULL;
  if(Msg == WM_NCCREATE) {
    CREATESTRUCTA* cs = (CREATESTRUCTA*)lParam;
    self = (PWindow*)cs->lpCreateParams;
    SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)self);
    ApiRawInputInit(self->hWindow);
  } else {
    self = (PWindow*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);
  }

  switch(Msg) {
    case WM_CLOSE: {
      PostMessageA(hWnd, WINDOW_CLOSE_CODE, 0, 0);
      break;
    }
    case WM_SETFOCUS:
    case WM_KILLFOCUS: {
      bool bFocused = (Msg == WM_SETFOCUS);
      bool bCapture = (bFocused) ? self->bMouseCaptured : false;
      InternalCaptureMouse(self, bCapture);
      PEvent pWindowFocus = {PE_WINDOW_FOCUS};
      pWindowFocus.windowFocus.bFocused = bFocused;
      PEventPush(pWindowFocus);
      break;
    }
    case WM_MOUSEMOVE: {
      PEvent pMouseMove = {PE_INPUT_MOUSE_POS};
      pMouseMove.mousePos.posX = (int32)GET_X_LPARAM(lParam);
      pMouseMove.mousePos.posY = (int32)GET_Y_LPARAM(lParam);
      PEventPush(pMouseMove);
      break;
    }
    case WM_SIZE: {
      if(wParam == SIZE_MINIMIZED) {
        break;
      }
      uint32 width = LOWORD(lParam);
      uint32 height = HIWORD(lParam);
      if(width != self->width || height != self->height) {
        self->width = width;
        self->height = height;
        self->bNeedsResize = true;
      }
      break;
    }
    case WM_INPUTLANGCHANGE: {
      ApiRawInputUpdateKeyMap();
      break;
    }
    case WM_INPUT: {
      ApiRawInputPollEvent((HRAWINPUT)lParam);
      break;
    }
    default: {
      return DefWindowProcA(hWnd, Msg, wParam, lParam);
    }
  }
  return 0;
}

static PWindow* InternalCreateWindow(uint32 Width, uint32 Height, cstring Title) {
  PWindow* pWindow = PMemAlloc(sizeof(PWindow));
  cstring className = "GameWindow";
  HINSTANCE hInstance = GetModuleHandleA(NULL);
  uint32 style = WS_OVERLAPPEDWINDOW;
  WNDCLASSEXA wc = {};
  if(!GetClassInfoExA(hInstance, className, &wc)) {
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = &InternalWinProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconA(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wc.hIconSm = LoadIconA(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME + 1);
    wc.lpszClassName = className;
    GT_ASSERT(RegisterClassExA(&wc) != 0);
  }
  RECT winRect = {0, 0, Width, Height};
  AdjustWindowRect(&winRect, style, false);

  int32 w = winRect.right - winRect.left;
  int32 h = winRect.bottom - winRect.top;
  HWND hWin = CreateWindowExA(0, className, Title, style, CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, hInstance, pWindow);
  HDC dc = GetDC(hWin);
  GT_ASSERT(hWin);

  pWindow->width = Width;
  pWindow->height = Height;
  pWindow->title = Title;
  pWindow->style = style;
  pWindow->hWindow = hWin;
  pWindow->hDevice = dc;
  pWindow->hContext = NULL;
  pWindow->bMouseCaptured = false;
  pWindow->bFullscreen = false;
  pWindow->bShouldClose = false;
  pWindow->bShow = false;
  pWindow->style = GetWindowLongA(hWin, GWL_STYLE);
  GetWindowPlacement(hWin, &pWindow->placement);

  PEvent pEvent = {PE_WINDOW_RESIZE};
  pEvent.windowResize.width = Width;
  pEvent.windowResize.height = Height;
  PEventPush(pEvent);
  return pWindow;
}

static void InternalDestroyWindow(PWindow* Self) {
  GT_ASSERT(Self);
  if(Self->hDevice) {
    ReleaseDC(Self->hWindow, Self->hDevice);
  }
  if(Self->hContext) {
    wglMakeCurrent(0, 0);
    wglDeleteContext(Self->hContext);
  }
  DestroyWindow(Self->hWindow);
  PMemFree(Self);
}

static void InternalCaptureMouse(PWindow* Window, bool bCapture) {
  if(SPS.bMouseCaptured == bCapture) {
    return;
  }
  SPS.bMouseCaptured = bCapture;
  if(bCapture) {
    RECT rect;
    GetClientRect(Window->hWindow, &rect);
    POINT point = {rect.left, rect.top};
    ClientToScreen(Window->hWindow, &point);
    OffsetRect(&rect, point.x - rect.left, point.y - rect.top);
    ClipCursor(&rect);
    ShowCursor(false);
  } else {
    ClipCursor(NULL);
    ShowCursor(true);
    PWindowSetMousePos(Window->lastMousePosX, Window->lastMousePosY);
  }
}

// Extern Log //=============================================================================================//
void PlatformLog(ELogLevel Level, cstring FuncName, cstring Context, cstring Format, ...) {
  cstring logTag = "";
  char buffer[GT_LOG_BUFFER] = "";
  uint64 offset = 0;
  va_list args;

  if(Format == NULL) {
    fprintf(stderr, "Log Format invalid\n");
    return;
  }

  enum {
    LC_WHITE = 7,       //
    LC_GREEN = 10,      //
    LC_YELLOW = 14,     //
    LC_RED = 12,        //
    LC_DARK_RED = 4,    //
    LC_DARK_YELLOW = 6  //
  } logColor = LC_WHITE;

  switch(Level) {
    case LOG_INFO: {
      logTag = "[LOG INFO]";
      logColor = LC_WHITE;
    } break;
    case LOG_ALERT: {
      logTag = "[LOG ALERT]";
      logColor = LC_YELLOW;
    } break;
    case LOG_SUCCESS: {
      logTag = "[LOG SUCCESS]";
      logColor = LC_GREEN;
    } break;
    case LOG_WARNING: {
      logTag = "[LOG WARNING]";
      logColor = LC_DARK_YELLOW;
    } break;
    case LOG_ERROR: {
      logTag = "[LOG ERROR]";
      logColor = LC_RED;
    } break;
    case LOG_FATAL: {
      logTag = "[LOG FATAL]";
      logColor = LC_DARK_RED;
    } break;
  }

  if(Level != LOG_INFO && FuncName != NULL) {
    offset = snprintf(buffer, sizeof(buffer) - offset, "%s %s() => ", logTag, FuncName);
  } else {
    offset = snprintf(buffer, sizeof(buffer) - offset, "%s => ", logTag);
  }

  va_start(args, Format);
  offset += vsnprintf(buffer + offset, sizeof(buffer) - offset, Format, args);
  va_end(args);

  if(Level != LOG_INFO && Level != LOG_ALERT && Context != NULL) {  //
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, " -> %s", Context);
  }

  SetConsoleTextAttribute(SPS.hConsole, logColor);
  puts(buffer);
  SetConsoleTextAttribute(SPS.hConsole, SPS.consoleDefaultAttribute);
}
#endif  // PLATFORM_WINDOWS
