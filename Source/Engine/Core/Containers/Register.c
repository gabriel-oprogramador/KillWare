#include "Register.h"
#include "Core/Log.h"
#include "Platform/Platform.h"

#define INITIAL_LIST_CAPACITY 256

struct TRegister {
  void** list;
  uint32 count;
  uint32 capacity;
};

TRegister* RegisterNew(uint64 InitialCapacity) {
  TRegister* reg = (TRegister*)PMemAlloc(sizeof(TRegister));
  GT_ASSERT(reg);
  reg->capacity = (InitialCapacity > 0) ? InitialCapacity : INITIAL_LIST_CAPACITY;
  reg->count = 0;
  reg->list = (void**)PMemAlloc(reg->capacity * sizeof(void*));
  GT_ASSERT(reg->list);
  return reg;
}

void RegisterFree(TRegister* Self) {
  GT_ASSERT(Self && Self->list);
  PMemFree(Self->list);
  PMemFree(Self);
}

uint32 RegisterGetCount(TRegister* Self) {
  GT_ASSERT(Self);
  return Self->count;
}

uint32 RegisterPush(TRegister* Self, void* Iten) {
  GT_ASSERT(Self && Iten);
  if(Self->count >= Self->capacity) {
    uint32 newCap = (Self->capacity <= 0) ? INITIAL_LIST_CAPACITY : Self->capacity * 2;
    void** newList = PMemRealloc(Self->list, newCap * sizeof(void*));
    GT_ASSERT(newList);
    Self->list = newList;
    Self->capacity = newCap;
  }
  uint32 idx = Self->count++;
  Self->list[idx] = Iten;
  return idx;
}

void* RegisterGet(TRegister* Self, uint32 runtimeID) {
  GT_ASSERT(Self && runtimeID < Self->count);
  return Self->list[runtimeID];
}
