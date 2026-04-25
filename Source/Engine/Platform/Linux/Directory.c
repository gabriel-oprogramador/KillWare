#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include "Platform/Platform.h"
#include "Platform/Log.h"

bool PDirExists(cstring Path) {
  GT_ASSERT(Path);
  if(!Path) {
    return false;
  }
  struct stat st;
  if(stat(Path, &st) != 0) {
    return false;
  }
  return S_ISDIR(st.st_mode);
}

bool PDirCreate(cstring Path) {
  if(mkdir(Path, 0755) != 0) {
    if(errno != EEXIST) {
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
  struct stat st;
  if(stat(Path, &st) != 0) {
    if(errno == ENOENT) {
      return true;
    }
    return false;
  }
  if(!S_ISDIR(st.st_mode)) {
    return false;
  }
  bool retVal = (rmdir(Path) == 0);
  return retVal;
}

bool PDirOpen(cstring Path, PDir* OutDir) {
  GT_ASSERT(Path && OutDir);
  if(!Path || !OutDir) {
    return false;
  }
  PMemSet(OutDir, 0, sizeof(PDir));
  DIR* dir = opendir(Path);
  if(!dir) {
    return false;
  }
  OutDir->handle = dir;
  OutDir->bIsOpen = true;
  snprintf(OutDir->path, sizeof(OutDir->path), "%s", Path);
  return true;
}

void PDirClose(PDir* Self) {
  GT_ASSERT(Self);
  if(!Self || !Self->bIsOpen) {
    return;
  }
  closedir((DIR*)Self->handle);
  Self->bIsOpen = false;
}

bool PDirRead(PDir* Self, PDirEntry* OutEntry) {
  GT_ASSERT(Self && OutEntry);
  if(!Self || !OutEntry) {
    return false;
  }
  struct dirent* entry;
  while(true) {
    entry = readdir((DIR*)Self->handle);
    if(!entry) {
      return false;
    }
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    PMemSet(OutEntry, 0, sizeof(PDirEntry));
    snprintf(OutEntry->name, sizeof(OutEntry->name), "%s", entry->d_name);
    snprintf(OutEntry->path, sizeof(OutEntry->path), "%s/%s", Self->path, entry->d_name);
    struct stat st;
    if(stat(OutEntry->path, &st) == 0) {
      OutEntry->bIsDirectory = S_ISDIR(st.st_mode);
      OutEntry->size = (uint64)st.st_size;
      OutEntry->createdTime = st.st_ctime;  // TODO: não é criação real no Linux usar statx
      OutEntry->lastWriteTime = st.st_mtime;
      OutEntry->lastAccessTime = st.st_atime;
    }
    return true;
  }
}

#endif  // PLATFORM_LINUX
