#include "Player.h"

// TODO: Mover para Engine
REGISTER_COMPONENT(UTransform) {
  UPROPERTY(UTransform, FVector3, location);
  UPROPERTY(UTransform, FVector3, scale);
  UPROPERTY(UTransform, FQuat, rotation);
}

REGISTER_COMPONENT(URigidBody) {
  UPROPERTY(URigidBody, FVector3, velocity);
  UPROPERTY(URigidBody, FVector3, acceleration);
}

REGISTER_COMPONENT(USpriteRenderer) {
  UPROPERTY(USpriteRenderer, uint32, textureID);
  UPROPERTY(USpriteRenderer, uint32, zOrder);
  UPROPERTY(USpriteRenderer, FVector2, uv0);
  UPROPERTY(USpriteRenderer, FVector2, uv1);
}

REGISTER_COMPONENT(UPlayerState) {
  UPROPERTY(UPlayerState, uint32, score);
  UPROPERTY(UPlayerState, uint32, level);
}

REGISTER_COMPONENT(UEnemyState) {
  UPROPERTY(UEnemyState, uint32, aggression);
  UPROPERTY(UEnemyState, float, patrolRadius);
}

DECLARE_ARCHETYPE(APlayer) {
  ADD_COMPONENT(UTransform);
  ADD_COMPONENT(UPlayerState);
  ADD_COMPONENT(USpriteRenderer);
  //ADD_COMPONENT(URigidBody);
}
DECLARE_ARCHETYPE(AEnemy) {
  ADD_COMPONENT(UTransform);
  ADD_COMPONENT(UEnemyState);
  ADD_COMPONENT(USpriteRenderer);
}

#define VIEW_FIELD(Type)        Type* Type##Array;
#define ARC_GEM_COMPONENT_ID(X) BitsetSet(&query->queryMask, GetUComponent(X)->runtimeID);
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

DECLARE_QUERY(PlayerMovement, UTransform, URigidBody);
DECLARE_QUERY(PlayerLevelUp, UPlayerState);
DECLARE_QUERY(EnemyAI, UTransform, UEnemyState);
DECLARE_QUERY(SubmitSprite, UTransform, USpriteRenderer);

#include "Platform/Platform.h"
GT_EXTERN_C void Foo() {
  UArchetype* playerArc = ArcFindArchetypeByName("APlayer");
  //UArchetype* enemyArc = ArcFindArchetypeByName("AEnemy");
  playerArc->firstChunk = (FChunkStorage*)PMemAlloc(sizeof(FChunkStorage));
  playerArc->lastChunk = playerArc->firstChunk;
  playerArc->lastChunk->count = 3;

  GT_ALERT("Foreach:PlayerLevelUp");
  FOR_EACH(PlayerLevelUp, View) {
    GT_ALERT("Loop:%s", arc->name);
  }
  GT_ALERT("Foreach:SubmitSprite");
  FOR_EACH(SubmitSprite, View) {
    GT_ALERT("Loop:%s", arc->name);
  }
  GT_ALERT("Foreach:EnemyAI");
  FOR_EACH(EnemyAI, View) {
    GT_ALERT("Loop:%s", arc->name);
  }

  UComponent* component = GetUComponent(UPlayerState);
  FTypeInfo* cInfo = &component->typeInfo;
  FProperty* prop = component->propList;
  GT_ALERT("Component => Name:%s, Size:%llu, Align:%llu, Hash:%llu\n", cInfo->name, cInfo->size, cInfo->align, cInfo->hash);
  while(prop) {
    FTypeInfo* info = &prop->typeInfo;
    GT_ALERT("TypeInfo => Name:%s, Size:%llu, Align:%llu, Hash:%llu", info->name, info->size, info->align, info->hash);
    GT_ALERT("Property => %s::%s, Offset:%llu", component->typeInfo.name, prop->name, prop->offset);
    prop = prop->next;
  }
}
