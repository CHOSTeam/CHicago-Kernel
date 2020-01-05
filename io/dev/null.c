// File author is Ítalo Lima Marconato Matias
//
// Created on July 15 of 2018, at 13:16 BRT
// Last edited on January 04 of 2020, at 18:05 BRT

#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/panic.h>

UIntPtr NullDeviceWrite(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	(Void)dev; (Void)off; (Void)len; (Void)buf;									// Avoid compiler's unused parameter warning
	return len;
}

Void NullDeviceInit(Void) {
	if (!FsAddDevice(L"Null", Null, Null, NullDeviceWrite, Null)) {				// Let's add ourself
		DbgWriteFormated("PANIC! Failed to add the Null device\r\n");			// Failed...
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}
