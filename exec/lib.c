// File author is Ítalo Lima Marconato Matias
//
// Created on November 10 of 2018, at 21:18 BRT
// Last edited on August 27 of 2019, at 14:48 BRT

#include <chicago/alloc.h>
#include <chicago/chexec.h>
#include <chicago/exec.h>
#include <chicago/file.h>
#include <chicago/mm.h>
#include <chicago/process.h>
#include <chicago/string.h>

static PFsNode ExecFindFile(PWChar path) {
	if (path == Null) {																											// Sanity check
		return Null;
	} else if (path[0] == '\\') {																								// Non-relative path?
		return FsOpenFile(path);																								// Yes :)
	}
	
	PWChar full = FsJoinPath(L"\\System\\Libraries", path);																		// Let's search on \System\Libraries
	
	if (full == Null) {																											// Failed to join the path?
		return Null;																											// Yes :(
	}
	
	PFsNode file = FsOpenFile(full);																							// Try to open it!
	
	MemFree((UIntPtr)full);																										// Free the full path
	
	return file;																												// Return
}

static PWChar ExecGetName(PWChar path, Boolean user) {
	if (path == Null) {																											// Sanity check
		return Null;
	} else if (path[0] != '\\') {																								// We really need to get the name?
		return StrDuplicate(path);																								// Nope :)
	}
	
	PWChar last = Null;
	PWChar tok = StrTokenize(path, L"\\");
	
	while (tok != Null) {																										// Let's go!
		if (last != Null) {																										// Free the old last?
			MemFree((UIntPtr)last);																								// Yes
		}
		
		last = StrDuplicate(tok);																								// Duplicate the token
		
		if (last == Null) {
			return Null;																										// Failed...
		}
		
		tok = StrTokenize(Null, L"\\");																							// Tokenize next
	}
	
	if (last == Null) {
		return Null;																											// Failed...
	}
	
	if (user) {																													// Copy to userspace?
		PWChar ret = (PWChar)MmAllocUserMemory((StrGetLength(last) + 1) * 4);													// Yes! Alloc the required space
		
		if (ret == Null) {
			MemFree((UIntPtr)last);																								// Failed...
			return Null;
		}
		
		StrCopy(ret, last);																										// Copy!
		MemFree((UIntPtr)last);
		
		return ret;
	} else {
		return last;																											// No :)
	}
}

static PExecHandle ExecGetHandle(PWChar path) {
	if ((path == Null) || (PsCurrentThread == Null) || (PsCurrentProcess == Null)) {											// Sanity checks
		return Null;
	} else if (PsCurrentProcess->handle_list == Null) {
		return Null;	
	}
	
	PWChar name = ExecGetName(path, False);																						// Get the handle name
	
	if (name == Null) {
		return Null;																											// Failed...
	}
	
	ListForeach(PsCurrentProcess->handle_list, i) {																				// Let's search!
		PWChar hname = ((PExecHandle)i->data)->name;
		
		if (StrGetLength(hname) != StrGetLength(name)) {																		// Same length?
			continue;																											// No...
		} else if (StrCompare(hname, name)) {																					// Found?
			MemFree((UIntPtr)name);																								// Yes!
			return (PExecHandle)i->data;
		}
	}
	
	MemFree((UIntPtr)name);																										// We didn't found it...
	
	return Null;
}

static Boolean ExecGetSymbolLocation(PCHExecSymbol sym, UIntPtr base, PExecHandle handle, PUIntPtr outloc) {
	if ((sym == Null) || (base == 0) || (handle == Null) || (outloc == Null)) {													// Sanity checks
		return False;
	} else if ((sym->flags & CHEXEC_SYMBOL_FLAGS_UNDEF) == CHEXEC_SYMBOL_FLAGS_UNDEF) {											// Try to find in other handles?
		ListForeach(PsCurrentProcess->global_handle_list, i) {																	// Yes
			UIntPtr sm = ExecGetSymbol((PExecHandle)i->data, sym->name);														// Try to get the symbol in this handle
			
			if (sm != 0) {
				*outloc = sm;																									// Found!
				return True;
			}
		}
		
		ListForeach(handle->deps, i) {																							// Let's try to get the symbol in the private dependency handles
			UIntPtr sm = ExecGetSymbol((PExecHandle)i->data, sym->name);
			
			if (sm != 0) {
				*outloc = sm;																									// Found!
				return True;
			}
		}
		
		return False;
	} else {
		*outloc = base + sym->virt;																								// No, just add the base to it!
		return True;
	}
}

