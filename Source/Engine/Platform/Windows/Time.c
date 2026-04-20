#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <windowsx.h>

#include "Platform/Platform.h"

static struct {
  LARGE_INTEGER timerFrequency;
} SApiState;

GT_AUTO_EXEC(Init) {
  QueryPerformanceFrequency(&SApiState.timerFrequency);
}

double PGetTime() {
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / (double)SApiState.timerFrequency.QuadPart;
}

void PWait(double Seconds) {
  if(Seconds > 0.0) {
    DWORD ms = (DWORD)(Seconds * 1000.0);
    Sleep(ms);
  }
}
#endif  // PLATFORM_WINDOWS
