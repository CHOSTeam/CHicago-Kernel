// File author is Ítalo Lima Marconato Matias
//
// Created on November 16 of 2018, at 01:14 BRT
// Last edited on February 02 of 2020, at 12:00 BRT

#include <chicago/alloc.h>
#include <chicago/console.h>
#include <chicago/sc.h>
#include <chicago/shm.h>
#include <chicago/string.h>
#include <chicago/version.h>

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

Void ScFreeHandle(UInt8 type, PVoid data) {
	if (type == HANDLE_TYPE_FILE) {																															// If it's a file, we need to close it
		FsCloseFile((PFsNode)data);
	} else if (type == HANDLE_TYPE_IPC_RESPONSE_PORT) {																										// If it's a IPC response port, we just need to free it
		IpcFreeResponsePort((PIpcResponsePort)data);
	}
}

IntPtr ScAppendHandle(UInt8 type, PVoid data) {
	ListForeach(&PsCurrentProcess->handles, i) {																											// Try to find an unused entry
		PHandle hndl = (PHandle)i->data;
		
		if (!hndl->used) {																																	// Found?
			hndl->type = type;																																// Yes :)
			hndl->data = data;
			hndl->used = True;
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
	
	if (!ListAdd(&PsCurrentProcess->handles, hndl)) {																										// Try to add this file to the process handle list!
		MemFree((UIntPtr)hndl);																																// Failed, free the handle data (if required) and return -1
		ScFreeHandle(type, data);
		return -1;
	}
	
	return PsCurrentProcess->last_handle_id++;
}

Status ScSysGetVersion(PSystemVersion ver) {
	if (!ScCheckPointer(ver)) {																																// Check if the pointer is inside of the userspace
		return STATUS_INVALID_ARG;
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
	
	return STATUS_SUCCESS;
}

Status ScSysCloseHandle(IntPtr handle) {
	if (handle >= PsCurrentProcess->last_handle_id) {																										// Check if the handle is valid
		return STATUS_INVALID_ARG;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Try to get the handle data
	
	if (hndl == Null) {
		return STATUS_WRONG_HANDLE;
	}
	
	ScFreeHandle(hndl->type, hndl->data);																													// Free the data (if required)
	
	hndl->used = False;																																		// "Free" this entry
	
	return STATUS_SUCCESS;
}

Status ScPsCreateThread(UIntPtr entry, PIntPtr ret) {
	if (!ScCheckPointer(ret)) {																																// Sanity check
		return STATUS_INVALID_ARG;	
	}
	
	UIntPtr stack;
	Status status = MmAllocAddress(Null, 0x100000, MM_FLAGS_READ | MM_FLAGS_WRITE | MM_FLAGS_HIGHEST, &stack);												// Alloc the stack
	
	if (status != STATUS_SUCCESS) {
		return status;
	}
	
	PThread th;
	status = PsCreateThread(PS_PRIORITY_NORMAL, entry, stack + 0x100000 - sizeof(UIntPtr), True, &th);														// Create a new user thread
	
	if (status != STATUS_SUCCESS) {
		MmUnmapMemory(stack);																																// Failed
		return status;
	}
	
	IntPtr handle = ScAppendHandle(HANDLE_TYPE_THREAD, th);																									// Try to create the handle
	
	if (handle == -1) {
		PsFreeContext(th->ctx);																																// Failed, free the thread context and the thread struct itself
		MemFree((UIntPtr)th);
		return STATUS_OUT_OF_MEMORY;
	}
	
	PsAddThread(th);																																		// Add the thread!
	
	*ret = handle;
	
	return STATUS_SUCCESS;
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

Status ScPsWait(IntPtr handle, PUIntPtr ret) {
	if (handle >= PsCurrentProcess->last_handle_id || (!ScCheckPointer(ret))) {																				// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Try to get the handle data
	
	if (hndl == Null) {
		return STATUS_WRONG_HANDLE;
	} else if (hndl->type == HANDLE_TYPE_THREAD) {
		return PsWaitThread(((PThread)hndl->data)->id, ret);																								// If it's a handle to a thread, we need to call PsWaitThread
	} else if (hndl->type == HANDLE_TYPE_PROCESS) {
		return PsWaitProcess(((PProcess)hndl->data)->id, ret);																								// If it's a handle to a process, we need to call PsWaitProcess
	}
	
	return STATUS_WRONG_HANDLE;																																// None of the above, so this is not a process nor a thread...
}

Void ScPsExitThread(UIntPtr ret) {
	PsExitThread(ret);																																		// Just redirect
}

Void ScPsExitProcess(UIntPtr ret) {
	PsExitProcess(ret);																																		// Just redirect
}

Status ScFsOpenFile(PWChar path, UIntPtr flags, PIntPtr ret) {
	if (!ScCheckPointer(path)) {																															// Check if the pointer is inside of the userspace
		return STATUS_INVALID_ARG;
	}
	
	PFsNode file;
	Status status = FsOpenFile(path, flags, &file);																											// Try to open/create the file
	
	if (status != STATUS_SUCCESS) {
		return status;																																		// Failed
	}
	
	*ret = ScAppendHandle(HANDLE_TYPE_FILE, file);																											// Create the handle
	
	if (*ret == -1) {
		FsCloseFile(file);																																	// Failed
		return STATUS_OPEN_FAILED;
	}
	
	return STATUS_SUCCESS;
}

Status ScFsReadFile(IntPtr handle, UIntPtr len, PUInt8 buf, PUIntPtr ret) {
	if ((handle >= PsCurrentProcess->last_handle_id) || (!ScCheckPointer(buf)) || (!ScCheckPointer(ret))) {													// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return STATUS_WRONG_HANDLE;
	}
	
	PFsNode pfile = (PFsNode)hndl->data;																													// Get the real file
	Status status = FsReadFile(pfile, pfile->offset, len, buf, ret);																						// Redirect...
	
	if (status != STATUS_SUCCESS) {
		return status;
	}
	
	pfile->offset += *ret;																																	// And increase the offset
	
	return *ret;
}

Status ScFsWriteFile(IntPtr handle, UIntPtr len, PUInt8 buf, PUIntPtr ret) {
	if ((handle >= PsCurrentProcess->last_handle_id) || (!ScCheckPointer(buf)) || (!ScCheckPointer(ret))) {													// Sanity checks
		return STATUS_INVALID_ARG;
	} else if (handle == -1) {																																// Temporary stdout/Console device
		PChar cbuf = (PChar)buf;																															// Convert the buffer into a char buffer
		
		for (UIntPtr i = 0; i < len; i++) {																													// And write everything!
			if (cbuf[i] == '\n') {
				ConWriteCharacter('\r');
			}
			
			ConWriteCharacter(cbuf[i]);
		}
		
		*ret = len;
		
		return STATUS_SUCCESS;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return STATUS_WRONG_HANDLE;
	}
	
	PFsNode pfile = (PFsNode)hndl->data;																													// Get the real file
	Status status = FsWriteFile(pfile, pfile->offset, len, buf, ret);																						// Redirect...
	
	if (status != STATUS_SUCCESS) {
		return status;
	}
	
	pfile->offset += *ret;																																	// And increase the offset
	
	return *ret;
}

Status ScFsMountFile(PWChar path, PWChar file, PWChar type) {
	if ((!ScCheckPointer(path)) || (!ScCheckPointer(file)) || (!ScCheckPointer(type))) {																	// Sanity checks
		return STATUS_INVALID_ARG;	
	} else {
		return FsMountFile(path, file, type);																												// And redirect
	}
}

Status ScFsUmountFile(PWChar path) {
	if (!ScCheckPointer(path)) {																															// Sanity checks
		return STATUS_INVALID_ARG;
	} else {
		return FsUmountFile(path);																															// And redirect
	}
}

Status ScFsReadDirectoryEntry(IntPtr handle, UIntPtr entry, PWChar out) {
	if ((handle >= PsCurrentProcess->last_handle_id) || (!ScCheckPointer(out))) {																			// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return STATUS_WRONG_HANDLE;
	}
	
	PWChar name;
	Status status = FsReadDirectoryEntry((PFsNode)hndl->data, entry, &name);																				// Use the internal function
	
	if (status != STATUS_SUCCESS) {
		return status;																																		// Failed (probably this entry doesn't exists)
	}
	
	StrCopy(out, name);																																		// Copy the out pointer to the userspace
	MemFree((UIntPtr)name);																																	// Free the out pointer
	
	return STATUS_SUCCESS;
}

Status ScFsFindInDirectory(IntPtr handle, PWChar name, PIntPtr ret) {
	if ((handle >= PsCurrentProcess->last_handle_id) || (!ScCheckPointer(name)) || (!ScCheckPointer(ret))) {												// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return STATUS_WRONG_HANDLE;
	}
	
	PFsNode file;
	Status status = FsFindInDirectory((PFsNode)hndl->data, name, &file);																					// Use the internal function
	
	if (status != STATUS_SUCCESS) {
		return status;																																		// Failed (probably this file doesn't exists)
	}
	
	*ret = ScAppendHandle(HANDLE_TYPE_FILE, file);																											// Create the handle
	
	if (*ret == -1) {
		FsCloseFile(file);																																	// Failed...
		return STATUS_OPEN_FAILED;
	}
	
	return STATUS_SUCCESS;
}

Status ScFsControlFile(IntPtr handle, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf) {
	if ((handle >= PsCurrentProcess->last_handle_id) || (ibuf != Null && !ScCheckPointer(ibuf)) || (obuf != Null && !ScCheckPointer(obuf))) {				// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return STATUS_WRONG_HANDLE;
	}
	
	return FsControlFile((PFsNode)hndl->data, cmd, ibuf, obuf);																								// And redirect
}

Status ScFsGetFileSize(IntPtr handle, PUIntPtr ret) {
	if (handle >= PsCurrentProcess->last_handle_id || !ScCheckPointer(ret)) {																				// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return STATUS_WRONG_HANDLE;
	}
	
	PFsNode node = (PFsNode)hndl->data;																														// Get the real file
	
	if ((node->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																										// File?
		return STATUS_NOT_FILE;																																// We can't read the length of an directory (sorry)
	}
	
	*ret = node->length;
	
	return STATUS_SUCCESS;
}

Status ScFsGetPosition(IntPtr handle, PUIntPtr ret) {
	if (handle >= PsCurrentProcess->last_handle_id || !ScCheckPointer(ret)) {																				// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return STATUS_WRONG_HANDLE;
	}
	
	PFsNode node = (PFsNode)hndl->data;																														// Get the real file
	
	if ((node->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																										// File?
		return STATUS_NOT_FILE;																																// We can't read the offset of an directory (sorry)
	}
	
	*ret = node->offset;																																	// And return the offset
	
	return STATUS_SUCCESS;
}

Status ScFsSetPosition(IntPtr handle, UIntPtr base, UIntPtr off) {
	if (handle >= PsCurrentProcess->last_handle_id) {																										// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_FILE) {
		return STATUS_WRONG_HANDLE;
	}
	
	PFsNode node = (PFsNode)hndl->data;																														// Get the real file
	
	if ((node->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																										// File?
		return STATUS_NOT_FILE;																																// We can't read the offset of an directory (sorry)
	}
	
	if (base == 0) {																																		// Base = File Start
		node->offset = off;
	} else if (base == 1) {																																	// Base = Current Offset
		node->offset += off;
	} else if (base == 2) {																																	// Base = File End
		node->offset = node->length + off;
	} else {
		return STATUS_INVALID_ARG;
	}
	
	return STATUS_SUCCESS;
}

Status ScIpcCreatePort(PWChar name) {
	if (!ScCheckPointer(name)) {																															// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	return IpcCreatePort(name);
}

Status ScIpcCreateResponsePort(PIntPtr ret) {
	if (!ScCheckPointer(ret)) {																																// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	return IpcCreateResponsePort(ret);
}

Status ScIpcRemovePort(PWChar name) {
	if (!ScCheckPointer(name)) {																															// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	return IpcRemovePort(name);
}

Status ScIpcCheckPort(PWChar name) {
	if (!ScCheckPointer(name)) {																															// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	return IpcCheckPort(name);
}

Status ScIpcSendMessage(PWChar port, UInt32 msg, UIntPtr size, PUInt8 buf, IntPtr rport) {
	if (rport >= PsCurrentProcess->last_handle_id || !ScCheckPointer(port) || (buf != Null && !ScCheckPointer(buf))) {										// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	PIpcResponsePort rp = Null;
	
	if (rport != -1) {																																		// Should we get the response port from the rport handle?
		PHandle hndl = ListGet(&PsCurrentProcess->handles, rport);																							// Yes, get the handle data
		
		if (hndl == Null || hndl->type != HANDLE_TYPE_IPC_RESPONSE_PORT) {
			return STATUS_WRONG_HANDLE;
		}
		
		rp = (PIpcResponsePort)hndl->data;																													// And the response port struct
	}
	
	return IpcSendMessage(port, msg, size, buf, rp);																										// Now redirect!
}

Status ScIpcSendResponse(IntPtr handle, UInt32 msg, UIntPtr size, PUInt8 buf) {
	if (handle >= PsCurrentProcess->last_handle_id || (buf != Null && !ScCheckPointer(buf))) {																// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Get the handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_IPC_RESPONSE_PORT) {
		return STATUS_WRONG_HANDLE;
	}
	
	return IpcSendResponse((PIpcResponsePort)hndl->data, msg, size, buf);																					// And redirect
}

Status ScIpcReceiveMessage(PWChar name, PUInt32 msg, UIntPtr size, PUInt8 buf) {
	if (!ScCheckPointer(name) || !ScCheckPointer(name) || !ScCheckPointer(msg) || (buf != Null && !ScCheckPointer(buf))) {									// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	return IpcReceiveMessage(name, msg, size, buf);																											// And redirect
}

Status ScIpcReceiveResponse(IntPtr handle, PUInt32 msg, UIntPtr size, PUInt8 buf) {
	if (handle >= PsCurrentProcess->last_handle_id || !ScCheckPointer(msg) || (buf != Null && !ScCheckPointer(buf))) {										// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	PHandle hndl = ListGet(&PsCurrentProcess->handles, handle);																								// Get handle data
	
	if (hndl == Null || hndl->type != HANDLE_TYPE_IPC_RESPONSE_PORT) {
		return STATUS_WRONG_HANDLE;
	}
	
	return IpcReceiveResponse((PIpcResponsePort)hndl->data, msg, size, buf);																				// And redirect
}

Status ScShmCreateSection(UIntPtr size, PUIntPtr key, PUIntPtr ret) {
	if (!ScCheckPointer(key) || !ScCheckPointer(ret)) {																										// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	return ShmCreateSection(size, key, ret);																												// And redirect
}

Status ScShmMapSection(UIntPtr key, PUIntPtr ret) {
	if (!ScCheckPointer(ret)) {																																// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	return ShmMapSection(key, ret);																															// Redirect
}

Status ScShmUnmapSection(UIntPtr key) {
	return ShmUnmapSection(key);																															// Redirect
}
