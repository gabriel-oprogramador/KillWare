#ifdef PLATFORM_LINUX
#include <X11/extensions/Xrender.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <GL/glx.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/random.h>
#include <fcntl.h>

#include "Platform/Platform.h"
#include "Platform/Events.h"
#include "Platform/Log.h"
#include "ThirdParty/STB/stb_image.h"
#include "ThirdParty/STB/stb_image_resize2.h"

extern bool ApiGLLinuxLoadFunctions();
extern uint32 PlatformInitialize(ETargetPlatform TargetPlatform, ETargetRenderer TargetRenderer, const char** Args);
extern void PlatformTerminate();
extern void PlatformUpdate();

// InputRaw.c
extern bool ApiXInput2Init(Display* Display);
extern void ApiXInput2UpdateKeyMap();
extern void ApiXInput2PollCoreEvent(XEvent* Event);
extern void ApiXInput2PollRawEvent(XEvent* Event);

typedef struct PWindow {
  cstring title;
  Window xWindow;
  Atom wmDelete;
  uint32 width;
  uint32 height;
  int32 lastMousePosX;
  int32 lastMousePosY;
  bool bFocused;
  bool bFullscreen;
  bool bShouldClose;
  bool bNeedsResize;
  bool bMouseCaptured;
} PWindow;

static struct {
  char userDataPath[GT_MAX_FULLPATH];
  char workPath[GT_MAX_FULLPATH];
  PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT;
  XVisualInfo* visualInfo;
  Display* display;
  PWindow* mainWindow;
  Window rootWindow;
  GLXContext context;
  Cursor invisibleCursor;
  int32 xkbEventCode;
  int32 xiOpcode;
  bool bMouseCaptured;
  bool bVsync;
} SPS;

static PWindow* InternalCreateWindow(uint32 Width, uint32 Height, cstring Title);
static void InternalDestroyWindow(PWindow* Self);
static void InternalUpdateWindow(PWindow* Self);
static void InternalCaptureMouse(PWindow* Self, bool bCapture);
static void InternalWindowSetIcon(PWindow* Self, cstring FilePath);
static int32 InternalScoreFBConfig(Display* dpy, GLXFBConfig cfg);

int32 NativeMain(int argc, const char** argv) {
  GT_INFO("Work Path -> %s", SPS.workPath);
  GT_INFO("User Path -> %s", SPS.userDataPath);
  PlatformInitialize(TARGET_PLATFORM_LINUX, TARGET_RENDERER_GL_CORE_33, argv);
  InternalWindowSetIcon(SPS.mainWindow, "Content/Logo.png");
  XMapWindow(SPS.display, SPS.mainWindow->xWindow);
  while(!SPS.mainWindow->bShouldClose) {
    InternalUpdateWindow(SPS.mainWindow);
    PlatformUpdate();
    glXSwapBuffers(SPS.display, SPS.mainWindow->xWindow);
  }
  PlatformTerminate();
  InternalDestroyWindow(SPS.mainWindow);
  return 0;
}

int main(int argc, const char** argv) {
  char base[GT_MAX_PATH] = {0};
  const char* xdg = getenv("XDG_DATA_HOME");
  if(xdg && xdg[0] != '\0') {
    snprintf(base, sizeof(base), "%s", xdg);
  } else {
    const char* home = getenv("HOME");
    if(home) {
      snprintf(base, sizeof(base), "%s/.local/share", home);
    }
  }
  snprintf(SPS.userDataPath, sizeof(base), "%s/%s", base, STR(GAME_NAME));

#ifdef DEVELOPMENT_MODE
  char buffer[GT_MAX_FULLPATH] = "";
  snprintf(buffer, sizeof(buffer), "%s", STR(PROJECT_PATH));
  chdir(buffer);
#elif SHIPPING_MODE
  char buffer[GT_MAX_FULLPATH] = "";
  ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
  if(len != -1) {
    buffer[len] = '\0';
    char* dir = dirname(buffer);
    chdir(dir);
  }
#endif  // DEVELOPMENT_MODE
  snprintf(SPS.workPath, sizeof(buffer), "%s", buffer);
  return NativeMain(argc, argv);
}

cstring PGetUserDataPath() {
  return SPS.userDataPath;
}

