// File author is Ítalo Lima Marconato Matias
//
// Created on November 16 of 2018, at 01:14 BRT
// Last edited on December 24 of 2019, at 13:09 BRT

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

static IntPtr ScAppendHandle(UInt8 type, PVoid data) {
	ListForeach(PsCurrentProcess->handles, i) {																												// Try to find an unused entry
		PHandle hndl = (PHandle)i->data;
		
		if (!hndl->used) {																																	// Found?
			hndl->type = type;																																// Yes :)
			hndl->data = data;
			return hndl->num;
		}
	}
	
	PHandle hndl = (PHandle)MemAllocate(sizeof(Handle));																									// Nope, alloc an new one
	
	if (hndl == Null) {																																		// Failed?
		ScFreeHandle(type, data);																															// Yes, close the handle data (if required) and return -1
		return -1;
	}
	
	hndl->type = type;
	hndl->data = data;
	hndl->num = PsCurrentProcess->last_handle_id;
	hndl->used = True;
	
	if (!ListAdd(PsCurrentProcess->handles, hndl)) {																										// Try to add this file to the process handle list!
		MemFree((UIntPtr)hndl);																																// Failed, free the handle data (if required) and return -1
		ScFreeHandle(type, data);
		return -1;
	}
	
	return PsCurrentProcess->last_handle_id++;
}

