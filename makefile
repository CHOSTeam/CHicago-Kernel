# File author is Ítalo Lima Marconato Matias
#
# Created on May 11 of 2018, at 13:14 BRT
# Last edited on November 08 of 2019, at 16:16 BRT

ARCH ?= x86
VERBOSE ?= false
DEBUG ?= false

ifeq ($(ARCH),x86)
	SUBARCH ?= 32
	ARCH_CFLAGS := -no-pie -fno-pic
	ARCH_LDFLAGS := -no-pie -fno-pic
	
	ifeq ($(SUBARCH),32)
		TARGET ?= i686-chicago
		ARCH_CFLAGS += -DELF_MACHINE=3 -msse2 -Iarch/$(ARCH)/include32
		OBJCOPY_FORMAT := elf32-i386
	else ifeq ($(SUBARCH),64)
		TARGET ?= x86_64-chicago
		ARCH_CFLAGS += -DARCH_64 -DELF_MACHINE=62 -mcmodel=large -mno-red-zone -mno-mmx -Iarch/$(ARCH)/include64
		OBJCOPY_FORMAT := elf64-x86-64
	else
		UNSUPPORTED_ARCH := true
	endif
	
	ARCH_OBJECTS := start$(SUBARCH).s.o
	ARCH_OBJECTS += arch.c.o
	ARCH_OBJECTS += exec/rel.c.o
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
OBJECTS += exec/elf.c.o exec/exec.c.o exec/lib.c.o
OBJECTS += io/console.c.o io/debug.c.o io/device.c.o io/file.c.o
OBJECTS += io/dev/console.c.o io/dev/framebuffer.c.o io/dev/null.c.o io/dev/zero.c.o
OBJECTS += io/fs/devfs.c.o io/fs/iso9660.c.o
OBJECTS += mm/alloc.c.o mm/heap.c.o mm/pmm.c.o mm/ualloc.c.o
OBJECTS += mm/virt.c.o
OBJECTS += sys/panic.c.o sys/process.c.o sys/rand.c.o sys/sc.c.o
OBJECTS += sys/string.c.o
OBJECTS += vid/display.c.o vid/img.c.o
OBJECTS += nls/br.c.o nls/en.c.o nls/nls.c.o

OTHER_OBJECTS := font.psf splash.bmp

ARCH_OBJECTS := $(addprefix build/$(ARCH)_$(SUBARCH)/arch/$(ARCH)/,$(ARCH_OBJECTS))
OBJECTS := $(addprefix build/$(ARCH)_$(SUBARCH)/,$(OBJECTS))
OTHER_OBJECTS := $(addsuffix .oo, $(addprefix build/$(ARCH)_$(SUBARCH)/,$(OTHER_OBJECTS)))
LINKER_SCRIPT := arch/$(ARCH)/$(LINKER_SCRIPT)

ifeq ($(SUBARCH),)
	KERNEL := build/chkrnl-$(ARCH)
	MAP_FILE := build/chkrnl-$(ARCH).map
else
	KERNEL := build/chkrnl-$(ARCH)_$(SUBARCH)
	MAP_FILE := build/chkrnl-$(ARCH)_$(SUBARCH).map
endif

ifneq ($(VERBOSE),true)
NOECHO := @
endif

PATH := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/../toolchain/$(ARCH)-$(SUBARCH)/bin:$(PATH)
SHELL := env PATH=$(PATH) /bin/bash

all: $(KERNEL)

clean:
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)rm -f $(ARCH_OBJECTS) $(OBJECTS) $(OTHER_OBJECTS) $(KERNEL)

clean-all:
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)rm -rf build

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
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -DDEBUG -g -std=c11 -Iinclude -Iarch/$(ARCH)/include -ffreestanding -fshort-wchar -O0 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -std=c11 -Iinclude -Iarch/$(ARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
endif
else
ifeq ($(DEBUG),yes)
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -DDEBUG -g -std=c11 -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -ffreestanding -fshort-wchar -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -std=c11 -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
endif
endif
