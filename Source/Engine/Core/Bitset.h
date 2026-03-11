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

// 1️⃣ Testa se **um bit específico** está setado
static inline bool BitsetHas(const FBitset* self, uint32_t bitIndex) {
  GT_ASSERT(bitIndex < GT_BITSET_MAX_INDEX);
  if(bitIndex >= GT_BITSET_MAX_INDEX) {
    return false;
  }
  uint32 word = bitIndex / 64;
  uint32 bit = bitIndex % 64;
  bool result = false;
  switch(word) {
    case 0: result = (self->a & ((uint64)1 << bit)) != 0; break;
    case 1: result = (self->b & ((uint64)1 << bit)) != 0; break;
    case 2: result = (self->c & ((uint64)1 << bit)) != 0; break;
    case 3: result = (self->d & ((uint64)1 << bit)) != 0; break;
  }
  return result;
}

// 2️⃣ Testa se **todos os bits da máscara estão setados**
static inline bool BitsetHasAll(const FBitset* self, const FBitset* mask) {
  bool aOk = (self->a & mask->a) == mask->a;
  bool bOk = (self->b & mask->b) == mask->b;
  bool cOk = (self->c & mask->c) == mask->c;
  bool dOk = (self->d & mask->d) == mask->d;
  return aOk && bOk && cOk && dOk;
}

// 3️⃣ Testa se **qualquer bit da máscara está setado**
static inline bool BitsetHasAny(const FBitset* self, const FBitset* mask) {
  bool aAny = (self->a & mask->a) != 0;
  bool bAny = (self->b & mask->b) != 0;
  bool cAny = (self->c & mask->c) != 0;
  bool dAny = (self->d & mask->d) != 0;
  return aAny || bAny || cAny || dAny;
}

// 4️⃣ Testa se **nenhum bit da máscara está setado**
static inline bool BitsetHasNone(const FBitset* self, const FBitset* mask) {
  bool aNone = (self->a & mask->a) == 0;
  bool bNone = (self->b & mask->b) == 0;
  bool cNone = (self->c & mask->c) == 0;
  bool dNone = (self->d & mask->d) == 0;
  return aNone && bNone && cNone && dNone;
}

// 5️⃣ Testa se **Self é exatamente igual à Mask**
static inline bool BitsetEquals(const FBitset* self, const FBitset* mask) {
  bool aEq = self->a == mask->a;
  bool bEq = self->b == mask->b;
  bool cEq = self->c == mask->c;
  bool dEq = self->d == mask->d;
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
