// File author is Ítalo Lima Marconato Matias
//
// Created on November 15 of 2019, at 20:07 BRT
// Last edited on November 17 of 2019, at 18:21 BRT

#include <chicago/alloc.h>
#include <chicago/arch.h>
#include <chicago/drv.h>
#include <chicago/elf.h>
#include <chicago/mm.h>
#include <chicago/string.h>

List DrvList = { Null, Null, 0, False, False };

static PWChar DrvGetName(PWChar path, Boolean user) {
	if (path == Null) {																					// Sanity check
		return Null;
	}
	
	PWChar last = Null;
	Boolean free = False;
	
	if (path[0] != '/' && user) {																		// We really need to get the name?
		last = path;																					// Nope :)
		free = False;
		goto a;
	} else if (path[0] != '/') {
		return StrDuplicate(path);
	}
	
	PWChar tok = StrTokenize(path, L"/");
	
	while (tok != Null) {																				// Let's go!
		if (last != Null) {																				// Free the old last?
			MemFree((UIntPtr)last);																		// Yes
		}
		
		last = StrDuplicate(tok);																		// Duplicate the token
		
		if (last == Null) {
			return Null;																				// Failed...
		}
		
		tok = StrTokenize(Null, L"/");																	// Tokenize next
	}
	
	if (last == Null) {
		return Null;																					// Failed...
	}
	
a:	if (user) {																							// Copy to userspace?
		PWChar ret = (PWChar)MmAllocUserMemory((StrGetLength(last) + 1) * sizeof(WChar));				// Yes! Alloc the required space
		
		if (ret == Null) {
			MemFree((UIntPtr)last);																		// Failed...
			return Null;
		}
		
		StrCopy(ret, last);																				// Copy!
		
		if (free) {
			MemFree((UIntPtr)last);
		}
		
		return ret;
	} else {
		return last;																					// No :)
	}
}

static Boolean DrvFindLoadedDriver(PWChar name) {
	ListForeach(&DrvList, i) {																			// Just iterate the list and check if we are already loaded
		if (StrCompare((PWChar)(((PDrvHandle)i->data)->name), name)) {
			return True;
		}
	}
	
	return False;
}

static PFsNode DrvFindFile(PWChar path) {
	if (path == Null) {																					// Sanity check
		return Null;
	} else if (path[0] == '/') {																		// Non-relative path?
		return FsOpenFile(path);																		// Yes :)
	}
	
	PWChar full = FsJoinPath(L"/System/Drivers", path);													// Let's search on /System/Drivers
	
	if (full == Null) {																					// Failed to join the path?
		return Null;																					// Yes :(
	}
	
	PFsNode file = FsOpenFile(full);																	// Try to open it!
	
	MemFree((UIntPtr)full);																				// Free the full path
	
	return file;																						// Return
}

static PWChar DrvDuplicateString(PWChar str) {
	PWChar out = (PWChar)MmAllocUserMemory((StrGetLength(str) + 1) * sizeof(WChar));					// Alloc space for the string
	
	if (out != Null) {
		StrCopy(out, str);																				// And copy it
	}
	
	return out;
}

PLibHandle DrvLoadLibCHKrnl(Void) {
	if ((PsCurrentProcess->handle_list == Null) || (PsCurrentProcess->global_handle_list == Null)) {	// Init the handle list (or the global handle list)?
		if (PsCurrentProcess->handle_list == Null) {													// Yes, init the handle list?
			PsCurrentProcess->handle_list = ListNew(False, False);										// Yes
			
			if (PsCurrentProcess->handle_list == Null) {
				return Null;																			// Failed...
			}
		}
		
		if (PsCurrentProcess->global_handle_list == Null) {												// Init the global handle list?
			PsCurrentProcess->global_handle_list = ListNew(False, False);								// Yes
			
			if (PsCurrentProcess->global_handle_list == Null) {
				return Null;																			// Failed
			}
		}
	}
	
	PLibHandle handle = (PLibHandle)MmAllocUserMemory(sizeof(LibHandle));								// Alloc space for the handle
	
	if (handle == Null) {
		return Null;
	}
	
	handle->name = DrvDuplicateString(L"libchkrnl.so");													// Set the name
	
	if (handle->name == Null) {
		MmFreeUserMemory((UIntPtr)handle);
		return Null;
	}
	
	handle->refs = 1;
	handle->symbols = ListNew(True, True);																// Alloc the symbol list
	
	if (handle->symbols == Null) {
		MmFreeUserMemory((UIntPtr)handle->name);
		MmFreeUserMemory((UIntPtr)handle);
		return Null;
	}
	
	handle->deps = ListNew(False, True);																// And the dependency list (that is always going to be zero-sized lol)
	
	if (handle->symbols == Null) {
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle->name);
		MmFreeUserMemory((UIntPtr)handle);
		return Null;
	}
	
	handle->resolved = True;
	
	for (UIntPtr i = 0; i < KernelSymbolTableSize;) {													// Now, let's add all the symbols
		PExecSymbol sym = (PExecSymbol)MmAllocUserMemory(sizeof(ExecSymbol));							// Alloc space for this symbol
		
		if (sym == Null) {
			ListForeach(handle->symbols, i) {															// Failed, free the symbol table
				MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
			}
			
			ListFree(handle->deps);
			ListFree(handle->symbols);
			MmFreeUserMemory((UIntPtr)handle->name);
			MmFreeUserMemory((UIntPtr)handle);
			
			return Null;
		}
		
		sym->name = DrvDuplicateString((PWChar)(KernelSymbolTable + i));								// Duplicate the name
		
		if (sym->name == Null) {
			MmFreeUserMemory((UIntPtr)sym);																// Oh.. :/
			
			ListForeach(handle->symbols, i) {
				MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
			}
			
			ListFree(handle->deps);
			ListFree(handle->symbols);
			MmFreeUserMemory((UIntPtr)handle->name);
			MmFreeUserMemory((UIntPtr)handle);
			
			return Null;
		}
		
		i += (StrGetLength(sym->name) + 1) * sizeof(WChar);												// Increase 'i' by the name size
		sym->loc = *((PUIntPtr)(KernelSymbolTable + i));												// Get the symbol location
		i += sizeof(UIntPtr);																			// And increase 'i' by the symbol location size
		
		if (!ListAdd(handle->symbols, sym)) {															// Finally, add the symbol
			MmFreeUserMemory((UIntPtr)sym->name);														// :/
			MmFreeUserMemory((UIntPtr)sym);
			
			ListForeach(handle->symbols, i) {
				MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
			}
			
			ListFree(handle->deps);
			ListFree(handle->symbols);
			MmFreeUserMemory((UIntPtr)handle->name);
			MmFreeUserMemory((UIntPtr)handle);
			
			return Null;
		}
	}
	
	if (!ListAdd(PsCurrentProcess->handle_list, handle)) {												// Add the handle to the handle list
		ListForeach(handle->symbols, i) {																// Failed, free the symbol table
			MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
		}
		
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle->name);
		MmFreeUserMemory((UIntPtr)handle);
		
		return Null;
	}
	
	ListAdd(PsCurrentProcess->global_handle_list, handle);
	
	return handle;
}

