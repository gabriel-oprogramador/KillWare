# GT-Engine Project Configuration

# Game Project
GAME_NAME = KillWare
PROJECT_NAME = $(notdir $(CURDIR))
PROJECT_PATH = $(CURDIR)

INCLUDES = -ISource/Game -ISource/Engine -ISource/Engine/Runtime -ISource/Engine/ThirdParty 
DEFINES = -DGAME_NAME=$(GAME_NAME) -DPROJECT_NAME=$(PROJECT_NAME) -DPROJECT_PATH=$(PROJECT_PATH)

# Project Folders
BINARIES_DIR     = Binaries
INTERMEDIATE_DIR = Intermediate
OBJECT_DIR       = Build
SOURCE_DIR       = Source
CONFIG_DIR       = Config
CONTENT_DIR      = Content
PLATFORM_DIR     = $(SOURCE_DIR)/Engine/Platform/$(PLATFORM)

BIN_OUTPUT = $(BINARIES_DIR)/$(BUILD_TYPE)/$(PLATFORM)
OBJ_OUTPUT = $(INTERMEDIATE_DIR)/$(OBJECT_DIR)/$(BUILD_TYPE)/$(PLATFORM)

# Project Files
SRC_CC = $(shell find $(SOURCE_DIR) -type f -name "*.c")
SRC_CX = $(shell find $(SOURCE_DIR) -type f -name "*.cpp")
OBJ_CC = $(patsubst $(SOURCE_DIR)/%.c,$(OBJ_OUTPUT)/%.o,$(SRC_CC))
OBJ_CX = $(patsubst $(SOURCE_DIR)/%.cpp,$(OBJ_OUTPUT)/%.o,$(SRC_CX))
SRC    = $(SRC_CC) $(SRC_CX)
OBJ    = $(OBJ_CC) $(OBJ_CX)
DEPS   = $(OBJ:.o=.d)
DIRS   = $(dir $(OBJ))

# Setup Host
ifeq ($(OS), Windows_NT)
HOST = windows
PLATFORM ?= Windows
APP_EXTENSION = .exe
NULL_DEVICE = NUL
else ifeq ($(shell uname), Linux)
HOST = linux
PLATFORM ?= Linux
APP_EXTENSION=
NULL_DEVICE = /dev/null
endif

# Setup Build Type
ifeq ($(BUILD_TYPE), Development)
DEFINES += -DDEVELOPMENT_MODE
FLAGS += $(DEFINES) $(INCLUDES) -O0 -g -Wall
OUTPUT_NAME = $(PROJECT_NAME)
else
DEFINES += -DSHIPPING_MODE
FLAGS += $(DEFINES) $(INCLUDES) -O2
OUTPUT_NAME = $(GAME_NAME)
endif

# Setup Platform Target
TARGET_MAKEFILE = $(PLATFORM_DIR)/Makefile
APP_BINARY = $(BIN_OUTPUT)/$(OUTPUT_NAME)$(APP_EXTENSION)

# Setup Compiler
HAS_CLANG := $(shell command -v clang > $(NULL_DEVICE) && echo 1 || echo 0)
ifeq ($(HAS_CLANG), 1)
COMPILER = Clang
CC = clang
CX = clang++
else
COMPILER = GCC
CC = gcc
CX = g++
endif
LD = $(if $(strip $(SRC_CX)),$(CX),$(CC))

# Setup Defines
define CREATE_DIRS
	mkdir -p $(BIN_OUTPUT)
	mkdir -p $(OBJ_OUTPUT)
	mkdir -p $(CONFIG_DIR)
	mkdir -p $(CONTENT_DIR)
	mkdir -p $(DIRS)
endef

# Setup Compile Commands
define GENERATE_COMPILE_COMMANDS
	@echo 'Generate Compile Commands'
	@echo "[" > compile_commands.json; \
	i=0; \
	for src in $(SRC_CC); do \
		if [ $$i -ne 0 ]; then echo "," >> compile_commands.json; fi; \
		echo "  {" >> compile_commands.json; \
		echo '    "directory": "$(abspath .)",' >> compile_commands.json; \
		echo '    "command": "clang $(FLAGS) $(FLAGS_CC) -c '"$$src"'",' >> compile_commands.json; \
		echo '    "file": "'"$$src"'"' >> compile_commands.json; \
		echo "  }" >> compile_commands.json; \
		i=$$((i+1)); \
	done; \
	for src in $(SRC_CX); do \
		if [ $$i -ne 0 ]; then echo "," >> compile_commands.json; fi; \
		echo "  {" >> compile_commands.json; \
		echo '    "directory": "$(abspath .)",' >> compile_commands.json; \
		echo '    "command": "clang $(FLAGS) $(FLAGS_CX) -c '"$$src"'",' >> compile_commands.json; \
		echo '    "file": "'"$$src"'"' >> compile_commands.json; \
		echo "  }" >> compile_commands.json; \
		i=$$((i+1)); \
	done; \
	echo "]" >> compile_commands.json;
endef