bool PGetRandomBytes(void* OutBuffer, uint64 Size) {
  uint8_t* p = (uint8_t*)OutBuffer;
  while(Size > 0) {
    ssize_t r = getrandom(p, Size, 0);
    if(r > 0) {
      p += r;
      Size -= (size_t)r;
      continue;
    }
    // fallback: /dev/urandom
    int fd = open("/dev/urandom", O_RDONLY);
    if(fd < 0) {
      return false;
    }
    ssize_t rr = read(fd, p, Size);
    close(fd);
    return rr == (ssize_t)Size;
  }
  return true;
}

void PWindowInit(uint32 Width, uint32 Height, cstring Title) {
  Display* dpy = XOpenDisplay(NULL);
  Window root = DefaultRootWindow(dpy);
  int32 scr = DefaultScreen(dpy);
  XkbSetDetectableAutoRepeat(dpy, true, NULL);

  int32 xkbOpcode, xkbEvent, xkbError;
  int32 major = XkbMajorVersion;
  int32 minor = XkbMinorVersion;
  if(XkbQueryExtension(dpy, &xkbOpcode, &xkbEvent, &xkbError, &major, &minor)) {
    SPS.xkbEventCode = xkbEvent;
    XkbSelectEventDetails(dpy, XkbUseCoreKbd, XkbStateNotify, XkbAllStateComponentsMask, XkbGroupStateMask);
  }

  int32 opcode, event, error;
  if(XQueryExtension(dpy, "XInputExtension", &opcode, &event, &error)) {
    int32 xiMajor = 2;
    int32 xiMinor = 0;
    if(XIQueryVersion(dpy, &xiMajor, &xiMinor) == Success) {
      SPS.xiOpcode = opcode;
      ApiXInput2Init(dpy);
    }
  }
  ApiXInput2UpdateKeyMap();

  Pixmap noData;
  XColor black;
  char noDataBits[] = {0};
  noData = XCreateBitmapFromData(dpy, root, noDataBits, 1, 1);
  black.red = black.green = black.blue = 0;
  SPS.invisibleCursor = XCreatePixmapCursor(dpy, noData, noData, &black, &black, 0, 0);
  XFreePixmap(dpy, noData);

  uint32 glMajor = 3;
  uint32 glMinor = 3;
  uint32 colorBits = 8;
  uint32 depthBits = 24;

  // clang-format off
    int32 fbAttribs[] = {
    GLX_X_RENDERABLE, True,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
    GLX_STENCIL_SIZE, colorBits,
    GLX_RED_SIZE, colorBits,
    GLX_GREEN_SIZE, colorBits,
    GLX_BLUE_SIZE, colorBits,
    GLX_ALPHA_SIZE, colorBits,
    GLX_DEPTH_SIZE, depthBits,
    GLX_DOUBLEBUFFER, True,
    None
  };

  int32 glxFlags = 0;
#ifdef DEVELOPMENT_MODE
    glxFlags |= GLX_CONTEXT_DEBUG_BIT_ARB;
#endif

  int32 contextAttribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, glMajor,
    GLX_CONTEXT_MINOR_VERSION_ARB, glMinor,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    GLX_CONTEXT_FLAGS_ARB, glxFlags,
    None
  };
  // clang-format on

  int32 fbCount = 0;
  GLXFBConfig* fbConfigs = glXChooseFBConfig(dpy, scr, fbAttribs, &fbCount);
  GT_ASSERT(fbConfigs && fbCount);

  GLXFBConfig bestConfig = NULL;
  XVisualInfo* visualInfo = NULL;
  int bestScore = -1;

  for(int32 c = 0; c < fbCount; c++) {
    XVisualInfo* vi = glXGetVisualFromFBConfig(dpy, fbConfigs[c]);
    if(!vi) {
      continue;
    }
    XRenderPictFormat* fmt = XRenderFindVisualFormat(dpy, vi->visual);
    int alphaOk = (fmt && fmt->direct.alphaMask > 0);
    if(!alphaOk) {
      XFree(vi);
      continue;
    }
    int score = InternalScoreFBConfig(dpy, fbConfigs[c]);
    if(score > bestScore) {
      bestScore = score;
      bestConfig = fbConfigs[c];
      if(visualInfo) {
        XFree(visualInfo);
      }
      visualInfo = vi;
    } else {
      XFree(vi);
    }
  }

  if(visualInfo == NULL) {
    for(int32 c = 0; c < fbCount; c++) {
      visualInfo = glXGetVisualFromFBConfig(dpy, fbConfigs[c]);
      if(visualInfo) {
        bestConfig = fbConfigs[c];
        break;
      }
    }
  }
  XFree(fbConfigs);
  GT_ASSERT(visualInfo);

  PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = (void*)glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");
  PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (void*)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
  GT_ASSERT(glXCreateContextAttribsARB);

  GLXContext xContext = glXCreateContextAttribsARB(dpy, bestConfig, NULL, True, contextAttribs);
  GT_ASSERT(xContext);

  SPS.context = xContext;
  SPS.display = dpy;
  SPS.visualInfo = visualInfo;
  SPS.rootWindow = root;
  SPS.glXSwapIntervalEXT = glXSwapIntervalEXT;
  SPS.mainWindow = InternalCreateWindow(Width, Height, Title);
  glXMakeCurrent(dpy, SPS.mainWindow->xWindow, SPS.context);
  ApiGLLinuxLoadFunctions();

  if(SPS.bVsync) {
    SPS.glXSwapIntervalEXT(SPS.display, SPS.mainWindow->xWindow, (SPS.bVsync) ? 1 : 0);
  }
}

