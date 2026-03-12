#include "Actor.h"
#include "World/World.h"

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
