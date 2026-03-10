#include "PlatformEvents.h"
#include "Platform.h"
#include "Core/Log.h"

typedef struct {
  PEvent queue[MAX_PLATFORM_EVENTS];
  uint32 count;
  uint32 readIndex;
} PEventSystem;

static PEventSystem SEventSystem;

bool PEventPush(PEvent Event) {
  GT_ASSERT(SEventSystem.count < MAX_PLATFORM_EVENTS);
  if(SEventSystem.count >= MAX_PLATFORM_EVENTS) {
    return false;
  }
  SEventSystem.queue[SEventSystem.count++] = Event;
  return true;
}

bool PEventNext(PEvent* OutEvent) {
  GT_ASSERT(OutEvent);
  if(OutEvent == NULL) {
    return false;
  }
  if(SEventSystem.readIndex >= SEventSystem.count) {
    SEventSystem.readIndex = 0;
    SEventSystem.count = 0;
    return false;
  }
  *OutEvent = SEventSystem.queue[SEventSystem.readIndex++];
  return true;
}
