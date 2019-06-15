// File author is Ítalo Lima Marconato Matias
//
// Created on June 15 of 2019, at 09:55 BRT
// Last edited on June 15 of 2019, at 10:52 BRT

#include <chicago/arch/usb.h>

#include <chicago/debug.h>

Void USBInit(Void) {
	UIntPtr i = 0;																									// Let's find all the USB controllers
	PPCIDevice dev = PCIFindDevice2(&i, PCI_CLASS_SERIAL_BUS, PCI_SUBCLASS_USB);
	
	while (dev != Null) {
		if (dev->progif == 0x00) {																					// UHCI controller
			UHCIInit(dev);
		} else if (dev->progif == 0x10) {																			// OHCI controller
			
		} else if (dev->progif == 0x20) {																			// EHCI controller
			
		} else if (dev->progif == 0x30) {																			// XHCI controller
			
		}
		
		dev = PCIFindDevice2(&i, PCI_CLASS_SERIAL_BUS, PCI_SUBCLASS_USB);
	}
}
