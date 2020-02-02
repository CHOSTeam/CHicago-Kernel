# File author is Ítalo Lima Marconato Matias
#
# Created on May 11 of 2018, at 13:14 BRT
# Last edited on February 02 of 2020, at 16:08 BRT

ARCH ?= x86
VERBOSE ?= false
DEBUG ?= false

SYSROOT_DIR ?=
DEV_LIB_DIR ?= Development/Libraries
INC_DIR ?= Development/Headers

ifeq ($(ARCH),x86)
	SUBARCH ?= 32
	ARCH_CFLAGS := -Wno-return-local-addr -Wno-int-conversion -no-pie -fno-pic -Iarch/$(ARCH)/include$(SUBARCH)
	ARCH_LDFLAGS := -no-pie -fno-pic
	
	ifeq ($(SUBARCH),32)
		NOT_SUBARCH := 64
		TARGET ?= i686-chicago
		ARCH_CFLAGS += -DELF_MACHINE=3 -msse -msse2 -mfpmath=sse
		OBJCOPY_FORMAT := elf32-i386
		FULL_ARCH_STRING := x86
	else ifeq ($(SUBARCH),64)
		NOT_SUBARCH := 32
		TARGET ?= x86_64-chicago
		ARCH_CFLAGS += -DELF_MACHINE=62 -mcmodel=large -mno-red-zone
		OBJCOPY_FORMAT := elf64-x86-64
		FULL_ARCH_STRING := x86-64
	else
		UNSUPPORTED_ARCH := true
	endif
	
	ARCH_OBJECTS := $(addprefix build/$(FULL_ARCH_STRING)/, $(addsuffix .o, $(shell find arch/$(ARCH) -name "*.c" | grep -v **$(NOT_SUBARCH).* | sed 's|^arch/$(ARCH)|arch|') $(shell find arch/$(ARCH) -name "*.s" | grep -v **$(NOT_SUBARCH).* | sed 's|^arch/$(ARCH)|arch|')))
	OBJCOPY_ARCH := i386
	LINKER_SCRIPT := link$(SUBARCH).ld
else
	UNSUPPORTED_ARCH := true
endif

ifeq ($(FULL_ARCH_STRING),)
	FULL_ARCH_STRING := $(ARCH)
endif

OBJECTS := $(addprefix build/$(FULL_ARCH_STRING)/, $(addsuffix .o, $(shell find . -name '*.c' | grep -v arch | sed 's|^./||')))
OTHER_OBJECTS := $(addprefix build/$(FULL_ARCH_STRING)/, $(addsuffix .oo, font.psf splash.bmp))

ifeq ($(wildcard sc/sctable.c),)
	OBJECTS += build/$(FULL_ARCH_STRING)/sc/sctable.c.o
endif

OBJECTS := $(shell echo '$(ARCH_OBJECTS) $(OBJECTS) $(OTHER_OBJECTS)' | xargs -n1 | sort | xargs)
LINKER_SCRIPT := arch/$(ARCH)/$(LINKER_SCRIPT)

KERNEL := build/chkrnl-$(FULL_ARCH_STRING)
MAP_FILE := build/chkrnl-$(FULL_ARCH_STRING).map

ifneq ($(VERBOSE),true)
NOECHO := @
endif

PATH := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/../toolchain/$(ARCH)-$(SUBARCH)/bin:$(PATH)
SHELL := env PATH=$(PATH) /bin/bash

all: sc/sctable.c $(KERNEL)

clean:
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)rm -f $(OBJECTS) $(KERNEL) sc/sctable.c include/chicago/sc.h

clean-all:
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)rm -rf build sc/sctable.c include/chicago/sc.h

remake: clean all
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif

$(KERNEL): $(OBJECTS) $(LINKER_SCRIPT)
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Linking $@
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-gcc -T$(LINKER_SCRIPT) -ffreestanding -nostdlib -Xlinker -Map=$(MAP_FILE) -o $@ $(OBJECTS) $(ARCH_LDFLAGS) -lgcc

build/$(FULL_ARCH_STRING)/%.oo: %
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Compiling $<
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-objcopy -Ibinary -O$(OBJCOPY_FORMAT) -B$(OBJCOPY_ARCH) $< $@
	$(NOECHO)$(TARGET)-objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents $@ $@

build/$(FULL_ARCH_STRING)/arch/%.s.o: arch/$(ARCH)/%.s
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Compiling $<
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-as $(ARCH_AFLAGS) $< -o $@

build/$(FULL_ARCH_STRING)/%.c.o: %.c
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Compiling $<
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
ifeq ($(SUBARCH),)
ifeq ($(DEBUG),yes)
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(FULL_ARCH_STRING)\" -DARCH_C=\"$(FULL_ARCH_STRING)\" -DDEBUG -g3 -ggdb -std=c2x -Iinclude -Iarch/$(ARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -Og -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(FULL_ARCH_STRING)\" -DARCH_C=\"$(FULL_ARCH_STRING)\" -std=c2x -Iinclude -Iarch/$(ARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
endif
else
ifeq ($(DEBUG),yes)
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(FULL_ARCH_STRING)\" -DARCH_C=\"$(FULL_ARCH_STRING)\" -DDEBUG -g3 -ggdb -std=c2x -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -Og -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(FULL_ARCH_STRING)\" -DARCH_C=\"$(FULL_ARCH_STRING)\" -std=c2x -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
endif
endif

build/$(FULL_ARCH_STRING)/arch/%.c.o: arch/$(ARCH)/%.c
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Compiling $<
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
ifeq ($(SUBARCH),)
ifeq ($(DEBUG),yes)
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -DDEBUG -g3 -ggdb -std=c2x -Iinclude -Iarch/$(ARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -Og -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -std=c2x -Iinclude -Iarch/$(ARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
endif
else
ifeq ($(DEBUG),yes)
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -DDEBUG -g3 -ggdb -std=c2x -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -Og -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -std=c2x -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
endif
endif

include/chicago/sc.h: sc/syscalls.tbl
	$(NOECHO)echo Generating $@
	$(NOECHO)sc/makesc.sh sc/syscalls.tbl > $@

sc/sctable.c: include/chicago/sc.h
	$(NOECHO)echo Generating $@
	$(NOECHO)sc/makesctable.sh sc/syscalls.tbl > $@
