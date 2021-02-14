# File author is Ítalo Lima Marconato Matias
#
# Created on January 26 of 2021, at 21:00 BRT
# Last edited on February 14 of 2021, at 18:38 BRT

ARCH ?= amd64
DEBUG ?= false
VERBOSE ?= false

ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
TOOLCHAIN_DIR := $(ROOT_DIR)/../toolchain/kernel
PATH := $(TOOLCHAIN_DIR)/$(ARCH)/out/bin:$(PATH)
SHELL := env PATH=$(PATH) /bin/bash

ifneq ($(VERBOSE),true)
NOECHO := @
endif

# The .deps file will be rebuilt anyways (even if we use find instead of manually specifying the files).

OUT := build/$(ARCH)/oskrnl.elf
ARCH_SOURCES := $(shell find $(ROOT_DIR)/arch/$(ARCH) -name \*.cxx -print -o -name \*.S -print | \
													  sed -e "s@$(ROOT_DIR)/arch/$(ARCH)/@@g")
SOURCES := $(shell find $(ROOT_DIR) -path $(ROOT_DIR)/arch -prune -o -name \*.cxx -print -o -name \*.S -print | \
									sed -e "s@$(ROOT_DIR)/@@g")

# Include the arch-specific toolchain file BEFORE anything else (but after defining the SOURCES variable, as it will gen
# the object list based on it, and the makefile.deps file as well).

include $(TOOLCHAIN_DIR)/build.make
