#pragma once
#include "Core/EngineTypes.h"
#include "Core/Defines.h"
#include "Core/Log.h"

typedef struct {
  cstring typeName;
  uint64 typeSize;
  uint64 typeHash;
  uint64 typeAlign;
} FTypeInfo;

typedef struct FProperty {
  FTypeInfo info;
  cstring name;
  uint64 offset;
  struct FProperty* next;
} FProperty;

typedef struct {
  FTypeInfo info;
  FProperty* propList;
  uint32 runtimeID;
} UComponent;

#define ARC_CACHELINE_SIZE   64
#define ARC_SIMD_ALIGNMENT   16
#define ARC_GEN_COUNT(X)     +1
#define ARC_GEN_COMPONENT(X) components[index++] = GetUComponent(X);
#define GetUComponent(Type)  Type##GetUComponent()

#define DECLARE_COMPONENT(Name)                  \
  typedef struct Name Name;                      \
  GT_EXTERN_C UComponent* Name##GetUComponent(); \
  struct Name

#define UPROPERTY(TComponent, Type, Name)           \
  do {                                              \
    static FProperty Name = {};                     \
    Name.info.typeName = #Type;                     \
    Name.info.typeSize = sizeof(Type);              \
    Name.info.typeHash = HashFNV1a64FromStr(#Type); \
    Name.info.typeAlign = GT_ALIGNOF(Type);         \
    UComponent* comp = TComponent##GetUComponent(); \
    FProperty** it = &comp->propList;               \
    while(*it)                                      \
      it = &(*it)->next;                            \
    *it = &Name;                                    \
  } while(0);

#define REGISTER_COMPONENT(Type)                      \
  UComponent* Type##GetUComponent() {                 \
    static UComponent comp = {};                      \
    if(comp.info.typeSize == 0) {                     \
      comp.info.typeName = #Type;                     \
      comp.info.typeSize = sizeof(Type);              \
      comp.info.typeAlign = GT_ALIGNOF(Type);         \
      comp.info.typeHash = HashFNV1a64FromStr(#Type); \
    }                                                 \
    return &comp;                                     \
  }                                                   \
  GT_AUTO_EXEC(AutoRef##Type)

#define DECLARE_ARCHETYPE(Name, ...)                                                                     \
  GT_AUTO_EXEC(AutoRegArch##Name) {                                                                      \
    static UComponent* components[MAP(ARC_GEN_COUNT, __VA_ARGS__)];                                      \
    const uint32 numComponents = sizeof(components) / sizeof(components[0]);                             \
    uint32 index = 0;                                                                                    \
    MAP(ARC_GEN_COMPONENT, __VA_ARGS__);                                                                 \
    for(uint32 c = 0; c < numComponents; c++) {                                                          \
      GT_ALERT("Add => Name:%s, Size:%llu", components[c]->info.typeName, components[c]->info.typeSize); \
    }                                                                                                    \
  }

// Para Teste
DECLARE_COMPONENT(UPlayer) {
  FVector3 location;
  FVector3 rotation;
  FVector3 scale;
};
DECLARE_COMPONENT(USprite) {
  uint32 materialID;
  uint64 textureID;
  float rotation;
  uint32 zOrder;
  FVector2 uv0;
  FVector2 uv1;
};