// File author is Ítalo Lima Marconato Matias
//
// Created on June 15 of 2019, at 10:11 BRT
// Last edited on June 15 of 2019, at 15:18 BRT

#include <chicago/arch/pci.h>
#include <chicago/arch/port.h>
#include <chicago/arch/usb.h>

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/mm.h>
#include <chicago/timer.h>

static UInt32 UHCIReadRegister(PUHCIDevice dev, UInt8 reg) {
	if (dev == Null) {																													// Sanity check
		return 0;
	} else if (reg == UHCI_SOFMOD) {																									// Read a single byte?
		return PortInByte(dev->base + reg);																								// Yes
	} else if (reg == UHCI_FRBASEADD) {																									// Read four bytes?
		return PortInLong(dev->base + reg) & ~0xFFF;																					// Yes, let's also strip the first bits
	}
	
	return PortInWord(dev->base + reg);																									// Nope, read two bytes
}

static Void UHCIWriteRegister(PUHCIDevice dev, UInt8 reg, UInt32 val) {
	if (dev == Null) {																													// Sanity check
		return;
	} else if (reg == UHCI_SOFMOD) {																									// Write a single byte?
		PortOutByte(dev->base + reg, (UInt8)val);																						// Yes
	} else if (reg == UHCI_FRBASEADD) {																									// Write four bytes?
		PortOutLong(dev->base + reg, MmGetPhys(val & ~0xFFF));																			// Yes, let's get the physical address and also strip the first bits from it
	} else {
		PortOutWord(dev->base + reg, (UInt16)val);																						// Nope, write two bytes
	}
}

static Void UHCIHandler(PVoid priv) {
	Volatile PUHCIDevice dev = (PUHCIDevice)priv;
	(Void)dev;
}

static Boolean UHCIDetect(PUHCIDevice dev) {
	UHCIWriteRegister(dev, UHCI_USBCMD, 0x04);																							// Set the GRESET bit in the USBCMD register
	TimerSleep(50);																														// Wait 10ms
	UHCIWriteRegister(dev, UHCI_USBCMD, 0x00);																							// Clear the GRESET bit in the USBCMD register
	
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

Void UHCIInit(PPCIDevice pdev) {
	PUHCIDevice dev = (PUHCIDevice)MemAllocate(sizeof(UHCIDevice));																		// Alloc the UHCI device struct
	
	if (dev == Null) {
		return;																															// Failed
	}
	
	dev->pdev = pdev;
	
	DbgWriteFormated("[x86] Found UHCI controller at %d:%d:%d\r\n", pdev->bus, pdev->slot, pdev->func);
	
	if ((pdev->bar4 & 0x01) != 0x01) {																									// We have the IO base?
		DbgWriteFormated("      This device doesn't have the IO base\r\n");																// No, so let's just cancel and go to the next controller
		return;
	}
	
	dev->base = pdev->bar4 & ~1;																										// Get the IO base
	
	DbgWriteFormated("      The IO base is 0x%x\r\n", dev->base);
	
	PCIEnableBusMaster(pdev);																											// Enable bus mastering
	DbgWriteFormated("      Enabled bus matering\r\n");
	
	PCIWriteDWord(pdev, 0x34, 0);																										// Write 0 to dwords 0x34 and 0x38
	PCIWriteDWord(pdev, 0x38, 0);
	DbgWriteFormated("      Wrote 0 to dwords 0x34 and 0x38 of this device's PCI configuration space\r\n");
	
	PCIWriteDWord(pdev, UHCI_LEGACY_SUPPORT, 0x8F00);																					// Disable legacy support
	DbgWriteFormated("      Disabled legacy support\r\n");
	
	if (!UHCIDetect(dev)) {																												// Let's reset it and check if we really have a working USB host controller here
		DbgWriteFormated("      Couldn't reset and check the host controller\r\n");														// Nope, we don't have :(
		MemFree((UIntPtr)dev);
		return;
	}
	
	DbgWriteFormated("      Reseted and checked the host controller\r\n");
	
	PCIRegisterIRQHandler(pdev, UHCIHandler, dev);																						// Register the IRQ handler
	DbgWriteFormated("      Registered the IRQ handler\r\n");
	
	UHCIWriteRegister(dev, UHCI_USBINTR, 0x0F);																							// Set all the four bits from the USBINTR register
	DbgWriteFormated("      Set all the four bits of USBINTR\r\n");
	
	UHCIWriteRegister(dev, UHCI_FRNUM, 0x00);																							// Clear the FRNUM register
	DbgWriteFormated("      Cleared the FRNUM register\r\n");
	
	// TODO: Create a stack frame, the items, and set it
	
	UHCIWriteRegister(dev, UHCI_SOFMOD, 0x40);																							// Set the SOFMOD register to 0x40, it should already be, but let's make sure that it is
	DbgWriteFormated("      Set the SOFMOD register to 0x40\r\n");
	
	UHCIWriteRegister(dev, UHCI_USBSTS, 0xFFFF);																						// Clear the USBSTS register
	DbgWriteFormated("      Cleared the USBSTS register\r\n");
	
//	UHCIWriteRegister(dev, UHCI_USBCMD, 0x41);																							// Set the RS and the CF bits in the USBCMD register, this will finally start the controller, BUT FOR THIS WE NEED THE STACK FRAME, SO IT'S COMMENTED FOR NOW
//	DbgWriteFormated("      Initialized the UHCI controller\r\n");
}
