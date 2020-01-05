# File author is Ítalo Lima Marconato Matias
#
# Created on May 11 of 2018, at 13:14 BRT
# Last edited on January 04 of 2020, at 11:03 BRT

ARCH ?= x86
VERBOSE ?= false
DEBUG ?= false

SYSROOT_DIR ?=
DEV_LIB_DIR ?= Development/Libraries
INC_DIR ?= Development/Headers

ifeq ($(ARCH),x86)
	SUBARCH ?= 32
	ARCH_HEADER_FOLDER := include
	SUBARCH_HEADER_FOLDER := include$(SUBARCH)
	ARCH_CFLAGS := -no-pie -fno-pic -Iarch/$(ARCH)/$(SUBARCH_HEADER_FOLDER)
	ARCH_LIB_CFLAGS := -Iarch/$(ARCH)/$(SUBARCH_HEADER_FOLDER)
	ARCH_LDFLAGS := -no-pie -fno-pic
	
	ifeq ($(SUBARCH),32)
		TARGET ?= i686-chicago
		ARCH_CFLAGS += -DELF_MACHINE=3 -msse -msse2 -mfpmath=sse
		ARCH_LIB_CFLAGS += -DELF_MACHINE=3 -msse -msse2 -mfpmath=sse
		OBJCOPY_FORMAT := elf32-i386
	else ifeq ($(SUBARCH),64)
		TARGET ?= x86_64-chicago
		ARCH_CFLAGS += -DELF_MACHINE=62 -mcmodel=large -mno-red-zone
		ARCH_LIB_CFLAGS += -DELF_MACHINE=62 -mcmodel=large -mno-red-zone
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
OBJECTS += exec/drv.c.o exec/elf.c.o exec/exec.c.o exec/lib.c.o
OBJECTS += io/console.c.o io/debug.c.o io/device.c.o io/file.c.o
OBJECTS += io/dev/framebuffer.c.o io/dev/null.c.o io/dev/zero.c.o io/fs/devfs.c.o
OBJECTS += io/fs/iso9660.c.o
OBJECTS += mm/alloc.c.o mm/heap.c.o mm/pmm.c.o mm/shm.c.o
OBJECTS += mm/ualloc.c.o mm/virt.c.o
OBJECTS += sys/config.c.o sys/ipc.c.o sys/panic.c.o sys/process.c.o
OBJECTS += sys/rand.c.o sys/sc.c.o sys/string.c.o
OBJECTS += vid/display.c.o vid/img.c.o
OBJECTS += nls/br.c.o nls/en.c.o nls/nls.c.o

OTHER_OBJECTS := font.psf splash.bmp

ARCH_LIB_OBJECTS := $(addprefix build/lib_$(ARCH)_$(SUBARCH)/arch/$(ARCH)/,$(ARCH_OBJECTS))
ARCH_OBJECTS := $(addprefix build/$(ARCH)_$(SUBARCH)/arch/$(ARCH)/,$(ARCH_OBJECTS))
LIB_OBJECTS := $(addprefix build/lib_$(ARCH)_$(SUBARCH)/,$(OBJECTS))
OBJECTS := $(addprefix build/$(ARCH)_$(SUBARCH)/,$(OBJECTS))
LIB_OTHER_OBJECTS := $(addsuffix .oo, $(addprefix build/lib_$(ARCH)_$(SUBARCH)/,$(OTHER_OBJECTS)))
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

.PRECIOUS: $(ARCH_LIB_OBJECTS) $(LIB_OBJECTS) $(LIB_OTHER_OBJECTS)

all: $(KERNEL) $(LIB_KERNEL)

clean:
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)rm -f $(ARCH_LIB_OBJECTS) $(ARCH_OBJECTS) $(LIB_OBJECTS) $(OBJECTS) $(LIB_OTHER_OBJECTS) $(OTHER_OBJECTS) $(LIB_KERNEL) $(KERNEL) build/lib_$(ARCH)_$(SUBARCH)/arch/$(ARCH)/kdriver.s.o

clean-all:
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)rm -rf build

remake: clean all
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif

install: all
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Copying the kernel header files
	$(NOECHO)rm -rf $(SYSROOT_DIR)/$(INC_DIR)/kernel
	$(NOECHO)mkdir $(SYSROOT_DIR)/$(INC_DIR)/kernel
	$(NOECHO)cp -RT include/chicago $(SYSROOT_DIR)/$(INC_DIR)/kernel
ifneq ($(ARCH_HEADER_FOLDER),)
	$(NOECHO)mkdir $(SYSROOT_DIR)/$(INC_DIR)/kernel/arch
	$(NOECHO)cp -RT arch/$(ARCH)/$(ARCH_HEADER_FOLDER)/chicago/arch $(SYSROOT_DIR)/$(INC_DIR)/kernel/arch
endif
ifneq ($(SUBARCH_HEADER_FOLDER),)
ifeq ($(ARCH_HEADER_FOLDER),)
	$(NOECHO)mkdir $(SYSROOT_DIR)/$(INC_DIR)/kernel/arch
endif
	$(NOECHO)cp -RT arch/$(ARCH)/$(SUBARCH_HEADER_FOLDER)/chicago/arch $(SYSROOT_DIR)/$(INC_DIR)/kernel/arch
