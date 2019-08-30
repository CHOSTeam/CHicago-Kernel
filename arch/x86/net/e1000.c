// File author is Ítalo Lima Marconato Matias
//
// Created on December 11 of 2018, at 19:40 BRT
// Last edited on August 29 of 2019, at 15:00 BRT

#include <chicago/arch/e1000.h>
#include <chicago/arch/pci.h>
#include <chicago/arch/port.h>

#include <chicago/alloc.h>
#include <chicago/mm.h>
#include <chicago/string.h>

static UIntPtr E1000AllocCont(UIntPtr amount, PUIntPtr virt) {
	*virt = MemAAllocate(amount, MM_PAGE_SIZE);										// Alloc some space
	
	if (*virt == 0) {
		return 0;																	// Failed :(
	}
	
	return MmGetPhys(*virt);
}

static Void E1000WriteCommand(PE1000Device dev, UInt16 addr, UInt32 val) {
	if (dev->use_io) {																// Use the IO ports?
		PortOutLong(dev->io_base, addr);											// Yes
		PortOutLong(dev->io_base + 4, val);
	} else {
		*((Volatile PUInt32)(dev->mem_base + addr)) = val;							// No
	}
}

static UInt32 E1000ReadCommand(PE1000Device dev, UInt16 addr) {
	if (dev->use_io) {																// Use the IO ports?
		PortOutLong(dev->io_base, addr);											// Yes
		return PortInLong(dev->io_base + 4);
	} else {
		return *((Volatile PUInt32)(dev->mem_base + addr));							// No
	}
}

static UInt32 E1000ReadEEPROM(PE1000Device dev, UInt8 addr) {
	UInt32 data = 0;
	
	if (dev->eeprom) {																// We have the EEPROM?
		E1000WriteCommand(dev, 0x14, 1 | (addr << 8));								// Yes :)
		while (((data = E1000ReadCommand(dev, 0x14)) & 0x10) != 0x10);
	} else {
		E1000WriteCommand(dev, 0x14, 1 | (addr << 2));								// Nope...
		while (((data = E1000ReadCommand(dev, 0x14)) & 0x01) != 0x01);
	}
	
	return (data >> 16) & 0xFFFF;
}

static Void E1000Send(PVoid ndev, UIntPtr len, PUInt8 data) {
	Volatile PE1000Device dev = (PE1000Device)ndev;
	UInt8 old = dev->tx_cur;
	
	StrCopyMemory((PUInt8)(dev->tx_descs_buffs[old]), data, len);
	
	dev->tx_descs[old].length = len;
	dev->tx_descs[old].cmd = 0x1B;
	dev->tx_descs[old].status = 0;
	dev->tx_cur = (dev->tx_cur + 1) % 8;
	
	E1000WriteCommand(dev, 0x3818, dev->tx_cur);
	
	while (!(dev->tx_descs[old].status & 0xFF)) ;
}

static Void E1000Handler(PVoid priv) {
	Volatile PE1000Device dev = (PE1000Device)priv;
	UInt32 status = E1000ReadCommand(dev, 0xC0);									// Read the status
	
	E1000WriteCommand(dev, 0xD0, 1);												// Without this, the card may spam interrupts...
	
	if (status & 0x04) {															// Linkup
		UInt32 val = E1000ReadCommand(dev, 0);
		E1000WriteCommand(dev, 0, val | 0x40);
	} else if (status & 0x80) {														// Ok, handle receive!!!
		while ((dev->rx_descs[dev->rx_cur].status & 0x01) == 0x01) {
			UInt16 old = dev->rx_cur;
			UIntPtr len = dev->rx_descs[old].length;
			
			PUInt8 buf = (PUInt8)MemAllocate(len);									// Try to alloc some space
			
			if (buf != Null) {
				StrCopyMemory(buf, (PUInt8)dev->rx_descs_buffs[old], len);			// Ok, copy the data
				NetDevicePushPacket(dev->ndev, buf);								// And now our net layer should handle it!
			}
			
			dev->rx_descs[old].status = 0;
			dev->rx_cur = (dev->rx_cur + 1) % 32;
			
			E1000WriteCommand(dev, 0x2818, old);
		}
	}
}

