// File author is Ítalo Lima Marconato Matias
//
// Created on November 01 of 2019, at 16:50 BRT
// Last edited on January 04 of 2020, at 17:49 BRT

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
	
	PWChar *argv = PsCurrentProcess->exec_argv;																								// Let's copy the argv to userspace
	
	PsCurrentProcess->exec_argv = (PWChar*)MmAllocUserMemory(sizeof(PWChar) * PsCurrentProcess->exec_argc);									// Alloc space for the unicode copy
	
	if (PsCurrentProcess->exec_argv == Null) {
		for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																			// Failed, free everything
			MemFree((UIntPtr)argv[i]);
		}
		
		MemFree((UIntPtr)argv);
		MemFree((UIntPtr)PsCurrentProcess->exec_path);
		PsExitProcess((UIntPtr)-1);
	} else {
		StrSetMemory(PsCurrentProcess->exec_argv, 0, sizeof(PWChar) * PsCurrentProcess->exec_argc);											// Zero the allocated space
	}
	
	PsCurrentProcess->exec_cargv = (PChar*)MmAllocUserMemory(sizeof(PChar) * PsCurrentProcess->exec_argc);									// Alloc space for the C copy
	
	if (PsCurrentProcess->exec_cargv == Null) {
		for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																			// Failed, free everything
			MemFree((UIntPtr)argv[i]);
		}
		
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv);
		MemFree((UIntPtr)argv);
		MemFree((UIntPtr)PsCurrentProcess->exec_path);
		PsExitProcess((UIntPtr)-1);
	} else {
		StrSetMemory(PsCurrentProcess->exec_cargv, 0, sizeof(PChar) * PsCurrentProcess->exec_argc);											// Zero the allocated space
	}
	
	for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																				// Finally, do both copies!
		UIntPtr len = StrGetLength(argv[i]);																								// Get the length of this argv entry
		
		PsCurrentProcess->exec_argv[i] = (PWChar)MmAllocUserMemory(sizeof(WChar) * (len + 1));												// Alloc the space for the unicode arg
		PsCurrentProcess->exec_cargv[i] = (PChar)MmAllocUserMemory(len + 1);																// And for the C arg
		
		if (PsCurrentProcess->exec_argv[i] == Null || PsCurrentProcess->exec_cargv[i] == Null) {
			for (UIntPtr j = 0; j < PsCurrentProcess->exec_argc; j++) {																		// Failed, free everything
				MemFree((UIntPtr)argv[j]);
				
				if (PsCurrentProcess->exec_cargv[j] != Null) {
					MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv[j]);
				}
				
				if (PsCurrentProcess->exec_argv[j] != Null) {
					MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv[j]);
				}
			}
			
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv);
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv);
			MemFree((UIntPtr)argv);
			MemFree((UIntPtr)PsCurrentProcess->exec_path);
			PsExitProcess((UIntPtr)-1);
		}
		
		StrCopy(PsCurrentProcess->exec_argv[i], argv[i]);																					// Copy the unicode arg
		StrCFromUnicode(PsCurrentProcess->exec_cargv[i], argv[i], len);																		// And the C arg
	}
	
	for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																				// Ok, now we can free the saved argv, as we already copied it
		MemFree((UIntPtr)argv[i]);
	}
	
	MemFree((UIntPtr)argv);
	
	PExecHandle handle = (PExecHandle)MmAllocUserMemory(sizeof(ExecHandle));																// Alloc space for the exec handle
	
	if (handle == Null) {
		for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																			// Failed...
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv[i]);
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv[i]);
		}
		
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv);
		MemFree((UIntPtr)PsCurrentProcess->exec_path);
		PsExitProcess((UIntPtr)-1);
	}
	
	handle->symbols = ListNew(True, True);																									// Alloc the symbol list
	
	if (handle->symbols == Null) {
		for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																			// Failed...
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv[i]);
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv[i]);
		}
		
		MmFreeUserMemory((UIntPtr)handle);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv);
		MemFree((UIntPtr)PsCurrentProcess->exec_path);
		PsExitProcess((UIntPtr)-1);
	}
	
	handle->deps = ListNew(True, True);																										// Alloc the dependency list
	
	if (handle->deps == Null) {
		for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																			// Failed...
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv[i]);
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv[i]);
		}
		
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv);
		MemFree((UIntPtr)PsCurrentProcess->exec_path);
		PsExitProcess((UIntPtr)-1);
	}
	
	UIntPtr stack = VirtAllocAddress(0, 0x100000, VIRT_PROT_READ | VIRT_PROT_WRITE | VIRT_FLAGS_HIGHEST | VIRT_FLAGS_AOR);					// Alloc the stack
	
	if (stack == 0) {
		for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																			// Failed...
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv[i]);
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv[i]);
		}
		
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv);
		MemFree((UIntPtr)PsCurrentProcess->exec_path);
		PsExitProcess((UIntPtr)-1);
	}
	
	PFsNode file = FsOpenFile(PsCurrentProcess->exec_path);																					// Try to open the executable
	
	MemFree((UIntPtr)PsCurrentProcess->exec_path);																							// Free the executable path
	
	if (file == Null) {
		for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																			// Failed to open it...
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv[i]);
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv[i]);
		}
		
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv);
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	}
	
	PUInt8 buf = (PUInt8)MemAllocate(file->length);																							// Try to alloc the buffer to read it
	
	if (buf == Null) {
		for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																			// Failed...
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv[i]);
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv[i]);
		}
		
		FsCloseFile(file);
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv);
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	} else if (FsReadFile(file, 0, file->length, buf) != file->length) {																	// Read it!
		for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																			// Failed...
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv[i]);
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv[i]);
		}
		
		MemFree((UIntPtr)buf);
		FsCloseFile(file);
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv);
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
		
		for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv[i]);
			MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv[i]);
		}
		
		MemFree((UIntPtr)buf);																												// And free everything else
		ListFree(handle->deps);
		ListFree(handle->symbols);
		MmFreeUserMemory((UIntPtr)handle);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv);
		VirtFreeAddress(stack, 0x100000);
		PsExitProcess((UIntPtr)-1);
	}
	
	UIntPtr entry = handle->base + ((PELFHdr)buf)->entry;																					// Save the entry point
	UIntPtr argc = PsCurrentProcess->exec_argc;
	
	MemFree((UIntPtr)buf);																													// Free the buffer
	ArchUserJump(entry, stack + 0x100000 - sizeof(UIntPtr), argc, PsCurrentProcess->exec_argv, PsCurrentProcess->exec_cargv);				// Jump!
	MmFreeAlignedUserMemory(handle->base);																									// ... It returned, free the base
	
	ListForeach(handle->symbols, i) {																										// Free all the symbols
		MmFreeUserMemory((UIntPtr)(((PExecSymbol)i->data)->name));
	}
	
	ListForeach(handle->deps, i) {																											// Close all the dependencies
		ExecCloseLibrary((PLibHandle)i->data);
	}
	
	for (UIntPtr i = 0; i < PsCurrentProcess->exec_argc; i++) {																				// Free the allocated argv copies
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv[i]);
		MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv[i]);
	}
	
	ListFree(handle->deps);																													// Free the handle
	ListFree(handle->symbols);
	MmFreeUserMemory((UIntPtr)handle);
	MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_cargv);																				// Free the... allocated argv copies...
	MmFreeUserMemory((UIntPtr)PsCurrentProcess->exec_argv);
	VirtFreeAddress(stack, 0x100000);																										// Free the stack
	PsExitProcess((UIntPtr)-1);																												// And exit
}

