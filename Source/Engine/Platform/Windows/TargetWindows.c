#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <windowsx.h>
#include "Core/Log.h"

#include "Platform/PlatformEvents.h"
#include "Platform/Platform.h"
#include "Core/EngineTypes.h"
#include "Core/Defines.h"
#include "Core/Log.h"
#include "GL/glcorearb.h"
#include "GL/wglext.h"

#define WINDOW_CLOSE_CODE WM_USER + 1
#define IDI_APP_ICON      101
#define MOUSE_LEFT_CODE   0b0000  // 0x0 D0
#define MOUSE_MIDDLE_CODE 0b0001  // 0x1 D1
#define MOUSE_RIGHT_CODE  0b0010  // 0x2 D2

extern uint32 EngineInitialize(ETargetPlatform TargetPlatform, ETargetRenderer TargetRenderer, const char** Args);
extern void EngineTerminate();
extern void EngineUpdate();
extern bool ApiWindowsLoadGLFunctions();

typedef struct {
  uint8 scanCode;
  bool isExtended;
} PKeyScan;

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
  char userDataPath[GT_MAX_PATH];
  PKey keyMap[KEY_MAX];
  PWindow* mainWindow;
  HKL keyboardLayout;
  LARGE_INTEGER timerFrequency;
  int32 consoleDefaultAttribute;
  HANDLE hConsole;
  bool bMouseCaptured;
} FPlatformState;

// Internal Functions //=====================================================================================//
static PWindow* ApiWindowCreate(uint32 Width, uint32 Height, cstring Title);
static void ApiWindowDestroy(PWindow* Self);
static void ApiWindowUpdate(PWindow* Self);
static void ApiWindowSetShow(PWindow* Self, bool bShow);
static void ApiWindowSwapBuffers(PWindow* Self);
static PWindow* InitWindow(uint32 Width, uint32 Height, cstring Title);
static LRESULT InternalWinProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
static bool InternalInitRawInput(PWindow* Window);
static void InternalEventRawInput(PWindow* Window, HRAWINPUT RawInput);
static void InternalCaptureMouse(PWindow* Window, bool bCapture);

static void InternalUpdateKeyMap();
static PKeyScan InternalEKeyCodeToPKeyScan(EKeyCode KeyCode);
static cstring InternalVkCodeToStr(uint32 VkCode);
static PKey InternalMouseCodeToPKey(uint32 MouseCode);
static PKey InternalScancodeToPKey(uint32 Scancode, bool bIsExtended);

static FPlatformState SPS;  // Static Platform State

GT_AUTO_EXEC(InitConsole) {
  SPS.hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  GetConsoleScreenBufferInfo(SPS.hConsole, &consoleInfo);
  SPS.consoleDefaultAttribute = consoleInfo.wAttributes;
}

