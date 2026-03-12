#pragma once
#include "Core/Defines.h"
#include "Core/EngineTypes.h"

#define NAME(Str) (static FName name = 0, name ? name : (name = NameMake(Str)))

GT_EXTERN_C_BEGIN

ENGINE_API FName NameMake(cstring Name);
ENGINE_API uint32 NameMakeHash(cstring Name, uint32* OutLength);
ENGINE_API cstring NameToStr(FName Self);
ENGINE_API uint32 NameLength(FName Self);
ENGINE_API uint32 NameHash(FName Self);

GT_EXTERN_C_END
