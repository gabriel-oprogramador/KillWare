#include "Arc.h"
#include "Platform/Platform.h"

static FChunkStorage* InternalAllocChunk(UArchetype* Archetype);
static void InternalGenerateArchetypeLayout(UArchetype* Archetype);

static struct {
  UArchetype* archetypes[ARC_MAX_CAPACITY];
  UComponent* components[ARC_MAX_CAPACITY];
  UQuery* queries[ARC_MAX_CAPACITY];
  uint32 numArchetypes;
  uint32 numComponents;
  uint32 numQueries;
} SArc;

void ArcInitialize() {
  for(uint32 c = 0; c < SArc.numArchetypes; c++) {
    InternalGenerateArchetypeLayout(SArc.archetypes[c]);
  }
  for(uint32 q = 0; q < SArc.numQueries; q++) {
    for(uint32 a = 0; a < SArc.numArchetypes; a++) {
      UQuery* query = SArc.queries[q];
      UArchetype* arc = SArc.archetypes[a];
      if(BitsetHasAll(&arc->componentMask, &query->queryMask)) {
        BitsetSet(&query->arcMask, arc->runtimeID);
        query->arcCount++;
      }
    }
  }
  GT_ALERT("//====================// Arc State //====================//");
  for(uint32 c = 0; c < SArc.numQueries; c++) {
    UQuery* query = SArc.queries[c];
    GT_ALERT("Begin Query:%s, Archetypes:%u", query->name, query->arcCount);
    FBitset mask = query->arcMask;
    uint32 arcID = 0xFFFFFF;
    while(BitsetNextBitIndex(&mask, &arcID)) {
      UArchetype* arc = SArc.archetypes[arcID];
      GT_ALERT("\t\tFind: %s", arc->name);
    }
    GT_ALERT("End Query:%s", query->name);
  }
  GT_ALERT("//====================// Arc State //====================//");
}

void ArcTerminate() {
}

FArchetypeID RegisterArchetype(UArchetype* NewArchetype) {
  GT_ASSERT(NewArchetype && SArc.numArchetypes < ARC_MAX_CAPACITY);
  uint32 index = SArc.numArchetypes++;
  SArc.archetypes[index] = NewArchetype;
  return (FArchetypeID)index;
}

FComponentID RegisterComponent(UComponent* NewComponent) {
  GT_ASSERT(NewComponent && SArc.numComponents < ARC_MAX_CAPACITY);
  uint32 index = SArc.numComponents++;
  SArc.components[index] = NewComponent;
  return (FComponentID)index;
}

FQueryID RegisterQuery(UQuery* NewQuery) {
  GT_ASSERT(NewQuery && SArc.numQueries < ARC_MAX_CAPACITY);
  uint32 index = SArc.numQueries++;
  SArc.queries[index] = NewQuery;
  GT_ALERT("RegisterQuery:%s", NewQuery->name);
  return (FQueryID)index;
}

UArchetype* ArcFindArchetypeByID(FArchetypeID ID) {
  GT_ASSERT(ID < GT_BITSET_MAX_INDEX);
  if(ID >= SArc.numArchetypes) {
    return NULL;
  }
  return SArc.archetypes[ID];
}

UArchetype* ArcFindArchetypeByName(cstring Name) {
  for(uint32 c = 0; c < SArc.numArchetypes; c++) {
    if(strcmp(SArc.archetypes[c]->name, Name) == 0) {  // TODO:Trocar pra FName...
      return SArc.archetypes[c];
    }
  }
  return NULL;
}

static FChunkStorage* InternalAllocChunk(UArchetype* Archetype) {
  FChunkStorage* chunk = (FChunkStorage*)PMemAlloc(sizeof(FChunkStorage));
  chunk->count = 0;
  chunk->next = 0;
  if(Archetype->firstChunk == NULL) {
    Archetype->firstChunk = chunk;
    Archetype->lastChunk = chunk;
  } else {
    Archetype->lastChunk->next = chunk;
    Archetype->lastChunk = chunk;
  }
  return chunk;
}

static void InternalGenerateArchetypeLayout(UArchetype* Archetype) {
  {
    uint32 stride = sizeof(AActor);
    FBitset mask = Archetype->componentMask;
    uint32 id = 0;
    while(BitsetNextBitIndex(&mask, &id)) {
      UComponent* compInfo = SArc.components[id];
      if(compInfo == NULL) {
        continue;
      }
      stride += compInfo->typeInfo.size;
    }
    Archetype->chunkCapacity = GT_BUFFER_16K / stride;
    Archetype->entityStride = stride;
  }

  {
    uint32 offset = 0;
    FBitset mask = Archetype->componentMask;
    uint32 id = 0;
    while(BitsetNextBitIndex(&mask, &id)) {
      UComponent* compInfo = SArc.components[id];
      if(compInfo == NULL) {
        continue;
      }
      offset = GT_ALIGN_FORWARD(offset, compInfo->typeInfo.align);
      Archetype->componentOffset[id] = offset;
      Archetype->componentStride[id] = compInfo->typeInfo.size;
      offset += compInfo->typeInfo.size * Archetype->chunkCapacity;
    }
    offset = GT_ALIGN_FORWARD(offset, GT_ALIGNOF(AActor));
    Archetype->actorOffset = offset;
    offset += sizeof(AActor) * Archetype->chunkCapacity;
    GT_ASSERT(offset <= GT_BUFFER_16K);
  }
}

/*static void InternalGenerateArchetypeLayout(UArchetype* Archetype) {*/
/*{*/
/*uint32 perEntitySize = 0;*/
/*FBitset mask = Archetype->componentMask;*/
/*FComponentID id = 0;*/
/*while(BitsetNextBitIndex(&mask, &id)) {*/
/*UComponent* compInfo = SArc.components[id];*/
/*if(compInfo == NULL) {*/
/*continue;*/
/*}*/
/*perEntitySize += compInfo->typeInfo.size;*/
/*}*/
/*Archetype->chunkCapacity = GT_BUFFER_16K / perEntitySize;*/
/*}*/
/*{*/
/*uint32 offset = 0;*/
/*uint32 id = 0;*/
/*FBitset mask = Archetype->componentMask;*/
/*while(BitsetNextBitIndex(&mask, &id)) {*/
/*UComponent* compInfo = SArc.components[id];*/
/*if(compInfo == NULL) {*/
/*continue;*/
/*}*/
/*offset = GT_ALIGN_FORWARD(offset, compInfo->typeInfo.align);*/
/*Archetype->componentOffset[id] = offset;*/
/*offset += compInfo->typeInfo.size * Archetype->chunkCapacity;*/
/*}*/
/*GT_ASSERT(offset <= GT_BUFFER_16K);*/
/*}*/
/*}*/
