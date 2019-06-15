// File author is Ítalo Lima Marconato Matias
//
// Created on November 16 of 2018, at 21:03 BRT
// Last edited on April 27 of 2019, at 16:23 BRT

#include <chicago/alloc.h>
#include <chicago/arch.h>
#include <chicago/chexec.h>
#include <chicago/exec.h>
#include <chicago/mm.h>
#include <chicago/string.h>
#include <chicago/virt.h>

static PWChar ExecGetName(PWChar path) {
	if (path == Null) {																								// Sanity check
		return Null;
	}
	
	PWChar last = Null;
	PWChar dup = StrDuplicate(path);
	PWChar tok = StrTokenize(dup, L"\\");
	
	while (tok != Null) {																							// Let's go!
		if (last != Null) {																							// Free the old last?
			MemFree((UIntPtr)last);																					// Yes
		}
		
		last = StrDuplicate(tok);																					// Duplicate the token
		
		if (last == Null) {
			MemFree((UIntPtr)dup);																					// Failed...
			return Null;
		}
		
		tok = StrTokenize(Null, L"\\");																				// Tokenize next
	}
	
	MemFree((UIntPtr)dup);
	
	return last;
}

static Boolean ExecLoadDependencies(PUInt8 buf) {
	if (buf == Null) {																								// Sanity check
		return False;
	}
	
	PCHExecHeader hdr = (PCHExecHeader)buf;
	PCHExecDependency dep = (PCHExecDependency)(((UIntPtr)buf) + hdr->dep_start);
	
	for (UIntPtr i = 0; i < hdr->dep_count; i++) {																	// Let's do it!
		if (ExecLoadLibrary(dep->name, True) == Null) {																// Try to load this library
			return False;																							// Failed...
		}
		
		dep = (PCHExecDependency)(((UIntPtr)dep) + sizeof(CHExecDependency) + (dep->name_len * sizeof(WChar)));
	}
	
	return True;
}

static UIntPtr ExecGetSymbolLoc(UIntPtr base, PUInt8 buf, PWChar name) {
	if ((buf == Null) || (name == Null)) {																			// Sanity checks
		return False;
	}
	
	PCHExecHeader hdr = (PCHExecHeader)buf;
	PCHExecSymbol sym = (PCHExecSymbol)(((UIntPtr)buf) + hdr->st_start);
	
	for (UIntPtr i = 0; i < hdr->st_count; i++) {																	// Let's do it!
		if (StrGetLength(sym->name) != StrGetLength(name)) {														// Same length?
			continue;																								// No...
		} else if (StrCompare(sym->name, name)) {
			return base + sym->virt;																				// Found!
		}
		
		sym = (PCHExecSymbol)(((UIntPtr)sym) + sizeof(CHExecSymbol) + (sym->name_len * sizeof(WChar)));
	}
	
	return 0;
}

static Boolean ExecRelocate(UIntPtr base, PUInt8 buf) {
	if ((base == 0) || (buf == Null)) {																				// Sanity checks
		return False;
	}
	
	PCHExecHeader hdr = (PCHExecHeader)buf;
	PCHExecRelocation rel = (PCHExecRelocation)(((UIntPtr)buf) + hdr->rel_start);
	
	for (UIntPtr i = 0; i < hdr->rel_count; i++) {																	// Let's do it...
		UIntPtr incr = rel->incr;
		
		if ((rel->op & CHEXEC_REL_OP_REL) == CHEXEC_REL_OP_REL) {													// Relative
			incr -= rel->virt;
		} else if ((rel->op & CHEXEC_REL_OP_SYM) == CHEXEC_REL_OP_SYM) {											// Symbol
			UIntPtr sym = ExecGetSymbolLoc(base, buf, rel->name);													// Try to get the symbol
			
			if (sym < base) {
				return False;																						// Failed
			}
			
			incr += sym;
		} else if ((rel->op & CHEXEC_REL_OP_REL_SYM) == CHEXEC_REL_OP_REL_SYM) {									// Relative symbol
			UIntPtr sym = ExecGetSymbolLoc(base, buf, rel->name);													// Try to get the symbol
			
			if (sym < base) {
				return False;																						// Failed
			}
			
			incr += sym - (base + rel->virt);
		} else {
			incr += base;																							// Absolute
		}
		
		if ((rel->op & CHEXEC_REL_OP_BYTE) == CHEXEC_REL_OP_BYTE) {													// Byte
			*((PUInt8)(base + rel->virt)) += (UInt8)incr;
		} else if ((rel->op & CHEXEC_REL_OP_WORD) == CHEXEC_REL_OP_WORD) {											// Word
			*((PUInt16)(base + rel->virt)) += (UInt16)incr;
		} else if ((rel->op & CHEXEC_REL_OP_DWORD) == CHEXEC_REL_OP_DWORD) {										// DWord
			*((PUInt32)(base + rel->virt)) += (UInt32)incr;
		}
		
		rel = (PCHExecRelocation)(((UIntPtr)rel) + sizeof(CHExecRelocation) + (rel->name_len * sizeof(WChar)));
	}
	
	return True;
}

