// File author is Ítalo Lima Marconato Matias
//
// Created on November 15 of 2018, at 22:20 BRT
// Last edited on September 05 of 2019, at 14:43 BRT

#define __CHICAGO_IPC__

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/ipc.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/string.h>

PList IpcPortList = Null;

PIpcPort IpcCreatePort(PWChar name) {
	if (PsCurrentProcess == Null || IpcPortList == Null || name == Null) {				// Sanity checks
		return Null;																	// Nope...
	}
	
	PIpcPort port = (PIpcPort)MemAllocate(sizeof(IpcPort));								// Alloc the struct space
	
	if (port == Null) {
		return Null;																	// Failed
	}
	
	port->name = StrDuplicate(name);													// Duplicate the name
	
	if (port->name == Null) {
		MemFree((UIntPtr)port);															// Failed
		return Null;
	}
	
	port->queue.head = Null;															// Init the msg queue
	port->queue.tail = Null;
	port->queue.length = 0;
	port->queue.free = True;
	port->queue.user = False;
	port->proc = PsCurrentProcess;														// Save the port owner
	
	if (!ListAdd(IpcPortList, port)) {													// Add this port to the port list
		MemFree((UIntPtr)port->name);
		MemFree((UIntPtr)port);
		return Null;
	}
	
	return port;
}

Void IpcRemovePort(PWChar name) {
	if (PsCurrentProcess == Null || IpcPortList == Null || name == Null) {				// Sanity checks
		return;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	PIpcPort port = Null;
	
	for (; !found && idx < IpcPortList->length; idx++) {								// Let's search for this port in the port list!
		port = ListGet(IpcPortList, idx);
		
		if (StrGetLength(port->name) != StrGetLength(name)) {							// Same length?
			continue;																	// Nope
		} else if (StrCompare(port->name, name)) {										// Found it?
			found = True;																// Yes :)
		}
	}
	
	if (!found) {
		return;																			// Port not found...
	} else if (port->proc != PsCurrentProcess) {
		return;																			// Only the owner of this port can remove it
	}
	
	ListRemove(IpcPortList, idx);														// Remove this port from the list
	MemFree((UIntPtr)port->name);														// Free the name
	MemFree((UIntPtr)port);																// Free the struct itself
}

Void IpcSendMessage(PWChar name, UInt32 msg, UIntPtr size, PUInt8 buf) {
	if (PsCurrentProcess == Null || IpcPortList == Null || name == Null) {				// Sanity checks
		return;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	PIpcPort port = Null;
	
	for (; !found && idx < IpcPortList->length; idx++) {								// Let's search for this port in the port list!
		port = ListGet(IpcPortList, idx);
		
		if (StrGetLength(port->name) != StrGetLength(name)) {							// Same length?
			continue;																	// Nope
		} else if (StrCompare(port->name, name)) {										// Found it?
			found = True;																// Yes :)
		}
	}
	
	if (!found) {
		return;																			// Port not found...
	}
	
#if (MM_USER_START == 0)																// Let's fix an compiler warning :)
	if (((UIntPtr)buf) < MM_USER_END) {													// Check if the buffer is inside of the userspace!
#else
	if ((((UIntPtr)buf) >= MM_USER_START) && (((UIntPtr)buf) < MM_USER_END)) {			// Same as above
#endif
		PUInt8 new = (PUInt8)MemAllocate(size);											// Yes, let's copy it to the kernelspace
		
		if (new == Null) {
			return;																		// Failed...
		}
		
		StrCopyMemory(new, buf, size);													// Copy it
		buf = new;																		// And set the new buffer
	}
	
	PsLockTaskSwitch(old);																// Lock
	
	PAllocBlock oldab = PsCurrentProcess->alloc_base;									// Save the old alloc_base (we need to switch it)
	
	MmSwitchDirectory(port->proc->dir);													// Switch to the dir of the owner of this port
	PsCurrentProcess->alloc_base = port->proc->alloc_base;								// Switch the alloc_base
	
	PUInt8 new = (PUInt8)MmAllocUserMemory(size);										// Alloc the new buffer in the target process userspace
	
	if (new == Null) {
		MmSwitchDirectory(PsCurrentProcess->dir);										// Failed
		PsCurrentProcess->alloc_base = oldab;
		PsUnlockTaskSwitch(old);
		return;
	}
	
	StrCopyMemory(new, buf, size);														// Copy the data from the old buffer to the new one
	
	PIpcMessage mes = (PIpcMessage)MmAllocUserMemory(sizeof(IpcMessage));				// Alloc space for the message struct
	
	if (mes == Null) {
		MmFreeUserMemory((UIntPtr)new);													// Failed
		MmSwitchDirectory(PsCurrentProcess->dir);
		PsCurrentProcess->alloc_base = oldab;
		PsUnlockTaskSwitch(old);
		return;
	}
	
	mes->msg = msg;																		// Setup it
	mes->src = PsCurrentProcess->id;
	mes->size = size;
	mes->buffer = new;
	
	QueueAdd(&port->queue, mes);														// Add to the msg queue
	MmSwitchDirectory(PsCurrentProcess->dir);											// Switch back to the old dir
	PsCurrentProcess->alloc_base = oldab;												// Switch back to the old alloc_base
	PsUnlockTaskSwitch(old);															// Unlock
}

PIpcMessage IpcReceiveMessage(PWChar name) {
	if (PsCurrentProcess == Null || IpcPortList == Null || name == Null) {				// Sanity checks
		return Null;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	PIpcPort port = Null;
	
	for (; !found && idx < IpcPortList->length; idx++) {								// Let's search for this port in the port list!
		port = ListGet(IpcPortList, idx);
		
		if (StrGetLength(port->name) != StrGetLength(name)) {							// Same length?
			continue;																	// Nope
		} else if (StrCompare(port->name, name)) {										// Found it?
			found = True;																// Yes :)
		}
	}
	
	if (!found) {
		return Null;																	// Port not found...
	} else if (port->proc != PsCurrentProcess) {
		return Null;																	// Only the owner of this port can use the receive function
	}
	
	while (port->queue.length == 0) {													// Wait for anything to come in
		PsSwitchTask(Null);
	}
	
	return QueueRemove(&port->queue);													// And return!
}

Void IpcInit(Void) {
	IpcPortList = ListNew(True, False);													// Try to init the port list
	
	if (IpcPortList == Null) {
		DbgWriteFormated("PANIC! Failed to init IPC\r\n");								// Failed... but it's a critical component, so HALT
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}
