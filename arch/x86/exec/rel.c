// File author is Ítalo Lima Marconato Matias
//
// Created on November 02 of 2018, at 14:02 BRT
// Last edited on November 02 of 2019, at 15:00 BRT

#include <chicago/elf.h>

Boolean ArchELFRelocate(PELFHdr hdr, UIntPtr base, PELFRel rel, PChar strtab, PELFSym sym, UInt8 type) {
	if (hdr == Null || rel == Null || strtab == Null) {										// First, sanity check
		return False;
	}
	
	PUInt32 pc32 = (PUInt32)(base + rel->offset);											// Get some pointers that we need
	PUIntPtr addr = (PUIntPtr)pc32;
	
	if (type == 0x01 || type == 0x05 || type == 0x06) {										// 386/AMD64_32/64 and 386/AMD64_GLOB/JMP_SLOT
		*addr += sym->value;
	} else if (type == 0x02) {																// 386/AMD64_PC32
		*pc32 += (UInt32)(sym->value - (UIntPtr)addr);	
	} else if (type == 0x08) {																// 386/AMD64_RELATIVE
		*addr += base;
	} else {
		return False;
	}
	
	return True;
}

Boolean ArchELFRelocateA(PELFHdr hdr, UIntPtr base, PELFRelA rel, PChar strtab, PELFSym sym, UInt8 type) {
	if (hdr == Null || rel == Null || strtab == Null) {										// First, sanity check
		return False;
	}
	
	PUInt32 pc32 = (PUInt32)(base + rel->offset);											// Get some pointers that we need
	PUIntPtr addr = (PUIntPtr)pc32;
	
	if (type == 0x01) {																		// 386/AMD64_32/64
		*addr += sym->value + rel->addend;
	} else if (type == 0x02) {																// 386/AMD64_PC32
		*pc32 += (UInt32)(sym->value + rel->addend - (UIntPtr)addr);	
	} else if (type == 0x05 || type == 0x06) {												// 386/AMD64_GLOB/JMP_SLOT
		*addr += sym->value;
	} else if (type == 0x08) {																// 386/AMD64_RELATIVE
		*addr += base + rel->addend;
	} else {
		return False;
	}
	
	return True;
}
