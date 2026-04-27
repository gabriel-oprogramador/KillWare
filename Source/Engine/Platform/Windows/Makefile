include Project.mk
.SILENT:

FLAGS += -DPLATFORM_WINDOWS
FLAGS_CC = -std=c11
FLAGS_CX = -std=c++17
	FLAGS_LD = -luser32 -lgdi32 -lopengl32 -lbcrypt

ifeq ($(BUILD_TYPE), Shipping)
FLAGS_LD += -mwindows
define COPY_CONTENT
@echo "Packaging -> $(BIN_OUTPUT)/$(CONTENT_DIR)"
rm -rf $(BIN_OUTPUT)/$(CONTENT_DIR)
cp -a ./$(CONTENT_DIR) ./$(BIN_OUTPUT)/$(CONTENT_DIR)
cp -a ./$(CONFIG_DIR) ./$(BIN_OUTPUT)/$(CONFIG_DIR)
endef
endif

prebuild:
	$(GENERATE_COMPILE_COMMANDS)

build: $(OBJ)
	echo 'Linking -> $(APP_BINARY)'
	windres $(PLATFORM_DIR)/Resource.rc -O coff -o $(PLATFORM_DIR)/Resource.res
	$(LD) $(PLATFORM_DIR)/Resource.res $(OBJ) $(FLAGS) $(FLAGS_LD) -o $(APP_BINARY)

posbuild:
	$(COPY_CONTENT)

run:
	echo 'Running -> $(APP_BINARY)'
	./$(APP_BINARY)

$(OBJ_OUTPUT)/%.o: $(SOURCE_DIR)/%.c
	echo 'Compiling >> $<'
	$(CC) $< $(FLAGS) $(FLAGS_CC) -c -o $@ -MMD

$(OBJ_OUTPUT)/%.o: $(SOURCE_DIR)/%.cpp
	echo 'Compiling >> $<'
	$(CX) $< $(FLAGS) $(FLAGS_CX) -c -o $@ -MMD

sinclude $(DEPS)
