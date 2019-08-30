// File author is Ítalo Lima Marconato Matias
//
// Created on December 11 of 2018, at 18:17 BRT
// Last edited on August 29 of 2019, at 15:00 BRT

#ifndef __CHICAGO_ARCH_PCI_H__
#define __CHICAGO_ARCH_PCI_H__

#include <chicago/types.h>

#define PCI_VENDOR_ID 0x00
#define PCI_DEVICE_ID 0x02
#define PCI_COMMAND 0x04
#define PCI_STATUS 0x06
#define PCI_REVISION_ID 0x08
#define PCI_PROG_IF 0x09
#define PCI_SUBCLASS 0x0A
#define PCI_CLASS 0x0B
#define PCI_CACHE_LINE_SIZE 0x0C
#define PCI_LATENCY_TIMER 0x0D
#define PCI_HEADER_TYPE 0x0E
#define PCI_BIST 0x0F
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1C
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN 0x3D
#define PCI_SECONDARY_BUS 0x19

#define PCI_VENDOR_INTEL 0x8086

#define PCI_DEVICE_82543GC 0x1004
#define PCI_DEVICE_82540EM 0x100E
#define PCI_DEVICE_82577LM 0x10EA
#define PCI_DEVICE_I217LM 0x153A

#define PCI_CLASS_MASS_STORAGE 0x01
#define PCI_SUBCLASS_IDE 0x01
#define PCI_SUBCLASS_SATA 0x06

#define PCI_CLASS_SERIAL_BUS 0x0C
#define PCI_SUBCLASS_USB 0x03

typedef Void (*PPCIInterruptHandlerFunc)(PVoid);

typedef struct {
	PVoid priv;
	PPCIInterruptHandlerFunc func;
} PCIInterruptHandler, *PPCIInterruptHandler;

typedef struct {
	UInt16 bus;
	UInt8 slot;
	UInt8 func;
	UInt16 vendor;
	UInt16 device;
	UInt8 progif;
	UInt8 class;
	UInt8 subclass;
	UInt32 bar0;
	UInt32 bar1;
	UInt32 bar2;
	UInt32 bar3;
	UInt32 bar4;
	UInt32 bar5;
	UInt8 iline;
	UInt8 ipin;
} PCIDevice, *PPCIDevice;

UInt8 PCIReadByte(PPCIDevice dev, UInt8 off);
UInt16 PCIReadWord(PPCIDevice dev, UInt8 off);
UInt32 PCIReadDWord(PPCIDevice dev, UInt8 off);
Void PCIWriteByte(PPCIDevice dev, UInt8 off, UInt8 val);
Void PCIWriteWord(PPCIDevice dev, UInt8 off, UInt16 val);
Void PCIWriteDWord(PPCIDevice dev, UInt8 off, UInt32 val);
Void PCIRegisterIRQHandler(PPCIDevice dev, PPCIInterruptHandlerFunc handler, PVoid priv);
PPCIDevice PCIFindDevice1(PUIntPtr last, UInt16 vendor, UInt16 device);
PPCIDevice PCIFindDevice2(PUIntPtr last, UInt8 class, UInt8 subclass);
Void PCIEnableBusMaster(PPCIDevice dev);
Void PCIInit(Void);

#endif		// __CHICAGO_ARCH_PCI_H__
