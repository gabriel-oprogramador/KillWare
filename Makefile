include Project.mk
.SILENT:
MAKEFLAGS += --no-print-directory

all: dev

dev:
	$(MAKE) start BUILD_TYPE=Development

ship:
	$(MAKE) start BUILD_TYPE=Shipping

start:
	echo 'Build Project:$(PROJECT_NAME), Compiler:$(COMPILER), Configuration:$(BUILD_TYPE)|$(PLATFORM)'
	$(CREATE_DIRS)
	$(MAKE) -f $(TARGET_MAKEFILE) prebuild
	$(MAKE) -j -f $(TARGET_MAKEFILE) build
	$(MAKE) -f $(TARGET_MAKEFILE) posbuild

run: dev
	$(MAKE) -f $(TARGET_MAKEFILE) run BUILD_TYPE=Development PLATFORM=$(PLATFORM)

clean:
	echo 'Clean Everything'
	rm -rf .cache
	rm -rf $(BINARIES_DIR)
	rm -rf $(INTERMEDIATE_DIR)

