// File author is Ítalo Lima Marconato Matias
//
// Created on October 27 of 2018, at 21:34 BRT
// Last edited on January 25 of 2020, at 11:14 BRT

#ifndef __CHICAGO_PANIC_H__
#define __CHICAGO_PANIC_H__

#include <chicago/types.h>

#define PANIC_MM_READWRITE_TO_NONPRESENT_AREA 0x00
#define PANIC_MM_READWRITE_TO_KERNEL_AREA 0x01
#define PANIC_MM_WRITE_TO_READONLY_AREA 0x02
#define PANIC_BOOTDEV_MOUNT_FAILED 0x03
#define PANIC_KERNEL_PROCESS_CLOSED 0x04
#define PANIC_KERNEL_MAIN_THREAD_CLOSED 0x05
#define PANIC_OSMNGR_START_FAILED 0x06
#define PANIC_OSMNGR_PROCESS_CLOSED 0x07
#define PANIC_KERNEL_UNEXPECTED_ERROR 0x08
#define PANIC_KERNEL_INIT_FAILED 0x09
#define PANIC_COUNT 0x0A

Void PanicInt(UInt32 err, Boolean perr);
Void Panic(UInt32 err);

#endif		// __CHICAGO_PANIC_H__
