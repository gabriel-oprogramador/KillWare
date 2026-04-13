#pragma once
#include "Core/Types.h"

static inline uint32 HashFNV1a32FromStr(cstring Str) {
  if(!Str) {
    return 0;
  }
  uint32 hash = 2166136261u;
  while(*Str) {
    hash ^= (uint8)(*Str++);
    hash *= 16777619u;
  }
  hash ^= hash >> 16;
  hash *= 0x7feb352d;
  hash ^= hash >> 15;
  hash *= 0x846ca68b;
  hash ^= hash >> 16;
  return hash;
}

static inline uint64 HashFNV1a64FromStr(const char* Str) {
  uint64 hash = 1469598103934665603ULL;
  if(!Str) {
    return 0;
  }
  while(*Str) {
    hash ^= (uint8)(*Str++);
    hash *= 1099511628211ULL;
  }
  return hash;
}

static inline uint64 HashXX64FromStr(cstring Str) {
  if(!Str) {
    return 0;
  }

  const uint8* p = (const uint8*)Str;

  const uint64 PRIME1 = 0x9E3779B185EBCA87ULL;
  const uint64 PRIME2 = 0xC2B2AE3D27D4EB4FULL;
  const uint64 PRIME4 = 0x85EBCA77C2B2AE63ULL;
  const uint64 PRIME5 = 0x27D4EB2F165667C5ULL;

#define ROTL64(x, r) ((x << r) | (x >> (64 - r)))
#define AVALANCHE(h) (h ^= h >> 37, h *= 0x165667919E3779F9ULL, h ^= h >> 32)

#define MIX(acc, lane)           \
  do {                           \
    uint64 v = (lane) * PRIME2;  \
    v = ROTL64(v, 31);           \
    v *= PRIME1;                 \
    acc ^= v;                    \
    acc = acc * PRIME1 + PRIME4; \
  } while(0)

  size_t len = 0;
  while(p[len]) {
    len++;
  }

  uint64 acc = PRIME5 + len;

  size_t i = 0;

  while(i + 8 <= len) {
    uint64 lane;
    memcpy(&lane, p + i, 8);
    MIX(acc, lane);
    i += 8;
  }

  uint64 last = 0;
  switch(len - i) {
    case 7: last |= ((uint64)p[i + 6] << 48);
    case 6: last |= ((uint64)p[i + 5] << 40);
    case 5: last |= ((uint64)p[i + 4] << 32);
    case 4: last |= ((uint64)p[i + 3] << 24);
    case 3: last |= ((uint64)p[i + 2] << 16);
    case 2: last |= ((uint64)p[i + 1] << 8);
    case 1: last |= ((uint64)p[i + 0]);
    default: break;
  }

  if(last) {
    MIX(acc, last);
  }

  return (uint64)AVALANCHE(acc);
}
