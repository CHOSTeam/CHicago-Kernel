// File author is Ítalo Lima Marconato Matias
//
// Created on June 15 of 2019, at 09:47 BRT
// Last edited on June 18 of 2019, at 13:41 BRT

#ifndef __CHICAGO_ARCH_USB_H__
#define __CHICAGO_ARCH_USB_H__

#include <chicago/arch/pci.h>

#include <chicago/list.h>

#define UHCI_USBCMD 0x00
#define UHCI_USBSTS 0x02
#define UHCI_USBINTR 0x04
#define UHCI_FRNUM 0x06
#define UHCI_FRBASEADD 0x08
#define UHCI_SOFMOD 0x0C
#define UHCI_RESERVED 0x0D
#define UHCI_PORTSC 0x10
#define UHCI_LEGSUP 0xC0
#define UHCI_PORTSC_CCS (1 << 0)
#define UHCI_PORTSC_CSC (1 << 1)
#define UHCI_PORTSC_PE (1 << 2)
#define UHCI_PORTSC_PEC (1 << 3)
#define UHCI_PORTSC_RD (1 << 6)
#define UHCI_PORTSC_RES (1 << 7)
#define UHCI_PORTSC_LSD (1 << 8)
#define UHCI_PORTSC_PR (1 << 9)
#define UHCI_PORTSC_S (1 << 12)
#define UHCI_PID_IN 0x69
#define UHCI_PID_OUT 0xE1
#define UHCI_PID_SETUP 0x2D

typedef struct {
	UInt32 lp;
	UInt32 cs;
	UInt32 token;
	UInt32 buf;
	UInt8 used;
	UInt32 bufvirt;
	UInt8 unused[11];
} Packed UHCITD, *PUHCITD;

typedef struct UHCIQHStruct {
	UInt32 qhlp;
	UInt32 qelp;
	UInt8 used;
	UInt8 unused[7];
} Packed UHCIQH, *PUHCIQH;

typedef struct {
	UInt8 length;
	UInt8 type;
	UInt16 rnum;
	UInt8 class;
	UInt8 sclass;
	UInt8 protocol;
	UInt8 maxps;
	UInt16 vendorid;
	UInt16 productid;
	UInt16 release;
	UInt8 manufact;
	UInt8 product;
	UInt8 serial;
	UInt8 configs;
} Packed UHCIDD, *PUHCIDD;

typedef struct {
	UInt8 type;
	UInt8 request;
	UInt16 value;
	UInt16 index;
	UInt16 length;
} Packed UHCIDRP, *PUHCIDRP;

typedef struct {
	UInt16 fcount;
	UHCITD descs[8];
} UHCITE, *PUHCITE;

typedef struct {
	UInt16 fcount;
	UHCIQH heads[8];
} UHCIQE, *PUHCIQE;

typedef struct {
	PPCIDevice pdev;
	PUInt32 frames;
	PUHCIQH qbase;
	PUHCIQH qlast;
	PList qheads;
	PList tdescs;
	UInt32 base;
	UInt8 ports;
} UHCIDevice, *PUHCIDevice;

Void UHCIInit(PPCIDevice pdev);
Void USBInit(Void);

#endif		// __CHICAGO_ARCH_USB_H__
