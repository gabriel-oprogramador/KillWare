#include "CoreMinimal.h"
#include "Arc/Arc.h"

// TODO: Mover para Engine
DECLARE_COMPONENT(UTransform) {
  FVector3 location;  // 12 bytes
  FVector3 scale;     // 12 bytes
  FQuat rotation;     // 16 bytes
};  // 40 bytes

DECLARE_COMPONENT(URigidBody) {
  FVector3 velocity;
  FVector3 acceleration;
};

DECLARE_COMPONENT(USpriteRenderer) {
  uint32 textureID;
  uint32 zOrder;
  FVector2 uv0;
  FVector2 uv1;
};

DECLARE_COMPONENT(UEnemyState) {
  uint32 aggression;
  float patrolRadius;
};

DECLARE_COMPONENT(UPlayerState) {
  uint32 score;
  uint32 level;
};
