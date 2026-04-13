#include "CoreMinimal.h"
#include "Platform/Platform.h"
#include "ThirdParty/STB/stb_image.h"

// Arquivo de Testes codigos nunca iram pra Engine

static void FileSystemTests();
static void LoadTexture(cstring Path);
static void StreamUpdate();

typedef enum EStreamState {
  STREAM_STATE_UNLOADED,    // 0 - Não carregado
  STREAM_STATE_REQUESTED,   // 1 - Pedido para carregar
  STREAM_STATE_LOADING,     // 2 - Carregando dados do disco
  STREAM_STATE_PROCESSING,  // 3 - Processando (descompressão, decodificação)
  STREAM_STATE_READY,       // 4 - Pronto para usar
  STREAM_STATE_ERROR,       // 5 - Erro no carregamento
  STREAM_STATE_UNLOADING    // 6 - Descarregando da memória
} EStreamState;

typedef struct FImage {
  uint8* data;
  uint32 width;
  uint32 height;
  uint32 format;
} FImage;

typedef struct FStream {
  PFile pFile;
  cstring path;
  uint8* buffer;
  uint64 offset;
  EStreamState state;
  FImage image;
} FStream;

#define MAX_STREAM_QUEUE 1024
static struct {
  FStream assets[MAX_STREAM_QUEUE];
  uint32 count;
} SStreamQueue;

void TestsStart() {
  FileSystemTests();
  LoadTexture("Content/Logo.png");
  //LoadTexture("Content/Logo8K.png");
}

void TestsUpdate() {
  StreamUpdate();
}

void TestsStop() {
}

// Platform File System
static void FileSystemTests() {
  cstring inputOrige = "Config/InputMapping.cfg";
  cstring inputDelete = "Config/InputDelete.cfg";
  cstring inputCopy = "Config/InputCopy.cfg";
  cstring inputMoveTo = "Config/InputTo.cfg";
  cstring inputMoveFrom = "Config/InputFrom.cfg";
  PFile file;
  if(PFileOpen(inputOrige, PLATFORM_FILE_READ, &file)) {
    uint8* buffer = (uint8*)PMemAlloc(file.size);
    if(PFileRead(&file, 0, file.size, buffer, NULL)) {
      PFileCopy(file.path, inputCopy, true);
      PFileCopy(file.path, inputDelete, true);
      PFileCopy(file.path, inputMoveTo, true);
      PFileMove(inputMoveTo, inputMoveFrom);
      PFile inputNew;
      if(PFileOpen("Config/InputNew.cfg", PLATFORM_FILE_WRITE, &inputNew)) {
        PFileWrite(&inputNew, 0, file.size, buffer, NULL);
        PFileClose(&inputNew);
      }
    }
    PMemFree(buffer);
    PFileClose(&file);
  }
  if(PFileExists(inputDelete)) {
    GT_ALERT("File exists before PFileDelete() -> %s", inputDelete);
  }
  PFileDelete(inputDelete);
  if(PFileExists(inputDelete)) {
    GT_ALERT("File exists after PFileDelete() -> %s", inputDelete);
  }

  cstring soundsPath = "Content/Sounds";
  if(!PDirExists(soundsPath)) {
    if(!PDirCreate(soundsPath)) {
      GT_ALERT("%s not created", soundsPath);
    }
  } else {
    if(!PDirDelete(soundsPath)) {
      GT_ALERT("%s not deleted", soundsPath);
    }
  }
  PDir assets;
  if(PDirOpen("Content", &assets)) {
    PDirEntry entry;
    while(PDirRead(&assets, &entry)) {
      if(entry.bIsDirectory) {
        GT_INFO("[Dir ] Name:%s, Path:%s, size:%llu", entry.name, entry.path, entry.size);
      } else {
        GT_INFO("[FILE] Name:%s, Path:%s, size:%llu", entry.name, entry.path, entry.size);
      }
    }
  }
  PDirClose(&assets);
}

