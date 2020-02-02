// File author is Ítalo Lima Marconato Matias
//
// Created on July 15 of 2018, at 13:19 BRT
// Last edited on February 02 of 2020, at 10:53 BRT

#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/string.h>

Status ZeroDeviceRead(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr read) {
	(Void)dev; (Void)off;																						// Avoid compiler's unused parameter warning
	StrSetMemory(buf, 0, len);																					// Just fill the buffer with zeroes (this is what Zero device does)
	*read = len;
	return STATUS_SUCCESS;
}

Status ZeroDeviceWrite(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr write) {
	(Void)dev; (Void)off; (Void)buf;																			// Avoid compiler's unused parameter warning
	*write = len;
	return STATUS_SUCCESS;
}

static Status ZeroDeviceMap(PDevice dev, UIntPtr off, UIntPtr addr, UInt32 prot) {
	(Void)dev; (Void)off;																						// Avoid compiler's unused parameter warning
	
	Status status = MmPageFaultDoAOR(addr, prot);																// Just AOR it
	
	if (status == STATUS_SUCCESS) {
		StrSetMemory((PUInt8)addr, 0, MM_PAGE_SIZE);															// And clean it
	}
	
	return status;
}

static Status ZeroDeviceSync(PDevice dev, UIntPtr off, UIntPtr addr, UIntPtr size) {
	(Void)dev; (Void)off; (Void)addr; (Void)size;																// Avoid compiler's unused parameter warning
	return STATUS_SUCCESS;
}

Void ZeroDeviceInit(Void) {
	if (!FsAddDevice(L"Zero", Null, ZeroDeviceRead, ZeroDeviceWrite, Null, ZeroDeviceMap, ZeroDeviceSync)) {	// Let's add ourself
		DbgWriteFormated("PANIC! Failed to add the Zero device\r\n");											// Failed...
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}
