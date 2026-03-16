#include "Player.h"
#include "World/World.h"

// TODO: Para testes, ainda sem Storage real
#include "Platform/Platform.h"
//void TestChunkAlloc(cstring ArchetypeName, uint32 Count) {
//UArchetype* arc = ArcFindArchetypeByName(ArchetypeName);
//arc->firstChunk = (FChunkStorage*)PMemAlloc(sizeof(FChunkStorage));
//arc->lastChunk = arc->firstChunk;
//arc->lastChunk->count = Count;
//}

// TODO: Mover para Engine
REGISTER_COMPONENT(UTransform) {
  UPROPERTY(UTransform, FVector3, location);
  UPROPERTY(UTransform, FVector3, scale);
  UPROPERTY(UTransform, FQuat, rotation);
}

REGISTER_COMPONENT(URigidBody) {
  UPROPERTY(URigidBody, FVector3, velocity);
  UPROPERTY(URigidBody, FVector3, force);
  UPROPERTY(URigidBody, float, invMass);
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

static void TestActorsLifecycle();

GT_EXTERN_C void Foo() {
  AActor aPlayer = WorldSpawnActor("PlayerOne");
  FActorEntry* entry = WorldGetActorEntryByActor(aPlayer);

  GT_ALERT("Player Archetype: %s", NameToStr(entry->archetype->name));
  {
    ActorAddComponent(aPlayer, URigidBody);
    UTransform* transform = ActorAddComponent(aPlayer, UTransform);
    transform->location = (FVector3){1, 2, 3};
    transform->scale = (FVector3){1, 1, 1};
    transform->rotation = (FQuat){0, 1, 0, 1};
  }
  {
    GT_ALERT("Player Archetype: %s", NameToStr(entry->archetype->name));
    UTransform* trs = ActorGetComponent(aPlayer, UTransform);
    GT_ALERT("Player Location(%.2f, %.2f, %.2f)", trs->location.x, trs->location.y, trs->location.z);
  }
  {
    ActorAddComponent(aPlayer, UPlayerState);
    GT_ALERT("Player Archetype: %s", NameToStr(entry->archetype->name));
    UTransform* trs = ActorGetComponent(aPlayer, UTransform);
    GT_ALERT("Player Location(%.2f, %.2f, %.2f)", trs->location.x, trs->location.y, trs->location.z);
  }
  {
    ActorAddComponent(aPlayer, USpriteRenderer);
    GT_ALERT("Player Archetype: %s", NameToStr(entry->archetype->name));
    UTransform* trs = ActorGetComponent(aPlayer, UTransform);
    GT_ALERT("Player Location(%.2f, %.2f, %.2f)", trs->location.x, trs->location.y, trs->location.z);
  }
  {
    ActorRemoveComponent(aPlayer, UTransform);
    GT_ALERT("Player Archetype: %s", NameToStr(entry->archetype->name));
    UTransform* trs = ActorGetComponent(aPlayer, UTransform);
    if(trs) {
      GT_ALERT("Player Location(%.2f, %.2f, %.2f)", trs->location.x, trs->location.y, trs->location.z);
    } else {
      GT_ALERT("UTransform Component not found");
    }
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
  //TestActorsLifecycle();
}

static void TestActorsLifecycle() {
  AActor enemies[5] = {0};

  GT_INFO("[TEST] Spawn Actors");

  enemies[0] = WorldSpawnActor("EnemyOne");
  enemies[1] = WorldSpawnActor("EnemyTwo");
  enemies[2] = WorldSpawnActor("EnemyThree");
  enemies[3] = WorldSpawnActor("EnemyFour");
  enemies[4] = WorldSpawnActor("EnemyFive");

  GT_INFO("[TEST] Add UEnemyState");

  for(uint32 i = 0; i < 5; i++) {
    UEnemyState* st = ActorAddComponent(enemies[i], UEnemyState);
    st->aggression = (i + 1) * 10;
  }

  GT_INFO("[STATE] After AddComponent");

  for(uint32 i = 0; i < 5; i++) {
    FActorEntry* entry = WorldGetActorEntryByActor(enemies[i]);

    if(entry) {
      UEnemyState* st = ActorGetComponent(enemies[i], UEnemyState);

      if(st) {
        GT_ALERT("Actor:%s ChunkIndex:%u Aggression:%u", NameToStr(entry->actorName), entry->chunkIndex, st->aggression);
      }
    }
  }

  /* ------------------------------------- */
  /* Remove component de um ator no meio   */
  /* ------------------------------------- */

  GT_INFO("[TEST] Remove UEnemyState from EnemyThree");

  ActorRemoveComponent(enemies[2], UEnemyState);

  for(uint32 i = 0; i < 5; i++) {
    FActorEntry* entry = WorldGetActorEntryByActor(enemies[i]);

    if(entry) {
      UEnemyState* st = ActorGetComponent(enemies[i], UEnemyState);

      if(st) {
        GT_ALERT("Actor:%s ChunkIndex:%u Aggression:%u", NameToStr(entry->actorName), entry->chunkIndex, st->aggression);
      } else {
        GT_ALERT("Actor:%s without UEnemyState", NameToStr(entry->actorName));
      }
    }
  }

  /* ------------------------------------- */
  /* Destroy um ator no meio (swap-back)   */
  /* ------------------------------------- */

  GT_INFO("[TEST] Destroy EnemyTwo");

  WorldDestroyActor(enemies[1]);

  for(uint32 i = 0; i < 5; i++) {
    FActorEntry* entry = WorldGetActorEntryByActor(enemies[i]);

    if(entry) {
      UEnemyState* st = ActorGetComponent(enemies[i], UEnemyState);

      if(st) {
        GT_ALERT("Actor:%s ChunkIndex:%u Aggression:%u", NameToStr(entry->actorName), entry->chunkIndex, st->aggression);
      } else {
        GT_ALERT("Actor:%s alive but without component", NameToStr(entry->actorName));
      }
    } else {
      GT_ALERT("Actor[%u] destroyed or invalid", i);
    }
  }

  /* ------------------------------------- */
  /* Adiciona novamente componente         */
  /* ------------------------------------- */

  GT_INFO("[TEST] Re-add UEnemyState to EnemyThree");

  UEnemyState* st = ActorAddComponent(enemies[2], UEnemyState);

  if(st) {
    st->aggression = 999;
  }

  for(uint32 i = 0; i < 5; i++) {
    FActorEntry* entry = WorldGetActorEntryByActor(enemies[i]);

    if(entry) {
      UEnemyState* s = ActorGetComponent(enemies[i], UEnemyState);

      if(s) {
        GT_ALERT("Actor:%s ChunkIndex:%u Aggression:%u", NameToStr(entry->actorName), entry->chunkIndex, s->aggression);
      }
    }
  }

  GT_INFO("[TEST END]");
}
