// File author is Ítalo Lima Marconato Matias
//
// Created on June 15 of 2019, at 09:47 BRT
// Last edited on June 15 of 2019, at 14:37 BRT

#ifndef __CHICAGO_ARCH_USB_H__
#define __CHICAGO_ARCH_USB_H__

#include <chicago/arch/pci.h>

#define UHCI_LEGACY_SUPPORT 0xC0
#define UHCI_USBCMD 0x00
#define UHCI_USBSTS 0x02
#define UHCI_USBINTR 0x04
#define UHCI_FRNUM 0x06
#define UHCI_FRBASEADD 0x08
#define UHCI_SOFMOD 0x0C
#define UHCI_RESERVED 0x0D
#define UHCI_PORTSC 0x10

typedef struct {
	PPCIDevice pdev;
	UInt32 base;
} UHCIDevice, *PUHCIDevice;

Void UHCIInit(PPCIDevice pdev);
Void USBInit(Void);

#endif		// __CHICAGO_ARCH_USB_H__
