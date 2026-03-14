#pragma once
#include "Core/Defines.h"
#include "Core/EngineTypes.h"

typedef struct FWorld FWorld;
typedef struct FChunkStorage FChunkStorage;
typedef struct FArchetype FArchetype;

typedef struct FActorEntry {
  FChunkStorage* chunk;
  FArchetype* archetype;
  FName actorName;
  FName archName;
  uint32 chunkIndex;
  uint32 version;
  uint32 nextFree;
} FActorEntry;

GT_EXTERN_C_BEGIN

ENGINE_API AActor WorldSpawnActor(cstring Name);
ENGINE_API void WorldDestroyActor(AActor Actor);
ENGINE_API AActor WorldFindActorByName(FName ActorName);
ENGINE_API FActorEntry* WorldGetActorEntryByActor(AActor Actor);
ENGINE_API FActorEntry* WorldGetActorEntryByName(FName ActorName);

GT_EXTERN_C_END
