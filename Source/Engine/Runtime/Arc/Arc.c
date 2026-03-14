#include "Arc.h"
#include "Platform/Platform.h"
#include "World/World.h"
#include "Core/Name.h"

static void InternalSwapBack(FArchetype* Archetype, FChunkStorage* CurrentChunk, uint32 RemoveIndex);
static FArchetype* InternalFindOrCreateArchetype(FBitset Mask);
static FChunkStorage* InternalAllocChunk(FArchetype* Archetype);
static void InternalBuildArchetypeLayout(FArchetype* Archetype);
static void InternalBuildEdge(FArchetype* Src, FArchetype* Dst, FArchetypeEdge* Edge);

static struct {
  FArchetype* archetypes[ARC_MAX_CAPACITY];
  UComponent* components[ARC_MAX_CAPACITY];
  UQuery* queries[ARC_MAX_CAPACITY];
  uint32 numArchetypes;
  uint32 numComponents;
  uint32 numQueries;
} SArc;

void ArcInitialize() {
}

void ArcTerminate() {
}

static void InternalUpdateArchetypeDebugName(FArchetype* Archetype) {
  char buffer[GT_BUFFER_1KB];
  uint32 used = 0;
  FBitset mask = Archetype->componentMask;
  uint32 id;
  while(BitsetNextBitIndex(&mask, &id)) {
    UComponent* compInfo = SArc.components[id];
    uint32 length = strlen(compInfo->typeInfo.name);
    PMemCopy(buffer + used, (void*)compInfo->typeInfo.name, length);
    used += length;
    buffer[used++] = ' ';
    buffer[used++] = '|';
    buffer[used++] = ' ';
  }
  buffer[used] = '\0';
  Archetype->name = NameMake(buffer);
}

void ArcAddEntity(AActor Actor) {
  FActorEntry* entry = WorldGetActorEntryByActor(Actor);
  entry->archetype = ArcGetEmptyArchetype();
  entry->chunk = NULL;
  entry->chunkIndex = 0;
}

void ArcRemoveEntity(AActor Actor) {
  FActorEntry* entry = WorldGetActorEntryByActor(Actor);
  if(entry && entry->chunk && entry->archetype) {
    InternalSwapBack(entry->archetype, entry->chunk, entry->chunkIndex);
    entry->archetype = ArcGetEmptyArchetype();
    entry->chunkIndex = 0;
    entry->chunk = NULL;
    return;
  }
}

FComponentID ArcRegisterComponent(UComponent* NewComponent) {
  GT_ASSERT(NewComponent && SArc.numComponents < ARC_MAX_CAPACITY);
  uint32 index = SArc.numComponents++;
  SArc.components[index] = NewComponent;
  return (FComponentID)index;
}

FArchetype* ArcGetEmptyArchetype() {
  static FArchetype* emptyArc = NULL;
  if(emptyArc == NULL) {
    emptyArc = (FArchetype*)PMemAlloc(sizeof(FArchetype));
  }
  return emptyArc;
}

FArchetype* ArcFindArchetype(FBitset Mask) {
  for(uint32 c = 0; c < SArc.numArchetypes; c++) {
    FArchetype* arc = SArc.archetypes[c];
    if(BitsetEquals(&arc->componentMask, &Mask)) {
      return arc;
    }
  }
  return NULL;
}

FArchetype* ArcAddComponent(FArchetype* Archetype, FComponentID ComponentID) {
  if(ComponentID >= ARC_MAX_COMPONENTS_TYPES || BitsetHas(&Archetype->componentMask, ComponentID)) {
    return Archetype;
  }
  FArchetypeEdge* edge = &Archetype->add[ComponentID];
  if(edge->archetype) {
    return edge->archetype;
  }
  FBitset newMask = Archetype->componentMask;
  BitsetSet(&newMask, ComponentID);
  FArchetype* dst = InternalFindOrCreateArchetype(newMask);
  InternalBuildEdge(Archetype, dst, &Archetype->add[ComponentID]);
  InternalBuildEdge(dst, Archetype, &dst->remove[ComponentID]);
  return dst;
}

FArchetype* ArcRemoveComponent(FArchetype* Archetype, FComponentID ComponentID) {
  if(ComponentID >= ARC_MAX_COMPONENTS_TYPES || !BitsetHas(&Archetype->componentMask, ComponentID)) {
    return Archetype;
  }
  FArchetypeEdge* edge = &Archetype->remove[ComponentID];
  if(edge->archetype) {
    return edge->archetype;
  }
  FBitset newMask = Archetype->componentMask;
  BitsetReset(&newMask, ComponentID);
  FArchetype* dst = InternalFindOrCreateArchetype(newMask);
  InternalBuildEdge(Archetype, dst, &Archetype->remove[ComponentID]);
  InternalBuildEdge(dst, Archetype, &dst->add[ComponentID]);
  return dst;
}