static Boolean DrvLoadInt(PDrvHandle handle, PELFHdr hdr) {
	if (!ELFCheck(hdr, True)) {																			// Validate the header
		return False;
	}
	
	handle->base = ELFLoadSections(hdr);																// Load the sections
	
	if (handle->base == 0) {
		return False;
	} else if (!ELFLoadDeps(hdr, (PLibHandle)handle)) {													// Load the deps
		MmFreeAlignedUserMemory(handle->base);
		return False;
	} else if (!ELFAddSymbols(hdr, (PLibHandle)handle)) {												// Add the symbols
		MmFreeAlignedUserMemory(handle->base);
		return False;
	} else if (!ELFRelocate(hdr, (PLibHandle)handle, handle->base)) {									// And relocate!
		MmFreeAlignedUserMemory(handle->base);
		return False;
	}
	
	handle->thread = PsCreateThread(handle->base + hdr->entry, 0, False);								// Now, let's create the driver entry thread
	
	if (handle->thread == Null) {
		MmFreeAlignedUserMemory(handle->base);
		return False;
	}
	
	PsAddThread(handle->thread);																		// And, let's add it!
	
	return True;
}

PDrvHandle DrvLoad(PWChar path) {
	if (path == Null) {																					// Null pointer check
		return Null;
	}
	
	PWChar name = DrvGetName(path, True);																// Get the driver name (in case we were passed a full path)
	
	if (name == Null) {
		return Null;																					// ...
	} else if (DrvFindLoadedDriver(name)) {																// Let's see if this driver is already loaded
		MmFreeUserMemory((UIntPtr)name);																// Yup, it is...
		return Null;
	}
	
	PFsNode file = DrvFindFile(path);																	// Let's try to open it
	
	if (file == Null) {
		MmFreeUserMemory((UIntPtr)name);
		return Null;																					// ...
	}
	
	PUInt8 buf = (PUInt8)MemAllocate(file->length);														// Let's read the file
	
	if (buf == Null) {
		FsCloseFile(file);
		MmFreeUserMemory((UIntPtr)name);
		return Null;
	} else if (!FsReadFile(file, 0, file->length, buf)) {
		MemFree((UIntPtr)buf);
		FsCloseFile(file);
		MmFreeUserMemory((UIntPtr)name);
		return Null;
	}
	
	FsCloseFile(file);																					// Now, we can close it :)
	
	PDrvHandle handle = (PDrvHandle)MmAllocUserMemory(sizeof(DrvHandle));								// Alloc the handle
	
	if (handle == Null) {
		MemFree((UIntPtr)buf);
		MmFreeUserMemory((UIntPtr)name);
		return Null;
	}
	
	handle->name = name;
	handle->symbols = ListNew(True, True);																// Alloc the symbol list
	
	if (handle->symbols == Null) {
		MmFreeUserMemory((UIntPtr)handle);
		MemFree((UIntPtr)buf);
		MmFreeUserMemory((UIntPtr)name);
		return Null;
	}
	
	handle->deps = ListNew(False, True);																// And the dependency list
	
	if (handle->deps == Null) {
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		MemFree((UIntPtr)buf);
		MmFreeUserMemory((UIntPtr)name);
		return Null;
	}
	
	handle->thread = Null;
	
	if (!ListAdd(&DrvList, handle)) {																	// Add it to the loaded driver list
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		MemFree((UIntPtr)buf);
		MmFreeUserMemory((UIntPtr)name);
		return Null;
	} else if (!DrvLoadInt(handle, (PELFHdr)buf)) {														// Call the internal routine for loading it
		ListForeach(handle->symbols, i) {																// Failed, let's free all the symbols
			MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
		}
		
		ListForeach(handle->deps, i) {																	// Close all the dependencies
			ExecCloseLibrary((PLibHandle)i->data);
		}
		
		ListFree(handle->deps);																			// And free everything else
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		MemFree((UIntPtr)buf);
		MmFreeUserMemory((UIntPtr)name);
		
		UIntPtr idx = 0;
		
		ListForeach(&DrvList, i) {
			if (i->data == handle) {
				ListRemove(&DrvList, idx);
				break;
			}
			
			idx++;
		}
		
		return Null;
	}
	
	MemFree((UIntPtr)buf);
	PsWaitThread(handle->thread->id);
	
	return handle;
}
