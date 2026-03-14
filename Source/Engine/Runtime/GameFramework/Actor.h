#pragma once
#include "Core/Defines.h"
#include "Core/EngineTypes.h"

typedef struct FArchetype FArchetype;

static inline uint32 ActorIndex(AActor Actor) {
  return (uint32)(Actor & 0xffffffff);
}

static inline uint32 ActorVersion(AActor Actor) {
  return (uint32)(Actor >> 32);
}

static inline AActor ActorMake(uint32 index, uint32 version) {
  return ((uint64)version << 32) | index;
}

#define ActorGetComponent(Actor, TComponent)    (TComponent*)ActorGetComponentByID(Actor, GetUComponent(TComponent)->runtimeID)
#define ActorAddComponent(Actor, TComponent)    (TComponent*)ActorAddComponentByID(Actor, GetUComponent(TComponent)->runtimeID)
#define ActorRemoveComponent(Actor, TComponent) ActorRemoveComponentByID(Actor, GetUComponent(TComponent)->runtimeID)

GT_EXTERN_C_BEGIN

ENGINE_API bool ActorIsValid(AActor Self);
ENGINE_API FName ActorGetDisplayName(AActor Self);
ENGINE_API FName ActorGetArchetypeName(AActor Self);
ENGINE_API void* ActorGetComponentByID(AActor Self, uint16 ComponentID);
ENGINE_API void* ActorAddComponentByID(AActor Self, uint16 ComponentID);
ENGINE_API void ActorRemoveComponentByID(AActor Self, uint16 ComponentID);
ENGINE_API FArchetype* ActorGetArchetype(AActor Self);

GT_EXTERN_C_END
