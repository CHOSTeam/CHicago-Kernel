// File author is Ítalo Lima Marconato Matias
//
// Created on December 24 of 2019, at 14:18 BRT
// Last edited on January 21 of 2020, at 23:21 BRT

#define __CHICAGO_IPC__

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/ipc.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/sc.h>
#include <chicago/string.h>

List IpcPortList = { Null, Null, 0, False };

Status IpcCreatePort(PWChar name) {
	if (PsCurrentProcess == Null || name == Null) {														// Sanity checks
		return STATUS_INVALID_ARG;																		// Nope...
	}
	
	PIpcPort port = (PIpcPort)MemAllocate(sizeof(IpcPort));												// Alloc the struct space
	
	if (port == Null) {
		return STATUS_OUT_OF_MEMORY;																	// Failed
	}
	
	port->name = StrDuplicate(name);																	// Duplicate the name
	
	if (port->name == Null) {
		MemFree((UIntPtr)port);																			// Failed
		return STATUS_OUT_OF_MEMORY;
	}
	
	port->queue.head = Null;																			// Init the msg queue
	port->queue.tail = Null;
	port->queue.length = 0;
	port->queue.free = False;
	port->proc = PsCurrentProcess;																		// Save the port owner
	
	if (!ListAdd(&IpcPortList, port)) {																	// Add this port to the port list
		MemFree((UIntPtr)port->name);
		MemFree((UIntPtr)port);
		return STATUS_OUT_OF_MEMORY;
	}
	
	return STATUS_SUCCESS;
}