endif
	$(NOECHO)echo Patching the kernel header files
	$(NOECHO)find $(SYSROOT_DIR)/$(INC_DIR)/kernel -type f -name "*.h" -exec sed -i 's/<chicago/<kernel/g' {} +
	$(NOECHO)echo Installing libchkrnl.so
	$(NOECHO)cp $(LIB_KERNEL) $(SYSROOT_DIR)/$(DEV_LIB_DIR)/libchkrnl.so
	$(NOECHO)echo Installing kdriver.o
	$(NOECHO)cp build/lib_$(ARCH)_$(SUBARCH)/arch/$(ARCH)/kdriver.s.o $(SYSROOT_DIR)/$(DEV_LIB_DIR)/kdriver.o

$(KERNEL): $(ARCH_OBJECTS) $(OBJECTS) $(OTHER_OBJECTS) $(LINKER_SCRIPT)
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Linking $@
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-gcc -T$(LINKER_SCRIPT) -ffreestanding -nostdlib -Xlinker -Map=$(MAP_FILE) -o $@ $(ARCH_OBJECTS) $(OBJECTS) $(OTHER_OBJECTS) $(ARCH_LDFLAGS) -lgcc

$(LIB_KERNEL): $(ARCH_OBJECTS) $(OBJECTS) $(OTHER_OBJECTS) build/lib_$(ARCH)_$(SUBARCH)/arch/$(ARCH)/kdriver.s.o
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Linking $@
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-gcc -shared -fno-plt -ffreestanding -nostdlib -o $@ $(ARCH_LIB_OBJECTS) $(LIB_OBJECTS) $(LIB_OTHER_OBJECTS) $(ARCH_LIB_LDFLAGS) -lgcc

build/lib_$(ARCH)_$(SUBARCH)/arch/$(ARCH)/kdriver.s.o: arch/$(ARCH)/kdriver.s
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Compiling arch/$(ARCH)/kdriver.s
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-gcc -kdriver $(ARCH_LIB_FLAGS) -c arch/$(ARCH)/kdriver.s -o build/lib_$(ARCH)_$(SUBARCH)/arch/$(ARCH)/kdriver.s.o

build/$(ARCH)_$(SUBARCH)/%.oo: % build/lib_$(ARCH)_$(SUBARCH)/%.oo
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Compiling $<
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-objcopy -Ibinary -O$(OBJCOPY_FORMAT) -B$(OBJCOPY_ARCH) $< $@
	$(NOECHO)$(TARGET)-objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents $@ $@

build/lib_$(ARCH)_$(SUBARCH)/%.oo: %
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-objcopy -Ibinary -O$(OBJCOPY_FORMAT) -B$(OBJCOPY_ARCH) $< $@
	$(NOECHO)$(TARGET)-objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents $@ $@

build/$(ARCH)_$(SUBARCH)/%.s.o: %.s build/lib_$(ARCH)_$(SUBARCH)/%.s.o
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Compiling $<
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-as $(ARCH_AFLAGS) $< -o $@

build/lib_$(ARCH)_$(SUBARCH)/%.s.o: %.s
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(NOECHO)$(TARGET)-as $(ARCH_LIB_AFLAGS) $< -o $@

build/$(ARCH)_$(SUBARCH)/%.c.o: %.c build/lib_$(ARCH)_$(SUBARCH)/%.c.o
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)echo Compiling $<
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
ifeq ($(SUBARCH),)
ifeq ($(DEBUG),yes)
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -DDEBUG -g -std=c11 -Iinclude -Iarch/$(ARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O0 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -std=c11 -Iinclude -Iarch/$(ARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
endif
else
ifeq ($(DEBUG),yes)
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -DDEBUG -g -std=c11 -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O0 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -std=c11 -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_CFLAGS) -c $< -o $@
endif
endif

build/lib_$(ARCH)_$(SUBARCH)/%.c.o: %.c
ifeq ($(UNSUPPORTED_ARCH),true)
	$(error Unsupported architecture $(ARCH), subarch $(SUBARCH))
endif
	$(NOECHO)if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
ifeq ($(SUBARCH),)
ifeq ($(DEBUG),yes)
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -DDEBUG -g -std=c11 -Iinclude -Iarch/$(ARCH)/include -fno-plt -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O0 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_LIB_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -std=c11 -Iinclude -Iarch/$(ARCH)/include -fno-plt -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_LIB_CFLAGS) -c $< -o $@
endif
else
ifeq ($(DEBUG),yes)
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -DDEBUG -g -std=c11 -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -fno-plt -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O0 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_LIB_CFLAGS) -c $< -o $@
else
	$(NOECHO)$(TARGET)-gcc -DARCH=L\"$(ARCH)\" -DARCH_C=\"$(ARCH)-$(SUBARCH)\" -std=c11 -Iinclude -Iarch/$(ARCH)/include -I arch/$(ARCH)/subarch/$(SUBARCH)/include -fno-plt -ffreestanding -fshort-wchar -funroll-loops -fomit-frame-pointer -ffast-math -O3 -Wall -Wextra -Wno-implicit-fallthrough -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast $(ARCH_LIB_CFLAGS) -c $< -o $@
endif
endif
