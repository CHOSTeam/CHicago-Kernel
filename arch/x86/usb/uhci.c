// File author is Ítalo Lima Marconato Matias
//
// Created on June 15 of 2019, at 10:11 BRT
// Last edited on August 25 of 2019, at 17:48 BRT

#include <chicago/arch/pci.h>
#include <chicago/arch/port.h>
#include <chicago/arch/usb.h>

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/mm.h>
#include <chicago/string.h>
#include <chicago/timer.h>

static UInt32 UHCIReadRegister(PUHCIDevice dev, UInt8 reg) {
	if (reg == UHCI_SOFMOD) {																											// Read a single byte?
		return PortInByte(dev->base + reg);																								// Yes
	} else if (reg == UHCI_FRBASEADD) {																									// Read four bytes?
		return PortInLong(dev->base + reg);																								// Yes
	}
	
	return PortInWord(dev->base + reg);																									// Nope, read two bytes
}

static Void UHCIWriteRegister(PUHCIDevice dev, UInt8 reg, UInt32 val) {
	if (reg == UHCI_SOFMOD) {																											// Write a single byte?
		PortOutByte(dev->base + reg, (UInt8)val);																						// Yes
	} else if (reg == UHCI_FRBASEADD) {																									// Write four bytes?
		PortOutLong(dev->base + reg, val);																								// Yes
	} else {
		PortOutWord(dev->base + reg, (UInt16)val);																						// Nope, write two bytes
	}
}

static Void UHCISetPortScBit(PUHCIDevice dev, UInt8 port, UInt32 bit) {
	UHCIWriteRegister(dev, UHCI_PORTSC + (port * 2), UHCIReadRegister(dev, UHCI_PORTSC + (port * 2)) | bit);							// Get the current value, set the bit and write back
}

static Void UHCIClearPortScBit(PUHCIDevice dev, UInt8 port, UInt32 bit) {
	UHCIWriteRegister(dev, UHCI_PORTSC + (port * 2), UHCIReadRegister(dev, UHCI_PORTSC + (port * 2)) & ~bit);							// Get the current value, clear the bit and write back
}

static Boolean UHCIDetect(PUHCIDevice dev) {
	for (UInt32 i = 0; i < 5; i++) {																									// Let's try to do the global reset (5 times)
		UHCIWriteRegister(dev, UHCI_USBCMD, 0x04);																						// Set the GRESET bit in the USBCMD register
		TimerSleep(50);																													// Wait 50ms
		UHCIWriteRegister(dev, UHCI_USBCMD, 0x00);																						// Clear the GRESET bit in the USBCMD register
	}
	
	TimerSleep(10);																														// Wait more 10ms
	
	if ((UHCIReadRegister(dev, UHCI_USBCMD) != 0x00) || (UHCIReadRegister(dev, UHCI_USBSTS) != 0x20)) {									// Check if the registers are in their default state
		return False;																													// Nope :(
	}
	
	UHCIWriteRegister(dev, UHCI_USBSTS, 0xFF);																							// Clear the write-clear out of the USBSTS
	
	if (UHCIReadRegister(dev, UHCI_SOFMOD) != 0x40) {																					// Check if the SOFMOD register is in it default state
		return False;																													// ...
	}
	
	UHCIWriteRegister(dev, UHCI_USBCMD, 0x02);																							// Let's set the HCRESET bit in the USBCMD register
	TimerSleep(50);																														// Wait some time
	
	return (UHCIReadRegister(dev, UHCI_USBCMD) & 0x02) != 0x02;																			// And check if it's unset again
}

static Boolean UHCIResetPort(PUHCIDevice dev, UInt8 port) {
	UHCISetPortScBit(dev, port, UHCI_PORTSC_PR);																						// Set the PR bit in the PORTSC<port> register
	TimerSleep(50);																														// Wait 50ms
	UHCIClearPortScBit(dev, port, UHCI_PORTSC_PR);																						// Clear the PR bit in the PORTSC<port> register
	TimerSleep(10);																														// Wait 10ms
	
	for (UInt32 i = 0; i < 10; i++) {																									// Let's wait until the port is enabled again
		UInt16 val = UHCIReadRegister(dev, UHCI_PORTSC + (port * 2));																	// Read the PORTSC<port> register
		
		if ((val & UHCI_PORTSC_CCS) != UHCI_PORTSC_CCS) {																				// Nothing attached here?
			return True;																												// Yes, so let's just return
		} else if ((val & (UHCI_PORTSC_CSC | UHCI_PORTSC_PEC)) != 0) {																	// Check if the CSC or the PEC bit is set
			UHCIClearPortScBit(dev, port, UHCI_PORTSC_CSC | UHCI_PORTSC_PEC);															// Clear if it's set
		} else if ((val & UHCI_PORTSC_PE) == UHCI_PORTSC_PE) {																			// The PE bit is set?
			return True;																												// Yes :)
		} else {
			UHCISetPortScBit(dev, port, UHCI_PORTSC_PE);																				// Just set the PE bit
		}
		
		TimerSleep(10);																													// Wait 10ms before the next try/check
	}
	
	return False;
}

