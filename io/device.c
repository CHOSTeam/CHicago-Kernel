// File author is Ítalo Lima Marconato Matias
//
// Created on July 14 of 2018, at 22:35 BRT
// Last edited on January 18 of 2020, at 15:22 BRT

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/list.h>
#include <chicago/panic.h>
#include <chicago/process.h>
#include <chicago/string.h>

PList FsDeviceList = Null;
PWChar FsBootDevice = Null;

Status FsReadDevice(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr read) {
	if (dev == Null || len == 0 || buf == Null || read == Null) {				// Sanity check
		return STATUS_INVALID_ARG;
	} else if (dev->read != Null) {												// We can call the device's function?
		return dev->read(dev, off, len, buf, read);								// Yes!
	} else {
		return STATUS_CANT_READ;												// Nope...
	}
}

Status FsWriteDevice(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr write) {
	if (dev == Null || len == 0 || buf == Null || write == Null) {				// Sanity check
		return STATUS_INVALID_ARG;
	} else if (dev->write != Null) {											// We can call the device's function?
		return dev->write(dev, off, len, buf, write);							// Yes!
	} else {
		return STATUS_CANT_WRITE;												// Nope...
	}
}

Status FsControlDevice(PDevice dev, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf) {
	if (dev == Null) {															// Sanity check
		return STATUS_INVALID_ARG;
	} else if (dev->control != Null) {											// We can call the device's function?
		return dev->control(dev, cmd, ibuf, obuf);								// Yes!
	} else {
		return STATUS_CANT_CONTROL;												// Nope...
	}
}

Boolean FsAddDevice(PWChar name, PVoid priv, Status (*read)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*write)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*control)(PDevice, UIntPtr, PUInt8, PUInt8)) {
	if (FsDeviceList == Null) {													// Device list was initialized?
		return False;															// No...
	}
	
	PDevice dev = (PDevice)MemAllocate(sizeof(Device));							// Allocate memory for the dev struct
	
	if (dev == Null) {															// Failed?
		return False;															// Yes...
	}
	
	dev->name = name;
	dev->priv = priv;
	dev->read = read;
	dev->write = write;
	dev->control = control;
	
	if (!ListAdd(FsDeviceList, dev)) {											// Try to add to the list
		MemFree((UIntPtr)dev);													// Failed, so let's free the dev struct
		return False;															// And return False
	}
	
	return True;
}

Boolean FsAddHardDisk(PVoid priv, Status (*read)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*write)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*control)(PDevice, UIntPtr, PUInt8, PUInt8)) {
	if (FsDeviceList == Null) {													// Device list was initialized?
		return False;															// No...
	}
	
	static UIntPtr count = 0;
	UIntPtr nlen = StrFormat(Null, L"HardDisk%d", count++);						// Get the length of the name
	PWChar name = (PWChar)MemAllocate(nlen);									// Alloc space for the name
	
	if (name == Null) {
		return False;															// Failed
	}
	
	StrFormat(name, L"HardDisk%d", count - 1);									// Format the name string!
	
	if (!FsAddDevice(name, priv, read, write, control)) {						// Try to add it!
		MemFree((UIntPtr)name);													// Failed, free the name and return
		return False;
	}
	
	return True;
}

Boolean FsAddCdRom(PVoid priv, Status (*read)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*write)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*control)(PDevice, UIntPtr, PUInt8, PUInt8)) {
	if (FsDeviceList == Null) {													// Device list was initialized?
		return False;															// No...
	}
	
	static UIntPtr count = 0;
	UIntPtr nlen = StrFormat(Null, L"CdRom%d", count++);						// Get the length of the name
	PWChar name = (PWChar)MemAllocate(nlen);									// Alloc space for the name
	
	if (name == Null) {
		return False;															// Failed
	}
	
	StrFormat(name, L"CdRom%d", count - 1);										// Format the name string!
	
	if (!FsAddDevice(name, priv, read, write, control)) {						// Try to add it!
		MemFree((UIntPtr)name);													// Failed, free the name and return
		return False;
	}
	
	return True;
}

Boolean FsRemoveDevice(PWChar name) {
	if (FsDeviceList == Null) {													// Device list was initialized?
		return False;															// No...
	}
	
	PDevice dev = FsGetDevice(name);											// Try to get the device
	UIntPtr idx = 0;
	Boolean found = False;
	
	if (dev == Null) {															// Failed?
		return False;															// Yes, so this device doesn't exists
	}
	
	for (; !found && idx < FsDeviceList->length; idx++) {
		if (ListGet(FsDeviceList, idx) == dev) {								// Found?
			found = True;														// Yes!
		}
	}
	
	if (!found) {																// Found?
		return False;															// No (What? But we found it earlier)
	}
	
	if (ListRemove(FsDeviceList, idx) == Null) {								// Try to remove it
		return False;															// Failed...
	}
	
	if (FsGetDevice(name) != Null) {											// We really removed it?
		return False;															// No (for some reason)
	}
	
	MemFree((UIntPtr)(dev->name));												// Free the name
	MemFree((UIntPtr)dev);														// And the dev struct
	
	return True;																// AND RETURN TRUE!
}

PDevice FsGetDevice(PWChar name) {
	if (FsDeviceList == Null) {													// Device list was initialized?
		return Null;															// No...
	}
	
	ListForeach(FsDeviceList, i) {												// Let's do an for in each (foreach) list entry
		PWChar dname = ((PDevice)(i->data))->name;								// Save the entry name
		
		if (StrGetLength(dname) != StrGetLength(name)) {						// Same length?
			continue;															// No, so we don't even need to compare this entry
		} else if (StrCompare(dname, name)) {									// dname == name?
			return (PDevice)(i->data);											// YES!
		}
	}
	
	return Null;
}

PDevice FsGetDeviceByID(UIntPtr id) {
	if (FsDeviceList == Null) {
		return Null;
	} else if (id >= FsDeviceList->length) {
		return Null;
	}
	
	return (PDevice)ListGet(FsDeviceList, id);
}

UIntPtr FsGetDeviceID(PWChar name) {
	if (FsDeviceList == Null) {
		return 0;
	}
	
	UIntPtr idx = 0;
	
	ListForeach(FsDeviceList, i) {
		PWChar dname = ((PDevice)(i->data))->name;
		
		if (StrGetLength(dname) != StrGetLength(name)) {
			idx++;
			continue;
		} else if (StrCompare(dname, name)) {
			return idx;
		} else {
			idx++;
		}
	}
	
	return (UIntPtr)-1;
}

Void FsSetBootDevice(PWChar name) {
	if (FsGetDevice(name) != Null) {												// Device with this name exists?
		FsBootDevice = name;														// Yes! So set it as boot device (just like the user asked)
	}
}

PWChar FsGetBootDevice(Void) {
	return FsBootDevice;
}

Void FsInitDeviceList(Void) {
	FsDeviceList = ListNew(True);													// Try to init the device list
	
	if (FsDeviceList == Null) {														// Failed?
		DbgWriteFormated("PANIC! Couldn't init the device list\r\n");				// Yes, so halt
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}

Void FsInitDevices(Void) {
	NullDeviceInit();																// Add the Null device
	ZeroDeviceInit();																// Add the Zero device
	FrameBufferDeviceInit();														// Add the FrameBuffer device
}