PProcess ExecCreateProcess(PWChar path, UIntPtr argc, PWChar *argv) {
	if (path == Null || (argc != 0 && argv == Null)) {																						// Check if everything is valid
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
	
	PWChar *argvv = (PWChar*)MemZAllocate(sizeof(PWChar) * (argc + 1));																		// Alloc space for duplicating the arguments
	
	if (argvv == Null) {
		MemFree((UIntPtr)pathh);																											// Failed
		MemFree((UIntPtr)name);
		return Null;
	}
	
	argvv[0] = StrDuplicate(path);																											// First arg is the path
	
	if (argvv[0] == Null) {
		MemFree((UIntPtr)argvv);																											// Failed to copy it...
		MemFree((UIntPtr)pathh);
		MemFree((UIntPtr)name);
		return Null;
	}
	
	for (UIntPtr i = 1; i < argc; i++) {																									// Now copy all the arguments
		argvv[i] = StrDuplicate(argv[i]);
		
		if (argvv[i] == Null) {
			for (UIntPtr j = 0; j < argc; j++) {																							// Failed, free everything
				if (argvv[j] != Null) {
					MemFree((UIntPtr)argvv[j]);
				}
			}
			
			MemFree((UIntPtr)argvv);
			MemFree((UIntPtr)pathh);
			MemFree((UIntPtr)name);
			
			return Null;
		}
	}
	
	PFsNode file = FsOpenFile(path);																										// Let's check if the file exists
	
	if (file == Null) {
		for (UIntPtr i = 0; i < argc; i++) {																								// Nope, so we can't continue
			MemFree((UIntPtr)argvv[i]);
		}
		
		MemFree((UIntPtr)argvv);
		MemFree((UIntPtr)pathh);
		MemFree((UIntPtr)name);
		
		return Null;
	}
	
	FsCloseFile(file);																														// Close the file, we don't need it anymore
	
	PProcess proc = PsCreateProcess(name, (UIntPtr)ExecCreateProcessInt);																	// Create the process pointing to the ExecCreateProcessInt function
	
	MemFree((UIntPtr)name);																													// Free the name, we don't need it anymore
	
	if (proc == Null) {
		for (UIntPtr i = 0; i < argc; i++) {																								// Failed to create the process :(
			MemFree((UIntPtr)argvv[i]);
		}
		
		MemFree((UIntPtr)argvv);
		MemFree((UIntPtr)pathh);
		
		return Null;
	}
	
	proc->exec_path = pathh;
	proc->exec_argc = argc + 1;
	proc->exec_argv = argvv;
	
	return proc;
}
