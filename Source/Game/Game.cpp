// This file uses C++ only to test compatibility between C and C++.
#include "CoreMinimal.h"
#include "GameModule.h"
#include "World/World.h"
#include "Player.h"

GT_EXTERN_C void Foo();

static struct {
  AActor aPlayer;
  AActor aEnemy;
  FActorEntry* playerEntry;
} SGame;

DECLARE_QUERY(qPlayerInput, UPlayerState, URigidBody);
DECLARE_QUERY(qPlayerLevelUp, UPlayerState);
DECLARE_QUERY(qPhysicsMove, UTransform, URigidBody);
DECLARE_QUERY(qEnemyPatrol, UEnemyState);

void PlayerLevelUpSystem(float DeltaTime) {
  const float speed = 450.f;

  QueryEach(qPlayerInput) {
    UPlayerState* state = QueryGetComponent(UPlayerState);
    URigidBody* body = QueryGetComponent(URigidBody);
    if(InputIsKeyRepeat(KEY_W)) {
      body->force.z += -1.f * speed;
    }
    if(InputIsKeyRepeat(KEY_S)) {
      body->force.z += 1.f * speed;
    }
    if(InputIsKeyRepeat(KEY_D)) {
      body->force.x += 1.f * speed;
    }
    if(InputIsKeyRepeat(KEY_A)) {
      body->force.x += -1.f * speed;
    }
    if(InputIsKeyPressed(KEY_SPACE)) {
      state->score += 20;
    }
  }

  QueryEach(qPhysicsMove) {
    AActor* actor = QueryGetActor();
    UTransform* transform = QueryGetComponent(UTransform);
    URigidBody* body = QueryGetComponent(URigidBody);

    float accelX = body->force.x * body->invMass;
    float accelY = body->force.y * body->invMass;
    float accelZ = body->force.z * body->invMass;

    body->velocity.x += accelX * DeltaTime;
    body->velocity.y += accelY * DeltaTime;
    body->velocity.z += accelZ * DeltaTime;

    float drag = 6.0f;
    body->velocity.x *= (1.0f - drag * DeltaTime);
    body->velocity.y *= (1.0f - drag * DeltaTime);
    body->velocity.z *= (1.0f - drag * DeltaTime);

    transform->location.x += body->velocity.x * DeltaTime;
    transform->location.y += body->velocity.y * DeltaTime;
    transform->location.z += body->velocity.z * DeltaTime;
    body->force = (FVector3){0.f, 0.f, 0.f};

    cstring actorName = NameToStr(ActorGetDisplayName(*actor));
    FVector3 loc = transform->location;
    GT_INFO("Actor:%s, Location(%.2f, %.2f, %.2f), Archetype:%s", actorName, loc.x, loc.y, loc.z, NameToStr(arc->name));
  }

  QueryEach(qPlayerLevelUp) {
    UPlayerState* state = QueryGetComponent(UPlayerState);
    if(state->score >= 100) {
      state->score = 0;
      state->level++;
    }
    //GT_INFO("Actor:Player, Score:%u, level:%u", state->score, state->level);
  }
  QueryEach(qEnemyPatrol) {
    AActor* actor QueryGetActor();
    UEnemyState* state = QueryGetComponent(UEnemyState);
    cstring actorName = NameToStr(ActorGetDisplayName(*actor));
    //GT_INFO("Actor:%s, patrolRadius(%.2f), Aggression(%u), Archetype:%s", actorName, state->patrolRadius, state->aggression, NameToStr(arc->name));
  }
}

void GameStart() {
  GT_INFO("Game Start");
  //Foo();
  SGame.aPlayer = WorldSpawnActor("Player");
  UTransform* trs = ActorAddComponent(SGame.aPlayer, UTransform);
  if(trs) {
    trs->location = (FVector3){10, 11, 12};
  }
  URigidBody* rb = ActorAddComponent(SGame.aPlayer, URigidBody);
  if(rb) {
    rb->invMass = 0.0125f;
  }
  UPlayerState* ps = ActorAddComponent(SGame.aPlayer, UPlayerState);
  if(ps) {
    ps->score = 0;
    ps->level = 0;
  }

  SGame.aEnemy = WorldSpawnActor("Enemy");
  ActorAddComponent(SGame.aEnemy, URigidBody);
  UTransform* enemyTrs = ActorAddComponent(SGame.aEnemy, UTransform);
  if(enemyTrs) {
    enemyTrs->location = (FVector3){20, 21, 22};
  }
  UEnemyState* enemySt = ActorAddComponent(SGame.aEnemy, UEnemyState);
  if(enemySt) {
    enemySt->aggression = 33;
    enemySt->patrolRadius = 100.f;
  }
}

void GameUpdate(float DeltaTime) {
  PlayerLevelUpSystem(DeltaTime);

  if(InputIsKeyPressed(KEY_BACKSPACE)) {
    ActorRemoveComponent(SGame.aPlayer, UTransform);
  }
  if(InputIsKeyPressed(KEY_ENTER)) {
    UTransform* trs = ActorAddComponent(SGame.aPlayer, UTransform);
    if(trs) {
      trs->location = (FVector3){33, 66, 99};
    }
  }

  if(InputIsKeyPressed(KEY_ESCAPE)) {
    GameQuit();
  }

  if(InputIsKeyPressed(KEY_ENTER)) {
    bool bCap = InputIsMouseCaptured();
    InputSetMouseCaptured(!bCap);
  }

  if(InputIsKeyPressed(KEY_F)) {
    GameToggleFullscreen();
  }

  if(InputIsKeyPressed(KEY_MOUSE_LEFT)) {
    FVector2 pos = InputGetMousePos();
    GT_ALERT("Mouse Pos:(X:%.2f, Y:%.f)", pos.x, pos.y);
  }

  float scroll = InputGetMouseScroll().y;
  if(scroll > 0) {
    GT_ALERT("Scroll Y Forward:%f", scroll);
  } else if(scroll < 0) {
    GT_ALERT("Scroll Y Backward:%f", scroll);
  }
}

void GameStop() {
  GT_INFO("Game Stop");
}
