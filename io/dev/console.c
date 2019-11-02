// File author is Ítalo Lima Marconato Matias
//
// Created on December 07 of 2018, at 10:41 BRT
// Last edited on October 28 of 2019, at 10:29 BRT

#include <chicago/console.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/panic.h>

Boolean ConsoleDeviceWrite(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	(Void)dev; (Void)off;																							// Avoid compiler's unused parameter warning
	
	if (buf != Null && len != 0) {																					// Redirect to the ConWriteString function
		ConWriteString((PWChar)buf);
		return True;
	}
	
	return False;
}

Void ConsoleDeviceInit(Void) {
	if (!FsAddDevice(L"Console", Null, Null, ConsoleDeviceWrite, Null)) {											// Try to add the console device
		DbgWriteFormated("PANIC! Failed to add the Console device\r\n");											// Failed...
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}
