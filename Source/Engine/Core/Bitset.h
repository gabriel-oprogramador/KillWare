#pragma once
#include "CoreTypes.h"
#include "Core/Log.h"

#define GT_BITSET_MAX_INDEX 256

static inline void BitsetClear(FBitset* Self) {
  Self->a = Self->b = Self->c = Self->d = 0;
}

static inline void BitsetSet(FBitset* Self, uint32 Index) {
  GT_ASSERT(Index < GT_BITSET_MAX_INDEX);
  if(Index >= GT_BITSET_MAX_INDEX) {
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
  GT_ASSERT(Index < GT_BITSET_MAX_INDEX);
  if(Index >= GT_BITSET_MAX_INDEX) {
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

// Testa se **um bit específico** está setado
static inline bool BitsetHas(const FBitset* Self, uint32 BitIndex) {
  GT_ASSERT(BitIndex < GT_BITSET_MAX_INDEX);
  if(BitIndex >= GT_BITSET_MAX_INDEX) {
    return false;
  }
  uint32 word = BitIndex / 64;
  uint32 bit = BitIndex % 64;
  bool result = false;
  switch(word) {
    case 0: result = (Self->a & ((uint64)1 << bit)) != 0; break;
    case 1: result = (Self->b & ((uint64)1 << bit)) != 0; break;
    case 2: result = (Self->c & ((uint64)1 << bit)) != 0; break;
    case 3: result = (Self->d & ((uint64)1 << bit)) != 0; break;
  }
  return result;
}

// Testa se **todos os bits da máscara estão setados**
static inline bool BitsetHasAll(const FBitset* Self, const FBitset* Mask) {
  bool aOk = (Self->a & Mask->a) == Mask->a;
  bool bOk = (Self->b & Mask->b) == Mask->b;
  bool cOk = (Self->c & Mask->c) == Mask->c;
  bool dOk = (Self->d & Mask->d) == Mask->d;
  return aOk && bOk && cOk && dOk;
}

// Testa se **qualquer bit da máscara está setado**
static inline bool BitsetHasAny(const FBitset* Self, const FBitset* Mask) {
  bool aAny = (Self->a & Mask->a) != 0;
  bool bAny = (Self->b & Mask->b) != 0;
  bool cAny = (Self->c & Mask->c) != 0;
  bool dAny = (Self->d & Mask->d) != 0;
  return aAny || bAny || cAny || dAny;
}

// Testa se **nenhum bit da máscara está setado**
static inline bool BitsetHasNone(const FBitset* Self, const FBitset* Mask) {
  bool aNone = (Self->a & Mask->a) == 0;
  bool bNone = (Self->b & Mask->b) == 0;
  bool cNone = (Self->c & Mask->c) == 0;
  bool dNone = (Self->d & Mask->d) == 0;
  return aNone && bNone && cNone && dNone;
}

// Testa se **Self é exatamente igual à Mask**
static inline bool BitsetEquals(const FBitset* Self, const FBitset* Mask) {
  bool aEq = Self->a == Mask->a;
  bool bEq = Self->b == Mask->b;
  bool cEq = Self->c == Mask->c;
  bool dEq = Self->d == Mask->d;
  return aEq && bEq && cEq && dEq;
}

static inline void BitsetFlip(FBitset* Self, uint32_t Index) {
  GT_ASSERT(Index < GT_BITSET_MAX_INDEX);
  if(Index >= GT_BITSET_MAX_INDEX) {
    return;
  }
  uint32 word = Index / 64;
  uint32 bit = Index % 64;
  switch(word) {
    case 0: Self->a ^= ((uint64)1 << bit); break;
    case 1: Self->b ^= ((uint64)1 << bit); break;
    case 2: Self->c ^= ((uint64)1 << bit); break;
    case 3: Self->d ^= ((uint64)1 << bit); break;
  }
}

static inline FBitset BitsetAnd(FBitset* Self, const FBitset* Other) {
  FBitset bitset;
  bitset.a = Self->a & Other->a;
  bitset.b = Self->b & Other->b;
  bitset.c = Self->c & Other->c;
  bitset.d = Self->d & Other->d;
  return bitset;
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

static inline bool BitsetNextBitIndex(FBitset* self, uint32* outIndex) {
  uint64* words = (uint64*)self;
  for(uint32 i = 0; i < 4; i++) {
    uint64 bits = words[i];
    if(bits) {
      uint32 bit = GT_BIT_CTZ64(bits);
      *outIndex = i * 64 + bit;
      words[i] &= words[i] - 1;
      return true;
    }
  }
  return false;
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

// Debug: imprime todos os bits do FBitset
static inline void BitsetPrint(const FBitset* self, const char* name) {
  uint64 words[4] = {self->a, self->b, self->c, self->d};
  for(int wordIndex = 0; wordIndex < 4; wordIndex++) {
    char buffer[65];  // 64 bits + '\0'
    for(int32 bit = 63; bit >= 0; bit--) {
      buffer[63 - bit] = (words[wordIndex] & ((uint64)1ULL << bit)) ? '1' : '0';
    }
    buffer[64] = '\0';
    GT_ALERT("%s Word %d: %s", name, wordIndex, buffer);
  }
}
