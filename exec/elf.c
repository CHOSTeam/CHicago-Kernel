// File author is Ítalo Lima Marconato Matias
//
// Created on October 31 of 2019, at 18:57 BRT
// Last edited on January 05 of 2020, at 14:09 BRT

#include <chicago/alloc.h>
#include <chicago/elf.h>
#include <chicago/mm.h>
#include <chicago/string.h>
#include <chicago/virt.h>

static PVoid ELFGetPHdr(PELFHdr hdr, UInt32 type) {
	if (hdr == Null) {																										// Sanity check
		return Null;
	}
	
	for (UIntPtr i = 0; i < hdr->ph_num; i++) {
		PELFPHdr ph = ELF_PH_GET(hdr, i);
		
		if (ph->type == type) {																								// Match?
			return ELF_SHPH_CONTENT(hdr, ph);																				// Yes, return it!
		}
	}
	
	return Null;
}

static UIntPtr ELFGetDynVal(PELFHdr hdr, PELFDyn dyn, IntPtr type) {
	if (hdr == Null || dyn == Null) {																						// Sanity check
		return 0;
	}
	
	for (PELFDyn cur = dyn; cur->tag != 0; cur++) {																			// Let's iterate!
		if (cur->tag == type) {																								// Found?
			return cur->val_ptr;																							// Yes, return!
		}
	}
	
	return 0;
}

static PVoid ELFGetDynPtr(PELFHdr hdr, PELFDyn dyn, IntPtr type) {
	if (hdr == Null || dyn == Null) {																						// Sanity check
		return Null;
	}
	
	for (PELFDyn cur = dyn; cur->tag != 0; cur++) {																			// Let's iterate!
		if (cur->tag == type) {																								// Found?
			return (PVoid)((UIntPtr)hdr + cur->val_ptr);																	// Yes, return!
		}
	}
	
	return Null;
}

Boolean ELFCheck(PELFHdr hdr, Boolean exec) {
	if (!StrCompareMemory(hdr->ident, "\177ELF", 4)) {																		// Check the header magic
		return False;
	} else if (hdr->type != 3) {																							// Check if this is a dynamic executable/library
		return False;
	} else if (hdr->machine != ELF_MACHINE) {																				// Check if this file is for this architecture
		return False;
	} else if (hdr->version == 0) {																							// And check if the version isn't zero
		return False;
	}
	
	if (exec) {																												// Check if this is a executable?
		PELFDyn dyn = ELFGetPHdr(hdr, 0x02);																				// Yes, get the dyn section of the executable
	
		if (dyn == Null) {
			return False;
		}
		
		UIntPtr flags1 = ELFGetDynVal(hdr, dyn, 0x6FFFFFFB);																// Get the DT_FLAGS_1
		
		if ((flags1 & 0x8000000) != 0x8000000) {																			// And check if the PIE flag is set
			return False;
		}
	}
	
	return True;
}

static UIntPtr ELFGetSize(PELFHdr hdr) {
	if (hdr == Null) {																										// Sanity check
		return 0;
	}
	
	UIntPtr size = 0;																										// Now, let's iterate the program headers!
	
	for (UIntPtr i = 0; i < hdr->ph_num; i++) {
		PELFPHdr ph = ELF_PH_GET(hdr, i);
		
		if (ph->type == 0x01) {																								// Section to load?
			UIntPtr maxaddr = ((ph->vaddr + ph->msize - 1) / ph->align + 1) * ph->align;									// Yes, get the max address
			
			if (maxaddr > size) {																							// Higher than our current size?
				size = maxaddr;																								// Yes, so let's increase the size
			}
		}
	}
	
	return size;
}

