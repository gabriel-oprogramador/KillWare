#include "World.h"
#include "Platform/Platform.h"
#include "Core/Log.h"
#include "Core/Name.h"
#include "Core/Bitset.h"
#include "Arc/Arc.h"
#include "GameFramework/Actor.h"

#define GT_INITIAL_ACTOR_CAPACITY 1024

static struct {
  FActorEntry* entries;
  uint32 count;
  uint32 capacity;
  uint32 firstFree;
  uint32 freeListCount;
} SActorTable;

AActor WorldSpawnActor(cstring Name) {
  FActorEntry* entry = NULL;
  uint32 index = 0;
  uint32 version = 0;
  if(SActorTable.entries == NULL) {
    SActorTable.count = 1;  // index 0 is reserved as invalid
    SActorTable.capacity = GT_INITIAL_ACTOR_CAPACITY;
    SActorTable.entries = (FActorEntry*)PMemAlloc(SActorTable.capacity * sizeof(FActorEntry));
    SActorTable.firstFree = 0;
    SActorTable.freeListCount = 0;
  }
  if(SActorTable.freeListCount > 0) {
    SActorTable.freeListCount--;
    index = SActorTable.firstFree;
    entry = &SActorTable.entries[index];
    version = entry->version;
    SActorTable.firstFree = entry->nextFree;
  } else {
    if(SActorTable.count >= SActorTable.capacity) {
      uint32 newCapacity = SActorTable.capacity * 2;
      FActorEntry* newEntries = PMemRealloc(SActorTable.entries, newCapacity * sizeof(FActorEntry));
      GT_ASSERT(newEntries);
      SActorTable.entries = newEntries;
      SActorTable.capacity = newCapacity;
    }
    index = SActorTable.count++;
    entry = &SActorTable.entries[index];
    version = 1;  // version 0 is reserved as invalid
  }
  FBitset emptyMask;
  BitsetClear(&emptyMask);
  AActor actor = ActorMake(index, version);
  entry->actorName = NameMake(Name);
  entry->archName = 0;
  entry->version = version;
  ArcAddEntity(actor);
  return actor;
}

void WorldDestroyActor(AActor Actor) {
  FActorEntry* entry = WorldGetActorEntryByActor(Actor);
  if(entry == NULL) {
    return;
  }

  ArcRemoveEntity(Actor);

  entry->actorName = 0;
  entry->archName = 0;
  entry->version = (++entry->version == 0) ? 1 : entry->version;
  entry->nextFree = SActorTable.firstFree;
  SActorTable.freeListCount++;
  SActorTable.firstFree = ActorIndex(Actor);
}

AActor WorldFindActorByName(FName ActorName) {
  if(ActorName == 0) {
    return 0;
  }
  FActorEntry* entry = NULL;
  for(uint32 c = 0; c < SActorTable.count; c++) {
    entry = &SActorTable.entries[c];
    if(entry->actorName == ActorName) {
      return ActorMake(c, entry->version);
    }
  }
  return 0;
}

FActorEntry* WorldGetActorEntryByActor(AActor Actor) {
  uint32 index = ActorIndex(Actor);
  uint32 version = ActorVersion(Actor);
  if(Actor == 0 || index == 0 || index >= SActorTable.count) {
    return NULL;
  }
  FActorEntry* entry = &SActorTable.entries[index];
  if(version == 0 || entry->version != version) {
    return NULL;
  }
  return entry;
}

FActorEntry* WorldGetActorEntryByName(FName ActorName) {
  if(ActorName == 0) {
    return 0;
  }
  FActorEntry* entry = NULL;
  for(uint32 c = 0; c < SActorTable.count; c++) {
    entry = &SActorTable.entries[c];
    if(entry->actorName == ActorName) {
      return entry;
    }
  }
  return NULL;
}
