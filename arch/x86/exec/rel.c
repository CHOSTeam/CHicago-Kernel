// File author is Ítalo Lima Marconato Matias
//
// Created on November 02 of 2018, at 14:02 BRT
// Last edited on November 16 of 2019, at 11:22 BRT

#include <chicago/alloc.h>
#include <chicago/elf.h>
#include <chicago/exec.h>
#include <chicago/string.h>

static Boolean ELFFindSymbol(PLibHandle handle, PChar cname, PUIntPtr out) {
	PWChar name = (PWChar)MemAllocate(StrFormat(Null, L"%S", cname));														// Alloc space for getting the wide string
	
	if (name == Null) {
		return False;
	}
	
	StrFormat(name, L"%S", cname);																							// Get the wide string
	
	ListForeach(PsCurrentProcess->global_handle_list, i) {																	// First, search on the global handle list
		UIntPtr sm = ExecGetSymbol((PLibHandle)i->data, name);																// Try to get the symbol in this handle
		
		if (sm != 0) {
			if (sm == 1) {																									// Found!
				sm = 0;
			}
			
			*out = sm;
			MemFree((UIntPtr)name);
			
			return True;
		}
	}
	
	if (handle != Null) {
		UIntPtr sm = ExecGetSymbol(handle, name);																			// First, search on this handle
		
		if (sm != 0) {
			if (sm == 1) {																									// Found!
				sm = 0;
			}
			
			*out = sm;
			MemFree((UIntPtr)name);
			
			return True;
		}
		
		ListForeach(handle->deps, i) {																						// Let's try to get the symbol in the private dependency handles
			UIntPtr sm = ExecGetSymbol((PLibHandle)i->data, name);

			if (sm != 0) {
				if (sm == 1) {																								// Found!
					sm = 0;
				}
				
				*out = sm;
				MemFree((UIntPtr)name);
				
				return True;
			}
		}
	}
	
	MemFree((UIntPtr)name);
	
	return False;
}

Boolean ArchELFRelocate(PELFHdr hdr, PLibHandle handle, UIntPtr base, PELFRel rel, PChar strtab, PELFSym sym, UInt8 type) {
	if (hdr == Null || rel == Null || strtab == Null) {																		// First, sanity check
		return False;
	}
	
	UIntPtr addr = base + rel->offset;
	PUInt32 ref32 = (PUInt32)addr;
	PUIntPtr ref = (PUIntPtr)addr;
	UIntPtr symaddr = 0;
	
	if (sym->shndx == 0 && type != 0x00 && type != 0x08 && !ELFFindSymbol(handle, strtab + sym->name, &symaddr)) {			// Get the symbol address (if we need to)
		return False;
	} else if (!(sym->shndx == 0 && type != 0x00 && type != 0x08)) {
		symaddr = base + sym->value;
	}
	
	if (type == 0x00) {																										// 386/AMD64_NONE
	} else if (type == 0x01) {																								// 386/AMD64_32/64
		*ref += symaddr;
	} else if (type == 0x02) {																								// 386/AMD64_PC32
		*ref32 += symaddr - addr;	
	} else if (type == 0x06 || type == 0x07) {																				// 386/AMD64_GLOB/JMP_SLOT
		*ref = symaddr;
	} else if (type == 0x08) {																								// 386/AMD64_RELATIVE
		*ref += base;
	} else {
		return False;
	}
	
	return True;
}

Boolean ArchELFRelocateA(PELFHdr hdr, PLibHandle handle, UIntPtr base, PELFRelA rel, PChar strtab, PELFSym sym, UInt8 type) {
	if (hdr == Null || rel == Null || strtab == Null) {																		// First, sanity check
		return False;
	}
	
	UIntPtr addr = base + rel->offset;
	PUInt32 ref32 = (PUInt32)addr;
	PUIntPtr ref = (PUIntPtr)addr;
	UIntPtr symaddr = 0;
	
	if (sym->shndx == 0 && type != 0x00 && type != 0x08 && !ELFFindSymbol(handle, strtab + sym->name, &symaddr)) {			// Get the symbol address (if we need to)
		return False;
	} else if (!(sym->shndx == 0 && type != 0x00 && type != 0x08)) {
		symaddr = base + sym->value;
	}
	
	if (type == 0x00) {																										// 386/AMD64_NONE
	} else if (type == 0x01) {																								// 386/AMD64_32/64
		*ref += symaddr + rel->addend;
	} else if (type == 0x02) {																								// 386/AMD64_PC32
		*ref32 += symaddr + rel->addend - (UIntPtr)addr;	
	} else if (type == 0x06 || type == 0x07) {																				// 386/AMD64_GLOB/JMP_SLOT
		*ref = symaddr + rel->addend;
	} else if (type == 0x08) {																								// 386/AMD64_RELATIVE
		*ref += base + rel->addend;
	} else {
		return False;
	}
	
	return True;
}
