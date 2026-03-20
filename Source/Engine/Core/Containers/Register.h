#pragma once
#include "Core/Defines.h"
#include "Core/EngineTypes.h"

/**
 * @struct TRegister
 * @brief Generic container that stores pointers in a contiguous array.
 *
 * TRegister holds an ordered list of pointers, growing automatically when capacity is exceeded.
 * It does not manage the memory of the objects it stores. Ideal for permanent assets, archetypes,
 * or systems that are registered once and never removed.
 */

GT_EXTERN_C_BEGIN

ENGINE_API TRegister* RegisterNew(uint64 InitialCapacity);
ENGINE_API void RegisterFree(TRegister* Self);
ENGINE_API uint32 RegisterGetCount(TRegister* Self);
ENGINE_API uint32 RegisterPush(TRegister* Self, void* Iten);
ENGINE_API void* RegisterGet(TRegister* Self, uint32 runtimeID);

GT_EXTERN_C_END
