#pragma once
#include "Core/CoreTypes.h"
#include "Core/Defines.h"

typedef enum EPlatformFileMode {
  PLATFORM_FILE_READ,    //
  PLATFORM_FILE_WRITE,   //
  PLATFORM_FILE_APPEND,  //
} EPlatformFileMode;

typedef struct {
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
  char path[GT_MAX_PATH];
  uint64 size;
  uint64 createdTime;
  uint64 lastWriteTime;
  uint64 lastAccessTime;
  bool bIsDirectory;
} PDirEntry;

typedef struct PFile {
  void* handle;
  uint64 size;
  uint64 createdTime;
  uint64 lastWriteTime;
  uint64 lastAccessTime;
  EPlatformFileMode mode;
} PFile;

typedef struct PFileStream {
} PFileStream;

GT_EXTERN_C_BEGIN

// Window Api //==============================================================================================//
ENGINE_API void PWindowClose();
ENGINE_API bool PWindowIsMouseCaptured();
ENGINE_API void PWindowSetMouseCaptured(bool bCapture);
ENGINE_API bool PWindowIsFullscreen();
ENGINE_API void PWindowSetFullscreen(bool bFullscreen);
ENGINE_API void PWindowGetMousePos(int32* OutPosX, int32* OutPosY);
ENGINE_API void PWindowSetMousePos(int32 PosX, int PosY);
ENGINE_API PKey* PWindowGetKeyMap(uint8* OutCount);

// Time Api //================================================================================================//
ENGINE_API double PGetTime();
ENGINE_API void PWait(double Seconds);

// File System Api //=========================================================================================//
ENGINE_API bool PDirExist(cstring Path);
ENGINE_API bool PDirMake(cstring Path);
ENGINE_API bool PDirDelete(cstring Path, bool bRecursive);
ENGINE_API bool PDirCopy(cstring Dst, cstring Src, uint8 Depth, bool bOverride);
ENGINE_API bool PDirOpen(cstring Path, PDir* OutDir);
ENGINE_API void PDirClose(PDir* Dir);
ENGINE_API bool PDirRead(PDir* Dir, PDirEntry* OutEntry);

ENGINE_API cstring PGetUserDataPath();

ENGINE_API bool PFileExist(cstring Path);
ENGINE_API bool PFileDelete(cstring Path);
ENGINE_API bool PFileCopy(cstring Dst, cstring Src, bool bOverride);
ENGINE_API bool PFileOpen(cstring Path, EPlatformFileMode Mode, PFile* OutFile);
ENGINE_API void PFileClose(PFile* File);
ENGINE_API bool PFileRead(PFile* File, uint64 Offset, uint64 ReadSize, uint8* OutFileBuffer);

// Memory Api //==============================================================================================//
ENGINE_API void PMemFree(void* Data);
ENGINE_API void* PMemAlloc(uint64 Size);
ENGINE_API void* PMemRealloc(void* Data, uint64 Size);
ENGINE_API void* PMemCopy(void* Dst, void* Src, uint64 Size);
ENGINE_API void* PMemMove(void* Dst, void* Src, uint64 Size);
ENGINE_API void* PMemSet(void* Dst, int32 Value, uint64 Size);

GT_EXTERN_C_END
