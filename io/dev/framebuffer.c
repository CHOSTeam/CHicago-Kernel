// File author is Ítalo Lima Marconato Matias
//
// Created on July 18 of 2018, at 22:24 BRT
// Last edited on January 04 of 2020, at 17:59 BRT

#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/display.h>
#include <chicago/panic.h>
#include <chicago/string.h>

UIntPtr FrameBufferDeviceRead(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	(Void)dev;																												// Avoid compiler's unused parameter warning
	
	if ((off + len) > (DispGetWidth() * DispGetHeight() * DispGetBytesPerPixel())) {										// Too high address?
		return 0;																											// Yes!
	} else {
		StrCopyMemory(buf, (PVoid)(DispGetFrameBuffer() + off), len);														// No, so let's read from the real framebuffer!
		return len;
	}
}

UIntPtr FrameBufferDeviceWrite(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	(Void)dev;																												// Avoid compiler's unused parameter warning
	
	if ((off + len) > (DispGetWidth() * DispGetHeight() * DispGetBytesPerPixel())) {										// Too high address?
		return 0;																											// Yes...
	} else {
		StrCopyMemory((PVoid)(DispGetFrameBuffer() + off), buf, len);														// No, so let's write to the real framebuffer!
		return len;
	}
}

Boolean FrameBufferDeviceControl(PDevice dev, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf) {
	(Void)dev; (Void)ibuf;																									// Avoid compiler's unused parameter warning
	
	PUInt32 out = (PUInt32)obuf;
	
	if (cmd == 0) {																											// Get bytes per pixel
		*out = DispGetBytesPerPixel();
	} else if (cmd == 1) {																									// Get width
		*out = DispGetWidth();
	} else if (cmd == 2) {																									// Get height
		*out = DispGetHeight();
	} else {
		return False;																										// ...
	}
	
	return True;
}

Void FrameBufferDeviceInit(Void) {
	if (!FsAddDevice(L"FrameBuffer", Null, FrameBufferDeviceRead, FrameBufferDeviceWrite, FrameBufferDeviceControl)) {		// Let's add ourself
		DbgWriteFormated("PANIC! Failed to add the FrameBuffer device\r\n");												// Failed
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}