Void ScFreeHandle(UInt8 type, PVoid data) {
	if (type == HANDLE_TYPE_FILE) {																															// If it's a file, we need to close it
		FsCloseFile((PFsNode)data);
	} else if (type == HANDLE_TYPE_LOCK) {																													// If it's a lock, we just need to unlock and free it
		PsUnlock((PLock)data);
		MemFree((UIntPtr)data);
	} else if (type == HANDLE_TYPE_LIBRARY) {																												// If it's a library, we also just need to close it
		ExecCloseLibrary((PLibHandle)data);
	}
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

Void ScSysCloseHandle(IntPtr handle) {
	if (handle >= PsCurrentProcess->last_handle_id) {																										// Check if the handle is valid
		return;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Try to get the handle data
	
	if (hndl == Null) {
		return;
	}
	
	ScFreeHandle(hndl->type, hndl->data);																													// Free the data (if required)
	
	hndl->used = False;																																		// "Free" this entry
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

IntPtr ScPsCreateThread(UIntPtr entry) {
	UIntPtr stack = VirtAllocAddress(0, 0x100000, VIRT_PROT_READ | VIRT_PROT_WRITE | VIRT_FLAGS_HIGHEST | VIRT_FLAGS_AOR);									// Alloc the stack
	
	if (stack == 0) {
		return -1;
	}
	
	PThread th = PsCreateThread(entry, stack + 0x100000 - sizeof(UIntPtr), True);																			// Create a new user thread
	
	if (th == Null) {
		VirtFreeAddress(stack, 0x100000);																													// Failed
		return -1;
	}
	
	IntPtr handle = ScAppendHandle(HANDLE_TYPE_THREAD, th);																									// Try to create the handle
	
	if (handle == -1) {
		PsFreeContext(th->ctx);																																// Failed, free the thread context and the thread struct itself
		MemFree((UIntPtr)th);
		return -1;
	}
	
	PsAddThread(th);																																		// Add the thread!
	
	return handle;
}
	
IntPtr ScPsGetCurrentThread(Void) {
	return ScAppendHandle(HANDLE_TYPE_THREAD, PsCurrentThread);																								// Just return a handle to the current thread
}

IntPtr ScPsGetCurrentProcess(Void) {
	return ScAppendHandle(HANDLE_TYPE_PROCESS, PsCurrentProcess);																							// Just return a handle to the current process
}

Void ScPsSleep(UIntPtr ms) {
	PsSleep(ms);																																			// Just redirect
}

UIntPtr ScPsWait(IntPtr handle) {
	if (handle >= PsCurrentProcess->last_handle_id) {																										// Check if the handle is valid
		return 1;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Try to get the handle data
	
	if (hndl == Null) {
		return 1;
	} else if (hndl->type == HANDLE_TYPE_THREAD) {
		return PsWaitThread(((PThread)hndl->data)->id);																										// If it's a handle to a thread, we need to call PsWaitThread
	} else if (hndl->type == HANDLE_TYPE_PROCESS) {
		return PsWaitProcess(((PProcess)hndl->data)->id);																									// If it's a handle to a process, we need to call PsWaitProcess
	}
	
	return 1;																																				// None of the above, so let's just return 1
}

IntPtr ScPsCreateLock(Void) {
	PLock lock = (PLock)MemAllocate(sizeof(Lock));																											// Let's alloc our lock
	
	if (lock == Null) {
		return -1;
	}
	
	lock->locked = False;																																	// Clean it
	lock->owner = Null;
	
	return ScAppendHandle(HANDLE_TYPE_LOCK, lock);																											// And create the handle!
}

Void ScPsLock(IntPtr handle) {
	if (handle >= PsCurrentProcess->last_handle_id) {																										// Check if the handle is valid
		return;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Try to get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_LOCK) {
		return;
	}
	
	PsLock((PLock)hndl->data);																																// Now we just need to redirect
}

Void ScPsUnlock(IntPtr handle) {
	if (handle >= PsCurrentProcess->last_handle_id) {																										// Check if the handle is valid
		return;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Try to get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_LOCK) {
		return;
	}
	
	PsUnlock((PLock)hndl->data);																															// Now we just need to redirect
}

Void ScPsExitThread(UIntPtr ret) {
	PsExitThread(ret);																																		// Just redirect
}

Void ScPsExitProcess(UIntPtr ret) {
	PsExitProcess(ret);																																		// Just redirect
}

IntPtr ScFsOpenFile(PWChar path) {
	if (!ScCheckPointer(path)) {																															// Check if the pointer is inside of the userspace
		return -1;
	}
	
	PFsNode file = FsOpenFile(path);																														// Try to open the file
	
	if (file == Null) {
		return -1;																																			// Failed
	}
	
	return ScAppendHandle(HANDLE_TYPE_FILE, file);																											// Create the handle
}

Boolean ScFsReadFile(IntPtr handle, UIntPtr len, PUInt8 buf) {
	if ((handle >= PsCurrentProcess->last_handle_id) || (!ScCheckPointer(buf))) {																			// Sanity checks
		return False;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return False;
	}
	
	PFsNode pfile = (PFsNode)hndl->data;																													// Get the real file
	
	if (!FsReadFile(pfile, pfile->offset, len, buf)) {																										// Redirect
		return False;
	}
	
	pfile->offset += len;																																	// And increase the offset
	
	return True;
}

Boolean ScFsWriteFile(IntPtr handle, UIntPtr len, PUInt8 buf) {
	if ((handle >= PsCurrentProcess->last_handle_id) || (!ScCheckPointer(buf))) {																			// Sanity checks
		return False;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return False;
	}
	
	PFsNode pfile = (PFsNode)hndl->data;																													// Get the real file
	
	if (!FsWriteFile(pfile, pfile->offset, len, buf)) {																										// Redirect
		return False;
	}
	
	pfile->offset += len;																																	// And increase the offset
	
	return True;
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

Boolean ScFsReadDirectoryEntry(IntPtr handle, UIntPtr entry, PWChar out) {
	if ((handle >= PsCurrentProcess->last_handle_id) || (!ScCheckPointer(out))) {																			// Sanity checks
		return False;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return False;
	}
	
	PWChar name = FsReadDirectoryEntry((PFsNode)hndl->data, entry);																							// Use the internal function
	
	if (name == Null) {
		return False;																																		// Failed (probably this entry doesn't exists)
	}
	
	StrCopy(out, name);																																		// Copy the out pointer to the userspace
	MemFree((UIntPtr)name);																																	// Free the out pointer
	
	return True;
}

IntPtr ScFsFindInDirectory(IntPtr handle, PWChar name) {
	if ((handle >= PsCurrentProcess->last_handle_id) || (!ScCheckPointer(name))) {																			// Sanity checks
		return -1;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return False;
	}
	
	PFsNode file = FsFindInDirectory((PFsNode)hndl->data, name);																							// Use the internal function
	
	if (file == Null) {
		return -1;																																			// Failed (probably this file doesn't exists)
	}
	
	return ScAppendHandle(HANDLE_TYPE_FILE, file);																											// Create the handle
}

Boolean ScFsCreateFile(IntPtr handle, PWChar name, UIntPtr flags) {
	if ((handle >= PsCurrentProcess->last_handle_id) || (!ScCheckPointer(name))) {																			// Sanity checks
		return False;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return False;
	}
	
	return FsCreateFile((PFsNode)hndl->data, name, flags);																									// And redirect
}

Boolean ScFsControlFile(IntPtr handle, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf) {
	if ((handle >= PsCurrentProcess->last_handle_id) || (ibuf != Null && !ScCheckPointer(ibuf)) || (obuf != Null && !ScCheckPointer(obuf))) {				// Sanity checks
		return False;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return False;
	}
	
	return FsControlFile((PFsNode)hndl->data, cmd, ibuf, obuf);																								// And redirect
}

UIntPtr ScFsGetFileSize(IntPtr handle) {
	if (handle >= PsCurrentProcess->last_handle_id) {																										// Sanity check
		return 0;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return False;
	}
	
	PFsNode node = (PFsNode)hndl->data;																														// Get the real file
	
	if ((node->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																										// File?
		return 0;																																			// We can't read the length of an directory (sorry)
	}
	
	return node->length;
}

UIntPtr ScFsGetPosition(IntPtr handle) {
	if (handle >= PsCurrentProcess->last_handle_id) {																										// Sanity check
		return 0;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return False;
	}
	
	PFsNode node = (PFsNode)hndl->data;																														// Get the real file
	
	if ((node->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																										// File?
		return 0;																																			// We can't read the offset of an directory (sorry)
	}
	
	return node->offset;																																	// And return the offset
}

Void ScFsSetPosition(IntPtr handle, UIntPtr base, UIntPtr off) {
	if (handle >= PsCurrentProcess->last_handle_id) {																										// Sanity check
		return;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return;
	}
	
	PFsNode node = (PFsNode)hndl->data;																														// Get the real file
	
	if ((node->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																										// File?
		return;																																				// We can't read the offset of an directory (sorry)
	}
	
	if (base == 0) {																																		// Base = File Start
		node->offset = off;
	} else if (base == 1) {																																	// Base = Current Offset
		node->offset += off;
	} else if (base == 2) {																																	// Base = File End
		node->offset = node->length + off;
	}
}
	
IntPtr ScExecCreateProcess(PWChar path) {
	if (!ScCheckPointer(path)) {																															// Sanity check
		return 0;
	}
	
	PProcess proc = ExecCreateProcess(path);																												// Try to create the process
	
	if (proc == Null) {
		return 0;
	}
	
	IntPtr handle = ScAppendHandle(HANDLE_TYPE_PROCESS, proc);																								// Create the handle
	
	if (handle == -1) {
		while (proc->threads->length != 0) {																												// Failed... let's start by freeing all the threads
			PThread th = ListRemove(proc->threads, 0);
			PsFreeContext(th->ctx);
			MemFree((UIntPtr)th);
		}
		
		ListFree(proc->handles);																															// Now free the rest of the process struct
		ListFree(proc->threads);
		MmFreeDirectory(proc->dir);
		MemFree((UIntPtr)proc->name);
		MemFree((UIntPtr)proc);
		
		return 0;
	}
	
	PsAddProcess(proc);																																		// Run the process and return the handle
	
	return handle;
}

IntPtr ScExecLoadLibrary(PWChar path, Boolean global) {
	if (!ScCheckPointer(path)) {																															// Sanity check
		return -1;
	}
	
	PLibHandle lib = ExecLoadLibrary(path, global);																											// Try to load the library
	
	if (lib == Null) {
		return -1;
	}
	
	return ScAppendHandle(HANDLE_TYPE_LIBRARY, lib);																										// And create the handle
}

UIntPtr ScExecGetSymbol(IntPtr handle, PWChar name) {
	if (handle >= PsCurrentProcess->last_handle_id || !ScCheckPointer(name)) {																				// Sanity check
		return 0;
	}
	
	PHandle hndl = ListGet(PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_LIBRARY) {
		return 0;
	}
	
	return ExecGetSymbol((PLibHandle)hndl->data, name);																										// And redirect
}
