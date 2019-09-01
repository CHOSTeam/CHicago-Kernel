// File author is Ítalo Lima Marconato Matias
//
// Created on December 07 of 2018, at 10:41 BRT
// Last edited on September 01 of 2019, at 15:04 BRT

#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/panic.h>
#include <chicago/process.h>

Void ConsoleDeviceClearKeyboard(Void) {
	PProcessPty pty = PsGetPty();																					// Get this process pty
	
	if (pty != Null && pty->kbdclear != Null) {																		// And redirect to the->kbdclear function
		pty->kbdclear(pty);
	}
}

Boolean ConsoleDeviceBackKeyboard(Void) {
	PProcessPty pty = PsGetPty();																					// Get this process pty
	return (pty != Null && pty->kbdback != Null) ? pty->kbdback(pty) : False;										// And redirect to the ->kbdback function
}

Boolean ConsoleDeviceReadKeyboard(UIntPtr len, PWChar buf) {
	PProcessPty pty = PsGetPty();																					// Get this process pty
	return (pty != Null && pty->read != Null) ? pty->read(pty, len, buf) : False;									// And redirect to the ->read function
}

Boolean ConsoleDeviceWriteKeyboard(WChar data) {
	PProcessPty pty = PsGetPty();																					// Get this process pty
	return (pty != Null && pty->kbdwrite != Null) ? pty->kbdwrite(pty, data) : False;								// And redirect to the ->kbdwrite function
}

Boolean ConsoleDeviceRead(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	(Void)dev; (Void)off;																							// Avoid compiler's unused parameter warning
	return ConsoleDeviceReadKeyboard(len, (PWChar)buf);																// Redirect to ConsoleDeviceReadKeyboard
}

Boolean ConsoleDeviceWrite(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	(Void)dev; (Void)off;																							// Avoid compiler's unused parameter warning
	PProcessPty pty = PsGetPty();																					// Get this process pty
	return (pty != Null && pty->write != Null) ? pty->write(pty, len, (PWChar)buf) : False;							// And redirect to the ->write function
}

Void ConsoleDeviceInit(Void) {
	if (!FsAddDevice(L"Console", Null, ConsoleDeviceRead, ConsoleDeviceWrite, Null)) {								// Try to add the console device
		DbgWriteFormated("PANIC! Failed to add the Console device\r\n");											// Failed...
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}
