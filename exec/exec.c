// File author is Ítalo Lima Marconato Matias
//
// Created on November 01 of 2019, at 16:50 BRT
// Last edited on December 28 of 2019, at 14:15 BRT

#include <chicago/alloc.h>
#include <chicago/arch.h>
#include <chicago/elf.h>
#include <chicago/mm.h>
#include <chicago/process.h>
#include <chicago/string.h>
#include <chicago/virt.h>

static PWChar ExecGetName(PWChar path) {
	if (path == Null) {																														// Sanity check
		return Null;
	}
	
	PWChar last = Null;
	PWChar dup = StrDuplicate(path);
	PWChar tok = StrTokenize(dup, L"/");
	
	while (tok != Null) {																													// Let's go!
		if (last != Null) {																													// Free the old last?
			MemFree((UIntPtr)last);																											// Yes
		}
		
		last = StrDuplicate(tok);																											// Duplicate the token
		
		if (last == Null) {
			MemFree((UIntPtr)dup);																											// Failed...
			return Null;
		}
		
		tok = StrTokenize(Null, L"/");																										// Tokenize next
	}
	
	MemFree((UIntPtr)dup);
	
	return last;
}

static Boolean ExecLoadInt(PExecHandle handle, PELFHdr hdr) {
	if (!ELFCheck(hdr, True)) {																												// Validate the header
		return False;
	}
	
	handle->base = ELFLoadSections(hdr);																									// Load the sections
	
	if (handle->base == 0) {
		return False;
	} else if (!ELFLoadDeps(hdr, (PLibHandle)handle)) {																						// Load the deps
		MmFreeAlignedUserMemory(handle->base);
		return False;
	} else if (!ELFAddSymbols(hdr, (PLibHandle)handle)) {																					// Add the symbols
		MmFreeAlignedUserMemory(handle->base);
		return False;
	} else if (!ELFRelocate(hdr, (PLibHandle)handle, handle->base)) {																		// And relocate!
		MmFreeAlignedUserMemory(handle->base);
		return False;
	}
	
	return True;
}

static Void ExecCreateProcessInt(Void) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (PsCurrentProcess->exec_path == Null)) {									// Sanity checks
		PsExitProcess((UIntPtr)-1);
	}
	
	PExecHandle handle = (PExecHandle)MmAllocUserMemory(sizeof(ExecHandle));																// Alloc space for the exec handle
	
	if (handle == Null) {
		MemFree((UIntPtr)PsCurrentProcess->exec_path);																						// Failed...
		PsExitProcess((UIntPtr)-1);
	}
	
	handle->symbols = ListNew(True, True);																									// Alloc the symbol list
	
	if (handle->symbols == Null) {
		MmFreeUserMemory((UIntPtr)handle);																									// Failed...
		MemFree((UIntPtr)PsCurrentProcess->exec_path);
		PsExitProcess((UIntPtr)-1);
	}
	
	handle->deps = ListNew(True, True);																										// Alloc the dependency list
	
	if (handle->deps == Null) {
		ListFree(handle->symbols);																											// Failed...
		MmFreeUserMemory((UIntPtr)handle);
		MemFree((UIntPtr)PsCurrentProcess->exec_path);
		PsExitProcess((UIntPtr)-1);
	}
	
	UIntPtr stack = VirtAllocAddress(0, 0x100000, VIRT_PROT_READ | VIRT_PROT_WRITE | VIRT_FLAGS_HIGHEST | VIRT_FLAGS_AOR);					// Alloc the stack
	
	if (stack == 0) {
		ListFree(handle->deps);																												// Failed...
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		MemFree((UIntPtr)PsCurrentProcess->exec_path);
		PsExitProcess((UIntPtr)-1);
	}
	
	PFsNode file = FsOpenFile(PsCurrentProcess->exec_path);																					// Try to open the executable
	
	MemFree((UIntPtr)PsCurrentProcess->exec_path);																							// Free the executable path
	
	if (file == Null) {
		ListFree(handle->deps);																												// Failed to open it...
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	}
	
	PUInt8 buf = (PUInt8)MemAllocate(file->length);																							// Try to alloc the buffer to read it
	
	if (buf == Null) {
		FsCloseFile(file);																													// Failed
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	} else if (!FsReadFile(file, 0, file->length, buf)) {																					// Read it!
		MemFree((UIntPtr)buf);																												// Failed
		FsCloseFile(file);
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	}
	
	FsCloseFile(file);																														// Ok, now we can close the file!
	
	if (!ExecLoadInt(handle, (PELFHdr)buf)) {																								// Call the internal routine
		ListForeach(handle->symbols, i) {																									// Failed, let's free all the symbols
			MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
		}
		
		ListForeach(handle->deps, i) {																										// Close all the dependencies
			ExecCloseLibrary((PLibHandle)i->data);
		}
		
		MemFree((UIntPtr)buf);																												// And free everything else
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	}
	
	UIntPtr entry = handle->base + ((PELFHdr)buf)->entry;																					// Save the entry point
	
	MemFree((UIntPtr)buf);																													// Free the buffer
	ArchUserJump(entry, stack + 0x100000 - sizeof(UIntPtr));																				// Jump!
	MmFreeAlignedUserMemory(handle->base);																									// ... It returned, free the base
	
	ListForeach(handle->symbols, i) {																										// Free all the symbols
		MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
	}
	
	ListForeach(handle->deps, i) {																											// Close all the dependencies
		ExecCloseLibrary((PLibHandle)i->data);
	}
	
	ListFree(handle->deps);																													// Free the handle
	ListFree(handle->symbols);
	MmFreeUserMemory((UIntPtr)handle);
	VirtFreeAddress(stack, 0x100000);																										// Free the stack
	PsExitProcess((UIntPtr)-1);																												// And exit
}

PProcess ExecCreateProcess(PWChar path) {
	if (path == Null) {																														// Check if wew have a valid path
		return Null;
	}
	
	PWChar name = ExecGetName(path);																										// Try to extract our module name
	
	if (name == Null) {
		return Null;																														// Failed
	}
	
	PWChar pathh = StrDuplicate(path);																										// Let's duplicate the path
	
	if (pathh == Null) {
		MemFree((UIntPtr)name);
		return Null;
	}
	
	PFsNode file = FsOpenFile(path);																										// Let's check if the file exists
	
	if (file == Null) {
		MemFree((UIntPtr)pathh);																											// Nope, so we can't continue
		MemFree((UIntPtr)name);
		return Null;
	}
	
	FsCloseFile(file);																														// Close the file, we don't need it anymore
	
	PProcess proc = PsCreateProcess(name, (UIntPtr)ExecCreateProcessInt);																	// Create the process pointing to the ExecCreateProcessInt function
	
	MemFree((UIntPtr)name);																													// Free the name, we don't need it anymore
	
	if (proc == Null) {
		MemFree((UIntPtr)pathh);																											// Failed to create the process :(
		return Null;
	}
	
	proc->exec_path = pathh;
	
	return proc;
}
