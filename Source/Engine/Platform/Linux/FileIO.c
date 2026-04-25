#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "Platform/Platform.h"
#include "Platform/Log.h"

bool PFileExists(cstring Path) {
  GT_ASSERT(Path);
  if(!Path) {
    return false;
  }
  struct stat st;
  if(stat(Path, &st) != 0) {
    return false;
  }
  return S_ISREG(st.st_mode);
}

bool PFileDelete(cstring Path) {
  GT_ASSERT(Path);
  if(!Path) {
    return false;
  }
  if(unlink(Path) == 0) {
    return true;
  }
  if(errno == ENOENT) {
    return true;
  }
  return false;
}

bool PFileMove(cstring SrcPath, cstring DstPath) {
  GT_ASSERT(SrcPath && DstPath);
  if(!SrcPath || !DstPath) {
    return false;
  }
  if(rename(SrcPath, DstPath) == 0) {
    return true;
  }
  if(errno == EXDEV) {
    // fallback cross-filesystem
    if(!PFileCopy(SrcPath, DstPath, true)) {
      return false;
    }
    return (unlink(SrcPath) == 0);
  }
  return false;
}

bool PFileCopy(cstring SrcPath, cstring DstPath, bool bOverride) {
  GT_ASSERT(SrcPath && DstPath);
  if(!SrcPath || !DstPath) {
    return false;
  }
  int src = open(SrcPath, O_RDONLY);
  if(src < 0) {
    return false;
  }
  int flags = O_WRONLY | O_CREAT;
  flags |= bOverride ? O_TRUNC : O_EXCL;
  int dst = open(DstPath, flags, 0644);
  if(dst < 0) {
    close(src);
    return false;
  }
  char buffer[64 * 1024];
  ssize_t bytesRead;
  while((bytesRead = read(src, buffer, sizeof(buffer))) > 0) {
    ssize_t totalWritten = 0;
    while(totalWritten < bytesRead) {
      ssize_t written = write(dst, buffer + totalWritten, bytesRead - totalWritten);
      if(written <= 0) {
        close(src);
        close(dst);
        return false;
      }
      totalWritten += written;
    }
  }
  close(src);
  close(dst);
  return (bytesRead == 0);
}

bool PFileOpen(cstring Path, EPlatformFileMode Mode, PFile* OutFile) {
  GT_ASSERT(Path && OutFile);
  if(!Path || !OutFile) {
    return false;
  }
  PMemSet(OutFile, 0, sizeof(PFile));
  int flags = 0;
  switch(Mode) {
    case PLATFORM_FILE_READ: {
      flags = O_RDONLY;
      break;
    }
    case PLATFORM_FILE_WRITE: {
      flags = O_WRONLY | O_CREAT | O_TRUNC;
      break;
    }
    case PLATFORM_FILE_APPEND: {
      flags = O_WRONLY | O_CREAT | O_APPEND;
      break;
    }
  }
  int fd = open(Path, flags, 0644);
  if(fd < 0) {
    return false;
  }
  struct stat st;
  if(fstat(fd, &st) == 0) {
    OutFile->size = st.st_size;
    OutFile->createdTime = st.st_ctime;
    OutFile->lastAccessTime = st.st_atime;
    OutFile->lastWriteTime = st.st_mtime;
  }
  OutFile->handle = (void*)(intptr_t)fd;
  OutFile->mode = Mode;
  snprintf(OutFile->path, sizeof(OutFile->path), "%s", Path);
  return true;
}

void PFileClose(PFile* Self) {
  GT_ASSERT(Self && Self->handle);
  if(!Self || !Self->handle) {
    return;
  }
  int fd = (int)(intptr_t)Self->handle;
  close(fd);
  Self->handle = NULL;
}

bool PFileRead(PFile* Self, uint64 Offset, uint64 ReadSize, uint8* OutBuffer, uint64* OutBytesRead) {
  static const uint32 MAX_READ_CHUNK = 64 * 1024 * 1024;
  GT_ASSERT(Self && Self->handle && OutBuffer);
  if(!Self || !Self->handle || !OutBuffer) {
    return false;
  }
  int fd = (int)(intptr_t)Self->handle;
  if(Offset >= Self->size) {
    return false;
  }
  uint64 remaining = ReadSize;
  if(Offset + ReadSize > Self->size) {
    remaining = Self->size - Offset;
  }
  if(lseek(fd, Offset, SEEK_SET) < 0) {
    return false;
  }
  uint8* buffer = OutBuffer;
  uint64 total = 0;
  while(remaining > 0) {
    size_t chunk = remaining > MAX_READ_CHUNK ? MAX_READ_CHUNK : (size_t)remaining;
    ssize_t bytes = read(fd, buffer, chunk);
    if(bytes < 0) {
      return false;
    }
    if(bytes == 0) {
      break;
    }
    buffer += bytes;
    remaining -= bytes;
    total += bytes;
  }
  if(OutBytesRead) {
    *OutBytesRead = total;
  }
  return true;
}

bool PFileWrite(PFile* Self, uint64 Offset, uint64 WriteSize, uint8* InBuffer, uint64* OutBytesWritten) {
  static const uint32 MAX_WRITE_CHUNK = 64 * 1024 * 1024;
  GT_ASSERT(Self && Self->handle && InBuffer);
  if(!Self || !Self->handle || !InBuffer) {
    return false;
  }
  if(Self->mode != PLATFORM_FILE_WRITE && Self->mode != PLATFORM_FILE_APPEND) {
    return false;
  }
  int fd = (int)(intptr_t)Self->handle;
  if(Self->mode != PLATFORM_FILE_APPEND) {
    if(lseek(fd, Offset, SEEK_SET) < 0) {
      return false;
    }
  }
  const uint8* buffer = InBuffer;
  uint64 remaining = WriteSize;
  uint64 total = 0;
  while(remaining > 0) {
    size_t chunk = remaining > MAX_WRITE_CHUNK ? MAX_WRITE_CHUNK : (size_t)remaining;
    ssize_t written = write(fd, buffer, chunk);
    if(written <= 0) {
      return false;
    }

    buffer += written;
    remaining -= written;
    total += written;
  }
  if(Offset + total > Self->size) {
    Self->size = Offset + total;
  }
  if(OutBytesWritten) {
    *OutBytesWritten = total;
  }
  return true;
}

#endif  // PLATFORM_LINUX
