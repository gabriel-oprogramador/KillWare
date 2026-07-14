include Project.mk
.SILENT:
EMSDK_UNIX := $(subst \,/,$(EMSDK))

FLAGS += -DPLATFORM_WEB
FLAGS += --target=wasm32-unknown-emscripten --sysroot=$(EMSDK_UNIX)/upstream/emscripten/cache/sysroot
FLAGS_CC = -std=gnu11
FLAGS_CX = -std=gnu++17
FLAGS_LD = -s ASYNCIFY -s ASSERTIONS=2 -s FETCH=1  -s USE_WEBGL2=1 -s FULL_ES3=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 --preload-file Content@/Content
LD = emcc
CC = emcc
CX = emcc++

OUTPUT_NAME = index
APP_EXTENSION = .js

prebuild:
	$(GENERATE_COMPILE_COMMANDS)

build: $(OBJ)
	echo 'Platform Dir -> $(PLATFORM_DIR)'
	echo 'Linking -> $(APP_BINARY)'
	$(LD) $(OBJ) $(FLAGS) $(FLAGS_LD) -o $(APP_BINARY)

posbuild:
	cp $(PLATFORM_DIR)/WebEntrypoint.html $(BIN_OUTPUT)/index.html
	cp $(PLATFORM_DIR)/favicon.ico $(BIN_OUTPUT)/favicon.ico

run:
	echo 'Running -> $(APP_BINARY)'
	live-server ./$(BIN_OUTPUT)

$(OBJ_OUTPUT)/%.o: $(SOURCE_DIR)/%.c
	echo 'Compiling >> $<'
	$(CC) $< $(FLAGS) $(FLAGS_CC) -c -o $@ -MMD

$(OBJ_OUTPUT)/%.o: $(SOURCE_DIR)/%.cpp
	echo 'Compiling >> $<'
	$(CX) $< $(FLAGS) $(FLAGS_CX) -c -o $@ -MMD

sinclude $(DEPS)
