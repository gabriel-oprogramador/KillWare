#pragma once
#include "Core/EngineTypes.h"
#include "Core/Defines.h"
#include "Core/Bitset.h"
#include "Core/Log.h"

//TODO: Mudar de cstring pra FName

// 256 é o valor maximo de bitset por contexto
//#define ARC_MAX_CAPACITY             256
#define ARC_MAX_ARCHETYPE_COMPONENTS 16  // maximo de components em um archetype
#define ARC_MAX_ARCHETYPE_SIZE       4096
#define ARC_MAX_COMPONENTS_TYPES     256  // maximo de components em registro
#define ARC_MAX_QUERIES_TYPE         256

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

typedef struct FQuery {
  FName name;
  FQueryID runtimeID;
  FBitset queryMask;
  FBitset archetypesMask;
} FQuery;

typedef struct FChunkStorage {
  struct FChunkStorage* next;
  uint32 count;
  uint8 data[GT_BUFFER_16K];
} FChunkStorage;

typedef struct FArchetypeEdge {
  struct FArchetype* archetype;
  uint16 opCount;
  uint16 srcColOffset[ARC_MAX_ARCHETYPE_COMPONENTS];
  uint16 dstColOffset[ARC_MAX_ARCHETYPE_COMPONENTS];
  uint16 elementSize[ARC_MAX_ARCHETYPE_COMPONENTS];
} FArchetypeEdge;

typedef struct FArchetype {
  FName name;
  FChunkStorage* firstChunk;
  FChunkStorage* lastChunk;
  FArchetypeID runtimeID;
  FBitset componentMask;
  uint16 entityStride;
  uint16 chunkCapacity;
  uint16 actorOffset;
  FArchetypeEdge add[ARC_MAX_COMPONENTS_TYPES];
  FArchetypeEdge remove[ARC_MAX_COMPONENTS_TYPES];
  uint16 componentOffset[ARC_MAX_ARCHETYPE_COMPONENTS];
  uint16 componentStride[ARC_MAX_ARCHETYPE_COMPONENTS];
} FArchetype;

GT_EXTERN_C_BEGIN

ENGINE_API

ENGINE_API void ArcAddEntity(AActor Actor);
ENGINE_API void ArcRemoveEntity(AActor Actor);
ENGINE_API FQueryID ArcRegisterQuery(FQuery* NewQuery);
ENGINE_API FComponentID ArcRegisterComponent(UComponent* NewComponent);
ENGINE_API FArchetype* ArcGetEmptyArchetype();
ENGINE_API FArchetype* ArcGetArchetypeByID(FArchetypeID ID);
ENGINE_API FArchetype* ArcFindArchetype(FBitset Mask);
ENGINE_API FArchetype* ArcAddComponent(FArchetype* Archetype, FComponentID ComponentID);
ENGINE_API FArchetype* ArcRemoveComponent(FArchetype* Archetype, FComponentID ComponentID);
ENGINE_API void ArcMoveActorArchetype(AActor Actor, FArchetypeEdge* Edge);

GT_EXTERN_C_END

#define ARC_CACHELINE_SIZE          64
#define ARC_SIMD_ALIGNMENT          16
#define ARC_GEN_COUNT(X)            +1
#define ARC_GEN_COMPONENT(X)        components[index++] = GetUComponent(X);
#define ARC_GEM_COMPONENT_ID(X)     BitsetSet(&query->queryMask, GetUComponent(X)->runtimeID);
#define VIEW_FIELD(Type)            Type* Type##Array;
#define VIEW_PTR(Type)              Type* Type##Array;
#define VIEW_BIND_PTR(Type)         view.Type##Array = (Type*)((uint8*)Chunk->data + Archetype->componentOffset[GetUComponent(Type)->runtimeID]);
#define GEM_ARRA_FIELD(X)           X* X##Array;
#define GEM_BITSET_SET_COMPONENT(X) BitsetSet(&query->queryMask, GetUComponent(X)->runtimeID);

#define QueryGetActor()         (&view.actorArray[index])
#define QueryGetComponent(Type) (&view.Type##Array[index])
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
      comp.runtimeID = ArcRegisterComponent(&comp);   \
    }                                                 \
    return &comp;                                     \
  }                                                   \
  GT_AUTO_EXEC(AutoReg##Type)

#define ADD_COMPONENT(Type)                                \
  do {                                                     \
    UComponent* comp = GetUComponent(Type);                \
    BitsetSet(&Archetype->componentMask, comp->runtimeID); \
  } while(0);

#define DECLARE_QUERY(Name, ...)                                                     \
  typedef struct Name##View Name##View;                                              \
  struct Name##View {                                                                \
    AActor* actorArray;                                                              \
    MAP(GEM_ARRA_FIELD, __VA_ARGS__)                                                 \
    int32 count;                                                                     \
  };                                                                                 \
  static FQuery Name;                                                                \
  GT_AUTO_EXEC(AutoReg##Name) {                                                      \
    FQuery* query = &Name;                                                           \
    query->name = NameMake(#Name);                                                   \
    BitsetClear(&query->archetypesMask);                                             \
    MAP(GEM_BITSET_SET_COMPONENT, __VA_ARGS__)                                       \
    query->runtimeID = ArcRegisterQuery(&Name);                                      \
  }                                                                                  \
  static inline Name##View Name##Bind(FArchetype* Archetype, FChunkStorage* Chunk) { \
    Name##View view;                                                                 \
    view.count = Chunk->count;                                                       \
    view.actorArray = (AActor*)((uint8*)Chunk->data + Archetype->actorOffset);       \
    MAP(VIEW_BIND_PTR, __VA_ARGS__)                                                  \
    return view;                                                                     \
  }

#define QueryEach(Query)                                                                     \
  FBitset Query##arcMask = Query.archetypesMask;                                             \
  uint32 Query##arcID = 0;                                                                   \
  while(BitsetNextBitIndex(&Query##arcMask, &Query##arcID))                                  \
    for(FArchetype* arc = ArcGetArchetypeByID(Query##arcID); arc; arc = NULL)                \
      for(FChunkStorage* chunk = arc->firstChunk; chunk; chunk = chunk->next)                \
        for(Query##View view = Query##Bind(arc, chunk), *qOnce = &view; qOnce; qOnce = NULL) \
          for(uint32 index = 0; index < view.count; index++)
