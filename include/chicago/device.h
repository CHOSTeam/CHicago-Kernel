// File author is Ítalo Lima Marconato Matias
//
// Created on July 14 of 2018, at 22:38 BRT
// Last edited on February 02 of 2020, at 10:48 BRT

#ifndef __CHICAGO_DEVICE_H__
#define __CHICAGO_DEVICE_H__

#include <chicago/status.h>

typedef struct DeviceStruct {
	PWChar name;
	PVoid priv;
	Status (*read)(struct DeviceStruct *, UIntPtr, UIntPtr, PUInt8, PUIntPtr);
	Status (*write)(struct DeviceStruct *, UIntPtr, UIntPtr, PUInt8, PUIntPtr);
	Status (*control)(struct DeviceStruct *, UIntPtr, PUInt8, PUInt8);
	Status (*map)(struct DeviceStruct *, UIntPtr, UIntPtr, UInt32);
	Status (*sync)(struct DeviceStruct *, UIntPtr, UIntPtr, UIntPtr);
} Device, *PDevice;

Void NullDeviceInit(Void);
Void ZeroDeviceInit(Void);
Void FrameBufferDeviceInit(Void);

Status FsReadDevice(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr read);
Status FsWriteDevice(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr write);
Status FsControlDevice(PDevice dev, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf);
Boolean FsAddDevice(PWChar name, PVoid priv, Status (*read)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*write)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*control)(PDevice, UIntPtr, PUInt8, PUInt8), Status (*map)(PDevice, UIntPtr, UIntPtr, UInt32), Status (*sync)(PDevice, UIntPtr, UIntPtr, UIntPtr));
Boolean FsAddHardDisk(PVoid priv, Status (*read)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*write)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*control)(PDevice, UIntPtr, PUInt8, PUInt8));
Boolean FsAddCdRom(PVoid priv, Status (*read)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*write)(PDevice, UIntPtr, UIntPtr, PUInt8, PUIntPtr), Status (*control)(PDevice, UIntPtr, PUInt8, PUInt8));
Boolean FsRemoveDevice(PWChar name);
PDevice FsGetDevice(PWChar name);
PDevice FsGetDeviceByID(UIntPtr id);
UIntPtr FsGetDeviceID(PWChar name);
Void FsSetBootDevice(PWChar name);
PWChar FsGetBootDevice(Void);
Void FsInitDevices(Void);

#endif		// __CHICAGO_DEVICE_H__
