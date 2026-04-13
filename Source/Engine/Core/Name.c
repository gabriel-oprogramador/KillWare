#include "Name.h"
#include "Platform/Log.h"
#include "Platform/Platform.h"

#define GT_MAX_NAME_ENTRIES 16384

typedef struct FNameEntry {
  uint32 offset;
  uint32 length;
  uint32 hash;
} FNameEntry;

typedef struct FNametable {
  char* stringPool;
  FNameEntry entries[GT_MAX_NAME_ENTRIES];
  uint32 numEntries;
  uint32 capacity;
  uint32 used;
} FNameTable;

static FNameTable SNameTable;

static inline void InternalCopyToPool(cstring Name, uint32 Length) {
  if(SNameTable.used + Length + 1 >= SNameTable.capacity) {
    SNameTable.capacity *= 2;
    SNameTable.stringPool = PMemRealloc(SNameTable.stringPool, SNameTable.capacity);
  }
  uint32 needed = Length + 1;
  char* addr = SNameTable.stringPool + SNameTable.used;
  PMemCopy((void*)Name, addr, needed);
  SNameTable.used += needed;
}

FName NameMake(cstring Name) {
  if(SNameTable.stringPool == NULL) {
    SNameTable.numEntries = 0;
    SNameTable.used = 0;
    SNameTable.capacity = GT_BUFFER_16K;
    SNameTable.stringPool = PMemAlloc(SNameTable.capacity);
  }

  GT_ASSERT(SNameTable.numEntries < GT_MAX_NAME_ENTRIES);

  uint32 length = 0;
  uint32 hash = NameMakeHash(Name, &length);
  uint32 slot = hash & (GT_MAX_NAME_ENTRIES - 1);
  uint32 dist = 0;
  FNameEntry newEntry;
  newEntry.hash = hash;
  newEntry.length = length;

  while(1) {
    FNameEntry* entry = &SNameTable.entries[slot];
    if(entry->hash == 0) {
      SNameTable.numEntries++;
      newEntry.offset = SNameTable.used;
      InternalCopyToPool(Name, length);
      *entry = newEntry;
      return (FName)slot;
    }
    cstring entryStr = SNameTable.stringPool + entry->offset;
    if(entry->hash == newEntry.hash && entry->length == newEntry.length && memcmp((void*)entryStr, (void*)Name, entry->length) == 0) {
      return slot;
    }
    uint32 ideal = entry->hash & (GT_MAX_NAME_ENTRIES - 1);
    uint32 existingDist = (slot + GT_MAX_NAME_ENTRIES - ideal) & (GT_MAX_NAME_ENTRIES - 1);
    if(existingDist < dist) {
      FNameEntry temp = *entry;
      *entry = newEntry;
      newEntry = temp;
      dist = existingDist;
    }
    slot = (slot + 1) & (GT_MAX_NAME_ENTRIES - 1);
    dist++;
  }
}

uint32 NameMakeHash(cstring Name, uint32* OutLength) {
  uint32 length = 0;
  uint32 hash = 2166136261u;
  while(*Name) {
    hash ^= (uint8)(*Name++);
    hash *= 16777619u;
    length++;
  }
  hash ^= hash >> 16;
  hash *= 0x7feb352d;
  hash ^= hash >> 15;
  hash *= 0x846ca68b;
  hash ^= hash >> 16;
  if(OutLength) {
    *OutLength = length;
  }
  return hash;
}

cstring NameToStr(FName Self) {
  GT_ASSERT(Self < GT_MAX_NAME_ENTRIES);
  FNameEntry* entry = &SNameTable.entries[Self];
  if(entry->hash == 0) {
    return "NoName";
  }
  char* str = SNameTable.stringPool + SNameTable.entries[Self].offset;
  return str;
}

uint32 NameLength(FName Self) {
  GT_ASSERT(Self < GT_MAX_NAME_ENTRIES);
  FNameEntry* entry = &SNameTable.entries[Self];
  if(entry->hash == 0) {
    return 0;
  }
  return entry->length;
}

uint32 NameHash(FName Self) {
  GT_ASSERT(Self < GT_MAX_NAME_ENTRIES);
  FNameEntry* entry = &SNameTable.entries[Self];
  if(entry->hash == 0) {
    return 0;
  }
  return entry->hash;
}
