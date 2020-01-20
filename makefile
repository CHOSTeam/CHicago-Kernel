# File author is Ítalo Lima Marconato Matias
#
# Created on May 11 of 2018, at 13:14 BRT
# Last edited on January 20 of 2020, at 10:52 BRT

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
		TARGET ?= i686-chicago
		ARCH_CFLAGS += -msse -msse2 -mfpmath=sse
		OBJCOPY_FORMAT := elf32-i386
	else ifeq ($(SUBARCH),64)
		TARGET ?= x86_64-chicago
		ARCH_CFLAGS += -mcmodel=large -mno-red-zone
		OBJCOPY_FORMAT := elf64-x86-64
	else
		UNSUPPORTED_ARCH := true
	endif
	
	ARCH_OBJECTS := start$(SUBARCH).s.o
	ARCH_OBJECTS += arch.c.o
	ARCH_OBJECTS += io/debug.c.o
	ARCH_OBJECTS += mm/pmm.c.o mm/vmm$(SUBARCH).c.o
	ARCH_OBJECTS += storage/ahci.c.o storage/ide.c.o
	ARCH_OBJECTS += sys/gdt$(SUBARCH).c.o sys/idt$(SUBARCH).c.o sys/panic$(SUBARCH).c.o sys/pci.c.o
	ARCH_OBJECTS += sys/pit.c.o sys/process$(SUBARCH).c.o sys/sc$(SUBARCH).c.o
	
	OBJCOPY_ARCH := i386
	LINKER_SCRIPT := link$(SUBARCH).ld
else
	UNSUPPORTED_ARCH := true
endif

OBJECTS := main.c.o
OBJECTS += ds/list.c.o ds/queue.c.o ds/stack.c.o
OBJECTS += io/console.c.o io/debug.c.o io/device.c.o io/file.c.o
OBJECTS += io/dev/framebuffer.c.o io/dev/null.c.o io/dev/zero.c.o io/fs/devfs.c.o
OBJECTS += io/fs/iso9660.c.o
OBJECTS += mm/alloc.c.o mm/heap.c.o mm/pmm.c.o mm/shm.c.o
OBJECTS += mm/virt.c.o
OBJECTS += sc/sc.c.o sc/sctable.c.o
OBJECTS += sys/ipc.c.o sys/panic.c.o sys/process.c.o sys/rand.c.o
OBJECTS += sys/string.c.o
OBJECTS += vid/display.c.o vid/img.c.o

OTHER_OBJECTS := font.psf splash.bmp

ARCH_OBJECTS := $(addprefix build/$(ARCH)_$(SUBARCH)/arch/$(ARCH)/,$(ARCH_OBJECTS))
OBJECTS := $(addprefix build/$(ARCH)_$(SUBARCH)/,$(OBJECTS))
OTHER_OBJECTS := $(addsuffix .oo, $(addprefix build/$(ARCH)_$(SUBARCH)/,$(OTHER_OBJECTS)))
LINKER_SCRIPT := arch/$(ARCH)/$(LINKER_SCRIPT)

ifeq ($(SUBARCH),)
	KERNEL := build/chkrnl-$(ARCH)
	LIB_KERNEL := build/libchkrnl-$(ARCH).so
	MAP_FILE := build/chkrnl-$(ARCH).map
else
	KERNEL := build/chkrnl-$(ARCH)_$(SUBARCH)
	LIB_KERNEL := build/libchkrnl-$(ARCH)_$(SUBARCH).so
	MAP_FILE := build/chkrnl-$(ARCH)_$(SUBARCH).map
endif

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
	$(NOECHO)rm -f $(ARCH_OBJECTS) $(OBJECTS) $(OTHER_OBJECTS) $(KERNEL) sc/sctable.c include/chicago/sc.h

clean-all:
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)rm -rf build sc/sctable.c include/chicago/sc.h

remake: clean all
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif

$(KERNEL): $(ARCH_OBJECTS) $(OBJECTS) $(OTHER_OBJECTS) $(LINKER_SCRIPT)
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Linking $@
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-gcc -T$(LINKER_SCRIPT) -ffreestanding -nostdlib -Xlinker -Map=$(MAP_FILE) -o $@ $(ARCH_OBJECTS) $(OBJECTS) $(OTHER_OBJECTS) $(ARCH_LDFLAGS) -lgcc

build/$(ARCH)_$(SUBARCH)/%.oo: %
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Compiling $<
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-objcopy -Ibinary -O$(OBJCOPY_FORMAT) -B$(OBJCOPY_ARCH) $< $@
	$(NOECHO)$(TARGET)-objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents $@ $@

build/$(ARCH)_$(SUBARCH)/%.s.o: %.s
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Compiling $<
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-as $(ARCH_AFLAGS) $< -o $@

build/$(ARCH)_$(SUBARCH)/%.c.o: %.c
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
