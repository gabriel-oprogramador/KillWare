#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <windowsx.h>

#include "Platform/Platform.h"
#include "Platform/Log.h"

bool PDirExists(cstring Path) {
  GT_ASSERT(Path);
  if(!Path) {
    return false;
  }
  DWORD attr = GetFileAttributesA(Path);
  if(attr == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  bool retVal = ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
  return retVal;
}

bool PDirCreate(cstring Path) {
  if(!CreateDirectoryA(Path, NULL)) {
    if(GetLastError() != ERROR_ALREADY_EXISTS) {
      return false;
    }
  }
  return true;
}

bool PDirDelete(cstring Path) {
  GT_ASSERT(Path);
  if(!Path) {
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
  if(!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
    return false;
  }
  SetFileAttributesA(Path, FILE_ATTRIBUTE_NORMAL);
  bool retVal = ((RemoveDirectoryA(Path) != 0));
  return retVal;
}

bool PDirOpen(cstring Path, PDir* OutDir) {
  GT_ASSERT(Path && OutDir);
  if(!Path || !OutDir) {
    return false;
  }
  PMemSet(OutDir, 0, sizeof(PDir));
  char searchPath[GT_MAX_PATH];
  snprintf(searchPath, sizeof(searchPath), "%s\\*", Path);
  WIN32_FIND_DATAA findData;
  HANDLE handle = FindFirstFileA(searchPath, &findData);
  if(handle == INVALID_HANDLE_VALUE) {
    return false;
  }
  OutDir->handle = handle;
  OutDir->bIsOpen = true;
  snprintf(OutDir->path, sizeof(OutDir->path), "%s", Path);
  return true;
}

void PDirClose(PDir* Self) {
  GT_ASSERT(Self);
  if(Self == NULL || Self->bIsOpen == false) {
    return;
  }
  FindClose(Self->handle);
  Self->bIsOpen = false;
}

bool PDirRead(PDir* Self, PDirEntry* OutEntry) {
  GT_ASSERT(Self && OutEntry);
  if(Self == NULL || OutEntry == NULL) {
    return false;
  }
  HANDLE handle = Self->handle;
  WIN32_FIND_DATAA findData;
  while(true) {
    if(!FindNextFileA(handle, &findData)) {
      return false;
    }
    if(strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
      continue;
    }
    break;
  }
  PMemSet(OutEntry, 0, sizeof(PDirEntry));
  snprintf(OutEntry->name, sizeof(OutEntry->name), "%s", findData.cFileName);
  snprintf(OutEntry->path, sizeof(OutEntry->path), "%s\\%s", Self->path, findData.cFileName);
  OutEntry->bIsDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  OutEntry->size = ((uint64)findData.nFileSizeHigh << 32) | (uint64)findData.nFileSizeLow;
  OutEntry->createdTime = ((uint64)findData.ftCreationTime.dwHighDateTime << 32) | (uint64)findData.ftCreationTime.dwLowDateTime;
  OutEntry->lastWriteTime = ((uint64)findData.ftLastWriteTime.dwHighDateTime << 32) | (uint64)findData.ftLastWriteTime.dwLowDateTime;
  OutEntry->lastAccessTime = ((uint64)findData.ftLastAccessTime.dwHighDateTime << 32) | (uint64)findData.ftLastAccessTime.dwLowDateTime;
  return true;
}
#endif  // PLATFORM_WINDOWS
