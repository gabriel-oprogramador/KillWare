#include "Player.h"

// TODO: Para testes, ainda sem Storage real
#include "Platform/Platform.h"
void TestChunkAlloc(cstring ArchetypeName, uint32 Count) {
  UArchetype* arc = ArcFindArchetypeByName(ArchetypeName);
  arc->firstChunk = (FChunkStorage*)PMemAlloc(sizeof(FChunkStorage));
  arc->lastChunk = arc->firstChunk;
  arc->lastChunk->count = Count;
}

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
  ADD_COMPONENT(URigidBody);
}
DECLARE_ARCHETYPE(AEnemy) {
  ADD_COMPONENT(UTransform);
  ADD_COMPONENT(UEnemyState);
  ADD_COMPONENT(USpriteRenderer);
}

DECLARE_QUERY(PlayerMovement, UTransform, URigidBody);
DECLARE_QUERY(PlayerLevelUp, UPlayerState);
DECLARE_QUERY(EnemyAI, UTransform, UEnemyState);
DECLARE_QUERY(SubmitSprite, UTransform, USpriteRenderer);

GT_EXTERN_C void Foo() {
  TestChunkAlloc("APlayer", 1);
  TestChunkAlloc("AEnemy", 2);

  GT_ALERT("Foreach:PlayerMovement");
  FOR_EACH(PlayerMovement, View) {
    GT_ALERT("Loop:%s", arc->name);
  }
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
