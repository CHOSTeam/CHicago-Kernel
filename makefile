# File author is Ítalo Lima Marconato Matias
#
# Created on May 11 of 2018, at 13:14 BRT
# Last edited on August 27 of 2019, at 16:10 BRT

ARCH ?= x86
VERBOSE ?= false
DEBUG ?= false

PATH := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/../toolchain/$(ARCH)/bin:$(PATH)
SHELL := env PATH=$(PATH) /bin/bash

ifeq ($(ARCH),x86)
	TARGET ?= i686-elf
	ARCH_CFLAGS := -DCHEXEC_ARCH=CHEXEC_HEADER_FLAGS_ARCH_X86 -msse2
	
	ARCH_OBJECTS := start.s.o
	ARCH_OBJECTS += arch.c.o
	ARCH_OBJECTS += io/debug.c.o io/keyboard.c.o io/mouse.c.o
	ARCH_OBJECTS += net/e1000.c.o
	ARCH_OBJECTS += storage/ahci.c.o storage/ide.c.o
	ARCH_OBJECTS += sys/gdt.c.o sys/idt.c.o sys/panic.c.o sys/pci.c.o
	ARCH_OBJECTS += sys/pit.c.o sys/process.c.o sys/sc.c.o
	ARCH_OBJECTS += usb/uhci.c.o usb/usb.c.o
	ARCH_OBJECTS += mm/pmm.c.o mm/vmm.c.o
	
	OBJCOPY_ARCH := i386
	OBJCOPY_FORMAT := elf32-i386
	
	LINKER_SCRIPT := link.ld
else
	UNSUPPORTED_ARCH := true
endif

OBJECTS := main.c.o
OBJECTS += ds/list.c.o ds/queue.c.o ds/stack.c.o
OBJECTS += exec/chexec.c.o exec/exec.c.o exec/lib.c.o
OBJECTS += io/console.c.o io/device.c.o io/debug.c.o io/file.c.o
OBJECTS += io/dev/console.c.o io/dev/framebuffer.c.o io/dev/rawkeyboard.c.o io/dev/rawmouse.c.o
OBJECTS += io/dev/null.c.o io/dev/zero.c.o
OBJECTS += io/fs/devfs.c.o io/fs/iso9660.c.o
OBJECTS += mm/alloc.c.o mm/heap.c.o mm/pmm.c.o mm/ualloc.c.o
OBJECTS += mm/virt.c.o
OBJECTS += net/arp.c.o net/dhcpv4.c.o net/eth.c.o net/icmp.c.o
OBJECTS += net/ipv4.c.o net/net.c.o net/udp.c.o
OBJECTS += nls/br.c.o nls/en.c.o nls/nls.c.o
OBJECTS += sys/config.c.o sys/ipc.c.o sys/panic.c.o sys/process.c.o
OBJECTS += sys/rand.c.o sys/sc.c.o sys/shell.c.o sys/string.c.o
OBJECTS += vid/display.c.o vid/img.c.o

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
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)\" -DDEBUG -g -std=c11 -Iinclude -Iarch/$(ARCH)/include -ffreestanding -O0 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)\" -std=c11 -Iinclude -Iarch/$(ARCH)/include -ffreestanding -funroll-loops -fomit-frame-pointer -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
endif
else
ifeq ($(DEBUG),yes)
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)\" -DDEBUG -g -std=c11 -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -ffreestanding -O0 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)\" -std=c11 -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -ffreestanding -funroll-loops -fomit-frame-pointer -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
endif
endif