void ArcMoveActorArchetype(AActor Actor, FArchetypeEdge* Edge) {
  FActorEntry* entry = WorldGetActorEntryByActor(Actor);
  if(entry == NULL) {
    return;
  }
  FArchetype* srcArc = entry->archetype;
  FArchetype* dstArc = Edge->archetype;

  FChunkStorage* srcChunk = entry->chunk;
  uint32 srcIndex = entry->chunkIndex;

  FChunkStorage* dstChunk = dstArc->lastChunk;
  if(dstChunk == NULL || dstChunk->count >= dstArc->chunkCapacity) {
    dstChunk = InternalAllocChunk(dstArc);
  }
  uint32 dstIndex = dstChunk->count++;
  uint8* srcBase = srcChunk->data;
  uint8* dstBase = dstChunk->data;

  if(srcChunk == NULL) {  // TODO: Pequena gambiarra pra atores vazios
    void* srcActor = &Actor;
    void* dstActor = dstBase + dstArc->actorOffset + dstIndex * sizeof(AActor);
    entry->archetype = dstArc;
    entry->chunk = dstChunk;
    entry->chunkIndex = dstIndex;
    PMemCopy(dstActor, srcActor, sizeof(AActor));
    return;
  }

  void* srcActor = srcBase + srcArc->actorOffset + srcIndex * sizeof(AActor);
  void* dstActor = dstBase + dstArc->actorOffset + dstIndex * sizeof(AActor);
  PMemCopy(dstActor, srcActor, sizeof(AActor));

  for(uint32 c = 0; c < Edge->opCount; c++) {
    uint16 srcOffset = Edge->srcColOffset[c];
    uint16 dstOffset = Edge->dstColOffset[c];
    uint16 size = Edge->elementSize[c];
    PMemCopy(dstBase + dstOffset + dstIndex * size, srcBase + srcOffset + srcIndex * size, size);
  }

  InternalSwapBack(srcArc, srcChunk, srcIndex);

  entry->archetype = dstArc;
  entry->chunk = dstChunk;
  entry->chunkIndex = dstIndex;
}

// Mover da Ultima Chunk  pra Atual
static void InternalSwapBack(FArchetype* Archetype, FChunkStorage* CurrentChunk, uint32 RemoveIndex) {
  if(CurrentChunk->count == 0) {
    return;
  }
  FChunkStorage* lastChunk = Archetype->lastChunk;
  uint32 lastIndex = lastChunk->count - 1;
  if(CurrentChunk == lastChunk && RemoveIndex == lastIndex) {
    CurrentChunk->count--;
    return;
  }
  void* srcActor = lastChunk->data + Archetype->actorOffset + lastIndex * sizeof(AActor);
  void* dstActor = CurrentChunk->data + Archetype->actorOffset + RemoveIndex * sizeof(AActor);
  PMemCopy(dstActor, srcActor, sizeof(AActor));
  FBitset mask = Archetype->componentMask;
  uint32 compID = 0;
  while(BitsetNextBitIndex(&mask, &compID)) {
    uint32 stride = Archetype->componentStride[compID];
    uint32 offset = Archetype->componentOffset[compID];
    void* srcComp = lastChunk->data + offset + lastIndex * stride;
    void* dstComp = CurrentChunk->data + offset + RemoveIndex * stride;
    PMemCopy(dstComp, srcComp, stride);
  }
  FActorEntry* movedEntry = WorldGetActorEntryByActor(*(AActor*)dstActor);
  movedEntry->chunk = CurrentChunk;
  movedEntry->chunkIndex = RemoveIndex;
  lastChunk->count--;
  // Nao vou desalocar a last Chunk caso vazia!
}

static FArchetype* InternalFindOrCreateArchetype(FBitset Mask) {
  FArchetype* archetype = ArcFindArchetype(Mask);
  if(archetype != NULL) {
    return archetype;
  }
  archetype = PMemAlloc(sizeof(FArchetype));
  GT_ASSERT(SArc.numComponents < ARC_MAX_CAPACITY);
  GT_ASSERT(archetype);
  archetype->componentMask = Mask;
  InternalBuildArchetypeLayout(archetype);
  InternalUpdateArchetypeDebugName(archetype);
  archetype->runtimeID = SArc.numArchetypes++;
  SArc.archetypes[archetype->runtimeID] = archetype;
  return archetype;
}

static FChunkStorage* InternalAllocChunk(FArchetype* Archetype) {
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

static void InternalBuildArchetypeLayout(FArchetype* Archetype) {
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

static void InternalBuildEdge(FArchetype* Src, FArchetype* Dst, FArchetypeEdge* Edge) {
  Edge->archetype = Dst;
  Edge->opCount = 0;
  FBitset common;
  BitsetClear(&common);
  common = BitsetAnd(&Src->componentMask, &Dst->componentMask);
  uint32 compID = 0;
  while(BitsetNextBitIndex(&common, &compID)) {
    uint16 i = Edge->opCount++;
    Edge->srcColOffset[i] = Src->componentOffset[compID];
    Edge->dstColOffset[i] = Dst->componentOffset[compID];
    Edge->elementSize[i] = Src->componentStride[compID];
  }
}
