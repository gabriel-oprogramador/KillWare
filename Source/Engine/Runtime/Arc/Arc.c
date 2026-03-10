#include "Arc.h"

REGISTER_COMPONENT(UPlayer) {
  UPROPERTY(UPlayer, FVector3, location);
  UPROPERTY(UPlayer, FVector3, rotation);
  UPROPERTY(UPlayer, FVector3, scale);
}

REGISTER_COMPONENT(USprite) {
  UPROPERTY(USprite, uint32, materialID);
  UPROPERTY(USprite, uint64, textureID);
  UPROPERTY(USprite, float, rotation);
  UPROPERTY(USprite, uint32, zOrder);
  UPROPERTY(USprite, FVector2, uv0);
  UPROPERTY(USprite, FVector2, uv1);
}

DECLARE_ARCHETYPE(APlayer, UPlayer, USprite)

void Foo() {
  UComponent* component = GetUComponent(UPlayer);
  FTypeInfo* cInfo = &component->info;
  FProperty* prop = component->propList;
  GT_ALERT("Component => Name:%s, Size:%llu, Align:%llu, Hash:%llu\n", cInfo->typeName, cInfo->typeSize, cInfo->typeAlign, cInfo->typeHash);
  while(prop) {
    FTypeInfo* info = &prop->info;
    GT_ALERT("TypeInfo => Name:%s, Size:%llu, Align:%llu, Hash:%llu", info->typeName, info->typeSize, info->typeAlign, info->typeHash);
    GT_ALERT("Property => %s::%s, Offset:%llu\n", component->info.typeName, prop->name, prop->offset);
    prop = prop->next;
  }
}