#pragma once
#include "Core/EngineTypes.h"
#include "Core/Defines.h"
#include "Core/Bitset.h"
#include "Core/Log.h"

//TODO: Mudar de cstring pra FName

// 256 é o valor maximo de bitset por contexto
#define ARC_MAX_CAPACITY 256

typedef uint32 FComponentID;
typedef uint32 FArchetypeID;
typedef uint32 FQueryID;

typedef struct FTypeInfo {
  cstring name;
  uint64 size;
  uint64 hash;
  uint64 align;
} FTypeInfo;

typedef struct FProperty {
  FTypeInfo typeInfo;
  cstring name;
  uint64 offset;
  struct FProperty* next;
} FProperty;

typedef struct UComponent {
  FTypeInfo typeInfo;
  FProperty* propList;
  FComponentID runtimeID;
} UComponent;

typedef struct UQuery {
  cstring name;
  FQueryID runtimeID;
  FBitset queryMask;
  FBitset arcMask;
  uint32 arcCount;
} UQuery;

typedef struct FChunkStorage {
  struct FChunkStorage* next;
  uint32 count;
  uint8 data[GT_BUFFER_16K];
} FChunkStorage;

typedef struct UActor {
  FChunkStorage* chunk;
  AActor owner;
  uint32 index;
  uint32 version;
} UActor;

typedef struct UArchetype {
  FChunkStorage* firstChunk;
  FChunkStorage* lastChunk;
  UActor* handles;
  cstring name;
  FBitset componentMask;
  FArchetypeID runtimeID;
  uint16 entityStride;
  uint16 chunkCapacity;
  uint16 actorOffset;
  uint16 componentOffset[ARC_MAX_CAPACITY];
  uint16 componentStride[ARC_MAX_CAPACITY];
} UArchetype;

GT_EXTERN_C_BEGIN

ENGINE_API FArchetypeID RegisterArchetype(UArchetype* NewArchetype);
ENGINE_API FComponentID RegisterComponent(UComponent* NewComponent);
ENGINE_API FQueryID RegisterQuery(UQuery* NewQuery);

ENGINE_API UActor ArcAddEntity();
ENGINE_API UArchetype* ArcFindArchetypeByID(FArchetypeID ID);
ENGINE_API UArchetype* ArcFindArchetypeByName(cstring Name);

GT_EXTERN_C_END

#define ARC_CACHELINE_SIZE      64
#define ARC_SIMD_ALIGNMENT      16
#define ARC_GEN_COUNT(X)        +1
#define ARC_GEN_COMPONENT(X)    components[index++] = GetUComponent(X);
#define ARC_GEM_COMPONENT_ID(X) BitsetSet(&query->queryMask, GetUComponent(X)->runtimeID);
#define VIEW_FIELD(Type)        Type* Type##Array;
#define GetUComponent(Type)     Type##GetUComponent()

#define DECLARE_COMPONENT(Name)                  \
  typedef struct Name Name;                      \
  GT_EXTERN_C UComponent* Name##GetUComponent(); \
  struct Name

#define UPROPERTY(TComponent, Type, Name)           \
  do {                                              \
    static FProperty Name = {};                     \
    Name.typeInfo.name = #Type;                     \
    Name.typeInfo.size = sizeof(Type);              \
    Name.typeInfo.hash = HashFNV1a64FromStr(#Type); \
    Name.typeInfo.align = GT_ALIGNOF(Type);         \
    Name.name = #Name;                              \
    Name.offset = offsetof(TComponent, Name);       \
    UComponent* comp = TComponent##GetUComponent(); \
    FProperty** it = &comp->propList;               \
    while(*it)                                      \
      it = &(*it)->next;                            \
    *it = &Name;                                    \
  } while(0);

#define REGISTER_COMPONENT(Type)                      \
  UComponent* Type##GetUComponent() {                 \
    static UComponent comp = {};                      \
    if(comp.typeInfo.size == 0) {                     \
      comp.typeInfo.name = #Type;                     \
      comp.typeInfo.size = sizeof(Type);              \
      comp.typeInfo.align = GT_ALIGNOF(Type);         \
      comp.typeInfo.hash = HashFNV1a64FromStr(#Type); \
      comp.runtimeID = RegisterComponent(&comp);      \
    }                                                 \
    return &comp;                                     \
  }                                                   \
  GT_AUTO_EXEC(AutoReg##Type)

#define ADD_COMPONENT(Type)                                \
  do {                                                     \
    UComponent* comp = GetUComponent(Type);                \
    BitsetSet(&Archetype->componentMask, comp->runtimeID); \
  } while(0);

#define DECLARE_ARCHETYPE(Name)                   \
  void RegArchetype##Name(UArchetype* Archetype); \
  GT_AUTO_EXEC(AutoRegArchetype##Name) {          \
    static UArchetype arc = {};                   \
    arc.name = #Name;                             \
    arc.entityStride = 0;                         \
    arc.firstChunk = NULL;                        \
    arc.lastChunk = NULL;                         \
    BitsetClear(&arc.componentMask);              \
    arc.runtimeID = RegisterArchetype(&arc);      \
    RegArchetype##Name(&arc);                     \
  }                                               \
  void RegArchetype##Name(UArchetype* Archetype)

#define DECLARE_QUERY(Name, ...)             \
  static UQuery Name;                        \
  typedef struct {                           \
    uint32 count;                            \
    MAP(VIEW_FIELD, __VA_ARGS__)             \
  } Name##View;                              \
  GT_AUTO_EXEC(AutoRegQuery##Name) {         \
    UQuery* query = &Name;                   \
    query->name = #Name;                     \
    query->runtimeID = RegisterQuery(&Name); \
    BitsetClear(&query->queryMask);          \
    BitsetClear(&query->arcMask);            \
    MAP(ARC_GEM_COMPONENT_ID, __VA_ARGS__);  \
  }

#define FOR_EACH(Query, Var)                                                   \
  FBitset Query##mask = Query.arcMask;                                         \
  uint32 Query##arcID = 0;                                                     \
  while(BitsetNextBitIndex(&Query##mask, &Query##arcID))                       \
    for(UArchetype* arc = ArcFindArchetypeByID(Query##arcID); arc; arc = NULL) \
      for(FChunkStorage* chunk = arc->firstChunk; chunk; chunk = chunk->next)  \
        for(Query##View Var = {0}; Var.count == 0; Var.count++)                \
          for(uint32 index = 0; index < chunk->count; index++)
