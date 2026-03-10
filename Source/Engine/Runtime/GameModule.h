#pragma once
#include "CoreMinimal.h"

GT_EXTERN_C_BEGIN

// Game.c/cpp - Implements the main Game entry points
void GameStart();
void GameUpdate(float DeltaTime);
void GameStop();

// Functions exposed by the Game module
ENGINE_API void GameQuit();
ENGINE_API void GameSetFullscreen(bool bFullscreen);
ENGINE_API void GameToggleFullscreen();
ENGINE_API bool GameIsFullscreen();

GT_EXTERN_C_END
