// File author is Ítalo Lima Marconato Matias
//
// Created on November 01 of 2019, at 16:50 BRT
// Last edited on November 06 of 2019, at 16:32 BRT

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
	PWChar tok = StrTokenize(dup, L"\\");
	
	while (tok != Null) {																													// Let's go!
		if (last != Null) {																													// Free the old last?
			MemFree((UIntPtr)last);																											// Yes
		}
		
		last = StrDuplicate(tok);																											// Duplicate the token
		
		if (last == Null) {
			MemFree((UIntPtr)dup);																											// Failed...
			return Null;
		}
		
		tok = StrTokenize(Null, L"\\");																										// Tokenize next
	}
	
	MemFree((UIntPtr)dup);
	
	return last;
}

static Void ExecCreateProcessInt(Void) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (PsCurrentProcess->exec_path == Null)) {									// Sanity checks
		PsExitProcess((UIntPtr)-1);
	}
	
	UIntPtr stack = VirtAllocAddress(0, 0x100000, VIRT_PROT_READ | VIRT_PROT_WRITE | VIRT_FLAGS_HIGHEST | VIRT_FLAGS_AOR);					// Alloc the stack
	
	if (stack == 0) {
		MemFree((UIntPtr)PsCurrentProcess->exec_path);																						// Failed...
		PsExitProcess((UIntPtr)-1);
	}
	
	PFsNode file = FsOpenFile(PsCurrentProcess->exec_path);																					// Try to open the executable
	
	MemFree((UIntPtr)PsCurrentProcess->exec_path);																							// Free the executable path
	
	if (file == Null) {
		VirtFreeAddress(stack, 0x100000);																									// Failed to open it
		PsExitProcess((UIntPtr)-1);
	}
	
	PUInt8 buf = (PUInt8)MemAllocate(file->length);																							// Try to alloc the buffer to read it
	
	if (buf == Null) {
		FsCloseFile(file);																													// Failed
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	} else if (!FsReadFile(file, 0, file->length, buf)) {																					// Read it!
		MemFree((UIntPtr)buf);																												// Failed...
		FsCloseFile(file);
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	}
	
	FsCloseFile(file);																														// Ok, now we can close the file!
	
	if (!ELFCheck((PELFHdr)buf)) {																											// Check if this is a valid ELF executable
		MemFree((UIntPtr)buf);																												// ...
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	}
	
	UIntPtr base = ELFLoadSections((PELFHdr)buf);																							// Load the sections into the memory
	UIntPtr entry = base + ((PELFHdr)buf)->entry;																							// Calculate the entry address
		
	if (base == 0) {
		MemFree((UIntPtr)buf);																												// Failed to load the sections...
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	} else if (!ELFLoadDeps((PELFHdr)buf, Null)) {																							// Load the dependencies
		MmFreeUserMemory(base);
		MemFree((UIntPtr)buf);
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	} else if (!ELFRelocate((PELFHdr)buf, Null, base)) {																					// And relocate!
		MmFreeUserMemory(base);
		MemFree((UIntPtr)buf);
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	}
	
	MemFree((UIntPtr)buf);																													// Free the buffer
	ArchUserJump(entry, stack + 0x100000);																									// Jump!
	MmFreeUserMemory(base);																													// If it returns (somehow), free the sections
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
