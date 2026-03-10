#pragma once
#include "CoreTypes.h"
#include "Defines.h"

#define VEC_PRINTF_BUFFERS 4
#define VEC_PRINTF_SIZE    64
#define NO_EXPAND(A)       #A
#define STR(A)             NO_EXPAND(A)
#define LOG_CONTEXT        STR(__FILE__) ":" STR(__LINE__)

#ifdef DEVELOPMENT_MODE
#define GT_INFO(Format, ...)    PlatformLog(LOG_INFO, __func__, LOG_CONTEXT, Format, ##__VA_ARGS__)
#define GT_ALERT(Format, ...)   PlatformLog(LOG_ALERT, __func__, LOG_CONTEXT, Format, ##__VA_ARGS__)
#define GT_SUCCESS(Format, ...) PlatformLog(LOG_SUCCESS, __func__, LOG_CONTEXT, Format, ##__VA_ARGS__)
#define GT_ERROR(Format, ...)   PlatformLog(LOG_ERROR, __func__, LOG_CONTEXT, Format, ##__VA_ARGS__)
#define GT_WARNING(Format, ...) PlatformLog(LOG_WARNING, __func__, LOG_CONTEXT, Format, ##__VA_ARGS__)
#define GT_FATAL(Format, ...)   PlatformLog(LOG_FATAL, __func__, LOG_CONTEXT, Format, ##__VA_ARGS__)
#else
#define GT_INFO(Format, ...)
#define GT_ALERT(Format, ...)
#define GT_SUCCESS(Format, ...)
#define GT_ERROR(Format, ...)
#define GT_WARNING(Format, ...)
#define GT_FATAL(Format, ...)
#endif  // DEVELOPMENT_MODE

#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#elif defined(__clang__)
#define DEBUG_BREAK() __builtin_trap()
#elif defined(__GNUC__)
#define DEBUG_BREAK() __builtin_trap()
#else
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif

#define GT_ASSERT(Expr)                           \
  do {                                            \
    if(!(Expr)) {                                 \
      GT_FATAL("Assertion Failure: [%s]", #Expr); \
      DEBUG_BREAK();                              \
    }                                             \
  } while(0)

typedef enum ELogLevel {
  LOG_INFO,     //
  LOG_ALERT,    //
  LOG_WARNING,  //
  LOG_ERROR,    //
  LOG_FATAL,    //
  LOG_SUCCESS   //
} ELogLevel;

GT_EXTERN_C void PlatformLog(ELogLevel Level, cstring FuncName, cstring Context, cstring Format, ...);
