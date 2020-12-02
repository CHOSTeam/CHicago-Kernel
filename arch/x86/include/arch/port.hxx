/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 26 of 2020, at 18:24 BRT
 * Last edited on October 24 of 2020, at 14:38 BRT */

#ifndef __ARCH_PORT_HXX__
#define __ARCH_PORT_HXX__

#include <types.hxx>

/* For accessing most of the devices (that aren't memory mapped), we need to
 * use the in/out instructions, this class let us do this easily. */

class Port {
public:
	/* First, the functions that allow us to call the out* instructions. */
	
	static inline Void OutByte(UInt16 Num, UInt8 Data) {
		asm volatile("outb %1, %0" :: "dN"(Num), "a"(Data));
	}
	
	static inline Void OutWord(UInt16 Num, UInt16 Data) {
		asm volatile("outw %1, %0" :: "dN"(Num), "a"(Data));
	}
	
	static inline Void OutDWord(UInt16 Num, UInt32 Data) {
		asm volatile("outl %1, %0" :: "dN"(Num), "a"(Data));
	}
	
	static inline Void OutBuffer(UInt16 Num, UInt8 *Data, UIntPtr Length) {
		asm volatile("rep outsw" : "+S"(Data), "+c"(Length) : "d"(Num));
	}
	
	/* And the functions that allow us to call the in* instructions. */
	
	static inline UInt8 InByte(UInt16 Num) {
		UInt8 ret;
		asm volatile("inb %1, %0" : "=a"(ret) : "dN"(Num));
		return ret;
	}
	
	static inline UInt16 InWord(UInt16 Num) {
		UInt16 ret;
		asm volatile("inw %1, %0" : "=a"(ret) : "dN"(Num));
		return ret;
	}
	
	static inline UInt32 InDWord(UInt16 Num) {
		UInt32 ret;
		asm volatile("inl %1, %0" : "=a"(ret) : "dN"(Num));
		return ret;
	}
	
	static inline Void InBuffer(UInt16 Num, UInt8 *Data, UIntPtr Length) {
		asm volatile("rep insw" : "+D"(Data), "+c"(Length) : "d"(Num) : "memory");
	}
};

#endif