Status IpcCreateResponsePort(PIntPtr ret) {
	if (ret == Null) {																					// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	PIpcResponsePort port = (PIpcResponsePort)MemAllocate(sizeof(IpcResponsePort));						// Alloc the struct space
	
	if (port == Null) {
		return STATUS_OUT_OF_MEMORY;																	// Failed
	}
	
	port->queue.head = Null;																			// Init the msg queue
	port->queue.tail = Null;
	port->queue.length = 0;
	port->queue.free = False;
	port->proc = PsCurrentProcess;																		// Save the port owner
	
	*ret = ScAppendHandle(HANDLE_TYPE_IPC_RESPONSE_PORT, port);											// Create the handle
	
	if (*ret == -1) {
		MemFree((UIntPtr)port);																			// Failed...
		return STATUS_OUT_OF_MEMORY;																	// Maybe we should add a different error here later...
	}
	
	return STATUS_SUCCESS;
}

Status IpcRemovePort(PWChar name) {
	if (PsCurrentProcess == Null || name == Null) {														// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	PIpcPort port = Null;
	
	for (; !found && idx < IpcPortList.length; idx++) {													// Let's search for this port in the port list!
		port = ListGet(&IpcPortList, idx);
		
		if (StrGetLength(port->name) != StrGetLength(name)) {											// Same length?
			continue;																					// Nope
		} else if (StrCompare(port->name, name)) {														// Found it?
			found = True;																				// Yes :)
		}
	}
	
	if (!found) {
		return STATUS_PORT_DOESNT_EXISTS;																// Port not found...
	} else if (port->proc != PsCurrentProcess) {
		return STATUS_NOT_OWNER;																		// Only the owner of this port can remove it
	}
	
	ListRemove(&IpcPortList, idx);																		// Remove this port from the list
	
	while (port->queue.length != 0) {																	// Let's free all the messages that are in the queue
		PIpcMessage msg = QueueRemove(&port->queue);
		ScSysCloseHandle(msg->src);																		// Close the handle for the source process
		MemFree((UIntPtr)msg->buffer);																	// And free everything
		MemFree((UIntPtr)msg);
	}
	
	MemFree((UIntPtr)port->name);																		// Free the name
	MemFree((UIntPtr)port);																				// Free the struct itself
	
	return STATUS_SUCCESS;
}

Status IpcCheckPort(PWChar name) {
	if (PsCurrentProcess == Null || name == Null) {														// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	ListForeach(&IpcPortList, i) {																		// Let's iterate the port list
		if (StrGetLength(((PIpcPort)i->data)->name) != StrGetLength(name)) {							// Check if both names have the same length
			continue;
		} else if (StrCompare(((PIpcPort)i->data)->name, name)) {										// And compare them!
			return STATUS_PORT_ALREADY_EXISTS;
		}
	}
	
	return STATUS_PORT_DOESNT_EXISTS;
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
		MemFree((UIntPtr)msg->buffer);																	// And free everything
		MemFree((UIntPtr)msg);
	}
	
	MemFree((UIntPtr)port);																				// Free the struct itself
}

Status IpcSendMessage(PWChar name, UInt32 msg, UIntPtr size, PUInt8 buf, PIpcResponsePort rport) {
	if (PsCurrentProcess == Null || name == Null) {														// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	PIpcPort port = Null;
	
	for (; !found && idx < IpcPortList.length; idx++) {													// Let's search for this port in the port list!
		port = ListGet(&IpcPortList, idx);
		
		if (StrGetLength(port->name) != StrGetLength(name)) {											// Same length?
			continue;																					// Nope
		} else if (StrCompare(port->name, name)) {														// Found it?
			found = True;																				// Yes :)
		}
	}
	
	if (!found) {
		return STATUS_PORT_DOESNT_EXISTS;																// Port not found...
	}
	
	PUInt8 new = (PUInt8)MemAllocate(size);																// Copy the buffer...
	
	if (new == Null) {
		return STATUS_OUT_OF_MEMORY;
	}
	
	StrCopyMemory(new, buf, size);
	
	buf = new;
	
	PsLockTaskSwitch(old);																				// Lock
	
	PProcess oldp = PsCurrentProcess;																	// Save the current process
	
	MmSwitchDirectory(port->proc->dir);																	// Switch to the dir of the owner of this port
	PsCurrentProcess = port->proc;																		// And switch the current process struct
	
	PIpcMessage mes = (PIpcMessage)MemAllocate(sizeof(IpcMessage));										// Alloc space for the message struct
	
	if (mes == Null) {
		MemFree((UIntPtr)buf);																			// Failed...
		MmSwitchDirectory(oldp->dir);
		PsCurrentProcess = oldp;
		PsUnlockTaskSwitch(old);
		
		return STATUS_OUT_OF_MEMORY;
	}
	
	mes->msg = msg;																						// Setup it
	mes->src = ScPsGetCurrentProcess();																	// Get a handle to the source process
	
	if (mes->src == -1) {
		MemFree((UIntPtr)buf);																			// Failed...
		MemFree((UIntPtr)mes);
		MmSwitchDirectory(oldp->dir);
		PsCurrentProcess = oldp;
		PsUnlockTaskSwitch(old);
		
		return STATUS_OUT_OF_MEMORY;
	}
	
	if (rport != Null) {																				// Set the rport?
		mes->rport = ScAppendHandle(HANDLE_TYPE_IPC_RESPONSE_PORT, rport);								// Yes, create the handle to it
		
		if (mes->rport == -1) {
			MemFree((UIntPtr)buf);																		// Failed...
			ScSysCloseHandle(mes->src);
			MemFree((UIntPtr)mes);
			MmSwitchDirectory(oldp->dir);
			PsCurrentProcess = oldp;
			PsUnlockTaskSwitch(old);
			
			return STATUS_OUT_OF_MEMORY;
		}
	} else {
		mes->rport = -1;
	}
	
	mes->size = size;
	mes->buffer = buf;
	
	QueueAdd(&port->queue, mes);																		// Add to the msg queue
	MmSwitchDirectory(oldp->dir);																		// Switch back to the old dir
	PsCurrentProcess = oldp;																			// Switch back the old proc struct
	PsUnlockTaskSwitch(old);																			// Unlock
	
	return STATUS_SUCCESS;
}
	
Status IpcSendResponse(PIpcResponsePort port, UInt32 msg, UIntPtr size, PUInt8 buf) {
	if (port == Null) {																					// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	PUInt8 new = (PUInt8)MemAllocate(size);																// Copy the buffer...
	
	if (new == Null) {
		return STATUS_OUT_OF_MEMORY;
	}
	
	StrCopyMemory(new, buf, size);
	
	buf = new;
	
	PsLockTaskSwitch(old);																				// Lock
	
	PProcess oldp = PsCurrentProcess;																	// Save the current process
	
	MmSwitchDirectory(port->proc->dir);																	// Switch to the dir of the owner of this port
	PsCurrentProcess = port->proc;																		// And switch the current process struc
	
	PIpcMessage mes = (PIpcMessage)MemAllocate(sizeof(IpcMessage));										// Alloc space for the message struct
	
	if (mes == Null) {
		MemFree((UIntPtr)buf);																			// Failed
		MmSwitchDirectory(oldp->dir);
		PsCurrentProcess = oldp;
		PsUnlockTaskSwitch(old);
		return STATUS_OUT_OF_MEMORY;
	}
	
	mes->msg = msg;																						// Setup it
	mes->src = ScPsGetCurrentProcess();																	// Get a handle to the source process
	
	if (mes->src == -1) {
		MemFree((UIntPtr)mes);																			// Failed
		MemFree((UIntPtr)buf);
		MmSwitchDirectory(oldp->dir);
		PsCurrentProcess = oldp;
		PsUnlockTaskSwitch(old);
		return STATUS_OUT_OF_MEMORY;
	}
		
	mes->size = size;
	mes->buffer = new;
	
	QueueAdd(&port->queue, mes);																		// Add to the msg queue
	MmSwitchDirectory(oldp->dir);																		// Switch back to the old dir
	PsCurrentProcess = oldp;																			// Switch back to the old process struct
	PsUnlockTaskSwitch(old);																			// Unlock
	
	return STATUS_SUCCESS;
}
	
Status IpcReceiveMessage(PWChar name, PUInt32 msg, UIntPtr size, PUInt8 buf) {
	if (PsCurrentProcess == Null || name == Null || msg == Null) {										// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	PIpcPort port = Null;
	
	for (; !found && idx < IpcPortList.length; idx++) {													// Let's search for this port in the port list!
		port = ListGet(&IpcPortList, idx);
		
		if (StrGetLength(port->name) != StrGetLength(name)) {											// Same length?
			continue;																					// Nope
		} else if (StrCompare(port->name, name)) {														// Found it?
			found = True;																				// Yes :)
		}
	}
	
	if (!found) {
		return STATUS_PORT_DOESNT_EXISTS;																// Port not found...
	} else if (port->proc != PsCurrentProcess) {
		return STATUS_NOT_OWNER;																		// Only the owner of this port can use the receive function
	}
	
	while (port->queue.length == 0) {																	// Wait for anything to come in
		PsSwitchTask(Null);
	}
	
	PIpcMessage mes = QueueRemove(&port->queue);														// Ok, get the data
	
	*msg = mes->msg;																					// Set the message
	
	if (size != 0 && buf != Null && mes->size != 0 && mes->buffer != Null) {							// Copy the data?
		StrCopyMemory(buf, mes->buffer, size > mes->size ? mes->size : size);							// Yes!
	}
	
	return STATUS_SUCCESS;
}

Status IpcReceiveResponse(PIpcResponsePort port, PUInt32 msg, UIntPtr size, PUInt8 buf) {
	if (port == Null || msg == Null) {																	// Sanity check
		return STATUS_INVALID_ARG;
	} else if (port->proc != PsCurrentProcess) {
		return STATUS_NOT_OWNER;																		// Only the owner of this port can use the receive function
	}
	
	while (port->queue.length == 0) {																	// Wait for anything to come in
		PsSwitchTask(Null);
	}
	
	PIpcMessage mes = QueueRemove(&port->queue);														// Ok, get the data
	
	*msg = mes->msg;																					// Set the message
	
	if (size != 0 && buf != Null && mes->size != 0 && mes->buffer != Null) {							// Copy the data?
		StrCopyMemory(buf, mes->buffer, size > mes->size ? mes->size : size);							// Yes!
	}
	
	return STATUS_SUCCESS;
}
