#include "Actor.h"
#include "World/World.h"
#include "Arc/Arc.h"

bool ActorIsValid(AActor Self) {
  FActorEntry* entry = WorldGetActorEntryByActor(Self);
  return (entry != NULL);
}

FName ActorGetDisplayName(AActor Self) {
  FActorEntry* entry = WorldGetActorEntryByActor(Self);
  if(entry == NULL) {
    return 0;
  }
  return entry->actorName;
}

FName ActorGetArchetypeName(AActor Self) {
  FActorEntry* entry = WorldGetActorEntryByActor(Self);
  if(entry == NULL) {
    return 0;
  }
  return entry->archName;
}

void* ActorGetComponentByID(AActor Self, uint16 ComponentID) {
  FActorEntry* entry = WorldGetActorEntryByActor(Self);
  if(entry && BitsetHas(&entry->archetype->componentMask, ComponentID)) {
    uint32 offset = entry->archetype->componentOffset[ComponentID];
    uint32 stride = entry->archetype->componentStride[ComponentID];
    uint32 index = entry->chunkIndex;
    return entry->chunk->data + offset + index * stride;
  }
  return NULL;
}

void* ActorAddComponentByID(AActor Self, uint16 ComponentID) {
  FActorEntry* entry = WorldGetActorEntryByActor(Self);
  if(entry == NULL) {
    return NULL;
  }
  FArchetype* newArchetype = ArcAddComponent(entry->archetype, ComponentID);
  if(newArchetype && newArchetype != entry->archetype) {
    ArcMoveActorArchetype(Self, &entry->archetype->add[ComponentID]);
  }
  return ActorGetComponentByID(Self, ComponentID);
}

void ActorRemoveComponentByID(AActor Self, uint16 ComponentID) {
  FActorEntry* entry = WorldGetActorEntryByActor(Self);
  if(entry == NULL) {
    return;
  }
  FArchetype* newArchetype = ArcRemoveComponent(entry->archetype, ComponentID);
  if(newArchetype && newArchetype != entry->archetype) {
    ArcMoveActorArchetype(Self, &entry->archetype->remove[ComponentID]);
  }
}

FArchetype* ActorGetArchetype(AActor Self) {
  FActorEntry* entry = WorldGetActorEntryByActor(Self);
  if(entry == NULL) {
    return 0;
  }
  return entry->archetype;
}
