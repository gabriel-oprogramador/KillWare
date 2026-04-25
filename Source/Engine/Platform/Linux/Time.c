#ifdef PLATFORM_LINUX
#include "time.h"

#include "Platform/Platform.h"

double PGetTime() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

void PWait(double Seconds) {
  struct timespec req;
  req.tv_sec = (time_t)Seconds;
  req.tv_nsec = (long)((Seconds - req.tv_sec) * 1e9);
  nanosleep(&req, NULL);
}

#endif  // PLATFORM_LINUX
