// File author is Ítalo Lima Marconato Matias
//
// Created on September 01 of 2019, at 14:20 BRT
// Last edited on September 01 of 2019, at 15:09 BRT

#define __CHICAGO_PROCESS__

#include <chicago/alloc.h>
#include <chicago/console.h>
#include <chicago/device.h>
#include <chicago/process.h>

UIntPtr KernelPtyKeyboardNewLineLoc = 0;
Boolean KernelPtyKeyboardNewLine = False;
Lock KernelPtyKeyboardQueueReadLock = { False, Null };
Lock KernelPtyKeyboardQueueWriteLock = { False, Null };
Queue KernelPtyKeyboardQueue = { Null, Null, 0, False, False };

Void KernelPtyClearKeyboard(PProcessPty pty) {
	(Void)pty;																			// We don't want the unused parameter warning
	
	PsLock(&KernelPtyKeyboardQueueReadLock);											// Lock
	PsLock(&KernelPtyKeyboardQueueWriteLock);
	
	while (KernelPtyKeyboardQueue.length != 0) {										// Clean!
		QueueRemove(&KernelPtyKeyboardQueue);
	}
	
	PsUnlock(&KernelPtyKeyboardQueueWriteLock);
	PsUnlock(&KernelPtyKeyboardQueueReadLock);											// Unlock!
}

Boolean KernelPtyBackKeyboard(PProcessPty pty) {
	(Void)pty;																			// We don't want the unused parameter warning
	
	Boolean ret = False;
	
	PsLock(&KernelPtyKeyboardQueueReadLock);											// Lock
	PsLock(&KernelPtyKeyboardQueueWriteLock);
	
	if (KernelPtyKeyboardQueue.length != 0) {											// We can do it?
		ListRemove(&KernelPtyKeyboardQueue, 0);											// Yes, remove the first entry!
		ret = True;
	}
	
	PsUnlock(&KernelPtyKeyboardQueueWriteLock);
	PsUnlock(&KernelPtyKeyboardQueueReadLock);											// Unlock!
	
	return ret;
}

Boolean KernelPtyWriteKeyboard(PProcessPty pty, WChar data) {
	(Void)pty;																			// We don't want the unused parameter warning
	
	PsLock(&KernelPtyKeyboardQueueWriteLock);											// Lock
	
	if (data == '\n') {																	// New line?
		KernelPtyKeyboardNewLine = True;												// Yes, set the position!
		KernelPtyKeyboardNewLineLoc = KernelPtyKeyboardQueue.length;
	}
	
	QueueAdd(&KernelPtyKeyboardQueue, (PVoid)data);										// Add to the queue
	PsUnlock(&KernelPtyKeyboardQueueWriteLock);											// Unlock!
	
	return True;
}

Boolean KernelPtyRead(PProcessPty pty, UIntPtr len, PWChar buf) {
	(Void)pty;																			// We don't want the unused parameter warning
	
	if (len == 0) {
		return False;
	}
	
	while ((KernelPtyKeyboardQueue.length < len) && !KernelPtyKeyboardNewLine) {		// Let's fill the queue with the chars that we need
		PsSwitchTask(Null);
	}
	
	PsLock(&KernelPtyKeyboardQueueReadLock);											// Lock
	
	UIntPtr alen = KernelPtyKeyboardQueue.length;										// Save the avaliable length
	UIntPtr nlen = KernelPtyKeyboardNewLineLoc + 1;
	UIntPtr flen = KernelPtyKeyboardNewLine ? nlen : alen;
	
	for (UIntPtr i = 0; i < flen; i++) {												// Fill the buffer!
		buf[i] = (UInt8)QueueRemove(&KernelPtyKeyboardQueue);
	}
	
	if (KernelPtyKeyboardNewLine) {														// Put a string terminator (NUL) in the end!
		buf[nlen - 1] = 0;
	} else {
		buf[flen] = 0;
	}
	
	KernelPtyKeyboardNewLine = False;													// Unset the KernelPtyKeyboardNewLine
	
	PsUnlock(&KernelPtyKeyboardQueueReadLock);											// Unlock!
	
	return True;
}

Boolean KernelPtyWrite(PProcessPty pty, UIntPtr len, PWChar buf) {
	(Void)pty;																			// We don't want the unused parameter warning
	
	if (buf == Null || len == 0) {														// First, sanity check!
		return False;
	} else if (len == 1) {																// We have only one character?
		ConWriteCharacter(buf[0]);														// Yes!
	} else {
		ConWriteString((PWChar)buf);													// No, so it's a string...
	}
	
	return True;
}

PProcessPty KernelInitPty(Void) {
	PProcessPty pty = (PProcessPty)MemAllocate(sizeof(ProcessPty));						// Let's try to init it
	
	if (pty == Null) {
		return Null;
	}
	
	pty->krnl = True;																	// Setup everything
	pty->kbdclear = KernelPtyClearKeyboard;
	pty->kbdback = KernelPtyBackKeyboard;
	pty->kbdwrite = KernelPtyWriteKeyboard;
	pty->read = KernelPtyRead;
	pty->write = KernelPtyWrite;
	
	return pty;
}