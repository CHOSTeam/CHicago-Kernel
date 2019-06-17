// File author is Ítalo Lima Marconato Matias
//
// Created on June 15 of 2019, at 09:55 BRT
// Last edited on June 16 of 2019, at 19:14 BRT

#include <chicago/arch/usb.h>

Void USBInit(Void) {
	UIntPtr i = 0;																									// Let's find all the USB controllers
	PPCIDevice dev = PCIFindDevice2(&i, PCI_CLASS_SERIAL_BUS, PCI_SUBCLASS_USB);
	
	while (dev != Null) {
		if (dev->progif == 0x00) {																					// UHCI controller
			UHCIInit(dev);
		}
		
		dev = PCIFindDevice2(&i, PCI_CLASS_SERIAL_BUS, PCI_SUBCLASS_USB);
	}
}
