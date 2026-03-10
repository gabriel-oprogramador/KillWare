#pragma once
#include "CoreTypes.h"

static inline void BitsetClear(FBitset* Self) {
  Self->a = Self->b = Self->c = Self->d = 0;
}
static inline void BitsetSet(FBitset* Self, uint32 Index) {
  if(Index >= 256) {
    return;
  }
  uint32 word = Index / 64;
  uint32 bit = Index % 64;
  switch(word) {
    case 0: Self->a |= ((uint64)1 << bit); break;
    case 1: Self->b |= ((uint64)1 << bit); break;
    case 2: Self->c |= ((uint64)1 << bit); break;
    case 3: Self->d |= ((uint64)1 << bit); break;
  }
}
static inline void BitsetReset(FBitset* Self, uint32 Index) {
  if(Index >= 256) {
    return;
  }
  uint32 word = Index / 64;
  uint32 bit = Index % 64;
  switch(word) {
    case 0: Self->a &= ~((uint64)1 << bit); break;
    case 1: Self->b &= ~((uint64)1 << bit); break;
    case 2: Self->c &= ~((uint64)1 << bit); break;
    case 3: Self->d &= ~((uint64)1 << bit); break;
  }
}
static inline bool BitsetTest(const FBitset* Self, uint32 Index) {
  if(Index >= 256) {
    return false;
  }
  uint32 word = Index / 64;
  uint32 bit = Index % 64;
  switch(word) {
    case 0: return (Self->a & ((uint64)1 << bit)) != 0;
    case 1: return (Self->b & ((uint64)1 << bit)) != 0;
    case 2: return (Self->c & ((uint64)1 << bit)) != 0;
    case 3: return (Self->d & ((uint64)1 << bit)) != 0;
  }
  return false;
}
static inline void BitsetFlip(FBitset* Self, uint32_t Index) {
  if(Index >= 256) {
    return;
  }
  uint32_t word = Index / 64;
  uint32_t bit = Index % 64;
  switch(word) {
    case 0: Self->a ^= ((uint64_t)1 << bit); break;
    case 1: Self->b ^= ((uint64_t)1 << bit); break;
    case 2: Self->c ^= ((uint64_t)1 << bit); break;
    case 3: Self->d ^= ((uint64_t)1 << bit); break;
  }
}

static inline void BitsetAnd(FBitset* Self, const FBitset* Other) {
  Self->a &= Other->a;
  Self->b &= Other->b;
  Self->c &= Other->c;
  Self->d &= Other->d;
}

static inline void BitsetOr(FBitset* Self, const FBitset* Other) {
  Self->a |= Other->a;
  Self->b |= Other->b;
  Self->c |= Other->c;
  Self->d |= Other->d;
}

static inline void BitsetXor(FBitset* Self, const FBitset* Other) {
  Self->a ^= Other->a;
  Self->b ^= Other->b;
  Self->c ^= Other->c;
  Self->d ^= Other->d;
}

static inline uint32_t BitsetCount(const FBitset* Self) {
  uint32 count = 0;
  uint64 words[4] = {Self->a, Self->b, Self->c, Self->d};
  for(int32 i = 0; i < 4; i++) {
    uint64 w = words[i];
    while(w) {
      w &= (w - 1);
      count++;
    }
  }
  return count;
}
