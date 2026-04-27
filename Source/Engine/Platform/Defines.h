#pragma once

#define ENGINE_API
#define GAME_API
#define INITIAL_WINDOW_ASPECT 1.7777f
#define INITIAL_WINDOW_WIDTH  800
#define INITIAL_WINDOW_HEIGHT ((int32)(INITIAL_WINDOW_WIDTH / INITIAL_WINDOW_ASPECT))

#define NO_EXPAND(A)      #A
#define STR(A)            NO_EXPAND(A)
#define GT_MAX_PATH       260
#define GT_MAX_FILENAME   260
#define GT_MAX_FULLPATH   520
#define GT_MAX_DIR_DEPTH  50
#define GT_UTF8_MAX_BYTES 4
#define GT_UTF8_BUFFER    5
#define GT_BUFFER_32B     32
#define GT_BUFFER_64B     64
#define GT_BUFFER_128B    128
#define GT_BUFFER_256B    256
#define GT_BUFFER_512B    512
#define GT_BUFFER_1KB     1024
#define GT_BUFFER_2KB     2048
#define GT_BUFFER_4KB     4096
#define GT_LOG_BUFFER     2048
#define GT_BUFFER_16K     (16 * 1024)

#define GT_PRINTF_BUFFERS 8
#define GT_PRINTF_SIZE    128

// TODO: Remover para Math
#define GT_M_PI          3.14159265358979323846
#define GT_EPSILON_VALUE 1e-6f

#ifdef __cplusplus
#define GT_EXTERN_C_BEGIN extern "C" {
#define GT_EXTERN_C_END   }
#define GT_EXTERN_C       extern "C"
#define GT_CLITERAL(type) type
#else
#define GT_EXTERN_C_BEGIN
#define GT_EXTERN_C_END
#define GT_EXTERN_C       extern
#define GT_CLITERAL(type) (type)
#endif

#if defined(__cplusplus)
#define GT_THREAD_LOCAL thread_local
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define GT_THREAD_LOCAL _Thread_local
#else
#error "Thread local not supported on this compiler"
#endif

// Use only in .c or .cpp files.
#if defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
typedef void(__cdecl* FOnAutoExec)();
#define GT_AUTO_EXEC(FuncName)                                          \
  static void FuncName(void);                                           \
  __declspec(allocate(".CRT$XCU")) FOnAutoExec On##FuncName = FuncName; \
  static void FuncName(void)
#elif defined(__GNUC__) || defined(__clang__)
#define GT_AUTO_EXEC(FuncName)                             \
  static void FuncName(void) __attribute__((constructor)); \
  static void FuncName(void)
#else
#error "Compiler not supported"
#endif

#define GT_ALIGN_FORWARD(x, a)     (((x) + ((a) - 1)) & ~((a) - 1))
#define GT_ALIGN_FORWARD_PTR(p, a) (void*)((((uintptr_t)(p)) + ((a) - 1)) & ~((uintptr_t)((a) - 1)))

#if defined(__cplusplus)
#define GT_ALIGNAS(x) alignas(x)
#define GT_ALIGNOF(x) alignof(x)
#else
#define GT_ALIGNAS(x) _Alignas(x)
#define GT_ALIGNOF(x) _Alignof(x)
#endif

#if defined(_WIN32)
#include <malloc.h>
#define GT_ALIGNED_ALLOC(size, align) _aligned_malloc((size), (align))
#define GT_ALIGNED_FREE(ptr)          _aligned_free(ptr)
#else
#include <stdlib.h>
#define GT_ALIGNED_ALLOC(size, align) aligned_alloc((align), (size))
#define GT_ALIGNED_FREE(ptr)          free(ptr)
#endif