static Boolean ELFLoadPHdrs(PELFHdr hdr, UIntPtr dest) {
	if (hdr == Null || dest == 0) {																							// Sanity check
		return False;
	}
	
	for (UIntPtr i = 0; i < hdr->ph_num; i++) {																				// Now, let's iterate the program headers!
		PELFPHdr ph = ELF_PH_GET(hdr, i);
		
		if (ph->type == 0x01) {																								// Section to load?
			StrCopyMemory((PUInt8)(dest + ph->vaddr), ELF_SHPH_CONTENT(hdr, ph), ph->fsize);								// Yes, load it
			StrSetMemory((PUInt8)(dest + ph->vaddr + ph->fsize), 0, ph->msize - ph->fsize);
		}
	}
	
	return True;
}

UIntPtr ELFLoadSections(PELFHdr hdr) {
	if (hdr == Null) {																										// Sanity check
		return 0;
	}
	
	UIntPtr size = ELFGetSize(hdr);
	UIntPtr base = MmAllocAlignedUserMemory(size, MM_PAGE_SIZE);															// Alloc space for loading it
	
	if (base == 0) {
		return 0;
	} else if (!VirtChangeProtection(base, size, VIRT_PROT_READ | VIRT_PROT_WRITE | VIRT_PROT_EXEC)) {						// Change the protection
		MmFreeAlignedUserMemory(base);																						// Failed, free everything
		return 0;
	} else if (!ELFLoadPHdrs(hdr, base)) {																					// And try to load it!
		MmFreeAlignedUserMemory(base);																						// Failed, free everything
		return 0;
	}
	
	return base;
}

static PWChar ELFGetWName(PChar name, Boolean user) {
	if (name == Null) {																										// Sanity check
		return Null;
	}
	
	UIntPtr len = StrFormat(Null, L"%S", name);																				// Get the len that we need to allocate
	PWChar ret = (PWChar)(user ? MmAllocUserMemory(len) : MemAllocate(len));												// Alloc the space
	
	if (ret == Null) {
		return Null;																										// Failed...
	}
	
	StrFormat(ret, L"%S", name);																							// Copy the string
	
	return ret;																												// And return
}

static Boolean ELFGetSymbol(PELFSym sym, PWChar name, PLibHandle handle, PUIntPtr out) {
	if (sym == Null || handle == Null || out == Null) {																		// Sanity check
		return False;
	} else if (sym->shndx == 0) {																							// Try to find it in other handles?
		ListForeach(PsCurrentProcess->global_exec_handles, i) {																// Yes, first, search on the global handle list
			UIntPtr sm = ExecGetSymbol((PLibHandle)i->data, name);															// Try to get the symbol in this handle
			
			if (sm != 0) {
				*out = sm;																									// Found!
				return True;
			}
		}
		
		ListForeach(handle->deps, i) {																						// Let's try to get the symbol in the private dependency handles
			UIntPtr sm = ExecGetSymbol((PLibHandle)i->data, name);
			
			if (sm != 0) {
				*out = sm;																									// Found!
				return True;
			}
		}
		
		if (((sym->info >> 4) & 0x02) == 0x02) {																			// Weak symbol?
			*out = 1;																										// Yeah, just set the location to one (if we set to zero, it's not going to work)
			return True;
		}
		
		return False;
	}
	
	*out = handle->base + sym->value;																						// Ok, so we just need to add the base to the virt
	
	return True;
}

