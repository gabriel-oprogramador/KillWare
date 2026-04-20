#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <windowsx.h>

#include "Platform/Platform.h"
#include "Platform/Log.h"

bool PFileExists(cstring Path) {
  GT_ASSERT(Path);
  if(Path == NULL) {
    return false;
  }
  DWORD attr = GetFileAttributesA(Path);
  if(attr == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  bool retVal = ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
  return retVal;
}

bool PFileDelete(cstring Path) {
  GT_ASSERT(Path);
  if(Path == NULL) {
    return false;
  }
  DWORD attr = GetFileAttributesA(Path);
  if(attr == INVALID_FILE_ATTRIBUTES) {
    DWORD err = GetLastError();
    if(err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
      return true;
    }
    return false;
  }
  if(attr & FILE_ATTRIBUTE_DIRECTORY) {
    return false;
  }
  SetFileAttributesA(Path, FILE_ATTRIBUTE_NORMAL);
  bool retVal = (DeleteFileA(Path) != 0);
  return retVal;
}

bool PFileMove(cstring SrcPath, cstring DstPath) {
  GT_ASSERT(SrcPath && DstPath);
  if(!SrcPath || !DstPath) {
    return false;
  }
  bool retVal = (MoveFileExA(SrcPath, DstPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) != 0);
  return retVal;
}

bool PFileCopy(cstring SrcPath, cstring DstPath, bool bOverride) {
  GT_ASSERT(SrcPath && DstPath);
  if(!SrcPath || !DstPath) {
    return false;
  }
  bool retVal = (CopyFileA(SrcPath, DstPath, !bOverride) != 0);
  return retVal;
}

bool PFileOpen(cstring Path, EPlatformFileMode Mode, PFile* OutFile) {
  GT_ASSERT(Path && OutFile);
  if(!Path || !OutFile) {
    return false;
  }
  PMemSet(OutFile, 0, sizeof(PFile));
  DWORD access = 0;
  DWORD creation = 0;
  switch(Mode) {
    case PLATFORM_FILE_READ: {
      access = GENERIC_READ;
      creation = OPEN_EXISTING;
      break;
    }
    case PLATFORM_FILE_WRITE: {
      access = GENERIC_WRITE;
      creation = CREATE_ALWAYS;
      break;
    }
    case PLATFORM_FILE_APPEND: {
      access = FILE_APPEND_DATA;
      creation = OPEN_ALWAYS;
      break;
    }
  }
  HANDLE hFile = CreateFileA(Path, access, FILE_SHARE_READ, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
  if(hFile == INVALID_HANDLE_VALUE) {
    return false;
  }
  if(Mode == PLATFORM_FILE_APPEND) {
    SetFilePointer(hFile, 0, NULL, FILE_END);
  }
  LARGE_INTEGER size;
  if(GetFileSizeEx(hFile, &size)) {
    OutFile->size = size.QuadPart;
  }
  FILETIME ftCreate, ftAccess, ftWrite;
  if(GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite)) {
    OutFile->createdTime = ((uint64)ftCreate.dwHighDateTime << 32) | ftCreate.dwLowDateTime;
    OutFile->lastAccessTime = ((uint64)ftAccess.dwHighDateTime << 32) | ftAccess.dwLowDateTime;
    OutFile->lastWriteTime = ((uint64)ftWrite.dwHighDateTime << 32) | ftWrite.dwLowDateTime;
  }
  OutFile->handle = (void*)hFile;
  OutFile->mode = Mode;
  snprintf(OutFile->path, sizeof(OutFile->path), "%s", Path);
  return true;
}

void PFileClose(PFile* Self) {
  GT_ASSERT(Self && Self->handle);
  if(!Self || !Self->handle) {
    return;
  }
  CloseHandle(Self->handle);
  Self->handle = NULL;
}

bool PFileRead(PFile* Self, uint64 Offset, uint64 ReadSize, uint8* OutBuffer, uint64* OutBytesRead) {
  static const uint32 MAX_READ_CHUNK = 64 * 1024 * 1024;  // 64MB
  GT_ASSERT(Self && Self->handle && OutBuffer);
  if(!Self || !Self->handle || !OutBuffer) {
    return false;
  }
  if(Offset >= Self->size) {
    return false;
  }
  uint8* buffer = OutBuffer;
  HANDLE handle = (HANDLE)Self->handle;
  uint64 remaining = ReadSize;
  if(Offset + ReadSize > Self->size) {
    remaining = Self->size - Offset;
  }
  LARGE_INTEGER li;
  li.QuadPart = Offset;
  if(!SetFilePointerEx(handle, li, NULL, FILE_BEGIN)) {
    return false;
  }
  uint64 totalBytesRead = 0;
  while(remaining > 0) {
    DWORD chunk = (remaining > MAX_READ_CHUNK) ? MAX_READ_CHUNK : (DWORD)remaining;
    DWORD bytesRead = 0;
    if(!ReadFile(handle, buffer, chunk, &bytesRead, NULL)) {
      return false;
    }
    if(bytesRead == 0) {
      break;
    }
    buffer += bytesRead;
    remaining -= bytesRead;
    totalBytesRead += bytesRead;
  }
  if(OutBytesRead) {
    *OutBytesRead = totalBytesRead;
  }
  return true;
}

bool PFileWrite(PFile* Self, uint64 Offset, uint64 WriteSize, uint8* InBuffer, uint64* OutBytesWritten) {
  static const uint32 MAX_WRITE_CHUNK = 64 * 1024 * 1024;  // 64MB
  GT_ASSERT(Self && Self->handle && InBuffer);
  if(!Self || !Self->handle || !InBuffer) {
    return false;
  }
  if(Self->mode != PLATFORM_FILE_WRITE && Self->mode != PLATFORM_FILE_APPEND) {
    return false;
  }
  const uint8* buffer = InBuffer;
  HANDLE handle = (HANDLE)Self->handle;
  uint64 remaining = WriteSize;
  LARGE_INTEGER li;
  li.QuadPart = Offset;
  if(!SetFilePointerEx(handle, li, NULL, FILE_BEGIN)) {
    return false;
  }
  uint64 totalWritten = 0;
  while(remaining > 0) {
    DWORD chunk = (remaining > MAX_WRITE_CHUNK) ? MAX_WRITE_CHUNK : (DWORD)remaining;
    DWORD bytesWritten = 0;
    if(!WriteFile(handle, buffer, chunk, &bytesWritten, NULL)) {
      return false;
    }
    if(bytesWritten == 0) {
      break;
    }
    buffer += bytesWritten;
    remaining -= bytesWritten;
    totalWritten += bytesWritten;
  }
  if(Offset + totalWritten > Self->size) {
    Self->size = Offset + totalWritten;
  }
  if(OutBytesWritten) {
    *OutBytesWritten = totalWritten;
  }
  return totalWritten > 0;
}

#endif  // PLATFORM_WINDOWS
