// File author is Ítalo Lima Marconato Matias
//
// Created on October 19 of 2018, at 21:40 BRT
// Last edited on September 05 of 2019, at 16:54 BRT

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/panic.h>
#include <chicago/process.h>
#include <chicago/queue.h>

Queue RawMouseDeviceQueue;
Lock RawMouseDeviceQueueReadLock = { False, Null };
Lock RawMouseDeviceQueueWriteLock = { False, Null };

static Boolean RawMouseDeviceReadInt(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	(Void)dev; (Void)off; (Void)len;
	RawMouseDeviceRead((PMousePacket)buf);													// Redirect to RawMouseDeviceRead
	return True;
}

Void RawMouseDeviceWrite(Int32 offx, Int32 offy, UInt8 buttons) {
	PsLock(&RawMouseDeviceQueueWriteLock);													// Lock
	
	PMousePacket packet = (PMousePacket)MemAllocate(sizeof(MousePacket));					// Alloc the mouse packet
	
	if (packet == Null) {
		return;
	}
	
	packet->offx = offx;																	// Set it up
	packet->offy = offy;
	packet->buttons = buttons;
	
	while (RawMouseDeviceQueue.length >= 1024) {											// Don't let the queue grow too much
		QueueRemove(&RawMouseDeviceQueue);
	}
	
	QueueAdd(&RawMouseDeviceQueue, packet);													// Add to the queue
	PsUnlock(&RawMouseDeviceQueueWriteLock);												// Unlock!
}

Void RawMouseDeviceRead(PMousePacket buf) {
	if (buf == Null) {
		return;
	}
	
	while (RawMouseDeviceQueue.length == 0) {												// Let's fill the queue with what we need
		PsSwitchTask(Null);
	}
	
	PsLock(&RawMouseDeviceQueueReadLock);													// Lock
	
	PMousePacket packet = QueueRemove(&RawMouseDeviceQueue);								// Get the packet
	
	buf->offx = packet->offx;																// And copy it into the buffer
	buf->offy = packet->offy;
	buf->buttons = packet->buttons;
	
	MemFree((UIntPtr)packet);																// Free the packet
	PsUnlock(&RawMouseDeviceQueueReadLock);													// Unlock!
}

Void RawMouseDeviceInit(Void) {
	RawMouseDeviceQueue.head = Null;
	RawMouseDeviceQueue.tail = Null;
	RawMouseDeviceQueue.length = 0;
	RawMouseDeviceQueue.free = True;
	RawMouseDeviceQueue.user = False;
	
	if (!FsAddDevice(L"RawMouse", Null, RawMouseDeviceReadInt, Null, Null)) {
		DbgWriteFormated("PANIC! Failed to add the RawMouse device\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}