static Boolean ExecFillDependencyTable(PList list, PUInt8 buf) {
	if ((list == Null) || (buf == Null)) {																						// Sanity checks
		return False;
	}
	
	PCHExecHeader hdr = (PCHExecHeader)buf;
	PCHExecDependency dep = (PCHExecDependency)(((UIntPtr)buf) + hdr->dep_start);
	
	for (UIntPtr i = 0; i < hdr->dep_count; i++) {																				// Let's do it!
		PExecHandle handle = ExecLoadLibrary(dep->name, False);																	// Try to load this library
		
		if (handle == Null) {
			ListForeach(list, i) {																								// Failed...
				ExecCloseLibrary((PExecHandle)i->data);
			}
			
			return False;
		} else if (!handle->resolved) {
			ExecCloseLibrary(handle);																							// Recursive...
			
			ListForeach(list, i) {
				ExecCloseLibrary((PExecHandle)i->data);
			}
			
			return False;
		} else if (!ListAdd(list, handle)) {																					// Try to add it to the list!
			ExecCloseLibrary(handle);																							// Failed...
			
			ListForeach(list, i) {
				ExecCloseLibrary((PExecHandle)i->data);
			}
			
			return False;
		}
		
		dep = (PCHExecDependency)(((UIntPtr)dep) + sizeof(CHExecDependency) + (dep->name_len * sizeof(WChar)));
	}
	
	return True;
}

Boolean ExecFillSymbolTable(PList list, UIntPtr base, PExecHandle handle, PUInt8 buf) {
	if ((list == Null) || (base == 0) || (handle == Null) || (buf == Null)) {													// Sanity checks
		return False;
	}
	
	PCHExecHeader hdr = (PCHExecHeader)buf;
	PCHExecSymbol sym = (PCHExecSymbol)(((UIntPtr)buf) + hdr->st_start);
	
	for (UIntPtr i = 0; i < hdr->st_count; i++) {																				// Let's do it!
		PExecSymbol sm = (PExecSymbol)MmAllocUserMemory(sizeof(ExecSymbol));													// Alloc space for the sumbol
		
		if (sm == Null) {
			ListForeach(list, i) {																								// Failed...
				MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
				MmFreeUserMemory((UIntPtr)i->data);
			}
			
			return False;
		}
		
		sm->name = (PWChar)MmAllocUserMemory(sym->name_len * sizeof(WChar));													// Alloc space for the name
		
		if (sm->name == Null) {
			MmFreeUserMemory((UIntPtr)sm);																						// Failed...
			
			ListForeach(list, i) {
				MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
				MmFreeUserMemory((UIntPtr)i->data);
			}
			
			return False;
		}
		
		StrCopy(sm->name, sym->name);																							// Copy the name
		
		if (!ExecGetSymbolLocation(sym, base, handle, &sm->loc)) {																// Get the symbol location
			MmFreeUserMemory((UIntPtr)sm->name);																				// Failed...
			MmFreeUserMemory((UIntPtr)sm);
			
			ListForeach(list, i) {
				MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
				MmFreeUserMemory((UIntPtr)i->data);
			}
			
			return False;
		} else if (!ListAdd(list, sm)) {																						// Try to add it to the list!
			MmFreeUserMemory((UIntPtr)sm->name);																				// Failed...
			MmFreeUserMemory((UIntPtr)sm);
			
			ListForeach(list, i) {
				MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
				MmFreeUserMemory((UIntPtr)i->data);
			}
			
			return False;
		}
		
		sym = (PCHExecSymbol)(((UIntPtr)sym) + sizeof(CHExecSymbol) + (sym->name_len * sizeof(WChar)));
	}
	
	return True;
}

Boolean ExecRelocate(UIntPtr base, PExecHandle handle, PUInt8 buf) {
	if ((base == 0) || (handle == Null) || (buf == Null)) {																		// Sanity checks
		return False;
	}
	
	PCHExecHeader hdr = (PCHExecHeader)buf;
	PCHExecRelocation rel = (PCHExecRelocation)(((UIntPtr)buf) + hdr->rel_start);
	
	for (UIntPtr i = 0; i < hdr->rel_count; i++) {																				// Let's do it...
		UIntPtr incr = rel->incr;
		
		if ((rel->op & CHEXEC_REL_OP_REL) == CHEXEC_REL_OP_REL) {																// Relative
			incr -= rel->virt;
		} else if ((rel->op & CHEXEC_REL_OP_SYM) == CHEXEC_REL_OP_SYM) {														// Symbol
			UIntPtr sym = ExecGetSymbol(Null, rel->name);																		// Try to get the symbol
			
			if (sym == 0) {
				return False;																									// Failed
			}
			
			incr += sym;
		} else if ((rel->op & CHEXEC_REL_OP_REL_SYM) == CHEXEC_REL_OP_REL_SYM) {												// Relative symbol
			UIntPtr sym = ExecGetSymbol(Null, rel->name);																		// Try to get the symbol
			
			if (sym == 0) {
				return False;																									// Failed
			}
			
			incr += sym - (base + rel->virt);
		} else {
			incr += base;																										// Absolute
		}
		
		if ((rel->op & CHEXEC_REL_OP_BYTE) == CHEXEC_REL_OP_BYTE) {																// Byte
			*((PUInt8)(base + rel->virt)) += (UInt8)incr;
		} else if ((rel->op & CHEXEC_REL_OP_WORD) == CHEXEC_REL_OP_WORD) {														// Word
			*((PUInt16)(base + rel->virt)) += (UInt16)incr;
		} else if ((rel->op & CHEXEC_REL_OP_DWORD) == CHEXEC_REL_OP_DWORD) {													// DWord
			*((PUInt32)(base + rel->virt)) += (UInt32)incr;
		}
		
		rel = (PCHExecRelocation)(((UIntPtr)rel) + sizeof(CHExecRelocation) + (rel->name_len * sizeof(WChar)));
	}
	
	return True;
}

