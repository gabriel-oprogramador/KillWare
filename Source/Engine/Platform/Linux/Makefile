include Project.mk
.SILENT:

FLAGS += -DPLATFORM_LINUX
FLAGS_CC = -std=gnu11
FLAGS_CX = -std=gnu++17
FLAGS_LD = -lm -ldl -lX11 -lXi -lGL -lXrender

ifeq ($(BUILD_TYPE), Shipping)
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
	$(LD) $(OBJ) $(FLAGS) $(FLAGS_LD) -o $(APP_BINARY)

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