static void LoadTexture(cstring Path) {
  if(SStreamQueue.count >= MAX_STREAM_QUEUE) {
    return;
  }
  FStream stream = {};
  stream.state = STREAM_STATE_UNLOADED;
  stream.buffer = NULL;
  stream.path = Path;
  SStreamQueue.assets[SStreamQueue.count++] = stream;
}

static void StreamUpdate() {
  for(uint32 c = 0; c < SStreamQueue.count;) {
    FStream* stream = &SStreamQueue.assets[c];
    bool bAdvance = true;

    switch((uint8)stream->state) {
      case STREAM_STATE_UNLOADED: {
        if(!PFileOpen(stream->path, PLATFORM_FILE_READ, &stream->pFile)) {
          stream->state = STREAM_STATE_ERROR;
          GT_ERROR("Failed to open: %s", stream->path);
        } else {
          stream->state = STREAM_STATE_REQUESTED;
        }
        break;
      }

      case STREAM_STATE_REQUESTED: {
        stream->buffer = (uint8*)PMemAlloc(stream->pFile.size);
        if(!stream->buffer) {
          stream->state = STREAM_STATE_ERROR;
          GT_ERROR("Failed to allocate %llu bytes", stream->pFile.size);
        } else {
          stream->offset = 0;
          stream->state = STREAM_STATE_LOADING;
        }
        break;
      }

      case STREAM_STATE_LOADING: {
        const uint64 CHUNK_SIZE = 64 * 1024;
        uint64 bytesRead = 0;
        uint64 readSize = CHUNK_SIZE;

        if(stream->offset + readSize > stream->pFile.size) {
          readSize = stream->pFile.size - stream->offset;
        }

        if(!PFileRead(&stream->pFile, stream->offset, readSize, stream->buffer + stream->offset, &bytesRead)) {
          stream->state = STREAM_STATE_ERROR;
          GT_ERROR("Read failed at offset %llu", stream->offset);
        } else {
          stream->offset += bytesRead;
          GT_INFO("Loaded %llu/%llu bytes", stream->offset, stream->pFile.size);

          if(stream->offset >= stream->pFile.size) {
            stream->state = STREAM_STATE_PROCESSING;
          }
        }
        break;
      }

      case STREAM_STATE_PROCESSING: {
        int32 w, h, f;
        stbi_uc* data = stbi_load_from_memory(stream->buffer, (int)stream->pFile.size, &w, &h, &f, 4);
        if(!data) {
          stream->state = STREAM_STATE_ERROR;
          GT_ERROR("STB failed: %s", stbi_failure_reason());
        } else {
          stream->image.data = data;
          stream->image.width = w;
          stream->image.height = h;
          stream->image.format = 4;
          stream->state = STREAM_STATE_READY;
        }
        break;
      }

      case STREAM_STATE_READY: {
        GT_INFO("Image loaded: %s (%ux%u)", stream->path, stream->image.width, stream->image.height);

        // UploadTextureToGPU(&stream->image);

        if(stream->image.data) {
          stbi_image_free(stream->image.data);
        }
        PFileClose(&stream->pFile);
        PMemFree(stream->buffer);

        for(uint32 i = c; i < SStreamQueue.count - 1; i++) {
          SStreamQueue.assets[i] = SStreamQueue.assets[i + 1];
        }
        SStreamQueue.count--;
        bAdvance = false;  // Não avança
        break;
      }

      case STREAM_STATE_ERROR: {
        GT_ERROR("Failed: %s", stream->path);

        if(stream->pFile.handle)
          PFileClose(&stream->pFile);
        if(stream->buffer)
          PMemFree(stream->buffer);

        for(uint32 i = c; i < SStreamQueue.count - 1; i++) {
          SStreamQueue.assets[i] = SStreamQueue.assets[i + 1];
        }
        SStreamQueue.count--;
        bAdvance = false;
        break;
      }
    }

    if(bAdvance) {
      c++;
    }
  }
}
