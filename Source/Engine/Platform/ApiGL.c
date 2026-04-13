#include "ApiGL.h"
#include "Platform/Log.h"
#include "Platform/Platform.h"

#define _GT_DECLARE_GL_FUNCTION(TType, TName) TType TName = NULL;
#define _GT_DEBUG_GL_FUNCTION(TName)          GT_ERROR("API:GL Not loaded function => " #TName);

#ifdef PLATFORM_WINDOWS
#include <windows.h>
XMACRO_GL(_GT_DECLARE_GL_FUNCTION)
#define GT_LOAD_GL_FUNCTION(TType, TName)       \
  TName = (TType)GetProcAddress(libGL, #TName); \
  if(TName == NULL) {                           \
    TName = (TType)wglGetProcAddress(#TName);   \
  } else if(TName == NULL) {                    \
    _GT_DEBUG_GL_FUNCTION(TName)                \
  }

bool ApiWindowsLoadGLFunctions() {
  HMODULE libGL = LoadLibraryA("opengl32.dll");
  if(libGL == NULL) {
    return false;
  }
  XMACRO_GL(GT_LOAD_GL_FUNCTION);
  return true;
}
#endif  // PLATFORM_WINDOWS
