// File author is Ítalo Lima Marconato Matias
//
// Created on December 24 of 2019, at 14:18 BRT
// Last edited on December 25 of 2019, at 18:53 BRT

#define __CHICAGO_IPC__

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/ipc.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/sc.h>
#include <chicago/string.h>

PList IpcPortList = Null;

Boolean IpcCreatePort(PWChar name) {
	if (PsCurrentProcess == Null || IpcPortList == Null || name == Null) {								// Sanity checks
		return False;																					// Nope...
	}
	
	PIpcPort port = (PIpcPort)MemAllocate(sizeof(IpcPort));												// Alloc the struct space
	
	if (port == Null) {
		return False;																					// Failed
	}
	
	port->name = StrDuplicate(name);																	// Duplicate the name
	
	if (port->name == Null) {
		MemFree((UIntPtr)port);																			// Failed
		return False;
	}
	
	port->queue.head = Null;																			// Init the msg queue
	port->queue.tail = Null;
	port->queue.length = 0;
	port->queue.free = False;
	port->queue.user = False;
	port->proc = PsCurrentProcess;																		// Save the port owner
	
	if (!ListAdd(IpcPortList, port)) {																	// Add this port to the port list
		MemFree((UIntPtr)port->name);
		MemFree((UIntPtr)port);
		return False;
	}
	
	return True;
}

IntPtr IpcCreateResponsePort(Void) {
	PIpcResponsePort port = (PIpcResponsePort)MemAllocate(sizeof(IpcResponsePort));						// Alloc the struct space
	
	if (port == Null) {
		return -1;																						// Failed
	}
	
	port->queue.head = Null;																			// Init the msg queue
	port->queue.tail = Null;
	port->queue.length = 0;
	port->queue.free = False;
	port->queue.user = False;
	port->proc = PsCurrentProcess;																		// Save the port owner
	
	IntPtr handle = ScAppendHandle(HANDLE_TYPE_IPC_RESPONSE_PORT, port);								// Create the handle
	
	if (handle == -1) {
		MemFree((UIntPtr)port);																			// Failed...
	}
	
	return handle;
}

