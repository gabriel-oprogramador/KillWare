#pragma once
#include <stdint.h>

#define ENGINE_API
#define GAME_API
#define INITIAL_WINDOW_ASPECT 1.7777f
#define INITIAL_WINDOW_WIDTH  800
#define INITIAL_WINDOW_HEIGHT ((int32)(INITIAL_WINDOW_WIDTH / INITIAL_WINDOW_ASPECT))

#define NO_EXPAND(A)      #A
#define STR(A)            NO_EXPAND(A)
#define GT_MAX_PATH       260
#define GT_MAX_FILENAME   64
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

#if defined(_MSC_VER)
#include <intrin.h>
#define BIT_CTZ64(x) BitCtz64(x)
#else
#define BIT_CTZ64(x) __builtin_ctzll(x)
#endif

#if defined(_MSC_VER)
#include <intrin.h>
static inline uint32_t GT_BIT_CTZ64(uint64_t x) {
  unsigned long index;
  _BitScanForward64(&index, x);
  return (uint32_t)index;
}
#else
static inline uint32_t GT_BIT_CTZ64(uint64_t x) {
  return (uint32_t)__builtin_ctzll(x);
}
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

#if defined(__cplusplus)
#define GT_ALIGNAS(x) alignas(x)
#define GT_ALIGNOF(x) alignof(x)
#else
#define GT_ALIGNAS(x) _Alignas(x)
#define GT_ALIGNOF(x) _Alignof(x)
#endif

#if defined(_MSC_VER)
#include <malloc.h>
#define GT_ALIGNED_ALLOC(size, align) _aligned_malloc((size), (align))
#define GT_ALIGNED_FREE(ptr)          _aligned_free(ptr)
#else
#define GT_ALIGNED_ALLOC(size, align) aligned_alloc((align), ALIGN_FORWARD((size), (align)))
#define GT_ALIGNED_FREE(ptr)          free(ptr)
#endif
#define GT_ALIGN_FORWARD(x, a)     (((x) + ((a) - 1)) & ~((a) - 1))
#define GT_ALIGN_FORWARD_PTR(p, a) (void*)((((uintptr_t)(p)) + ((a) - 1)) & ~((uintptr_t)((a) - 1)))

#define MAP_1(F, X)       F(X)
#define MAP_2(F, X, ...)  F(X) MAP_1(F, __VA_ARGS__)
#define MAP_3(F, X, ...)  F(X) MAP_2(F, __VA_ARGS__)
#define MAP_4(F, X, ...)  F(X) MAP_3(F, __VA_ARGS__)
#define MAP_5(F, X, ...)  F(X) MAP_4(F, __VA_ARGS__)
#define MAP_6(F, X, ...)  F(X) MAP_5(F, __VA_ARGS__)
#define MAP_7(F, X, ...)  F(X) MAP_6(F, __VA_ARGS__)
#define MAP_8(F, X, ...)  F(X) MAP_7(F, __VA_ARGS__)
#define MAP_9(F, X, ...)  F(X) MAP_8(F, __VA_ARGS__)
#define MAP_10(F, X, ...) F(X) MAP_9(F, __VA_ARGS__)
#define MAP_11(F, X, ...) F(X) MAP_10(F, __VA_ARGS__)
#define MAP_12(F, X, ...) F(X) MAP_11(F, __VA_ARGS__)
#define MAP_13(F, X, ...) F(X) MAP_12(F, __VA_ARGS__)
#define MAP_14(F, X, ...) F(X) MAP_13(F, __VA_ARGS__)
#define MAP_15(F, X, ...) F(X) MAP_14(F, __VA_ARGS__)
#define MAP_16(F, X, ...) F(X) MAP_15(F, __VA_ARGS__)

#define MAP_TOO_MANY_ARGS(...) static_assert(0, "MAP supports up to 16 arguments")

#define GET_MAP(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, NAME, ...) NAME

#define MAP(F, ...)                                                                                                                                              \
  GET_MAP(__VA_ARGS__, MAP_TOO_MANY_ARGS, MAP_16, MAP_15, MAP_14, MAP_13, MAP_12, MAP_11, MAP_10, MAP_9, MAP_8, MAP_7, MAP_6, MAP_5, MAP_4, MAP_3, MAP_2, MAP_1) \
  (F, __VA_ARGS__)

#define MAP2_1(F, S, X)      F(S, X)
#define MAP2_2(F, S, X, ...) F(S, X) MAP2_1(F, S, __VA_ARGS__)
#define MAP2_3(F, S, X, ...) F(S, X) MAP2_2(F, S, __VA_ARGS__)

#define GET_MAP2(_1, _2, _3, NAME, ...) NAME

#define MAP2(F, S, ...) GET_MAP2(__VA_ARGS__, MAP2_3, MAP2_2, MAP2_1)(F, S, __VA_ARGS__)
