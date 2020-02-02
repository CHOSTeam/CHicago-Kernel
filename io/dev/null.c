// File author is Ítalo Lima Marconato Matias
//
// Created on July 15 of 2018, at 13:16 BRT
// Last edited on February 02 of 2020, at 10:51 BRT

#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/panic.h>

Status NullDeviceWrite(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr write) {
	(Void)dev; (Void)off; (Void)buf;																			// Avoid compiler's unused parameter warning
	*write = len;
	return STATUS_SUCCESS;
}

static Status NullDeviceMap(PDevice dev, UIntPtr off, UIntPtr addr,  UInt32 prot) {
	(Void)dev; (Void)off; (Void)addr; (Void)prot;																// Avoid compiler's unused parameter warning
	return STATUS_MAPFILE_FAILED;
}

static Status NullDeviceSync(PDevice dev, UIntPtr off, UIntPtr start, UIntPtr size) {
	(Void)dev; (Void)off; (Void)start; (Void)size;																// Avoid compiler's unused parameter warning
	return STATUS_SUCCESS;
}

Void NullDeviceInit(Void) {
	if (!FsAddDevice(L"Null", Null, Null, NullDeviceWrite, Null, NullDeviceMap, NullDeviceSync)) {				// Let's add ourself
		DbgWriteFormated("PANIC! Failed to add the Null device\r\n");											// Failed...
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}
