#pragma once
#include "Platform/Types.h"
#include "Platform/Defines.h"

typedef enum EPlatformFileMode {
  PLATFORM_FILE_READ,    //
  PLATFORM_FILE_WRITE,   //
  PLATFORM_FILE_APPEND,  //
} EPlatformFileMode;

typedef struct PKey {
  EKeyCode keyCode;
  cstring physicalName;
  cstring layoutName;
  bool bIsControlKey;
} PKey;

typedef struct PDir {
  void* handle;
  char path[GT_MAX_PATH];
  bool bIsOpen;
} PDir;

typedef struct PDirEntry {
  char name[GT_MAX_FILENAME];
  char path[GT_MAX_FULLPATH];
  uint64 size;
  uint64 createdTime;
  uint64 lastWriteTime;
  uint64 lastAccessTime;
  bool bIsDirectory;
} PDirEntry;

typedef struct PFile {
  char path[GT_MAX_FULLPATH];
  void* handle;
  uint64 size;
  uint64 createdTime;
  uint64 lastWriteTime;
  uint64 lastAccessTime;
  EPlatformFileMode mode;
} PFile;

GT_EXTERN_C_BEGIN

// Window Api //==============================================================================================//
ENGINE_API void PWindowInit(uint32 Width, uint32 Height, cstring Title);
ENGINE_API void PWindowClose();
ENGINE_API bool PWindowIsVsync();
ENGINE_API void PWindowSetVsync(bool bEnable);
ENGINE_API bool PWindowIsMouseCaptured();
ENGINE_API void PWindowSetMouseCaptured(bool bCapture);
ENGINE_API bool PWindowIsFullscreen();
ENGINE_API void PWindowSetFullscreen(bool bFullscreen);
ENGINE_API void PWindowGetMousePos(int32* OutPosX, int32* OutPosY);
ENGINE_API void PWindowSetMousePos(int32 PosX, int PosY);

// Time Api //================================================================================================//
ENGINE_API double PGetTime();
ENGINE_API void PWait(double Seconds);

// File System Api //=========================================================================================//
ENGINE_API cstring PGetUserDataPath();
ENGINE_API bool PFileExists(cstring Path);
ENGINE_API bool PFileDelete(cstring Path);
ENGINE_API bool PFileMove(cstring SrcPath, cstring DstPath);
ENGINE_API bool PFileCopy(cstring SrcPath, cstring DstPath, bool bOverride);
ENGINE_API bool PFileOpen(cstring Path, EPlatformFileMode Mode, PFile* OutFile);
ENGINE_API void PFileClose(PFile* Self);
ENGINE_API bool PFileRead(PFile* Self, uint64 Offset, uint64 ReadSize, uint8* OutBuffer, uint64* OutBytesRead);
ENGINE_API bool PFileWrite(PFile* Self, uint64 Offset, uint64 WriteSize, uint8* InBuffer, uint64* OutBytesWritten);

ENGINE_API bool PDirExists(cstring Path);
ENGINE_API bool PDirCreate(cstring Path);
ENGINE_API bool PDirDelete(cstring Path);
ENGINE_API bool PDirOpen(cstring Path, PDir* OutDir);
ENGINE_API void PDirClose(PDir* Self);
ENGINE_API bool PDirRead(PDir* Self, PDirEntry* OutEntry);

// Memory Api //==============================================================================================//
ENGINE_API void PMemFree(void* Data);
ENGINE_API void* PMemAlloc(uint64 Size);
ENGINE_API void* PMemRealloc(void* Data, uint64 Size);
ENGINE_API void* PMemCopy(void* Src, void* Dst, uint64 Size);
ENGINE_API void* PMemMove(void* Src, void* Dst, uint64 Size);
ENGINE_API void* PMemSet(void* Dst, int32 Value, uint64 Size);
ENGINE_API void* PMemAllocAligned(uint64 Size, uint64 Alignment);
ENGINE_API void PMemFreeAligned(void* Data);

GT_EXTERN_C_END
