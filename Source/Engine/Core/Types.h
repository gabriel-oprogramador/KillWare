#pragma once
#include "Platform/Types.h"

// Math Types //=============================================================================================//
typedef struct FVector2 {
  float x, y;
} FVector2;

typedef struct FVector3 {
  float x, y, z;
} FVector3;

typedef struct FVector4 {
  float x, y, z, w;
} FVector4;

typedef struct FQuat {
  float x, y, z, w;
} FQuat;

typedef struct FRotator {
  float pitch, yaw, roll;
} FRotator;

typedef struct FMatrix4 {
  float e[4][4];
} FMatrix4;

// Others Type //============================================================================================//
typedef uint64 FName;
typedef uint64 AActor;  // Entity ID

typedef struct FBitset {
  uint64 a, b, c, d;
} FBitset;

typedef struct FColor {
  uint8 r, g, b, a;
} FColor;

typedef struct FInputState {
  bool keys[KEY_MAX];
  FVector2 mouseDelta;
  FVector2 mouseScroll;
  FVector2 mousePos;
} FInputState;

// Engine Type //============================================================================================//
typedef struct FGT {
  struct {
    ETargetPlatform targetPlatform;
    ETargetRenderer targetRenderer;
  } info;
  struct {
    float startTime;
    float deltaTime;
    uint32 framerate;
  } time;
  struct {
    FInputState currentState;
    FInputState previousState;
  } input;
  struct {
    uint32 width;
    uint32 height;
    float aspect;
  } windowInfo;
} FGT;