static Boolean UHCICheckPort(PUHCIDevice dev, UInt8 port) {
	if ((UHCIReadRegister(dev, UHCI_PORTSC + (port * 2)) & UHCI_PORTSC_RES) != UHCI_PORTSC_RES) {										// Check if the reserved bit is zero
		return False;																													// Yes, so this is not a valid port
	}
	
	UHCIClearPortScBit(dev, port, UHCI_PORTSC_RES);																						// Try to clear the reserved bit
	
	if ((UHCIReadRegister(dev, UHCI_PORTSC + (port * 2)) & UHCI_PORTSC_RES) != UHCI_PORTSC_RES) {										// Check if the reserved bit is zero
		return False;																													// Yes, so this is not a valid port
	}
	
	UHCISetPortScBit(dev, port, UHCI_PORTSC_RES);																						// Try to set the reserved bit
	
	if ((UHCIReadRegister(dev, UHCI_PORTSC + (port * 2)) & UHCI_PORTSC_RES) != UHCI_PORTSC_RES) {										// Check if the reserved bit is zero
		return False;																													// Yes, so this is not a valid port
	}
	
	UHCISetPortScBit(dev, port, 0x0A);																									// Try to set bits 3:1 to 1
	
	return (UHCIReadRegister(dev, UHCI_PORTSC + (port * 2)) & 0x0A) != 0x0A;															// Check if it got cleared, if it got, it's a valid port
}

static Boolean UHCIInitFrameList(PUHCIDevice dev) {
	(Void)dev;
	return True;
}

Void UHCIInit(PPCIDevice pdev) {
	PUHCIDevice dev = (PUHCIDevice)MemAllocate(sizeof(UHCIDevice));																		// Alloc the UHCI device struct
	
	if (dev == Null) {
		return;																															// Failed
	}
	
	dev->pdev = pdev;
	
	if ((pdev->bar4 & 0x01) != 0x01) {																									// We have the IO base?
		MemFree((UIntPtr)dev);
		return;
	}
	
	dev->base = pdev->bar4 & ~1;																										// Get the IO base
	
	PCIEnableBusMaster(pdev);																											// Enable bus mastering
	
	if (!UHCIDetect(dev)) {																												// Let's reset it and check if we really have a working USB host controller here
		MemFree((UIntPtr)dev);
		return;
	}
	
	if (!UHCIInitFrameList(dev)) {																										// Init the frame list
		MemFree((UIntPtr)dev);																											// Failed
		return;
	}
	
	UHCIWriteRegister(dev, UHCI_FRBASEADD, 0);																							// TODO: Setup the frame list
	UHCIWriteRegister(dev, UHCI_FRNUM, 0x00);																							// Clear the FRNUM register
	UHCIWriteRegister(dev, UHCI_SOFMOD, 0x40);																							// Set the SOFMOD register to 0x40, it should already be, but let's make sure that it is
	UHCIWriteRegister(dev, UHCI_USBINTR, 0x00);																							// Clear the USBINTR register
	UHCIWriteRegister(dev, UHCI_USBSTS, 0x1F);																							// Clear the USBSTS register
	UHCIWriteRegister(dev, UHCI_USBCMD, 0xC1);																							// Set the RS bit in the USBCMD register, this will finally start the controller
	dev->ports = 0;																														// Let's get the port count
	
	while (UHCICheckPort(dev, dev->ports)) {																							// Check if this port is valid
		if (!UHCIResetPort(dev, dev->ports)) {
			goto next;																													// Failed :(
		}
		
		UInt32 port = UHCIReadRegister(dev, UHCI_PORTSC + (dev->ports * 4));															// Read the port register, as we gonna check if the PE bit is set and also if this is an low speed device (LSD bit is check)
		
		if ((port & UHCI_PORTSC_PE) != UHCI_PORTSC_PE) {																				// We have any device here?
			goto next;																													// Nope
		}
		
		Boolean ls = (port & UHCI_PORTSC_LSD) == UHCI_PORTSC_LSD;																		// Get if this is an low speed device
		(Void)ls;
		
next:	dev->ports++;																													// And increase the amount of valid ports
	}
}
