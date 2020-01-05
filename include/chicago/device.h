// File author is Ítalo Lima Marconato Matias
//
// Created on July 14 of 2018, at 22:38 BRT
// Last edited on January 04 of 2020, at 17:58 BRT

#ifndef __CHICAGO_DEVICE_H__
#define __CHICAGO_DEVICE_H__

#include <chicago/types.h>

typedef struct DeviceStruct {
	PWChar name;
	PVoid priv;
	UIntPtr (*read)(struct DeviceStruct *, UIntPtr, UIntPtr, PUInt8);
	UIntPtr (*write)(struct DeviceStruct *, UIntPtr, UIntPtr, PUInt8);
	Boolean (*control)(struct DeviceStruct *, UIntPtr, PUInt8, PUInt8);
} Device, *PDevice;

Void NullDeviceInit(Void);
Void ZeroDeviceInit(Void);
Void FrameBufferDeviceInit(Void);

UIntPtr FsReadDevice(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf);
UIntPtr FsWriteDevice(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf);
Boolean FsControlDevice(PDevice dev, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf);
Boolean FsAddDevice(PWChar name, PVoid priv, UIntPtr (*read)(PDevice, UIntPtr, UIntPtr, PUInt8), UIntPtr (*write)(PDevice, UIntPtr, UIntPtr, PUInt8), Boolean (*control)(PDevice, UIntPtr, PUInt8, PUInt8));
Boolean FsAddHardDisk(PVoid priv, UIntPtr (*read)(PDevice, UIntPtr, UIntPtr, PUInt8), UIntPtr (*write)(PDevice, UIntPtr, UIntPtr, PUInt8), Boolean (*control)(PDevice, UIntPtr, PUInt8, PUInt8));
Boolean FsAddCdRom(PVoid priv, UIntPtr (*read)(PDevice, UIntPtr, UIntPtr, PUInt8), UIntPtr (*write)(PDevice, UIntPtr, UIntPtr, PUInt8), Boolean (*control)(PDevice, UIntPtr, PUInt8, PUInt8));
Boolean FsRemoveDevice(PWChar name);
PDevice FsGetDevice(PWChar name);
PDevice FsGetDeviceByID(UIntPtr id);
UIntPtr FsGetDeviceID(PWChar name);
Void FsSetBootDevice(PWChar name);
PWChar FsGetBootDevice(Void);
Void FsInitDeviceList(Void);
Void FsInitDevices(Void);

#endif		// __CHICAGO_DEVICE_H__