Boolean ELFAddSymbols(PELFHdr hdr, PLibHandle handle) {
	if (hdr == Null || handle == Null) {																					// Sanity check
		return False;
	}
	
	PELFDyn dyn = ELFGetPHdr(hdr, 0x02);																					// Get the dyn section of the executable/library
	
	if (dyn == Null) {
		return False;
	}
	
	PUInt32 hash = ELFGetDynPtr(hdr, dyn, 0x04);																			// Get some stuff that we need
	PChar strtab = ELFGetDynPtr(hdr, dyn, 0x05);
	PChar symtab = ELFGetDynPtr(hdr, dyn, 0x06);
	UIntPtr syment = ELFGetDynVal(hdr, dyn, 0x0B);
	
	if (hash == Null || strtab == Null || symtab == Null || syment == 0) {
		return False;																										// Failed, we can't continue
	}
	
	for (UInt32 i = 1; i < hash[1]; i++) {																					// Now, let's add all the symbols!
		PELFSym sym = (PELFSym)((UIntPtr)symtab + (i * syment));															// Get this symbol
		
		if (((sym->info >> 4) & 0x01) != 0x01 && ((sym->info >> 4) & 0x02) != 0x02) {										// We need to add it?
			continue;																										// Nope, it's just a local symbol
		}
		
		PExecSymbol sm = (PExecSymbol)MmAllocUserMemory(sizeof(ExecSymbol));												// Alloc space for the symbol
		
		if (sm == Null) {
			return False;
		}
		
		sm->name = ELFGetWName((PChar)((UIntPtr)strtab + sym->name), True);													// Get the name
		
		if (sm->name == Null) {
			MmFreeUserMemory((UIntPtr)sm);																					// Failed... :(
			return False;
		} else if (!ELFGetSymbol(sym, sm->name, handle, &sm->loc)) {														// Try to get the symbol location
			MmFreeUserMemory((UIntPtr)sm->name);																			// Failed...
			MmFreeUserMemory((UIntPtr)sm);
			return False;
		} else if (!ListAdd(handle->symbols, sm)) {																			// Finally, try to add it to the list!
			MmFreeUserMemory((UIntPtr)sm->name);
			MmFreeUserMemory((UIntPtr)sm);
			return False;
		}
	}
	
	return True;
}

PFsNode ELFFindFile(PWChar path) {
	if (path == Null) {																										// Sanity check
		return Null;
	} else if (path[0] == '/') {																							// Non-relative path?
		return FsOpenFile(path);																							// Yes :)
	}
	
	PWChar full = FsJoinPath(L"/System/Libraries", path);																	// Let's search on /System/Libraries
	
	if (full == Null) {																										// Failed to join the path?
		return Null;																										// Yes :(
	}
	
	PFsNode file = FsOpenFile(full);																						// Try to open it!
	
	MemFree((UIntPtr)full);																									// Free the full path
	
	if (file == Null) {																										// Try to search on the /Development/Libraries folder?
		full = FsJoinPath(L"/Development/Libraries", path);																	// Yes
		
		if (full == Null) {																									// Failed to join the path?
			return Null;																									// Yes :(
		}
		
		file = FsOpenFile(full);																							// Try to open it!
		
		MemFree((UIntPtr)full);																								// Free the full path
	}
	
	return file;																											// Return
}

Boolean ELFLoadDeps(PELFHdr hdr, PLibHandle handle) {
	if (hdr == Null) {																										// Sanity check
		return False;
	}
	
	PELFDyn dyn = ELFGetPHdr(hdr, 0x02);																					// Get the dyn section of the executable/library, we need it to get the strtab and all the deps
	
	if (dyn == Null) {
		return False;
	}
	
	PChar strtab = ELFGetDynPtr(hdr, dyn, 0x05);																			// Get the strtab
	
	if (strtab == Null) {
		return False;
	}
	
	for (PELFDyn cur = dyn; cur->tag != 0; cur++) {																			// Now, let's iterate and get all the libs
		if (cur->tag != 0x01) {																								// Dependency?
			continue;																										// Nope, skip this entry
		}
		
		PWChar name = ELFGetWName((PChar)((UIntPtr)strtab + cur->val_ptr), False);											// Get the lib name
		
		if (name == Null) {
			if (handle != Null) {																							// Failed :(
				ListForeach(handle->deps, i) {
					ExecCloseLibrary((PLibHandle)i->data);
				}
			}
			
			return False;
		}
		
		PLibHandle hndl = ExecLoadLibrary(name, handle == Null);															// Try to load it
		
		MemFree((UIntPtr)name);																								// Free the name
		
		if (hndl == Null || !hndl->resolved) {
			if (handle != Null) {																							// Failed to load/recursive loading...
				ListForeach(handle->deps, i) {
					ExecCloseLibrary((PLibHandle)i->data);
				}
			}
			
			return False;
		} else if (handle != Null && !ListAdd(handle->deps, hndl)) {														// Add it to the list!
			ExecCloseLibrary(hndl);																							// Failed...
			
			if (handle != Null) {																							// Failed :(
				ListForeach(handle->deps, i) {
					ExecCloseLibrary((PLibHandle)i->data);
				}
			}
			
			return False;
		}
	}
	
	return True;
}

