// File author is Ítalo Lima Marconato Matias
//
// Created on February 02 of 2020, at 16:59 BRT
// Last edited on February 02 of 2020, at 19:21 BRT

#ifndef __CHICAGO_ARCH_MSR__
#define __CHICAGO_ARCH_MSR__

#include <chicago/types.h>

#define MSR_SYSENTER_CS 0x174
#define MSR_SYSENTER_ESP 0x175
#define MSR_SYSENTER_EIP 0x176
#define MSR_EFER 0xC0000080
#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xc0000082
#define MSR_SFMASK 0xC0000084
#define MSR_FS_BASE 0xC0000100
#define MSR_GS_BASE 0xC0000101
#define MSR_KERNEL_GS_BASE 0xC0000102

static inline UInt64 MsrRead(UInt32 num) {
#if __INTPTR_WIDTH__ == 32
	UInt64 ret;
	Asm Volatile("rdmsr" : "=A"(ret) : "c"(num));
	return ret;
#else
	UInt32 low;
	UInt32 high;
	Asm Volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(num));
	return ((UInt64)high << 32) | low;
#endif
}

static inline Void MsrWrite(UInt32 num, UInt64 val) {
#if __INTPTR_WIDTH__ == 32
	Asm Volatile("wrmsr" :: "c"(num), "A"(num));
#else
	Asm Volatile("wrmsr" :: "c"(num), "a"(val & 0xFFFFFFFF), "d"(val >> 32));
#endif
}

#endif		// __CHICAGO_ARCH_MSR__