static Boolean ExecLoadLibraryInt(PExecHandle handle, PUInt8 buf) {
	if (!CHExecValidateHeader(buf, CHEXEC_HEADER_FLAGS_LIBRARY)) {																// Validate the header
		ListFree(handle->deps);																									// Invalid CHExec file :(
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle->name);
		
		return False;
	}
	
	handle->base = CHExecLoadSections(buf);																						// Load the sections
	
	if (handle->base == 0) {
		ListFree(handle->deps);																									// Failed, free everything
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle->name);
		
		return False;
	}
	
	if (!ExecFillDependencyTable(handle->deps, buf)) {																			// Load the dependencies
		MmFreeUserMemory(handle->base);																							// Failed, free everything
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle->name);
		
		return False;
	}
	
	if (!ExecFillSymbolTable(handle->symbols, handle->base, handle, buf)) {														// Fill the symbol table
		ListForeach(handle->deps, i) {																							// Failed, close all the dep handles
			ExecCloseLibrary((PExecHandle)i->data);
		}
		
		MmFreeUserMemory(handle->base);																							// And free everything
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle->name);
		
		return False;
	}
	
	if (!ExecRelocate(handle->base, handle, buf)) {																				// Relocate
		ListForeach(handle->symbols, i) {																						// Failed, free the symbol table
			MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
			MmFreeUserMemory((UIntPtr)i->data);
		}
		
		ListForeach(handle->deps, i) {																							// Close all the dep handles
			ExecCloseLibrary((PExecHandle)i->data);
		}
		
		MmFreeUserMemory(handle->base);																							// And free everything
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle->name);
		
		return False;
	}
	
	handle->resolved = True;																									// :)
	
	return True;
}

PExecHandle ExecLoadLibrary(PWChar path, Boolean global) {
	if ((path == Null) || (PsCurrentThread == Null) || (PsCurrentProcess == Null)) {											// Sanity checks
		return Null;
	} else if ((PsCurrentProcess->handle_list == Null) || (PsCurrentProcess->global_handle_list == Null)) {						// Init the handle list (or the global handle list)?
		if (PsCurrentProcess->handle_list == Null) {																			// Yes, init the handle list?
			PsCurrentProcess->handle_list = ListNew(False, False);																// Yes
			
			if (PsCurrentProcess->handle_list == Null) {
				return Null;																									// Failed...
			}
		}
		
		if (PsCurrentProcess->global_handle_list == Null) {																		// Init the global handle list?
			PsCurrentProcess->global_handle_list = ListNew(False, False);														// Yes
			
			if (PsCurrentProcess->global_handle_list == Null) {
				return Null;																									// Failed
			}
		}
	}
	
	PExecHandle handle = ExecGetHandle(path);																					// First, let's try to get this handle
	
	if (handle != Null) {
		handle->refs++;																											// Ok, just increase the refs
		return handle;
	}
	
	PWChar name = ExecGetName(path, True);																						// Get the handle name
	
	if (name == Null) {
		return Null;																											// Failed
	}
	
	PFsNode file = ExecFindFile(path);																							// Try to open it
	
	if (file == Null) {
		MemFree((UIntPtr)name);																									// Failed
		return Null;
	}
	
	PUInt8 buf = (PUInt8)MemAllocate(file->length);																				// Alloc space for reading the file
	
	if (buf == Null) {
		FsCloseFile(file);																										// Failed
		MemFree((UIntPtr)name);
		
		return Null;
	}
	
	if (!FsReadFile(file, 0, file->length, buf)) {																				// Try to read it!
		MemFree((UIntPtr)buf);																									// Failed
		FsCloseFile(file);
		MemFree((UIntPtr)name);
		
		return Null;
	}
	
	FsCloseFile(file);																											// Ok, now we can close the file
	
	handle = (PExecHandle)MmAllocUserMemory(sizeof(ExecHandle));																// Alloc space for the handle
	
	if (handle == Null) {
		MemFree((UIntPtr)buf);																									// Failed...
		MemFree((UIntPtr)name);
		
		return Null;
	}
	
	handle->name = name;																										// Set the name
	handle->refs = 1;																											// Set the refs
	handle->symbols = ListNew(True, True);																						// Alloc the symbol list
	
	if (handle->symbols == Null) {
		MmFreeUserMemory((UIntPtr)handle);																						// Failed...
		MemFree((UIntPtr)buf);
		MemFree((UIntPtr)name);
		
		return Null;
	}
	
	handle->deps = ListNew(False, True);																						// Alloc the dep handle list
	
	if (handle->deps == Null) {
		ListFree(handle->symbols);																								// Failed
		MmFreeUserMemory((UIntPtr)handle);
		MemFree((UIntPtr)buf);
		MemFree((UIntPtr)name);
		
		return Null;
	}
	
	handle->resolved = False;
	
	if (!ListAdd(PsCurrentProcess->handle_list, handle)) {																		// Add this handle to the handle list
		ListFree(handle->deps);																									// Failed...
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		MemFree((UIntPtr)buf);
		MemFree((UIntPtr)name);
		
		return Null;
	}
	
	if (!ExecLoadLibraryInt(handle, buf)) {																						// Run the internal routines for loading it!
		MmFreeUserMemory((UIntPtr)handle);																						// Failed to load it...
		MemFree((UIntPtr)buf);
		
		return Null;
	}
	
	MemFree((UIntPtr)buf);																										// Free our buffer
	
	if (global) {
		if (!ListAdd(PsCurrentProcess->global_handle_list, handle)) {															// Add it to the global handle list
			ExecCloseLibrary(handle);																							// Failed...
			return Null;
		}
	}
	
	return handle;
}

