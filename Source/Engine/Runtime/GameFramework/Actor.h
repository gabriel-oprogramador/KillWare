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

GT_EXTERN_C_BEGIN

ENGINE_API bool ActorIsValid(AActor Self);
ENGINE_API FName ActorGetDisplayName(AActor Self);

GT_EXTERN_C_END
