# Windows orchestration; SGDK's own Makefile compiles C and resources unchanged.
# First dot-source scripts/env.ps1, then use SGDK's bundled make.exe.
.DEFAULT_GOAL := build
# SGDK 2.11 ships 32-bit Make: Sysnative keeps host DLL/core tests in 64-bit PS 5.1.
SHELL := $(subst \,/,$(WINDIR))/sysnative/WindowsPowerShell/v1.0/powershell.exe
.SHELLFLAGS := -NoProfile -Command
GDK := $(subst \,/,$(MB_SGDK))
MAKE := $(GDK)/bin/make.exe
REGION ?= ntsc
CYCLES ?= 1
SCENARIO ?= all
TEST_REGION ?= both
APP ?= calculator
PREVIEW ?=
CONTRACTS ?= 0
LITTLEFS ?= 0
MUSIC_SMOKE ?= 0
HOST_CC := $(subst \,/,$(MB_HOST_CC))
HOST_SOURCES := $(wildcard tools/native/*.c tests/emulator/*.c) third_party/cjson/cJSON.c third_party/unity/src/unity.c
HOST_HEADERS := $(wildcard tools/native/*.h third_party/cjson/*.h third_party/unity/src/*.h)
UNIT_SOURCES := $(wildcard tests/unit/*.c) tests/host/creative.c tests/host/paint.c third_party/unity/src/unity.c apps/calculator/model.c apps/snake/model.c desktop/layout.c kernel/time/format.c kernel/storage/ramdisk.c kernel/storage/ram_adapter.c kernel/storage/rom.c kernel/storage/vfs.c kernel/media/format.c apps/creative/model.c apps/creative/paint_model.c
UNIT_HEADERS := $(wildcard third_party/unity/src/*.h sdk/include/*.h inc/*.h apps/*/*.h kernel/*/*.h desktop/*.h tests/host/*.h)
HOST_TOOL := out/host/megabench-tools.exe
UNIT_TEST := out/host/unit-tests.exe

ifeq ($(CONTRACTS)$(LITTLEFS),11)
$(error Select only one build variant)
endif
ifneq ($(PREVIEW),)
ifneq ($(filter 1,$(CONTRACTS) $(LITTLEFS)),)
$(error PREVIEW cannot be combined with CONTRACTS or LITTLEFS)
endif
endif
ifeq ($(MUSIC_SMOKE),1)
ifneq ($(PREVIEW),megaplayer)
$(error MUSIC_SMOKE=1 requires PREVIEW=megaplayer)
endif
endif

PROJECT := out/project
OPTIONS :=
FLAGS := -std=c99 -Werror -Wno-misleading-indentation -Wno-sign-compare
ifeq ($(CONTRACTS),1)
PROJECT := out/contracts
OPTIONS += -Contracts
FLAGS += -DMB_SELF_TEST
else ifeq ($(LITTLEFS),1)
PROJECT := out/littlefs-project
OPTIONS += -LittleFS
FLAGS += -DMB_LFS_EXPERIMENT -DLFS_CONFIG=tests/littlefs/sgdk_config.h -DLFS_NO_MALLOC -DLFS_NO_DEBUG -DLFS_NO_WARN -DLFS_NO_ERROR -DLFS_NO_ASSERT -DLFS_NAME_MAX=19 -DLFS_FILE_MAX=3592 -DLFS_ATTR_MAX=32
else ifneq ($(PREVIEW),)
PROJECT := out/preview/$(PREVIEW)
OPTIONS += -PreviewDemo $(PREVIEW)
ID_snake := 6
ID_megabounce := 7
ID_colourstorm := 8
ID_ocean := 9
ID_stardrive := 10
ID_megapaint := 12
ID_megaplayer := 13
FLAGS += -DMB_PREVIEW_APP=$(ID_$(PREVIEW))
ifeq ($(PREVIEW),megaplayer)
FLAGS += -DMB_PREVIEW_MUSIC
endif
endif
ifeq ($(MUSIC_SMOKE),1)
PROJECT := out/preview/megaplayer-smoke
OPTIONS += -MusicSmoke
FLAGS += -DMB_NATIVE_MUSIC
endif

.PHONY: all build prepare test test-unit test-emulator host-tools check compatibility run run-genplus generate check-generated clean package package-app contracts littlefs test-littlefs format lint verify-clean
all: build
host-tools: $(HOST_TOOL)
$(HOST_TOOL): $(HOST_SOURCES) $(HOST_HEADERS)
	New-Item -ItemType Directory -Force out/host | Out-Null; & '$(HOST_CC)' -std=c99 -D_CRT_SECURE_NO_WARNINGS -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -Wno-sign-compare -Itools/native -Ithird_party/cjson -Ithird_party/unity/src $(HOST_SOURCES) -lbcrypt -o $(HOST_TOOL); exit $$LASTEXITCODE
$(UNIT_TEST): $(UNIT_SOURCES) $(UNIT_HEADERS)
	New-Item -ItemType Directory -Force out/host | Out-Null; & '$(HOST_CC)' -std=c99 -D_CRT_SECURE_NO_WARNINGS -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -Wno-sign-compare -DMB_HOST_TEST -Ithird_party/unity/src -Isdk/include -Iinc $(UNIT_SOURCES) -o $(UNIT_TEST); exit $$LASTEXITCODE
prepare: host-tools
	powershell.exe -NoProfile -File scripts/build-support.ps1 -Action Prepare $(OPTIONS) -CompilerFlags '$(FLAGS)'
build: prepare
	& '$(MAKE)' -C '$(PROJECT)' -f makefile.gen 'GDK=$(GDK)' 'EXTRA_FLAGS=$(FLAGS)' release 2>&1 | Tee-Object -FilePath '$(PROJECT)/compile.log'; if ($$LASTEXITCODE -ne 0 -or (Select-String -Path '$(PROJECT)/compile.log' -Pattern 'warning:')) { exit 1 }
	powershell.exe -NoProfile -File scripts/build-support.ps1 -Action Publish $(OPTIONS)
# Fast everyday checks; compatibility runs every GX scenario in PAL and NTSC.
test-unit: $(UNIT_TEST)
	New-Item -ItemType Directory -Force reports | Out-Null; & './$(UNIT_TEST)'; exit $$LASTEXITCODE
test: build test-unit check-generated
	& './$(HOST_TOOL)' check; exit $$LASTEXITCODE
check: test
test-emulator: build contracts
	& './$(HOST_TOOL)' emulator --scenario $(SCENARIO) --region $(TEST_REGION) --cycles $(CYCLES); exit $$LASTEXITCODE
compatibility: test test-emulator
run: build
	powershell.exe -NoProfile -File scripts/run-blastem.ps1 -Region $(REGION)
run-genplus: build
	powershell.exe -NoProfile -File scripts/run-genplus.ps1
generate: host-tools
	& './$(HOST_TOOL)' generate; exit $$LASTEXITCODE
check-generated: host-tools
	& './$(HOST_TOOL)' check-generated; if ($$LASTEXITCODE) { exit $$LASTEXITCODE }
	& './$(HOST_TOOL)' music-check; exit $$LASTEXITCODE
format: host-tools
	& './$(HOST_TOOL)' format; exit $$LASTEXITCODE
lint: host-tools
	& './$(HOST_TOOL)' lint; exit $$LASTEXITCODE
verify-clean: build
	& './$(HOST_TOOL)' clean-workflow --skip-emulator 1; exit $$LASTEXITCODE
clean:
	powershell.exe -NoProfile -File scripts/build-support.ps1 -Action Clean
package: build
	powershell.exe -NoProfile -File scripts/package.ps1 -Release
package-app:
	powershell.exe -NoProfile -File scripts/package.ps1 -App $(APP)
contracts:
	$(MAKE) build CONTRACTS=1
littlefs:
	$(MAKE) build LITTLEFS=1
test-littlefs: host-tools
	New-Item -ItemType Directory -Force out/littlefs | Out-Null; & '$(HOST_CC)' -std=c99 -D_CRT_SECURE_NO_WARNINGS -O2 -Wall -Wextra -Werror -DLFS_NO_MALLOC -DLFS_NO_DEBUG -DLFS_NO_WARN -DLFS_NO_ERROR -DLFS_NO_ASSERT -DLFS_NAME_MAX=19 -DLFS_FILE_MAX=3592 -DLFS_ATTR_MAX=32 third_party/littlefs/lfs.c third_party/littlefs/lfs_util.c tests/littlefs/experiment.c tests/littlefs/host.c -o out/littlefs/benchmark.exe; if ($$LASTEXITCODE) { exit $$LASTEXITCODE }; & './out/littlefs/benchmark.exe' | Set-Content -Encoding UTF8 out/littlefs/results.json; if ($$LASTEXITCODE) { exit $$LASTEXITCODE }; & './$(HOST_TOOL)' littlefs-check --input out/littlefs/results.json; exit $$LASTEXITCODE