Void ExecCloseLibrary(PExecHandle handle) {
	if ((handle == Null) || (PsCurrentThread == Null) || (PsCurrentProcess == Null)) {											// Sanity checks
		return;
	} else if ((PsCurrentProcess->handle_list == Null) || (PsCurrentProcess->global_handle_list == Null)) {						// Even more sanity checks
		return;
	} else if (handle->refs > 1) {
		handle->refs--;																											// Just decrease the refs count
		return;
	}
	
	UIntPtr idx = 0;
	
	ListForeach(PsCurrentProcess->global_handle_list, i) {																		// Remove it from the global handle list
		if (i->data == handle) {
			ListRemove(PsCurrentProcess->global_handle_list, idx);
			break;
		} else {
			idx++;	
		}
	}
	
	idx = 0;
	
	ListForeach(PsCurrentProcess->handle_list, i) {																				// Remove it from the handle list
		if (i->data == handle) {
			ListRemove(PsCurrentProcess->handle_list, idx);
			break;
		} else {
			idx++;	
		}
	}
	
	ListForeach(handle->deps, i) {																								// Close all the dep handles
		ExecCloseLibrary((PExecHandle)i->data);
	}
	
	ListForeach(handle->symbols, i) {																							// Free all the symbols
		MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
	}
	
	ListFree(handle->deps);																										// Free the dep handle list
	ListFree(handle->symbols);																									// Free the symbol list
	MmFreeUserMemory((UIntPtr)handle->name);																					// Free the name
	MmFreeUserMemory((UIntPtr)handle->base);																					// Free the loaded sections
	MmFreeUserMemory((UIntPtr)handle);																							// And free the handle itself
}

UIntPtr ExecGetSymbol(PExecHandle handle, PWChar name) {
	if ((name == Null) || (PsCurrentThread == Null) || (PsCurrentProcess == Null)) {											// Sanity checks
		return 0;
	} else if (handle == Null) {																								// Search in the global handle list?
		if (PsCurrentProcess->global_handle_list == Null) {																		// Yes! But the global handle list is initialized?
			return 0;																											// No :(
		}
		
		ListForeach(PsCurrentProcess->global_handle_list, i) {																	// Let's search!
			UIntPtr sym = ExecGetSymbol((PExecHandle)i->data, name);
			
			if (sym != 0) {
				return sym;																										// Found :)
			}
		}
		
		return 0;
	}
	
	ListForeach(handle->symbols, i) {																							// Search in the handle!
		PExecSymbol sym = (PExecSymbol)i->data;
		
		if (StrGetLength(sym->name) != StrGetLength(name)) {																	// Same length?
			continue;																											// No...
		} else if (StrCompare(sym->name, name)) {
			return sym->loc;																									// Found!
		}
	}
	
	return 0;
}
