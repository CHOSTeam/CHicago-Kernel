// File author is Ítalo Lima Marconato Matias
//
// Created on May 11 of 2018, at 23:07 BRT
// Last edited on February 02 of 2020, at 17:17 BRT

#ifndef __CHICAGO_ARCH_PORT_H__
#define __CHICAGO_ARCH_PORT_H__

#include <chicago/types.h>

static inline Void PortOutByte(UInt16 port, UInt8 data) {
	Asm Volatile("outb %1, %0" :: "dN"(port), "a"(data));
}

static inline UInt8 PortInByte(UInt16 port) {
	UInt8 ret;
	Asm Volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
	return ret;
}

static inline Void PortOutWord(UInt16 port, UInt16 data) {
	Asm Volatile("outw %1, %0" :: "dN"(port), "a"(data));
}

static inline UInt16 PortInWord(UInt16 port) {
	UInt16 ret;
	Asm Volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
	return ret;
}

static inline Void PortOutLong(UInt16 port, UInt32 data) {
	Asm Volatile("outl %1, %0" :: "dN"(port), "a"(data));
}

static inline UInt32 PortInLong(UInt16 port) {
	UInt32 ret;
	Asm Volatile("inl %1, %0" : "=a"(ret) : "dN"(port));
	return ret;
}

static inline Void PortOutMultiple(UInt16 port, PUInt8 data, ULong size) {
	Asm Volatile("rep outsw" : "+S"(data), "+c"(size) : "d"(port));
}

static inline Void PortInMultiple(UInt16 port, PUInt8 data, ULong size) {
	Asm Volatile("rep insw" : "+D"(data), "+c"(size) : "d"(port) : "memory");
}

#endif		// __CHICAGO_ARCH_PORT_H__