Void IpcRemovePort(PWChar name) {
	if (PsCurrentProcess == Null || IpcPortList == Null || name == Null) {								// Sanity checks
		return;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	PIpcPort port = Null;
	
	for (; !found && idx < IpcPortList->length; idx++) {												// Let's search for this port in the port list!
		port = ListGet(IpcPortList, idx);
		
		if (StrGetLength(port->name) != StrGetLength(name)) {											// Same length?
			continue;																					// Nope
		} else if (StrCompare(port->name, name)) {														// Found it?
			found = True;																				// Yes :)
		}
	}
	
	if (!found) {
		return;																							// Port not found...
	} else if (port->proc != PsCurrentProcess) {
		return;																							// Only the owner of this port can remove it
	}
	
	ListRemove(IpcPortList, idx);																		// Remove this port from the list
	
	while (port->queue.length != 0) {																	// Let's free all the messages that are in the queue
		PIpcMessage msg = QueueRemove(&port->queue);
		
		ScSysCloseHandle(msg->src);																		// Close the handle for the source process
		
		if (msg->buffer != Null) {																		// Free the message data (if we need to)
			MmFreeUserMemory((UIntPtr)msg->buffer);
		}
	}
	
	MemFree((UIntPtr)port->name);																		// Free the name
	MemFree((UIntPtr)port);																				// Free the struct itself
}

Void IpcFreeResponsePort(PIpcResponsePort port) {
	if (port == Null) {																					// Sanity check
		return;
	} else if (port->proc != PsCurrentProcess) {														// Only the owner of this port can remove/free it
		return;
	}
	
	while (port->queue.length != 0) {																	// Let's free all the messages that are in the queue
		PIpcMessage msg = QueueRemove(&port->queue);
		
		ScSysCloseHandle(msg->src);																		// Close the handle for the source process
		
		if (msg->buffer != Null) {																		// Free the message data (if we need to)
			MmFreeUserMemory((UIntPtr)msg->buffer);
		}
	}
	
	MemFree((UIntPtr)port);																				// Free the struct itself
}

PIpcMessage IpcSendMessage(PWChar name, UInt32 msg, UIntPtr size, PUInt8 buf, PIpcResponsePort rport) {
	if (PsCurrentProcess == Null || IpcPortList == Null || name == Null) {								// Sanity checks
		return Null;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	PIpcPort port = Null;
	
	for (; !found && idx < IpcPortList->length; idx++) {												// Let's search for this port in the port list!
		port = ListGet(IpcPortList, idx);
		
		if (StrGetLength(port->name) != StrGetLength(name)) {											// Same length?
			continue;																					// Nope
		} else if (StrCompare(port->name, name)) {														// Found it?
			found = True;																				// Yes :)
		}
	}
	
	if (!found) {
		return Null;																					// Port not found...
	}
	
#if (MM_USER_START == 0)																				// Let's fix an compiler warning :)
	if (buf != Null && ((UIntPtr)buf) < MM_USER_END) {													// Check if the buffer is inside of the userspace!
#else
	if (buf != Null && (((UIntPtr)buf) >= MM_USER_START) && (((UIntPtr)buf) < MM_USER_END)) {			// Same as above
#endif
		PUInt8 new = (PUInt8)MemAllocate(size);															// Yes, let's copy it to the kernelspace
		
		if (new == Null) {
			return Null;																				// Failed...
		}
		
		StrCopyMemory(new, buf, size);																	// Copy it
		buf = new;																						// And set the new buffer
	}
	
	PsLockTaskSwitch(old);																				// Lock
	
	PAllocBlock oldab = PsCurrentProcess->alloc_base;													// Save some stuff
	PList oldh = PsCurrentProcess->handles;
	IntPtr oldlhi = PsCurrentProcess->last_handle_id;
	
	MmSwitchDirectory(port->proc->dir);																	// Switch to the dir of the owner of this port
	PsCurrentProcess->alloc_base = port->proc->alloc_base;												// And switch some of the proc structs
	PsCurrentProcess->handles = port->proc->handles;
	PsCurrentProcess->last_handle_id = port->proc->last_handle_id;
	
	PUInt8 new = buf == Null ? Null : (PUInt8)MmAllocUserMemory(size);									// Alloc the new buffer in the target process userspace
	
	if (buf != Null && new == Null) {
		MmSwitchDirectory(PsCurrentProcess->dir);														// Failed
		PsCurrentProcess->alloc_base = oldab;
		PsCurrentProcess->handles = oldh;
		PsCurrentProcess->last_handle_id = oldlhi;
		PsUnlockTaskSwitch(old);
		return Null;
	} else if (buf != Null) {
		StrCopyMemory(new, buf, size);																	// Copy the data from the old buffer to the new one
	}
	
	PIpcMessage mes = (PIpcMessage)MmAllocUserMemory(sizeof(IpcMessage));								// Alloc space for the message struct
	
	if (mes == Null) {
		MmFreeUserMemory((UIntPtr)new);																	// Failed
		MmSwitchDirectory(PsCurrentProcess->dir);
		PsCurrentProcess->alloc_base = oldab;
		PsCurrentProcess->handles = oldh;
		PsCurrentProcess->last_handle_id = oldlhi;
		PsUnlockTaskSwitch(old);
		return Null;
	}
	
	mes->msg = msg;																						// Setup it
	mes->src = ScPsGetCurrentProcess();																	// Get a handle to the source process
	
	if (mes->src == -1) {
		MmFreeUserMemory((UIntPtr)mes);																	// Failed
		MmFreeUserMemory((UIntPtr)new);
		MmSwitchDirectory(PsCurrentProcess->dir);
		PsCurrentProcess->alloc_base = oldab;
		PsCurrentProcess->handles = oldh;
		PsCurrentProcess->last_handle_id = oldlhi;
		PsUnlockTaskSwitch(old);
		return Null;
	}
	
	if (rport != Null) {																				// Set the rport?
		mes->rport = ScAppendHandle(HANDLE_TYPE_IPC_RESPONSE_PORT, rport);								// Yes, create the handle to it
		
		if (mes->rport == -1) {
			ScSysCloseHandle(mes->src);
			MmFreeUserMemory((UIntPtr)mes);																// Failed
			MmFreeUserMemory((UIntPtr)new);
			MmSwitchDirectory(PsCurrentProcess->dir);
			PsCurrentProcess->alloc_base = oldab;
			PsCurrentProcess->handles = oldh;
			PsCurrentProcess->last_handle_id = oldlhi;
			PsUnlockTaskSwitch(old);
			return Null;
		}
	} else {
		mes->rport = -1;
	}
	
	mes->size = size;
	mes->buffer = new;
	
	QueueAdd(&port->queue, mes);																		// Add to the msg queue
	MmSwitchDirectory(PsCurrentProcess->dir);															// Switch back to the old dir
	port->proc->last_handle_id = PsCurrentProcess->last_handle_id;										// Save any change in the handle count
	PsCurrentProcess->alloc_base = oldab;																// Switch back to the old proc structs
	PsCurrentProcess->handles = oldh;
	PsCurrentProcess->last_handle_id = oldlhi;
	PsUnlockTaskSwitch(old);																			// Unlock
	
	if (rport != Null) {																				// Receive the response?
		return IpcReceiveResponse(rport);																// Yes!
	}
	
	return Null;																						// Nope, just return Null
}
	
Void IpcSendResponse(PIpcResponsePort port, UInt32 msg, UIntPtr size, PUInt8 buf) {
	if (port == Null) {																					// Sanity check
		return;
	}
	
#if (MM_USER_START == 0)																				// Let's fix an compiler warning :)
	if (buf != Null && ((UIntPtr)buf) < MM_USER_END) {													// Check if the buffer is inside of the userspace!
#else
	if (buf != Null && (((UIntPtr)buf) >= MM_USER_START) && (((UIntPtr)buf) < MM_USER_END)) {			// Same as above
#endif
		PUInt8 new = (PUInt8)MemAllocate(size);															// Yes, let's copy it to the kernelspace
		
		if (new == Null) {
			return;																						// Failed...
		}
		
		StrCopyMemory(new, buf, size);																	// Copy it
		buf = new;																						// And set the new buffer
	}
	
	PsLockTaskSwitch(old);																				// Lock
	
	PAllocBlock oldab = PsCurrentProcess->alloc_base;													// Save some stuff
	PList oldh = PsCurrentProcess->handles;
	IntPtr oldlhi = PsCurrentProcess->last_handle_id;
	
	MmSwitchDirectory(port->proc->dir);																	// Switch to the dir of the owner of this port
	PsCurrentProcess->alloc_base = port->proc->alloc_base;												// And switch some of the proc structs
	PsCurrentProcess->handles = port->proc->handles;
	PsCurrentProcess->last_handle_id = port->proc->last_handle_id;
	
	PUInt8 new = buf == Null ? Null : (PUInt8)MmAllocUserMemory(size);									// Alloc the new buffer in the target process userspace
	
	if (buf != Null && new == Null) {
		MmSwitchDirectory(PsCurrentProcess->dir);														// Failed
		PsCurrentProcess->alloc_base = oldab;
		PsCurrentProcess->handles = oldh;
		PsCurrentProcess->last_handle_id = oldlhi;
		PsUnlockTaskSwitch(old);
		return;
	} else if (buf != Null) {
		StrCopyMemory(new, buf, size);																	// Copy the data from the old buffer to the new one
	}
	
	PIpcMessage mes = (PIpcMessage)MmAllocUserMemory(sizeof(IpcMessage));								// Alloc space for the message struct
	
	if (mes == Null) {
		MmFreeUserMemory((UIntPtr)new);																	// Failed
		MmSwitchDirectory(PsCurrentProcess->dir);
		PsCurrentProcess->alloc_base = oldab;
		PsCurrentProcess->handles = oldh;
		PsCurrentProcess->last_handle_id = oldlhi;
		PsUnlockTaskSwitch(old);
		return;
	}
	
	mes->msg = msg;																						// Setup it
	mes->src = ScPsGetCurrentProcess();																	// Get a handle to the source process
	
	if (mes->src == -1) {
		MmFreeUserMemory((UIntPtr)mes);																	// Failed
		MmFreeUserMemory((UIntPtr)new);
		MmSwitchDirectory(PsCurrentProcess->dir);
		PsCurrentProcess->alloc_base = oldab;
		PsCurrentProcess->handles = oldh;
		PsCurrentProcess->last_handle_id = oldlhi;
		PsUnlockTaskSwitch(old);
		return;
	}
		
	mes->size = size;
	mes->buffer = new;
	
	QueueAdd(&port->queue, mes);																		// Add to the msg queue
	MmSwitchDirectory(PsCurrentProcess->dir);															// Switch back to the old dir
	PsCurrentProcess->alloc_base = oldab;																// Switch back to the old proc structs
	PsCurrentProcess->handles = oldh;
	PsCurrentProcess->last_handle_id = oldlhi;
	PsUnlockTaskSwitch(old);																			// Unlock
}
	
PIpcMessage IpcReceiveMessage(PWChar name) {
	if (PsCurrentProcess == Null || IpcPortList == Null || name == Null) {								// Sanity checks
		return Null;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	PIpcPort port = Null;
	
	for (; !found && idx < IpcPortList->length; idx++) {												// Let's search for this port in the port list!
		port = ListGet(IpcPortList, idx);
		
		if (StrGetLength(port->name) != StrGetLength(name)) {											// Same length?
			continue;																					// Nope
		} else if (StrCompare(port->name, name)) {														// Found it?
			found = True;																				// Yes :)
		}
	}
	
	if (!found) {
		return Null;																					// Port not found...
	} else if (port->proc != PsCurrentProcess) {
		return Null;																					// Only the owner of this port can use the receive function
	}
	
	while (port->queue.length == 0) {																	// Wait for anything to come in
		PsSwitchTask(Null);
	}
	
	return QueueRemove(&port->queue);																	// And return!
}

PIpcMessage IpcReceiveResponse(PIpcResponsePort port) {
	if (port == Null) {																					// Sanity check
		return Null;
	} else if (port->proc != PsCurrentProcess) {
		return Null;																					// Only the owner of this port can use the receive function
	}
	
	while (port->queue.length == 0) {																	// Wait for anything to come in
		PsSwitchTask(Null);
	}
	
	return QueueRemove(&port->queue);																	// And return!
}

Void IpcInit(Void) {
	IpcPortList = ListNew(True, False);																	// Try to init the port list
	
	if (IpcPortList == Null) {
		DbgWriteFormated("PANIC! Failed to init IPC\r\n");												// Failed... but it's a critical component, so HALT
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}