static Void ExecCreateProcessInt(Void) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (PsCurrentProcess->exec_path == Null)) {			// Sanity checks
		PsExitProcess(EXEC_INVALID_CHEXEC);
	}
	
	UIntPtr stack = VirtAllocAddress(0, 0x8000, VIRT_PROT_READ | VIRT_PROT_WRITE | VIRT_FLAGS_HIGHEST);				// Alloc the stack
	
	if (stack == 0) {
		MemFree((UIntPtr)PsCurrentProcess->exec_path);																// Failed...
		PsExitProcess(EXEC_INVALID_CHEXEC);
	}
	
	PFsNode file = FsOpenFile(PsCurrentProcess->exec_path);															// Try to open the executable
	
	MemFree((UIntPtr)PsCurrentProcess->exec_path);																	// Free the executable path
	
	if (file == Null) {
		VirtFreeAddress(stack, 0x8000);																				// Failed to open it
		PsExitProcess(EXEC_INVALID_CHEXEC);
	}
	
	PUInt8 buf = (PUInt8)MemAllocate(file->length);																	// Try to alloc the buffer to read it
	
	if (buf == Null) {
		FsCloseFile(file);																							// Failed
		VirtFreeAddress(stack, 0x8000);
		PsExitProcess(EXEC_INVALID_CHEXEC);
	} else if (!FsReadFile(file, 0, file->length, buf)) {															// Read it!
		MemFree((UIntPtr)buf);																						// Failed...
		FsCloseFile(file);
		VirtFreeAddress(stack, 0x8000);
		PsExitProcess(EXEC_INVALID_CHEXEC);
	}
	
	FsCloseFile(file);																								// Ok, now we can close the file!
	
	if (!CHExecValidateHeader(buf, CHEXEC_HEADER_FLAGS_EXECUTABLE)) {												// Check if this is a valid CHExec executable
		MemFree((UIntPtr)buf);																						// ...
		VirtFreeAddress(stack, 0x8000);
		PsExitProcess(EXEC_INVALID_CHEXEC);
	}
	
	UIntPtr base = CHExecLoadSections(buf);																			// Load the sections into the memory
	UIntPtr entry = base + ((PCHExecHeader)buf)->entry;																// Calculate the base address
	
	if (base == 0) {
		MemFree((UIntPtr)buf);																						// Failed to load the sections...
		VirtFreeAddress(stack, 0x8000);
		PsExitProcess(EXEC_INVALID_CHEXEC);
	} else if (!ExecLoadDependencies(buf)) {																		// Load the dependencies
		MmFreeUserMemory(base);
		MemFree((UIntPtr)buf);
		VirtFreeAddress(stack, 0x8000);
		PsExitProcess(EXEC_INVALID_CHEXEC);
	} else if (!ExecRelocate(base, buf)) {																			// And relocate!
		MmFreeUserMemory(base);
		MemFree((UIntPtr)buf);
		VirtFreeAddress(stack, 0x8000);
		PsExitProcess(EXEC_INVALID_CHEXEC);
	}
	
	MemFree((UIntPtr)buf);																							// Free the buffer
	ArchUserJump(entry, stack + 0x8000);																			// Jump!
	MmFreeUserMemory(base);																							// If it returns (somehow), free the sections
	VirtFreeAddress(stack, 0x8000);																					// Free the stack
	PsExitProcess(0);																								// And exit
}

PProcess ExecCreateProcess(PWChar path) {
	if (path == Null) {																								// Check if wew have a valid path
		return Null;
	}
	
	PWChar name = ExecGetName(path);																				// Try to extract our module name
	
	if (name == Null) {
		return Null;																								// Failed
	}
	
	PWChar pathh = StrDuplicate(path);																				// Let's duplicate the path
	
	if (pathh == Null) {
		MemFree((UIntPtr)name);
		return Null;
	}
	
	PFsNode file = FsOpenFile(path);																				// Let's check if the file exists
	
	if (file == Null) {
		MemFree((UIntPtr)pathh);																					// Nope, so we can't continue
		MemFree((UIntPtr)name);
		return Null;
	}
	
	FsCloseFile(file);																								// Close the file, we don't need it anymore
	
	PProcess proc = PsCreateProcess(name, (UIntPtr)ExecCreateProcessInt);											// Create the process pointing to the ExecCreateProcessInt function
	
	MemFree((UIntPtr)name);																							// Free the name, we don't need it anymore
	
	if (proc == Null) {
		MemFree((UIntPtr)pathh);																					// Failed to create the process :(
		return Null;
	}
	
	proc->exec_path = pathh;
	
	return proc;
}
