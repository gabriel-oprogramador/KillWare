#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <windowsx.h>

#include "Platform/Platform.h"
#include "Platform/Log.h"

static bool IsPowerOfTwo(uint64 Value) {
  return Value && ((Value & (Value - 1)) == 0);
}

void PMemFree(void* Data) {
  GT_ASSERT(Data);
  free(Data);
}

void* PMemAlloc(uint64 Size) {
  GT_ASSERT(Size > 0);
  return calloc(1, Size);
}

void* PMemRealloc(void* Data, uint64 Size) {
  GT_ASSERT(Size > 0);
  return realloc(Data, Size);
}

void* PMemCopy(void* Src, void* Dst, uint64 Size) {
  GT_ASSERT(Dst && Src && Size > 0);
  return memcpy(Dst, Src, Size);
}

void* PMemMove(void* Src, void* Dst, uint64 Size) {
  GT_ASSERT(Dst && Src && Size > 0);
  return memmove(Dst, Src, Size);
}

void* PMemSet(void* Dst, int32 Value, uint64 Size) {
  GT_ASSERT(Dst && Size > 0);
  return memset(Dst, Value, Size);
}

void* PMemAllocAligned(uint64 Size, uint64 Alignment) {
  GT_ASSERT(Size > 0);
  GT_ASSERT(IsPowerOfTwo(Alignment));
  Alignment = (Alignment < sizeof(void*)) ? sizeof(void*) : Alignment;
  uint64 alignedSize = GT_ALIGN_FORWARD(Size, Alignment);
  void* ptr = GT_ALIGNED_ALLOC(alignedSize, Alignment);
  GT_ASSERT(ptr != NULL);
  return ptr;
}

void PMemFreeAligned(void* Data) {
  GT_ASSERT(Data);
  GT_ALIGNED_FREE(Data);
}

#endif  // PLATFORM_WINDOWS
