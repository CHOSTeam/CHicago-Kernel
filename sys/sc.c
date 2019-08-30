// File author is Ítalo Lima Marconato Matias
//
// Created on November 16 of 2018, at 01:14 BRT
// Last edited on August 30 of 2019, at 14:03 BRT

#include <chicago/alloc.h>
#include <chicago/mm.h>
#include <chicago/sc.h>
#include <chicago/string.h>
#include <chicago/version.h>
#include <chicago/virt.h>

static Boolean ScCheckPointer(PVoid ptr) {
#if (MM_USER_START == 0)																																	// Let's fix an compiler warning :)
	if ((ptr == Null) || (((UIntPtr)ptr) >= MM_USER_END)) {																									// Check if the pointer is inside of the userspace!
#else
	if ((ptr == Null) || ((UIntPtr)ptr) < MM_USER_START || ((UIntPtr)ptr) >= MM_USER_END) {																	// Same as above
#endif
		return False;
	} else {
		return True;	
	}
}

static IntPtr ScAppendProcessFile(PFsNode file) {
	if (file == Null) {																																		// Valid file?
		return -1;																																			// No...
	}
	
	ListForeach(PsCurrentProcess->files, i) {																												// Try to find an unused entry
		PProcessFile pf = (PProcessFile)i->data;
		
		if (pf->file == Null) {																																// Found?
			pf->file = file;																																// Yes :)
			return pf->num;
		}
	}
	
	PProcessFile pf = (PProcessFile)MemAllocate(sizeof(ProcessFile));																						// Nope, alloc an new one
	
	if (pf == Null) {																																		// Failed?
		FsCloseFile(file);																																	// Yes, close the file and return -1
		return -1;
	}
	
	pf->file = file;
	pf->num = PsCurrentProcess->last_fid++;
	
	if (!ListAdd(PsCurrentProcess->files, pf)) {																											// Try to add this file to the process file list!
		MemFree((UIntPtr)pf);																																// Failed, close the file and return -1
		FsCloseFile(file);
		return -1;
	}
	
	return pf->num;
}

Void ScSysGetVersion(PSystemVersion ver) {
	if (!ScCheckPointer(ver)) {																																// Check if the pointer is inside of the userspace
		return;
	}
	
	if (ScCheckPointer(ver->major)) {																														// And let's do it!
		*ver->major = CHICAGO_MAJOR;
	}
	
	if (ScCheckPointer(ver->minor)) {
		*ver->minor = CHICAGO_MINOR;
	}
	
	if (ScCheckPointer(ver->build)) {
		*ver->build = CHICAGO_BUILD;
	}
	
	if (ScCheckPointer(ver->codename)) {
		StrCopy(ver->codename, CHICAGO_CODENAME);
	}
	
	if (ScCheckPointer(ver->arch)) {
		StrCopy(ver->arch, CHICAGO_ARCH);
	}
}

UIntPtr ScMmAllocMemory(UIntPtr size) {
	return MmAllocUserMemory(size);																															// Just redirect
}

Void ScMmFreeMemory(UIntPtr block) {
	return MmFreeUserMemory(block);																															// Just redirect
}

UIntPtr ScMmReallocMemory(UIntPtr block, UIntPtr size) {
	return MmReallocUserMemory(block, size);																												// Just redirect
}

UIntPtr ScMmGetUsage(Void) {
	return MmGetUsage();																																	// Just redirect
}

UIntPtr ScVirtAllocAddress(UIntPtr addr, UIntPtr size, UInt32 flags) {
	return VirtAllocAddress(addr, size, flags);																												// Just redirect
}

Boolean ScVirtFreeAddress(UIntPtr addr, UIntPtr size) {
	return VirtFreeAddress(addr, size);																														// Just redirect
}

UInt32 ScVirtQueryProtection(UIntPtr addr) {
	return VirtQueryProtection(addr);																														// Just redirect
}

Boolean ScVirtChangeProtection(UIntPtr addr, UIntPtr size, UInt32 flags) {
	return VirtChangeProtection(addr, size, flags);																											// Just redirect
}

UIntPtr ScVirtGetUsage(Void) {
	return VirtGetUsage();																																	// Just redirect
}

UIntPtr ScPsCreateThread(UIntPtr entry) {
	UIntPtr stack = VirtAllocAddress(0, 0x8000, VIRT_PROT_READ | VIRT_PROT_WRITE | VIRT_FLAGS_HIGHEST);														// Alloc the stack
	
	if (stack == 0) {
		return 0;
	}
	
	PThread th = PsCreateThread(entry, stack, True);																										// Create a new thread
	
	if (th == Null) {
		VirtFreeAddress(stack, 0x8000);																														// Failed
		return 0;
	}
	
	PsAddThread(th);																																		// Try to add it
	
	return th->id;
}

UIntPtr ScPsGetTID(Void) {
	return (PsCurrentThread == Null) ? 0 : PsCurrentThread->id;																								// Return the ID of the current thread
}

UIntPtr ScPsGetPID(Void) {
	return ((PsCurrentThread == Null) || (PsCurrentProcess == Null)) ? 0 : PsCurrentProcess->id;															// Return the ID of the current process
}

Void ScPsSleep(UIntPtr ms) {
	PsSleep(ms);																																			// Just redirect
}

UIntPtr ScPsWaitThread(UIntPtr id) {
	return PsWaitThread(id);																																// Just redirect
}

UIntPtr ScPsWaitProcess(UIntPtr id) {
	return PsWaitProcess(id);																																// Just redirect
}

Void ScPsLock(PLock lock) {
	if (!ScCheckPointer(lock)) {																															// Check if the pointer is inside of the userspace
		return;
	}
	
	PsLock(lock);																																			// Just redirect
}

Void ScPsUnlock(PLock lock) {
	if (!ScCheckPointer(lock)) {																															// Check if the pointer is inside of the userspace
		return;
	}
	
	PsUnlock(lock);																																			// Just redirect
}

Void ScPsExitThread(UIntPtr ret) {
	PsExitThread(ret);																																		// Just redirect
}

Void ScPsExitProcess(UIntPtr ret) {
	PsExitProcess(ret);																																		// Just redirect
}

Void ScPsForceSwitch(Void) {
	PsSwitchTask(Null);																																		// Just redirect
}

IntPtr ScFsOpenFile(PWChar path) {
	if (!ScCheckPointer(path)) {																															// Check if the pointer is inside of the userspace
		return -1;
	}
	
	PFsNode file = FsOpenFile(path);																														// Try to open the file
	
	if (file == Null) {
		return -1;																																			// Failed
	}
	
	return ScAppendProcessFile(file);																														// Append the file to the process file list
}

Void ScFsCloseFile(IntPtr file) {
	if (file >= PsCurrentProcess->last_fid) {																												// Check if the pointer is inside of the userspace
		return;
	}
	
	PProcessFile pf = ListGet(PsCurrentProcess->files, file);																								// Try to get the file
	
	if (pf == Null) {
		return;
	}
	
	FsCloseFile(pf->file);																																	// Close it
	
	pf->file = Null;																																		// "Free" this entry
}

Boolean ScFsReadFile(IntPtr file, UIntPtr len, PUInt8 buf) {
	if ((file >= PsCurrentProcess->last_fid) || (!ScCheckPointer(buf))) {																					// Sanity checks
		return False;
	} else {
		PFsNode pfile = ((PProcessFile)(ListGet(PsCurrentProcess->files, file)))->file;																		// Get the real file
		
		if (!FsReadFile(pfile, pfile->offset, len, buf)) {																									// Redirect
			return False;
		}
		
		pfile->offset += len;																																// And increase the offset
		
		return True;
	}
}

Boolean ScFsWriteFile(IntPtr file, UIntPtr len, PUInt8 buf) {
	if ((file >= PsCurrentProcess->last_fid) || (!ScCheckPointer(buf))) {																					// Sanity checks
		return False;
	} else {
		PFsNode pfile = ((PProcessFile)(ListGet(PsCurrentProcess->files, file)))->file;																		// Get the real file
		
		if (!FsWriteFile(pfile, pfile->offset, len, buf)) {																									// Redirect
			return False;
		}
		
		pfile->offset += len;																																// And increase the offset
		
		return True;
	}
}

Boolean ScFsMountFile(PWChar path, PWChar file, PWChar type) {
	if ((!ScCheckPointer(path)) || (!ScCheckPointer(file)) || (!ScCheckPointer(type))) {																	// Sanity checks
		return False;	
	} else {
		return FsMountFile(path, file, type);																												// And redirect
	}
}

Boolean ScFsUmountFile(PWChar path) {
	if (!ScCheckPointer(path)) {																															// Sanity checks
		return False;
	} else {
		return FsUmountFile(path);																															// And redirect
	}
}

Boolean ScFsReadDirectoryEntry(IntPtr dir, UIntPtr entry, PWChar out) {
	if ((dir >= PsCurrentProcess->last_fid) || (!ScCheckPointer(out))) {																					// Sanity checks
		return False;
	}
	
	PWChar name = FsReadDirectoryEntry(((PProcessFile)(ListGet(PsCurrentProcess->files, dir)))->file, entry);												// Use the internal function
	
	if (name == Null) {
		return False;																																		// Failed (probably this entry doesn't exists)
	}
	
	StrCopy(out, name);																																		// Copy the out pointer to the userspace
	MemFree((UIntPtr)name);																																	// Free the out pointer
	
	return True;
}

IntPtr ScFsFindInDirectory(IntPtr dir, PWChar name) {
	if ((dir >= PsCurrentProcess->last_fid) || (!ScCheckPointer(name))) {																					// Sanity checks
		return -1;
	}
	
	PFsNode file = FsFindInDirectory(((PProcessFile)(ListGet(PsCurrentProcess->files, dir)))->file, name);													// Use the internal function
	
	if (file == Null) {
		return -1;																																			// Failed (probably this file doesn't exists)
	}
	
	return ScAppendProcessFile(file);																														// Append the file to the process file list
}

Boolean ScFsCreateFile(IntPtr dir, PWChar name, UIntPtr flags) {
	if ((dir >= PsCurrentProcess->last_fid) || (!ScCheckPointer(name))) {																					// Sanity checks
		return False;
	} else {
		return FsCreateFile(((PProcessFile)(ListGet(PsCurrentProcess->files, dir)))->file, name, flags);													// Amd redirect
	}
}

Boolean ScFsControlFile(IntPtr file, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf) {
	if ((file >= PsCurrentProcess->last_fid) || (!ScCheckPointer(ibuf)) || (!ScCheckPointer(obuf))) {														// Sanity checks
		return False;
	} else {
		return FsControlFile(((PProcessFile)(ListGet(PsCurrentProcess->files, file)))->file, cmd, ibuf, obuf);												// And redirect
	}
}

UIntPtr ScFsGetFileSize(IntPtr file) {
	if (file >= PsCurrentProcess->last_fid) {																												// Sanity check
		return 0;
	}
	
	PFsNode node = ((PProcessFile)(ListGet(PsCurrentProcess->files, file)))->file;																			// Try to get the file
	
	if (node == Null) {																																		// Valid?
		return 0;																																			// Nope
	} else if ((node->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																								// File?
		return 0;																																			// We can't read the length of an directory (sorry)
	}
	
	return node->length;
}

UIntPtr ScFsGetPosition(IntPtr file) {
	if (file >= PsCurrentProcess->last_fid) {																												// Sanity check
		return 0;
	} else {
		PFsNode node = ((PProcessFile)(ListGet(PsCurrentProcess->files, file)))->file;																		// Try to get the file
		
		if (node == Null) {																																	// Valid?
			return 0;																																		// Nope
		} else if ((node->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																							// File?
			return 0;																																		// We can't read the offset of an directory (sorry)
		}
		
		return node->offset;																																// And return the offset
	}
}

Void ScFsSetPosition(IntPtr file, UIntPtr base, UIntPtr off) {
	if (file >= PsCurrentProcess->last_fid) {																												// Sanity check
		return;
	} else {
		PFsNode node = ((PProcessFile)(ListGet(PsCurrentProcess->files, file)))->file;																		// Try to get the file
		
		if (node == Null) {																																	// Valid?
			return;																																			// Nope
		} else if ((node->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																							// File?
			return;																																			// We can't read the offset of an directory (sorry)
		}
		
		if (base == 0) {																																	// Base = File Start
			node->offset = off;
		} else if (base == 1) {																																// Base = Current Offset
			node->offset += off;
		} else if (base == 2) {																																// Base = File End
			node->offset = node->length + off;
		}
	}
}
	
Boolean ScIpcCreatePort(PWChar name) {
	if (!ScCheckPointer(name)) {																															// Sanity checks
		return False;
	} else {
		return IpcCreatePort(name) != Null;																													// Just redirect
	}
}

Void ScIpcRemovePort(PWChar name) {
	if (ScCheckPointer(name)) {																																// Sanity checks
		IpcRemovePort(name);																																// Just redirect
	}
}

Void ScIpcSendMessage(PWChar port, UInt32 msg, UIntPtr size, PUInt8 buf) {
	if (ScCheckPointer(port) && ScCheckPointer(buf)) {																										// Sanity checks
		IpcSendMessage(port, msg, size, buf);																												// Just redirect
	}
}

PIpcMessage ScIpcReceiveMessage(PWChar name) {
	if (!ScCheckPointer(name)) {																															// Sanity checks
		return Null;
	} else {
		return IpcReceiveMessage(name);																														// Just redirect
	}
}