void PWindowClose() {
  SPS.mainWindow->bShouldClose = true;
}

bool PWindowIsVsync() {
  return SPS.bVsync;
}

void PWindowSetVsync(bool bEnable) {
  if(SPS.glXSwapIntervalEXT) {
    SPS.glXSwapIntervalEXT(SPS.display, SPS.mainWindow->xWindow, (bEnable) ? 1 : 0);
  }
  SPS.bVsync = bEnable;
}

bool PWindowIsMouseCaptured() {
  return SPS.mainWindow->bMouseCaptured;
}

void PWindowSetMouseCaptured(bool bCapture) {
  PWindow* pWin = SPS.mainWindow;
  if(pWin->bMouseCaptured == bCapture) {
    return;
  }
  pWin->bMouseCaptured = bCapture;
  if(bCapture) {
    int32 posX = 0;
    int32 posY = 0;
    PWindowGetMousePos(&posX, &posY);
    pWin->lastMousePosX = posX;
    pWin->lastMousePosY = posY;
  }
  InternalCaptureMouse(pWin, bCapture);
}

bool PWindowIsFullscreen() {
  return SPS.mainWindow->bFullscreen;
}

void PWindowSetFullscreen(bool bFullscreen) {
  Display* dpy = SPS.display;
  Window win = SPS.mainWindow->xWindow;
  XEvent event;
  if(SPS.mainWindow->bFullscreen == bFullscreen) {
    return;
  }
  SPS.mainWindow->bFullscreen = bFullscreen;
  Atom wmState = XInternAtom(dpy, "_NET_WM_STATE", false);
  Atom wmFullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", false);
  memset(&event, 0, sizeof(event));
  event.type = ClientMessage;
  event.xclient.window = win;
  event.xclient.message_type = wmState;
  event.xclient.format = 32;
  event.xclient.data.l[0] = (bFullscreen) ? 1 : 0;
  event.xclient.data.l[1] = wmFullscreen;
  event.xclient.data.l[2] = 0;
  event.xclient.data.l[3] = 1;
  XSendEvent(dpy, DefaultRootWindow(dpy), false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

void PWindowGetMousePos(int32* OutPosX, int32* OutPosY) {
  PWindow* pWin = SPS.mainWindow;
  Window root;
  Window child;
  int32 winX = 0;
  int32 winY = 0;
  int32 rootX;
  int32 rootY;
  uint32 mask = 0;
  XQueryPointer(SPS.display, pWin->xWindow, &root, &child, &rootX, &rootY, &winX, &winY, &mask);
  if(OutPosX) {
    *OutPosX = winX;
  }
  if(OutPosY) {
    *OutPosY = winY;
  }
}

void PWindowSetMousePos(int32 PosX, int PosY) {
  XWarpPointer(SPS.display, None, SPS.mainWindow->xWindow, 0, 0, 0, 0, PosX, PosY);
}

// Internal Functions //=====================================================================================//
PWindow* InternalCreateWindow(uint32 Width, uint32 Height, cstring Title) {
  Display* dpy = SPS.display;
  Window root = SPS.rootWindow;
  int32 depth = SPS.visualInfo->depth;
  Visual* visual = SPS.visualInfo->visual;
  uint32 mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;
  uint32 wMask = ExposureMask | StructureNotifyMask | FocusChangeMask;
  uint32 kMask = PointerMotionMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask;
  int32 posX = 0;
  int32 posY = 0;

  XSetWindowAttributes attrs = {};
  attrs.colormap = XCreateColormap(dpy, root, visual, AllocNone);
  attrs.background_pixel = 0;
  attrs.border_pixel = 0;
  attrs.event_mask = wMask | kMask;
  Window win = XCreateWindow(dpy, root, posX, posY, Width, Height, 0, depth, InputOutput, visual, mask, &attrs);
  GT_ASSERT(win);

  Atom wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(dpy, win, &wmDelete, true);
  XStoreName(dpy, win, Title);

  XClassHint classHint;
  classHint.res_name = "GT-ENGINE";
  classHint.res_class = STR(GAME_NAME);
  XSetClassHint(dpy, win, &classHint);
  XFlush(dpy);

  PWindow* pWindow = (PWindow*)PMemAlloc(sizeof(PWindow));
  GT_ASSERT(pWindow);
  pWindow->width = Width;
  pWindow->height = Height;
  pWindow->title = Title;
  pWindow->xWindow = win;
  pWindow->wmDelete = wmDelete;
  pWindow->bFocused = false;
  pWindow->bFullscreen = false;
  pWindow->bNeedsResize = true;
  pWindow->bShouldClose = false;
  pWindow->bMouseCaptured = false;

  return pWindow;
}

void InternalDestroyWindow(PWindow* Self) {
  GT_ASSERT(Self);
  if(SPS.context) {
    glXMakeCurrent(SPS.display, 0, 0);
    glXDestroyContext(SPS.display, SPS.context);
  }
  if(Self->xWindow) {
    XDestroyWindow(SPS.display, Self->xWindow);
  }
  XFree(SPS.visualInfo);
  XCloseDisplay(SPS.display);
  PMemFree(Self);
}

void InternalUpdateWindow(PWindow* Self) {
  GT_ASSERT(Self);
  Display* dpy = SPS.display;
  uint32 eventCount = 0;
  XEvent event;
  while(XPending(dpy)) {
    XNextEvent(dpy, &event);
    eventCount++;

    if(event.type == SPS.xkbEventCode) {
      XkbEvent* xkbEvent = (XkbEvent*)&event;
      if(xkbEvent->any.xkb_type == XkbStateNotify) {
        ApiXInput2UpdateKeyMap();
        continue;
      }
    }

    if(Self->bFocused && event.xcookie.type == GenericEvent && event.xcookie.extension == SPS.xiOpcode) {
      ApiXInput2PollRawEvent(&event);
      continue;
    }

    switch(event.type) {
      case KeyPress:
      case KeyRelease:
      case ButtonPress:
      case ButtonRelease: {
        ApiXInput2PollCoreEvent(&event);
        break;
      }
      case ConfigureNotify: {
        int32 newW = event.xconfigure.width;
        int32 newH = event.xconfigure.height;
        if(Self->width != newW || Self->height != newH) {
          Self->width = newW;
          Self->height = newH;
          Self->bNeedsResize = true;
        }
        break;
      }
      case FocusIn: {
        Window focused;
        int32 revertTO;
        XGetInputFocus(dpy, &focused, &revertTO);
        if(Self->xWindow == focused && Self->bFocused != true) {
          Self->bFocused = true;
          InternalCaptureMouse(Self, Self->bMouseCaptured);
          PEvent pEvent = {PE_WINDOW_FOCUS};
          pEvent.windowFocus.bFocused = true;
          PEventPush(pEvent);
        }
        break;
      }
      case FocusOut: {
        Window focused;
        int32 revertTO;
        XGetInputFocus(dpy, &focused, &revertTO);
        if(Self->xWindow != focused && Self->bFocused != false) {
          Self->bFocused = false;
          InternalCaptureMouse(Self, false);
          PEvent pEvent = {PE_WINDOW_FOCUS};
          pEvent.windowFocus.bFocused = false;
          PEventPush(pEvent);
        }
        break;
      }
      case MotionNotify: {
        PEvent pEvent = {PE_INPUT_MOUSE_POS};
        pEvent.mousePos.posX = event.xmotion.x;
        pEvent.mousePos.posY = event.xmotion.y;
        PEventPush(pEvent);
        break;
      }
      case ClientMessage: {
        if(event.xclient.data.l[0] == Self->wmDelete) {
          Self->bShouldClose = true;
        }
        break;
      }
    }
  }
  if(eventCount > 0) {
    XSync(dpy, false);
    if(Self->bNeedsResize) {
      Self->bNeedsResize = false;
      PEvent pEvent = {PE_WINDOW_RESIZE};
      pEvent.windowResize.width = Self->width;
      pEvent.windowResize.height = Self->height;
      PEventPush(pEvent);
    }
  }
}

static void InternalCaptureMouse(PWindow* Self, bool bCapture) {
  Display* dpy = SPS.display;
  Window win = Self->xWindow;
  if(SPS.bMouseCaptured == bCapture) {
    return;
  }
  SPS.bMouseCaptured = bCapture;
  if(bCapture) {
    XDefineCursor(dpy, win, SPS.invisibleCursor);
    const int maxAttempts = 10;
    const useconds_t retryDelay = 10000;
    int32 res;
    int attempts = 0;
    do {
      res = XGrabPointer(dpy,
          win,
          True,
          ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
          GrabModeAsync,
          GrabModeAsync,
          win,
          None,
          CurrentTime  //
      );
      attempts++;
      if(res != GrabSuccess && attempts < maxAttempts) {
        usleep(retryDelay);
      }
    } while(res != GrabSuccess && attempts < maxAttempts);
    if(res != GrabSuccess) {
      GT_ERROR("Failed to grab pointer after %d attempts", attempts);
      SPS.bMouseCaptured = false;
    }
  } else {
    SPS.bMouseCaptured = false;
    XUndefineCursor(dpy, win);
    XUngrabPointer(dpy, CurrentTime);
    PWindowSetMousePos(Self->lastMousePosX, Self->lastMousePosY);
  }
}

static int32 InternalScoreFBConfig(Display* dpy, GLXFBConfig cfg) {
  int32 score = 0;
  int32 r, g, b, a, d, s, dbl, sampBuf, samples;

  glXGetFBConfigAttrib(dpy, cfg, GLX_RED_SIZE, &r);
  glXGetFBConfigAttrib(dpy, cfg, GLX_GREEN_SIZE, &g);
  glXGetFBConfigAttrib(dpy, cfg, GLX_BLUE_SIZE, &b);
  glXGetFBConfigAttrib(dpy, cfg, GLX_ALPHA_SIZE, &a);

  glXGetFBConfigAttrib(dpy, cfg, GLX_DEPTH_SIZE, &d);
  glXGetFBConfigAttrib(dpy, cfg, GLX_STENCIL_SIZE, &s);

  glXGetFBConfigAttrib(dpy, cfg, GLX_DOUBLEBUFFER, &dbl);
  glXGetFBConfigAttrib(dpy, cfg, GLX_SAMPLE_BUFFERS, &sampBuf);
  glXGetFBConfigAttrib(dpy, cfg, GLX_SAMPLES, &samples);

  // base quality
  if(r == 8 && g == 8 && b == 8) {
    score += 10;
  }
  if(a >= 8) {
    score += 20;
  }

  // depth quality
  if(d >= 24) {
    score += 10;
  }
  if(s >= 8) {
    score += 5;
  }
  // double buffer
  if(dbl) {
    score += 10;
  }
  // MSAA
  if(sampBuf) {
    score += samples;
  }
  return score;
}

static void InternalWindowSetIcon(PWindow* Self, cstring FilePath) {
  if(Self == NULL || !FilePath) {
    return;
  }
  int32 srcW, srcH, channels;
  uint8* src = stbi_load(FilePath, &srcW, &srcH, &channels, 4);
  if(!src) {
    return;
  }
  char* path = strstr(FilePath, STR(PROJECT_NAME));
  if(!path) {
    path = (char*)FilePath;
  }
  GT_INFO("API:X11 App Icon loaded -> %s", path);

  const int32 iconSizes[] = {16, 32, 64, 128};
  const int32 sizeCount = sizeof(iconSizes) / sizeof(iconSizes[0]);
  int64 totalPixels = 0;
  for(int32 i = 0; i < sizeCount; i++) {
    totalPixels += iconSizes[i] * iconSizes[i] + 2;
  }

  uint64* iconData = (uint64*)PMemAlloc(totalPixels * sizeof(uint64));
  if(!iconData) {
    stbi_image_free(src);
    return;
  }

  int64 offset = 0;
  for(int32 i = 0; i < sizeCount; i++) {
    int32 dstW = iconSizes[i];
    int32 dstH = iconSizes[i];
    uint8* dst = (uint8*)PMemAlloc(dstW * dstH * 4);
    if(!dst) {
      continue;
    }
    stbir_resize_uint8_linear(src, srcW, srcH, 0, dst, dstW, dstH, 0, 4);
    iconData[offset++] = dstW;
    iconData[offset++] = dstH;
    for(int32 y = 0; y < dstH; y++) {
      for(int32 x = 0; x < dstW; x++) {
        uint8 r = dst[(y * dstW + x) * 4 + 0];
        uint8 g = dst[(y * dstW + x) * 4 + 1];
        uint8 b = dst[(y * dstW + x) * 4 + 2];
        uint8 a = dst[(y * dstW + x) * 4 + 3];
        iconData[offset++] = ((uint64)a << 24) | ((uint64)r << 16) | ((uint64)g << 8) | (uint64)b;
      }
    }
    PMemFree(dst);
  }

  Display* dpy = SPS.display;
  Atom net_wm_icon = XInternAtom(dpy, "_NET_WM_ICON", False);
  Atom cardinal = XInternAtom(dpy, "CARDINAL", False);
  XChangeProperty(dpy, Self->xWindow, net_wm_icon, cardinal, 32, PropModeReplace, (uint8*)iconData, (int)totalPixels);
  PMemFree(iconData);
  stbi_image_free(src);
}

// Log //=====================================================================================================//
extern void PlatformLog(ELogLevel Level, cstring FuncName, cstring Context, cstring Format, ...) {
  cstring logTag = "";
  cstring logColor = "";
  char buffer[GT_LOG_BUFFER] = "";
  uint64 offset = 0;
  va_list args;

  if(Format == NULL) {
    fprintf(stderr, "Log Format invalid\n");
    return;
  }

  switch(Level) {
    case LOG_INFO: {
      logTag = "[LOG INFO]";
      logColor = "\e[97m";
    } break;
    case LOG_ALERT: {
      logTag = "[LOG ALERT]";
      logColor = "\e[93m";
    } break;
    case LOG_SUCCESS: {
      logTag = "[LOG SUCCESS]";
      logColor = "\e[32m";
    } break;
    case LOG_WARNING: {
      logTag = "[LOG WARNING]";
      logColor = "\e[33m";
    } break;
    case LOG_ERROR: {
      logTag = "[LOG ERROR]";
      logColor = "\e[91m";
    } break;
    case LOG_FATAL: {
      logTag = "[LOG FATAL]";
      logColor = "\e[31m";
    } break;
  }

  if(Level != LOG_INFO && FuncName != NULL) {
    offset = snprintf(buffer, sizeof(buffer) - offset, "%s%s %s() => ", logColor, logTag, FuncName);
  } else {
    offset = snprintf(buffer, sizeof(buffer) - offset, "%s%s => ", logColor, logTag);
  }

  va_start(args, Format);
  offset += vsnprintf(buffer + offset, sizeof(buffer) - offset, Format, args);
  va_end(args);

  if(Level != LOG_INFO && Level != LOG_ALERT && Context != NULL) {  //
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, " -> %s%s", Context, "\e[0m");
  } else {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", "\e[m");
  }

  puts(buffer);
}

#endif  // PLATFORM_LINUX