static Void E1000InitInt(PPCIDevice pdev) {
	PE1000Device dev = (PE1000Device)MemAllocate(sizeof(E1000Device));				// Alloc space for our priv struct
	
	if (dev == Null) {
		return;																		// Failed...
	}
	
	if ((pdev->bar0 & 0x01) == 0x01) {												// We have IO base?
		dev->use_io = True;															// Yes :)
		dev->io_base = pdev->bar0 & ~1;
	} else {
		dev->use_io = False;														// No, so we're going to use the mem base
		dev->mem_base = MemAAllocate(0x10000, MM_PAGE_SIZE);						// Alloc some virt space
		
		if (dev->mem_base == 0) {
			MemFree((UIntPtr)dev);													// Failed :(
			return;
		}
		
		UIntPtr phys = pdev->bar0 & ~3;												// Let's map the phys address to the new virt address
		
		for (UIntPtr i = 0; i < 0x10000; i += MM_PAGE_SIZE) {
			MmDereferencePage(MmGetPhys(dev->mem_base + i));						// MemAAllocate allocated some phys addr as well, free it
			
			if (!MmMap(dev->mem_base + i, phys + i, MM_MAP_KDEF)) {
				MemAFree((UIntPtr)dev->mem_base);									// Failed... unmap everything
				MemFree((UIntPtr)dev);												// And free our device
				
				return;
			}
		}
	}
	
	PCIEnableBusMaster(pdev);														// Let's enable bus mastering!
	
	E1000WriteCommand(dev, 0x14, 0x01);												// Let's see if our E1000 have EEPROM!
	
	dev->eeprom = False;															// For now, set it to False (we still don't know!)
	
	for (UInt32 i = 0; i < 1000 && !dev->eeprom; i++) {
		UInt32 val = E1000ReadCommand(dev, 0x14);									// Let's try to discover reading the status field!
		
		if ((val & 0x10) == 0x10) {													// We have?
			dev->eeprom = True;														// Yes :)
		}
	}
	
	if (dev->eeprom) {																// Let's read the MAC Address!
		UInt32 tmp = E1000ReadEEPROM(dev, 0);										// We can use the EEPROM!
		
		dev->mac_address[0] = (UInt8)(tmp & 0xFF);
		dev->mac_address[1] = (UInt8)(tmp >> 8);
		
		tmp = E1000ReadEEPROM(dev, 1);
		
		dev->mac_address[2] = (UInt8)(tmp & 0xFF);
		dev->mac_address[3] = (UInt8)(tmp >> 8);
		
		tmp = E1000ReadEEPROM(dev, 2);
		
		dev->mac_address[4] = (UInt8)(tmp & 0xFF);
		dev->mac_address[5] = (UInt8)(tmp >> 8);
	} else {
		PUInt8 mac8 = (PUInt8)(dev->mem_base + 0x5400);								// We can't use the EEPROM :(
		PUInt32 mac32 = (PUInt32)(dev->mem_base + 0x5400);							// But probably it is in the address 0x5400 (from the mem base)
		
		if (mac32[0] == 0) {
			MemAFree((UIntPtr)dev->mem_base);										// Ok, this isn't right... we failed, unmap everything
			MemFree((UIntPtr)dev);													// And free our device
			
			return;
		}
		
		for (UInt32 i = 0; i < 6; i++) {
			dev->mac_address[i] = mac8[i];
		}
	}
	
	dev->ndev = NetAddDevice(dev, dev->mac_address, E1000Send);						// Add this network device
	
	if (dev->ndev == Null) {
		MemAFree((UIntPtr)dev->mem_base);											// Ok, this isn't right... we failed, unmap everything
		MemFree((UIntPtr)dev);														// And free our device
		
		return;
	}
	
	dev->rx_descs_phys = E1000AllocCont(0x1000, (PUIntPtr)(&dev->rx_descs));		// Let's alloc the physical (and the virtual) address of the receive buffer
	
	if (dev->rx_descs_phys == 0) {
		NetRemoveDevice(dev->ndev);													// We failed
		MemAFree((UIntPtr)dev->mem_base);											// Unmap everything
		MemFree((UIntPtr)dev);														// And free our device
		
		return;
	}
	
	for (UInt32 i = 0; i < 32; i++) {
		dev->rx_descs[i].addr = E1000AllocCont(0x3000, &dev->rx_descs_buffs[i]);	// Alloc the phys/virt address of this receive desc
		
		if (dev->rx_descs[i].addr == 0) {
			for (UIntPtr j = 0; j < i; j++) {										// We failed, unmap everything
				MemAFree(dev->rx_descs_buffs[j]);
			}
			
			MemAFree((UIntPtr)dev->rx_descs);
			NetRemoveDevice(dev->ndev);												// Remove the network device
			MemAFree((UIntPtr)dev->mem_base);
			MemFree((UIntPtr)dev);													// And free our device

			return;
		}
		
		dev->rx_descs[i].status = 0;
	}
	
	dev->tx_descs_phys = E1000AllocCont(0x1000, (PUIntPtr)(&dev->tx_descs));		// And alloc the phys/virt address of the transmit buffer
	
	if (dev->tx_descs_phys == 0) {
		for (UIntPtr i = 0; i < 32; i++) {											// We failed, unmap everything
			MemAFree(dev->rx_descs_buffs[i]);
		}
		
		MemAFree((UIntPtr)dev->rx_descs);
		NetRemoveDevice(dev->ndev);													// Remove the network device
		MemAFree((UIntPtr)dev->mem_base);
		MemFree((UIntPtr)dev);														// And free our device
		
		return;
	}
	
	for (UInt32 i = 0; i < 8; i++) {
		dev->tx_descs[i].addr = E1000AllocCont(0x3000, &dev->tx_descs_buffs[i]);	// Alloc the phys/virt address of this transmit desc
		
		if (dev->tx_descs[i].addr == 0) {
			for (UIntPtr j = 0; j < i; j++) {										// We failed, unmap everything
				MemAFree(dev->tx_descs_buffs[j]);
			}
			
			MemAFree((UIntPtr)dev->tx_descs);
			
			for (UIntPtr j = 0; j < 32; j++) {
				MemAFree(dev->rx_descs_buffs[j]);
			}
			
			MemAFree((UIntPtr)dev->rx_descs);
			NetRemoveDevice(dev->ndev);												// Remove the network device
			MemAFree((UIntPtr)dev->mem_base);
			MemFree((UIntPtr)dev);													// And free our device

			return;
		}
		
		dev->tx_descs[i].cmd = 0;
		dev->tx_descs[i].status = 1;
	}
	
	UInt32 val = E1000ReadCommand(dev, 0);											// Linkup
	E1000WriteCommand(dev, 0, val | 0x40);
	
	for (UInt32 i = 0; i < 0x80; i++) {
		E1000WriteCommand(dev, 0x5200 + (i * 4), 0);
	}
	
	dev->rx_cur = dev->tx_cur = 0;
	
	PCIRegisterIRQHandler(pdev, E1000Handler, dev);									// Register the IRQ handler
	E1000WriteCommand(dev, 0xD0, 0x1F6DC);											// Enable interrupts
	E1000WriteCommand(dev, 0xD0, 0xFB);
	E1000ReadCommand(dev, 0xC0);
	E1000WriteCommand(dev, 0x2800, dev->rx_descs_phys);								// Pass the physical address (and some other informations) of the receive buffer
	E1000WriteCommand(dev, 0x2804, 0);
	E1000WriteCommand(dev, 0x2808, 512);
	E1000WriteCommand(dev, 0x2810, 0);
	E1000WriteCommand(dev, 0x2818, 31);
	E1000WriteCommand(dev, 0x100, 0x602801E);
	E1000WriteCommand(dev, 0x3800, dev->tx_descs_phys);								// Pass the physical address (and some other informations) of the transmit buffer
	E1000WriteCommand(dev, 0x3804, 0);
	E1000WriteCommand(dev, 0x3808, 128);
	E1000WriteCommand(dev, 0x3810, 0);
	E1000WriteCommand(dev, 0x3818, 0);
	E1000WriteCommand(dev, 0x400, 0x10400FA);
}

Void E1000InitDevices(UInt16 vendor, UInt16 device) {
	UIntPtr i = 0;																	// Let's find and init all the E1000 cards with the specified vendor ID and device ID
	PPCIDevice dev = PCIFindDevice1(&i, vendor, device);
	
	while (dev != Null) {
		E1000InitInt(dev);
		dev = PCIFindDevice1(&i, vendor, device);
	}
}

Void E1000Init(Void) {
	E1000InitDevices(PCI_VENDOR_INTEL, PCI_DEVICE_82543GC);							// Let's find and init all the E1000 cards
	E1000InitDevices(PCI_VENDOR_INTEL, PCI_DEVICE_82540EM);
	E1000InitDevices(PCI_VENDOR_INTEL, PCI_DEVICE_82577LM);
	E1000InitDevices(PCI_VENDOR_INTEL, PCI_DEVICE_I217LM);
}
