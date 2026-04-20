#pragma once
#include "Platform/Defines.h"
#include <stdint.h>

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

//#if defined(_MSC_VER)
//#define FORCEINLINE static __forceinline
//#define NOINLINE    __declspec(noinline)
//#elif defined(__GNUC__) || defined(__clang__)
//#define FORCEINLINE static inline __attribute__((always_inline))
//#define NOINLINE    __attribute__((noinline))
//#else
//#define FORCEINLINE inline
//#define NOINLINE
//#endif

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
