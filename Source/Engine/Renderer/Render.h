#pragma once
#include "Core/Types.h"
#include "Core/Defines.h"

typedef struct FViewport {
  uint32 posX;
  uint32 posY;
  uint32 width;
  uint32 height;
} FViewport;

GT_EXTERN_C_BEGIN

ENGINE_API void RenderClear();
ENGINE_API void RenderSetClearColor(FColor Color);
ENGINE_API void RenderSetViewport(FViewport Viewport);

GT_EXTERN_C_END
