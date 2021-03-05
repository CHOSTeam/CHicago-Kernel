# File author is Ítalo Lima Marconato Matias
#
# Created on March 04 of 2021, at 12:18 BRT
# Last edited on March 05 of 2021, at 14:08 BRT

ARCH ?= amd64
DEBUG ?= false
VERBOSE ?= false

ifneq ($(VERBOSE),true)
NOECHO := @
endif

build:
	+$(NOECHO)make -C lib ARCH=$(ARCH) DEBUG=$(DEBUG) VERBOSE=$(VERBOSE) build
	+$(NOECHO)make -C src ARCH=$(ARCH) DEBUG=$(DEBUG) VERBOSE=$(VERBOSE) build

clean:
	+$(NOECHO)make -C lib ARCH=$(ARCH) VERBOSE=$(VERBOSE) clean
	+$(NOECHO)make -C src ARCH=$(ARCH) VERBOSE=$(VERBOSE) clean

clean-all:
	+$(NOECHO)make -C lib VERBOSE=$(VERBOSE) clean-all
	+$(NOECHO)make -C src VERBOSE=$(VERBOSE) clean-all