static Boolean ELFRelocateInt(PELFHdr hdr, PLibHandle handle, UIntPtr base, PChar rel, PChar strtab, PChar symtab, UIntPtr entsize, UIntPtr syment, UIntPtr limit, Boolean a) {
	if (hdr == Null || rel == Null || strtab == Null || symtab == Null) {													// Sanity checks
		return False;	
	}
	
	for (PChar end = rel + limit; rel < end; rel += entsize) {																// Let's iterate!
		PELFRel cur = (PELFRel)rel;																							// Get the ELFRel
#if __INTPTR_WIDTH__ == 64																									// Get the symbol that it may need
		PELFSym sym = (PELFSym)(symtab + ((cur->info >> 32) * syment));
#else
		PELFSym sym = (PELFSym)(symtab + ((cur->info >> 8) * syment));
#endif
		
		if (a && !ArchELFRelocateA(hdr, handle, base, (PELFRelA)cur, strtab, sym, (UInt8)cur->info)) {						// Call the arch specific function
			return False;
		} else if (!a && !ArchELFRelocate(hdr, handle, base, cur, strtab, sym, (UInt8)cur->info)) {
			return False;
		}
	}
	
	return True;
}

Boolean ELFRelocate(PELFHdr hdr, PLibHandle handle, UIntPtr base) {
	if (hdr == Null) {																										// Sanity check
		return False;
	}
	
	PELFDyn dyn = ELFGetPHdr(hdr, 0x02);																					// Get the dyn section of the executable/library
	
	if (dyn == Null) {
		return False;
	}
	
	UIntPtr pltrelsz = ELFGetDynVal(hdr, dyn, 0x02);																		// Get some stuff that we need
	PChar strtab = ELFGetDynPtr(hdr, dyn, 0x05);
	PChar symtab = ELFGetDynPtr(hdr, dyn, 0x06);
	PChar rela = ELFGetDynPtr(hdr, dyn, 0x07);
	UIntPtr relasz = ELFGetDynVal(hdr, dyn, 0x08);
	UIntPtr relaent = ELFGetDynVal(hdr, dyn, 0x09);
	UIntPtr syment = ELFGetDynVal(hdr, dyn, 0x0B);
	PChar rel = ELFGetDynPtr(hdr, dyn, 0x11);
	UIntPtr relsz = ELFGetDynVal(hdr, dyn, 0x12);
	UIntPtr relent = ELFGetDynVal(hdr, dyn, 0x13);
	UIntPtr pltrel = ELFGetDynVal(hdr, dyn, 0x14);
	PChar jmprel = ELFGetDynPtr(hdr, dyn, 0x17);
	
	if (strtab == Null || symtab == Null || syment == 0) {
		return False;																										// Failed, we can't continue
	} else if (rel != Null && !ELFRelocateInt(hdr, handle, base, rel, strtab, symtab, relent, syment, relsz, False)) {		// First, use the ELFRel section
		return False;
	}
	
	if (rela != Null && !ELFRelocateInt(hdr, handle, base, rela, strtab, symtab, relaent, syment, relasz, True)) {			// Now, use the ELFRelA section
		return False;
	}
	
	if (jmprel != Null) {																									// We have the jmprel section?
		Boolean a = pltrel == 0x07;																							// Yes, get if it is ELFRelA or ELFRel
		UIntPtr entsz = a ? sizeof(ELFRelA) : sizeof(ELFRel);
		
		if (!ELFRelocateInt(hdr, handle, base, jmprel, strtab, symtab, entsz, syment, pltrelsz, a)) {						// And reloc!
			return False;
		}
	}
	
	return True;
}