// Entrypoint //=============================================================================================//
int32 NativeMain(int argc, const char** argv) {
  QueryPerformanceFrequency(&SPS.timerFrequency);
  GetEnvironmentVariableA("LOCALAPPDATA", SPS.userDataPath, sizeof(SPS.userDataPath));
  snprintf(SPS.userDataPath, sizeof(SPS.userDataPath), "%s\\%s", SPS.userDataPath, STR(GAME_NAME));

  SPS.mainWindow = InitWindow(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, STR(GAME_NAME));
  if(SPS.mainWindow == NULL) {
    return 1;
  }
  ApiWindowSetShow(SPS.mainWindow, true);
  EngineInitialize(TARGET_PLATFORM_WINDOWS, TARGET_RENDERER_GL_CORE_33, argv);

  while(!SPS.mainWindow->bShouldClose) {
    ApiWindowUpdate(SPS.mainWindow);
    EngineUpdate();
    ApiWindowSwapBuffers(SPS.mainWindow);
  }
  EngineTerminate();
  ApiWindowDestroy(SPS.mainWindow);
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

// Window Api //=============================================================================================//
void PWindowClose() {
  SPS.mainWindow->bShouldClose = true;
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

PKey* PWindowGetKeyMap(uint8* OutCount) {
  if(OutCount) {
    *OutCount = KEY_MAX;
  }
  return SPS.keyMap;
}

// Time Api //===============================================================================================//
double PGetTime() {
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / (double)SPS.timerFrequency.QuadPart;
}

void PWait(double Seconds) {
  if(Seconds > 0.0) {
    DWORD ms = (DWORD)(Seconds * 1000.0);
    Sleep(ms);
  }
}

// File System Api //=========================================================================================//
bool PDirExist(cstring Path) {
  GT_ASSERT(Path);
  if(!Path) {
    return false;
  }
  DWORD attr = GetFileAttributesA(Path);
  if(attr == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool PDirMake(cstring Path) {
  GT_ASSERT(Path);
  if(!Path) {
    return false;
  }
  char tempBuff[GT_MAX_PATH];
  snprintf(tempBuff, sizeof(tempBuff), "%s", Path);
  char* ptr = tempBuff;
  if(ptr[1] == ':') {
    ptr += 2;
  }
  while(*ptr) {
    if(*ptr == '\\' || *ptr == '/') {
      *ptr = '\0';
      if(!CreateDirectoryA(tempBuff, NULL)) {
        if(GetLastError() != ERROR_ALREADY_EXISTS) {
          return false;
        }
      }
      *ptr = '\\';
    }
    ptr++;
  }
  if(!CreateDirectoryA(tempBuff, NULL)) {
    if(GetLastError() != ERROR_ALREADY_EXISTS) {
      return false;
    }
  }
  return true;
}

bool PDirDelete(cstring Path, bool bRecursive) {
  GT_ASSERT(Path);
  if(!Path) {
    return false;
  }
  DWORD attr = GetFileAttributesA(Path);
  if(attr == INVALID_FILE_ATTRIBUTES) {
    DWORD err = GetLastError();
    if(err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
      return true;
    }
    return false;
  }
  if(!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
    return false;
  }
  if(!bRecursive) {
    SetFileAttributesA(Path, FILE_ATTRIBUTE_NORMAL);
    return (RemoveDirectoryA(Path) != 0);
  }

  PDir dir;
  if(!PDirOpen(Path, &dir)) {
    return false;
  }
  PDirEntry entry;
  while(PDirRead(&dir, &entry)) {
    if(entry.bIsDirectory) {
      if(!PDirDelete(entry.path, true)) {
        PDirClose(&dir);
        return false;
      }
    } else {
      if(!PFileDelete(entry.path)) {
        PDirClose(&dir);
        return false;
      }
    }
  }
  PDirClose(&dir);
  SetFileAttributesA(Path, FILE_ATTRIBUTE_NORMAL);
  return (RemoveDirectoryA(Path) != 0);
}

bool PDirCopy(cstring Dst, cstring Src, uint8 Depth, bool bOverride) {
  GT_ASSERT(Dst && Src);
  if(!Dst || !Src) {
    return false;
  }
  if(Depth > GT_MAX_DIR_DEPTH) {
    GT_ALERT("Max recursion depth reached for directory copy");
    return false;
  }
  if(!PDirExist(Dst)) {
    PDirMake(Dst);
  }
  PDir srcDir;
  if(!PDirOpen(Src, &srcDir)) {
    return false;
  }
  char dstPath[GT_MAX_PATH];
  PDirEntry entry;
  while(PDirRead(&srcDir, &entry)) {
    snprintf(dstPath, sizeof(dstPath), "%s\\%s", Dst, entry.name);
    if(entry.bIsDirectory) {
      if(Depth > 0 && Depth <= GT_MAX_DIR_DEPTH) {
        PDirMake(dstPath);
        PDirCopy(dstPath, entry.path, Depth - 1, bOverride);
      }
      continue;
    }
    PFileCopy(dstPath, entry.path, bOverride);
  }
  PDirClose(&srcDir);
  return true;
}

bool PDirOpen(cstring Path, PDir* OutDir) {
  GT_ASSERT(Path && OutDir);
  if(!Path || !OutDir) {
    return false;
  }
  PMemSet(OutDir, 0, sizeof(PDir));
  char searchPath[GT_MAX_PATH];
  snprintf(searchPath, sizeof(searchPath), "%s\\*", Path);
  WIN32_FIND_DATAA findData;
  HANDLE handle = FindFirstFileA(searchPath, &findData);
  if(handle == INVALID_HANDLE_VALUE) {
    return false;
  }
  OutDir->handle = handle;
  OutDir->bIsOpen = true;
  snprintf(OutDir->path, sizeof(OutDir->path), "%s", Path);
  return true;
}

void PDirClose(PDir* Dir) {
  GT_ASSERT(Dir);
  if(Dir == NULL || Dir->bIsOpen == false) {
    return;
  }
  FindClose(Dir->handle);
  Dir->bIsOpen = false;
}

bool PDirRead(PDir* Dir, PDirEntry* OutEntry) {
  GT_ASSERT(Dir && OutEntry);
  if(Dir == NULL || OutEntry == NULL) {
    return false;
  }
  HANDLE handle = Dir->handle;
  WIN32_FIND_DATAA findData;
  while(true) {
    if(!FindNextFileA(handle, &findData)) {
      return false;
    }
    if(strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
      continue;
    }
    break;
  }
  PMemSet(OutEntry, 0, sizeof(PDirEntry));
  snprintf(OutEntry->name, sizeof(OutEntry->name), "%s", findData.cFileName);
  snprintf(OutEntry->path, sizeof(OutEntry->path), "%s\\%s", Dir->path, findData.cFileName);
  OutEntry->bIsDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  OutEntry->size = ((uint64)findData.nFileSizeHigh << 32) | (uint64)findData.nFileSizeLow;
  OutEntry->createdTime = ((uint64)findData.ftCreationTime.dwHighDateTime << 32) | (uint64)findData.ftCreationTime.dwLowDateTime;
  OutEntry->lastWriteTime = ((uint64)findData.ftLastWriteTime.dwHighDateTime << 32) | (uint64)findData.ftLastWriteTime.dwLowDateTime;
  OutEntry->lastAccessTime = ((uint64)findData.ftLastAccessTime.dwHighDateTime << 32) | (uint64)findData.ftLastAccessTime.dwLowDateTime;
  return true;
}

cstring PGetUserDataPath() {
  return SPS.userDataPath;
}

bool PFileExist(cstring Path) {
  GT_ASSERT(Path);
  if(!Path) {
    return false;
  }
  DWORD attr = GetFileAttributesA(Path);
  if(attr == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool PFileDelete(cstring Path) {
  GT_ASSERT(Path);
  if(!Path) {
    return false;
  }
  DWORD attr = GetFileAttributesA(Path);
  if(attr == INVALID_FILE_ATTRIBUTES) {
    DWORD err = GetLastError();
    if(err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
      return true;
    }
    return false;
  }
  if(attr & FILE_ATTRIBUTE_DIRECTORY) {
    return false;
  }
  SetFileAttributesA(Path, FILE_ATTRIBUTE_NORMAL);
  return DeleteFileA(Path) != 0;
}

bool PFileCopy(cstring Dst, cstring Src, bool bOverride) {
  return (CopyFileA(Src, Dst, !bOverride) != 0);
}

bool PFileOpen(cstring Path, EPlatformFileMode Mode, PFile* OutFile) {
  GT_ASSERT(Path && OutFile);
  if(!Path || !OutFile) {
    return false;
  }
  PMemSet(OutFile, 0, sizeof(PFile));
  DWORD access = 0;
  DWORD creation = 0;
  switch(Mode) {
    case PLATFORM_FILE_READ: {
      access = GENERIC_READ;
      creation = OPEN_EXISTING;
      break;
    }
    case PLATFORM_FILE_WRITE: {
      access = GENERIC_WRITE;
      creation = CREATE_ALWAYS;
      break;
    }
    case PLATFORM_FILE_APPEND: {
      access = FILE_APPEND_DATA;
      creation = OPEN_ALWAYS;
      break;
    }
  }
  HANDLE hFile = CreateFileA(Path, access, FILE_SHARE_READ, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
  if(hFile == INVALID_HANDLE_VALUE) {
    return false;
  }
  if(Mode == PLATFORM_FILE_APPEND) {
    SetFilePointer(hFile, 0, NULL, FILE_END);
  }
  LARGE_INTEGER size;
  if(GetFileSizeEx(hFile, &size)) {
    OutFile->size = size.QuadPart;
  }
  FILETIME ftCreate, ftAccess, ftWrite;
  if(GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite)) {
    OutFile->createdTime = ((uint64)ftCreate.dwHighDateTime << 32) | ftCreate.dwLowDateTime;
    OutFile->lastAccessTime = ((uint64)ftAccess.dwHighDateTime << 32) | ftAccess.dwLowDateTime;
    OutFile->lastWriteTime = ((uint64)ftWrite.dwHighDateTime << 32) | ftWrite.dwLowDateTime;
  }
  OutFile->handle = (void*)hFile;
  OutFile->mode = Mode;
  return true;
}

void PFileClose(PFile* File) {
  GT_ASSERT(File && File->handle);
  if(!File || !File->handle) {
    return;
  }
  CloseHandle(File->handle);
  File->handle = NULL;
}

bool PFileRead(PFile* File, uint64 Offset, uint64 ReadSize, uint8* OutFileBuffer) {
  static const uint32 MAX_READ_CHUNK = 0xFFFFFFFFu;
  GT_ASSERT(File && File->handle && OutFileBuffer);
  if(!File || !File->handle || !OutFileBuffer) {
    return false;
  }
  if(Offset >= File->size) {
    return false;
  }
  uint8* dst = OutFileBuffer;
  HANDLE handle = (HANDLE)File->handle;
  uint64 remaining = ReadSize;
  if(Offset + ReadSize > File->size) {
    remaining = File->size - Offset;
  }
  LARGE_INTEGER li;
  li.QuadPart = Offset;
  if(!SetFilePointerEx(handle, li, NULL, FILE_BEGIN)) {
    return false;
  }
  while(remaining > 0) {
    DWORD chunk = (remaining > MAX_READ_CHUNK) ? MAX_READ_CHUNK : (DWORD)remaining;
    DWORD bytesRead = 0;
    if(!ReadFile(handle, dst, chunk, &bytesRead, NULL)) {
      return false;
    }
    if(bytesRead == 0) {
      break;
    }
    remaining -= bytesRead;
    dst += bytesRead;
  }
  return true;
}

// Memory Api //=============================================================================================//
void PMemFree(void* Data) {
  GT_ASSERT(Data);
  free(Data);
}

void* PMemAlloc(uint64 Size) {
  GT_ASSERT(Size > 0);
  return calloc(1, Size);
}

void* PMemRealloc(void* Data, uint64 Size) {
  GT_ASSERT(Size > 0);
  return realloc(Data, Size);
}

void* PMemCopy(void* Dst, void* Src, uint64 Size) {
  GT_ASSERT(Dst && Src && Size > 0);
  return memcpy(Dst, Src, Size);
}

void* PMemMove(void* Dst, void* Src, uint64 Size) {
  GT_ASSERT(Dst && Src && Size > 0);
  return memmove(Dst, Src, Size);
}

void* PMemSet(void* Dst, int32 Value, uint64 Size) {
  GT_ASSERT(Dst && Size > 0);
  return memset(Dst, Value, Size);
}

static PWindow* ApiWindowCreate(uint32 Width, uint32 Height, cstring Title) {
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

  return pWindow;
}

static void ApiWindowDestroy(PWindow* Self) {
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

static void ApiWindowUpdate(PWindow* Self) {
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

static void ApiWindowSetShow(PWindow* Self, bool bShow) {
  GT_ASSERT(Self);
  if(Self->bShow == bShow) {
    return;
  }
  Self->bShow = bShow;
  int32 cmd = (bShow) ? SW_SHOWNORMAL : SW_HIDE;
  ShowWindow(Self->hWindow, cmd);
  UpdateWindow(Self->hWindow);
}

static void ApiWindowSwapBuffers(PWindow* Self) {
  GT_ASSERT(Self);
  SwapBuffers(Self->hDevice);
}

// Internal Functions //=====================================================================================//
static PWindow* InitWindow(uint32 Width, uint32 Height, cstring Title) {
  const uint32 glMajor = 3;
  const uint32 glMinor = 3;
  const BYTE colorBits = 32;
  const BYTE depthBits = 24;
  const BYTE stencilBits = 8;

  PWindow* legacyWindow = ApiWindowCreate(0, 0, "LegacyWindow");
  HDC legacyDC = legacyWindow->hDevice;

  PIXELFORMATDESCRIPTOR pfd = {};
  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = colorBits;
  pfd.cDepthBits = depthBits;
  pfd.cStencilBits = stencilBits;
  pfd.iLayerType = PFD_MAIN_PLANE;

  int32 pixeFormat = ChoosePixelFormat(legacyDC, &pfd);
  if(!pixeFormat) {
    GT_FATAL("API:WIN32 Not Choose Pixel Format");
    ApiWindowDestroy(legacyWindow);
    return NULL;
  }

  if(!SetPixelFormat(legacyDC, pixeFormat, &pfd)) {
    GT_FATAL("API:WIN32 Not Set Pixel Format");
    ApiWindowDestroy(legacyWindow);
    return NULL;
  }

  HGLRC legacyContext = wglCreateContext(legacyDC);
  if(legacyContext == NULL) {
    GT_FATAL("API:WIN32 Not Create Context");
    ApiWindowDestroy(legacyWindow);
    return NULL;
  }

  if(!wglMakeCurrent(legacyDC, legacyContext)) {
    GT_FATAL("API:WIN32 Not make current context");
    ApiWindowDestroy(legacyWindow);
    return NULL;
  }

  PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
  PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
  GT_ASSERT(wglChoosePixelFormatARB && wglCreateContextAttribsARB);

  wglMakeCurrent(0, 0);
  wglDeleteContext(legacyContext);
  ApiWindowDestroy(legacyWindow);

  {
    int32 pixelFormatAttribs[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,    //
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,    //
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,     //
        WGL_COLOR_BITS_ARB, colorBits,      //
        WGL_STENCIL_BITS_ARB, stencilBits,  //
        WGL_DEPTH_BITS_ARB, depthBits,      //
        0                                   //
    };

    int32 glFlags = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
#ifdef DEVELOPMENT_MODE
    glFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif  // DEVELOPMENT_MODE

    int32 contextAttribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, glMajor,  //
        WGL_CONTEXT_MINOR_VERSION_ARB, glMinor,  //
        WGL_CONTEXT_FLAGS_ARB, glFlags,          //
        0                                        //
    };

    PWindow* pWindow = ApiWindowCreate(Width, Height, Title);
    int32 pixelFormat = 0;
    uint32 numPixelFormat = 0;
    wglChoosePixelFormatARB(pWindow->hDevice, pixelFormatAttribs, NULL, 1, &pixelFormat, (UINT*)&numPixelFormat);
    if(numPixelFormat <= 0) {
      GT_FATAL("API:WIN32 Not valid modern Pixel Format");
      ApiWindowDestroy(pWindow);
      return NULL;
    }

    if(!SetPixelFormat(pWindow->hDevice, pixelFormat, &pfd)) {
      GT_FATAL("API:WIN32 Not set modern Pixel Format");
      ApiWindowDestroy(pWindow);
      return NULL;
    }

    pWindow->hContext = wglCreateContextAttribsARB(pWindow->hDevice, NULL, contextAttribs);
    if(pWindow->hContext == NULL) {
      GT_FATAL("API:WIN32 Not create modern context");
      ApiWindowDestroy(pWindow);
      return NULL;
    }

    wglMakeCurrent(pWindow->hDevice, pWindow->hContext);
    typedef BOOL(WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int interval);
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if(wglSwapIntervalEXT) {
      wglSwapIntervalEXT(1);
    }
    ApiWindowsLoadGLFunctions();

    GT_INFO("API:WIN32 Window created: '%s' (%ux%u)", Title, Width, Height);
    GT_INFO("API:WIN32 OpenGL context created: %u.%u Core Profile", glMajor, glMinor);
    return pWindow;
  }
}

static LRESULT InternalWinProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
  PWindow* self = NULL;
  if(Msg == WM_NCCREATE) {
    CREATESTRUCTA* cs = (CREATESTRUCTA*)lParam;
    self = (PWindow*)cs->lpCreateParams;
    SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)self);
    InternalInitRawInput(self);
    InternalUpdateKeyMap();
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
      InternalUpdateKeyMap();
      break;
    }
    case WM_INPUT: {
      InternalEventRawInput(self, (HRAWINPUT)lParam);
      break;
    }
    default: {
      return DefWindowProcA(hWnd, Msg, wParam, lParam);
    }
  }
  return 0;
}

static bool InternalInitRawInput(PWindow* Window) {
#define MAX_DEVICES 2
  RAWINPUTDEVICE rid[MAX_DEVICES];
  // Keybord
  rid[0].usUsagePage = 0x01;  // Generic Desktop Controls
  rid[0].usUsage = 0x06;      // Keybord
  rid[0].dwFlags = RIDEV_DEVNOTIFY;
  rid[0].hwndTarget = Window->hWindow;

  // Mouse
  rid[1].usUsagePage = 0x01;  // Generic Desktop Controls
  rid[1].usUsage = 0x02;      // Mouse
  rid[1].dwFlags = 0;
  rid[1].hwndTarget = Window->hWindow;

  if(!RegisterRawInputDevices(rid, MAX_DEVICES, sizeof(rid[0]))) {
    DWORD err = GetLastError();
    GT_FATAL("API:WIN32 RawInput failed, error = %lu", err);
    return false;
  }
  return true;
}

static void InternalEventRawInput(PWindow* Window, HRAWINPUT RawInput) {
  static struct {
    char* data;
    uint64 size;
  } inputData = {NULL, 0};
  static struct {
    USHORT flagDown;
    USHORT flagUp;
    int code;
  } buttons[] = {
      {RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_LEFT_BUTTON_UP, MOUSE_LEFT_CODE},       //
      {RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_UP, MOUSE_RIGHT_CODE},    //
      {RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_UP, MOUSE_MIDDLE_CODE}  //
  };

  UINT dwSize = 0;
  GetRawInputData(RawInput, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
  if(inputData.size < dwSize) {
    inputData.data = PMemRealloc(inputData.data, dwSize);
    inputData.size = dwSize;
  }
  if(!GetRawInputData(RawInput, RID_INPUT, inputData.data, &dwSize, sizeof(RAWINPUTHEADER))) {
    return;
  }
  RAWINPUT* raw = (RAWINPUT*)inputData.data;
  if(raw->header.dwType == RIM_TYPEKEYBOARD) {
    RAWKEYBOARD kb = raw->data.keyboard;
    bool bIsExtended = (kb.Flags & RI_KEY_E0) != 0;
    bool bIsE1 = (kb.Flags & RI_KEY_E1) != 0;
    bool isKeyUp = (kb.Flags & RI_KEY_BREAK) != 0;
    if(bIsE1) {
      return;
    }
    PKey pKey = InternalScancodeToPKey(kb.MakeCode, bIsExtended);
    if(pKey.keyCode == KEY_UNKNOWN) {
      return;
    }
    PEvent pKeyEvent;
    pKeyEvent.eventType = PE_INPUT_KEY;
    pKeyEvent.inputKey.keyCode = pKey.keyCode;
    pKeyEvent.inputKey.bState = !isKeyUp;
    PEventPush(pKeyEvent);
  } else if(raw->header.dwType == RIM_TYPEMOUSE) {
    RAWMOUSE mouse = raw->data.mouse;
    if((mouse.usFlags & MOUSE_MOVE_RELATIVE) && Window->bMouseCaptured) {
      PEvent pMouseDetaEvent = {PE_INPUT_MOUSE_DELTA};
      pMouseDetaEvent.mouseDelta.deltaX = mouse.lLastX;
      pMouseDetaEvent.mouseDelta.deltaY = mouse.lLastY;
      PEventPush(pMouseDetaEvent);
    }
    if(mouse.usButtonFlags & RI_MOUSE_HWHEEL) {
      SHORT wheelDelta = (SHORT)mouse.usButtonData;
      PEvent pMouseWheel = {PE_INPUT_MOUSE_SCROLL};
      pMouseWheel.mouseScroll.scrollX = (float)wheelDelta / 120.f;
      pMouseWheel.mouseScroll.scrollY = 0.f;
      PEventPush(pMouseWheel);
    }
    if(mouse.usButtonFlags & RI_MOUSE_WHEEL) {
      SHORT wheelDelta = (SHORT)mouse.usButtonData;
      PEvent pMouseWheel = {PE_INPUT_MOUSE_SCROLL};
      pMouseWheel.mouseScroll.scrollX = 0.f;
      pMouseWheel.mouseScroll.scrollY = (float)wheelDelta / 120.f;
      PEventPush(pMouseWheel);
    }
    for(int32 i = 0; i < 3; ++i) {
      PKey pKey = InternalMouseCodeToPKey(buttons[i].code);
      int32 bPressed = -1;
      if(mouse.usButtonFlags & buttons[i].flagDown) {
        bPressed = 1;
      }
      if(mouse.usButtonFlags & buttons[i].flagUp) {
        bPressed = 0;
      }
      if(bPressed != -1 && pKey.keyCode != KEY_UNKNOWN) {
        PEvent pEvent = {PE_INPUT_KEY};
        pEvent.inputKey.keyCode = pKey.keyCode;
        pEvent.inputKey.bState = bPressed;
        PEventPush(pEvent);
      }
    }
  }
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

static void InternalUpdateKeyMap() {
  SPS.keyboardLayout = GetKeyboardLayout(0);
  for(uint32 c = 0; c < KEY_MAX; c++) {
    PKeyScan keyScan = InternalEKeyCodeToPKeyScan(c);
    PKey pKey = InternalScancodeToPKey(keyScan.scanCode, keyScan.isExtended);
    if(pKey.keyCode == KEY_UNKNOWN) {
      continue;
    }
    if(keyScan.isExtended) {
      keyScan.scanCode |= 0xE000;
    }
    UINT vk = MapVirtualKeyExA(keyScan.scanCode, MAPVK_VSC_TO_VK_EX, SPS.keyboardLayout);
    cstring name = InternalVkCodeToStr(vk);
    if(name) {
      pKey.layoutName = name;
    }
    SPS.keyMap[pKey.keyCode] = pKey;
  }
  SPS.keyMap[KEY_MOUSE_LEFT] = InternalMouseCodeToPKey(MOUSE_LEFT_CODE);
  SPS.keyMap[KEY_MOUSE_MIDDLE] = InternalMouseCodeToPKey(MOUSE_MIDDLE_CODE);
  SPS.keyMap[KEY_MOUSE_RIGHT] = InternalMouseCodeToPKey(MOUSE_RIGHT_CODE);
}

static PKeyScan InternalEKeyCodeToPKeyScan(EKeyCode KeyCode) {
  switch(KeyCode) {
    case KEY_ZERO: return (PKeyScan){0x0B, false};
    case KEY_ONE: return (PKeyScan){0x02, false};
    case KEY_TWO: return (PKeyScan){0x03, false};
    case KEY_THREE: return (PKeyScan){0x04, false};
    case KEY_FOUR: return (PKeyScan){0x05, false};
    case KEY_FIVE: return (PKeyScan){0x06, false};
    case KEY_SIX: return (PKeyScan){0x07, false};
    case KEY_SEVEN: return (PKeyScan){0x08, false};
    case KEY_EIGHT: return (PKeyScan){0x09, false};
    case KEY_NINE: return (PKeyScan){0x0A, false};
    case KEY_A: return (PKeyScan){0x1E, false};
    case KEY_B: return (PKeyScan){0x30, false};
    case KEY_C: return (PKeyScan){0x2E, false};
    case KEY_D: return (PKeyScan){0x20, false};
    case KEY_E: return (PKeyScan){0x12, false};
    case KEY_F: return (PKeyScan){0x21, false};
    case KEY_G: return (PKeyScan){0x22, false};
    case KEY_H: return (PKeyScan){0x23, false};
    case KEY_I: return (PKeyScan){0x17, false};
    case KEY_J: return (PKeyScan){0x24, false};
    case KEY_K: return (PKeyScan){0x25, false};
    case KEY_L: return (PKeyScan){0x26, false};
    case KEY_M: return (PKeyScan){0x32, false};
    case KEY_N: return (PKeyScan){0x31, false};
    case KEY_O: return (PKeyScan){0x18, false};
    case KEY_P: return (PKeyScan){0x19, false};
    case KEY_Q: return (PKeyScan){0x10, false};
    case KEY_R: return (PKeyScan){0x13, false};
    case KEY_S: return (PKeyScan){0x1F, false};
    case KEY_T: return (PKeyScan){0x14, false};
    case KEY_U: return (PKeyScan){0x16, false};
    case KEY_V: return (PKeyScan){0x2F, false};
    case KEY_W: return (PKeyScan){0x11, false};
    case KEY_X: return (PKeyScan){0x2D, false};
    case KEY_Y: return (PKeyScan){0x15, false};
    case KEY_Z: return (PKeyScan){0x2C, false};
    case KEY_ESCAPE: return (PKeyScan){0x01, false};
    case KEY_TAB: return (PKeyScan){0x0F, false};
    case KEY_BACKSPACE: return (PKeyScan){0x0E, false};
    case KEY_ENTER: return (PKeyScan){0x1C, false};
    case KEY_SPACE: return (PKeyScan){0x39, false};
    case KEY_F1: return (PKeyScan){0x3B, false};
    case KEY_F2: return (PKeyScan){0x3C, false};
    case KEY_F3: return (PKeyScan){0x3D, false};
    case KEY_F4: return (PKeyScan){0x3E, false};
    case KEY_F5: return (PKeyScan){0x3F, false};
    case KEY_F6: return (PKeyScan){0x40, false};
    case KEY_F7: return (PKeyScan){0x41, false};
    case KEY_F8: return (PKeyScan){0x42, false};
    case KEY_F9: return (PKeyScan){0x43, false};
    case KEY_F10: return (PKeyScan){0x44, false};
    case KEY_F11: return (PKeyScan){0x57, false};
    case KEY_F12: return (PKeyScan){0x58, false};
    case KEY_LEFT_SHIFT: return (PKeyScan){0x2A, false};
    case KEY_RIGHT_SHIFT: return (PKeyScan){0x36, false};
    case KEY_LEFT_CONTROL: return (PKeyScan){0x1D, false};
    case KEY_RIGHT_CONTROL: return (PKeyScan){0x1D, true};
    case KEY_LEFT_ALT: return (PKeyScan){0x38, false};
    case KEY_RIGHT_ALT: return (PKeyScan){0x38, true};
    case KEY_LEFT_SUPER: return (PKeyScan){0x5B, true};
    case KEY_RIGHT_SUPER: return (PKeyScan){0x5C, true};
    case KEY_KB_MENU: return (PKeyScan){0x5D, true};
    case KEY_CAPS_LOCK: return (PKeyScan){0x3A, false};
    case KEY_NUM_LOCK: return (PKeyScan){0x45, false};
    case KEY_SCROLL_LOCK: return (PKeyScan){0x46, false};
    case KEY_UP: return (PKeyScan){0x48, true};
    case KEY_DOWN: return (PKeyScan){0x50, true};
    case KEY_LEFT: return (PKeyScan){0x4B, true};
    case KEY_RIGHT: return (PKeyScan){0x4D, true};
    case KEY_HOME: return (PKeyScan){0x47, true};
    case KEY_END: return (PKeyScan){0x4F, true};
    case KEY_PAGE_UP: return (PKeyScan){0x49, true};
    case KEY_PAGE_DOWN: return (PKeyScan){0x51, true};
    case KEY_INSERT: return (PKeyScan){0x52, true};
    case KEY_DELETE: return (PKeyScan){0x53, true};
    case KEY_KP_0: return (PKeyScan){0x52, false};
    case KEY_KP_1: return (PKeyScan){0x4F, false};
    case KEY_KP_2: return (PKeyScan){0x50, false};
    case KEY_KP_3: return (PKeyScan){0x51, false};
    case KEY_KP_4: return (PKeyScan){0x4B, false};
    case KEY_KP_5: return (PKeyScan){0x4C, false};
    case KEY_KP_6: return (PKeyScan){0x4D, false};
    case KEY_KP_7: return (PKeyScan){0x47, false};
    case KEY_KP_8: return (PKeyScan){0x48, false};
    case KEY_KP_9: return (PKeyScan){0x49, false};
    case KEY_KP_DECIMAL: return (PKeyScan){0x53, false};
    case KEY_KP_DIVIDE: return (PKeyScan){0x35, true};
    case KEY_KP_MULTIPLY: return (PKeyScan){0x37, false};
    case KEY_KP_SUBTRACT: return (PKeyScan){0x4A, false};
    case KEY_KP_ADD: return (PKeyScan){0x4E, false};
    case KEY_KP_ENTER: return (PKeyScan){0x1C, true};
    case KEY_KP_EQUAL: return (PKeyScan){0x0D, false};
    default: return (PKeyScan){0, false};
  }
}

static cstring InternalVkCodeToStr(uint32 VkCode) {
  switch(VkCode) {
    case 'A': return "A";
    case 'B': return "B";
    case 'C': return "C";
    case 'D': return "D";
    case 'E': return "E";
    case 'F': return "F";
    case 'G': return "G";
    case 'H': return "H";
    case 'I': return "I";
    case 'J': return "J";
    case 'K': return "K";
    case 'L': return "L";
    case 'M': return "M";
    case 'N': return "N";
    case 'O': return "O";
    case 'P': return "P";
    case 'Q': return "Q";
    case 'R': return "R";
    case 'S': return "S";
    case 'T': return "T";
    case 'U': return "U";
    case 'V': return "V";
    case 'W': return "W";
    case 'X': return "X";
    case 'Y': return "Y";
    case 'Z': return "Z";
    default: return NULL;
  }
}

static PKey InternalMouseCodeToPKey(uint32 MouseCode) {
  switch(MouseCode) {
    case MOUSE_LEFT_CODE: return (PKey){KEY_MOUSE_LEFT, "Mouse Left", "Mouse Left"};
    case MOUSE_MIDDLE_CODE: return (PKey){KEY_MOUSE_MIDDLE, "Mouse Middler", "Mouse Middler"};
    case MOUSE_RIGHT_CODE: return (PKey){KEY_MOUSE_RIGHT, "Mouse Right", "Mouse Right"};
    default: return (PKey){KEY_UNKNOWN, "Unknown", "Unknown"};
  }
}

static PKey InternalScancodeToPKey(uint32 Scancode, bool bIsExtended) {
  if(bIsExtended) {
    switch(Scancode) {
      case 0x47: return (PKey){KEY_HOME, "Home", "Home"};
      case 0x48: return (PKey){KEY_UP, "Up Arrow", "Up Arrow"};
      case 0x49: return (PKey){KEY_PAGE_UP, "Page Up", "Page Up"};
      case 0x4B: return (PKey){KEY_LEFT, "Left Arrow", "Left Arrow"};
      case 0x4D: return (PKey){KEY_RIGHT, "Right Arrow", "Right Arrow"};
      case 0x4F: return (PKey){KEY_END, "End", "End"};
      case 0x50: return (PKey){KEY_DOWN, "Down Arrow", "Down Arrow"};
      case 0x51: return (PKey){KEY_PAGE_DOWN, "Page Down", "Page Down"};
      case 0x52: return (PKey){KEY_INSERT, "Insert", "Insert"};
      case 0x53: return (PKey){KEY_DELETE, "Delete", "Delete"};
      case 0x1C: return (PKey){KEY_KP_ENTER, "Numpad Enter", "Numpad Enter"};
      case 0x1D: return (PKey){KEY_RIGHT_CONTROL, "Right Control", "Right Control"};
      case 0x38: return (PKey){KEY_RIGHT_ALT, "Right Alt", "Right Alt"};
      case 0x5B: return (PKey){KEY_LEFT_SUPER, "Left Super", "Left Super"};
      case 0x5C: return (PKey){KEY_RIGHT_SUPER, "Right Super", "Right Super"};
      case 0x5D: return (PKey){KEY_KB_MENU, "Menu", "Menu"};
      case 0x37: return (PKey){KEY_PRINT_SCREEN, "Print Screen", "Print Screen"};
    }
    return (PKey){KEY_UNKNOWN, "Unknown", "Unknown"};
  }

  switch(Scancode) {
    case 0x1E: return (PKey){KEY_A, "A", "A"};
    case 0x30: return (PKey){KEY_B, "B", "B"};
    case 0x2E: return (PKey){KEY_C, "C", "C"};
    case 0x20: return (PKey){KEY_D, "D", "D"};
    case 0x12: return (PKey){KEY_E, "E", "E"};
    case 0x21: return (PKey){KEY_F, "F", "F"};
    case 0x22: return (PKey){KEY_G, "G", "G"};
    case 0x23: return (PKey){KEY_H, "H", "H"};
    case 0x17: return (PKey){KEY_I, "I", "I"};
    case 0x24: return (PKey){KEY_J, "J", "J"};
    case 0x25: return (PKey){KEY_K, "K", "K"};
    case 0x26: return (PKey){KEY_L, "L", "L"};
    case 0x32: return (PKey){KEY_M, "M", "M"};
    case 0x31: return (PKey){KEY_N, "N", "N"};
    case 0x18: return (PKey){KEY_O, "O", "O"};
    case 0x19: return (PKey){KEY_P, "P", "P"};
    case 0x10: return (PKey){KEY_Q, "Q", "Q"};
    case 0x13: return (PKey){KEY_R, "R", "R"};
    case 0x1F: return (PKey){KEY_S, "S", "S"};
    case 0x14: return (PKey){KEY_T, "T", "T"};
    case 0x16: return (PKey){KEY_U, "U", "U"};
    case 0x2F: return (PKey){KEY_V, "V", "V"};
    case 0x11: return (PKey){KEY_W, "W", "W"};
    case 0x2D: return (PKey){KEY_X, "X", "X"};
    case 0x15: return (PKey){KEY_Y, "Y", "Y"};
    case 0x2C: return (PKey){KEY_Z, "Z", "Z"};
    case 0x0B: return (PKey){KEY_ZERO, "0", "0"};
    case 0x02: return (PKey){KEY_ONE, "1", "1"};
    case 0x03: return (PKey){KEY_TWO, "2", "2"};
    case 0x04: return (PKey){KEY_THREE, "3", "3"};
    case 0x05: return (PKey){KEY_FOUR, "4", "4"};
    case 0x06: return (PKey){KEY_FIVE, "5", "5"};
    case 0x07: return (PKey){KEY_SIX, "6", "6"};
    case 0x08: return (PKey){KEY_SEVEN, "7", "7"};
    case 0x09: return (PKey){KEY_EIGHT, "8", "8"};
    case 0x0A: return (PKey){KEY_NINE, "9", "9"};
    case 0x3B: return (PKey){KEY_F1, "F1", "F1"};
    case 0x3C: return (PKey){KEY_F2, "F2", "F2"};
    case 0x3D: return (PKey){KEY_F3, "F3", "F3"};
    case 0x3E: return (PKey){KEY_F4, "F4", "F4"};
    case 0x3F: return (PKey){KEY_F5, "F5", "F5"};
    case 0x40: return (PKey){KEY_F6, "F6", "F6"};
    case 0x41: return (PKey){KEY_F7, "F7", "F7"};
    case 0x42: return (PKey){KEY_F8, "F8", "F8"};
    case 0x43: return (PKey){KEY_F9, "F9", "F9"};
    case 0x44: return (PKey){KEY_F10, "F10", "F10"};
    case 0x57: return (PKey){KEY_F11, "F11", "F11"};
    case 0x58: return (PKey){KEY_F12, "F12", "F12"};
    case 0x2A: return (PKey){KEY_LEFT_SHIFT, "Left Shift", "Left Shift"};
    case 0x36: return (PKey){KEY_RIGHT_SHIFT, "Right Shift", "Right Shift"};
    case 0x1D: return (PKey){KEY_LEFT_CONTROL, "Left Control", "Left Control"};
    case 0x38: return (PKey){KEY_LEFT_ALT, "Left Alt", "Left Alt"};
    case 0x01: return (PKey){KEY_ESCAPE, "Escape", "Escape"};
    case 0x0F: return (PKey){KEY_TAB, "Tab", "Tab"};
    case 0x0E: return (PKey){KEY_BACKSPACE, "Backspace", "Backspace"};
    case 0x1C: return (PKey){KEY_ENTER, "Enter", "Enter"};
    case 0x39: return (PKey){KEY_SPACE, "Space", "Space"};
    case 0x3A: return (PKey){KEY_CAPS_LOCK, "Caps Lock", "Caps Lock"};
    case 0x45: return (PKey){KEY_NUM_LOCK, "Num Lock", "Num Lock"};
    case 0x46: return (PKey){KEY_SCROLL_LOCK, "Scroll Lock"};
    case 0x52: return (PKey){KEY_KP_0, "Numpad 0", "Numpad 0"};
    case 0x4F: return (PKey){KEY_KP_1, "Numpad 1", "Numpad 1"};
    case 0x50: return (PKey){KEY_KP_2, "Numpad 2", "Numpad 2"};
    case 0x51: return (PKey){KEY_KP_3, "Numpad 3", "Numpad 3"};
    case 0x4B: return (PKey){KEY_KP_4, "Numpad 4", "Numpad 4"};
    case 0x4C: return (PKey){KEY_KP_5, "Numpad 5", "Numpad 5"};
    case 0x4D: return (PKey){KEY_KP_6, "Numpad 6", "Numpad 6"};
    case 0x47: return (PKey){KEY_KP_7, "Numpad 7", "Numpad 7"};
    case 0x48: return (PKey){KEY_KP_8, "Numpad 8", "Numpad 8"};
    case 0x49: return (PKey){KEY_KP_9, "Numpad 9", "Numpad 9"};
    case 0x4A: return (PKey){KEY_KP_SUBTRACT, "Numpad -", "Numpad -"};
    case 0x4E: return (PKey){KEY_KP_ADD, "Numpad +", "Numpad +"};
    case 0x37: return (PKey){KEY_KP_MULTIPLY, "Numpad *", "Numpad *"};
    case 0x53: return (PKey){KEY_KP_DECIMAL, "Numpad .", "Numpad ."};
    case 0x0D: return (PKey){KEY_KP_EQUAL, "Numpad =", "Numpad ="};
  }
  return (PKey){KEY_UNKNOWN, "Unknown", "Unknown"};
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
