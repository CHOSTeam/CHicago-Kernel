// File author is Ítalo Lima Marconato Matias
//
// Created on July 14 of 2018, at 23:40 BRT
// Last edited on January 04 of 2020, at 17:56 BRT

#include <chicago/arch/ide.h>
#include <chicago/arch/idt.h>
#include <chicago/arch/pci.h>
#include <chicago/arch/port.h>

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/string.h>

static UInt8 IDEBuffer[512] = { 0 };
static Boolean IDEHandlerInit = False;
static Volatile Boolean IDEIRQInvoked = False;

static Void IDEHandler(PRegisters regs) {
	(Void)regs;
	IDEIRQInvoked = True;
}

Void IDEWaitIRQ(Void) {
	while (!IDEIRQInvoked) ;
	IDEIRQInvoked = False;
}

static Void IDEReset(UInt16 base, UInt16 ctrl) {
	PortOutByte(ctrl, 4);																						// Let's reset this drive
	
	for (UInt32 i = 0; i < 4; i++) {																			// 400 nanoseconds delay
		PortInByte(base + ATA_REG_ALTSTATUS);
	}
	
	PortOutByte(ctrl, 0);
}

static Boolean IDEPoll(UInt16 base, Boolean adv) {
	while (PortInByte(base + ATA_REG_STATUS) & ATA_SR_BSY) ;													// Wait for BSY to be cleared
	
	if (adv) {																									// Advanced check?
		UInt8 status = PortInByte(base + ATA_REG_STATUS);														// Yes, read the status register
		
		if (status & ATA_SR_ERR || status & ATA_SR_DF || (!(status & ATA_SR_DRQ))) {							// Check for error
			return False;
		}
	}
	
	return True;
}

static Boolean IDESendSCSICommand(UInt16 base, Boolean slave, UInt8 cmd, UInt32 lba, UInt16 size, PUInt8 buf) {
	UInt8 scmd[12] = { 0 };
	
	PortOutByte(base + ATA_REG_CONTROL, IDEIRQInvoked = 0);														// Enable the IRQs
	
	scmd[0] = cmd;																								// Setup the SCSI packet
	scmd[2] = (lba >> 24) & 0xFF;
	scmd[3] = (lba >> 16) & 0xFF;
	scmd[4] = (lba >> 8) & 0xFF;
	scmd[5] = lba & 0xFF;
	scmd[9] = 1;
	
	PortOutByte(base + ATA_REG_HDDEVSEL, slave << 4);															// Select the drive
	
	for (UInt32 i = 0; i < 4; i++) {																			// 400 nanoseconds delay
		PortInByte(base + ATA_REG_ALTSTATUS);
	}
	
	PortOutByte(base + ATA_REG_FEATURES, 0);																	// We're going to use PIO mode
	PortOutByte(base + ATA_REG_LBA1, size & 0xFF);																// Tell the size of the buffer
	PortOutByte(base + ATA_REG_LBA2, (size >> 8) & 0xFF);
	PortOutByte(base + ATA_REG_COMMAND, ATA_CMD_PACKET);
	
	if (!IDEPoll(base, True)) {																					// Poll and return error if error
		return False;
	}
	
	PortOutMultiple(base + ATA_REG_DATA, scmd, 6);																// Send the packet
	
	IDEWaitIRQ();																								// Wait for an IRQ
	
	if (!IDEPoll(base, True)) {																					// Poll, and... alright let's stop this
		return False;
	}
	
	PortInMultiple(base + ATA_REG_DATA, buf, size / 2);															// Read the data
	
	IDEWaitIRQ();																								// And wait for another IRQ
	
	while (PortInByte(base + ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ)) ;										// Wait for BSY and DRQ to clear
	
	return True;
}

static Boolean IDEATAReadSector(UInt16 base, Boolean slave, Boolean addr48, UIntPtr lba, PUInt8 buf) {
	PortOutByte(base + ATA_REG_CONTROL, (IDEIRQInvoked = 0) + 2);												// Disable IRQs
	
	while (PortInByte(base + ATA_REG_STATUS) & ATA_SR_BSY) ;													// Poll until BSY is clear
	
	if (addr48 && lba >= 0x10000000) {																			// We support, and need 48-bit addressing?
		PortOutByte(base + ATA_REG_HDDEVSEL, 0xE0 | (slave << 4));												// Yes!
	} else if (lba >= 0x10000000) {																				// We don't support, and need 48-bit addressing?
		return False;																							// No...
	} else {																									// So, it's normal addressing
		PortOutByte(base + ATA_REG_HDDEVSEL, 0xE0 | (slave << 4) | ((lba & 0xF0000000) >> 24));
	}
	
	if (lba >= 0x10000000) {																					// 48-bit addressing?
		PortOutByte(base + ATA_REG_SECCOUNT0, 0);																// Yes, so setup 48-bit addressing mode
		PortOutByte(base + ATA_REG_LBA3, ((lba & 0xFF000000) >> 24));
		PortOutByte(base + ATA_REG_LBA4, 0);
		PortOutByte(base + ATA_REG_LBA5, 0);
	}
	
	PortOutByte(base + ATA_REG_SECCOUNT1, 1);																	// 1 sector per time
	PortOutByte(base + ATA_REG_LBA0, (lba & 0xFF));																// Let's send the LBA
	PortOutByte(base + ATA_REG_LBA1, ((lba & 0xFF00) >> 8));
	PortOutByte(base + ATA_REG_LBA2, ((lba & 0xFF0000) >> 16));
	
	if (lba >= 0x10000000) {																					// Send extended (48-bits addressing mode) command?
		PortOutByte(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO_EXT);												// Yes
	} else {
		PortOutByte(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);													// No
	}
	
	if (!IDEPoll(base, True)) {																					// Poll, and return error if error
		return False;
	}
	
	PortInMultiple(base + ATA_REG_DATA, buf, 256);																// Read the data
	
	return True;
}

static Boolean IDEATAWriteSector(UInt16 base, Boolean slave, Boolean addr48, UIntPtr lba, PUInt8 buf) {
	PortOutByte(base + ATA_REG_CONTROL, (IDEIRQInvoked = 0) + 2);												// Disable IRQs
	
	while (PortInByte(base + ATA_REG_STATUS) & ATA_SR_BSY) ;													// Poll until BSY is clear
	
	if (addr48 && lba >= 0x10000000) {																			// We support, and need 48-bit addressing?
		PortOutByte(base + ATA_REG_HDDEVSEL, 0xE0 | (slave << 4));												// Yes!
	} else if (lba >= 0x10000000) {																				// We don't support, and need 48-bit addressing?
		return False;																							// No...
	} else {																									// So, it's normal addressing
		PortOutByte(base + ATA_REG_HDDEVSEL, 0xE0 | (slave << 4) | ((lba & 0xF0000000) >> 24));
	}
	
	if (lba >= 0x10000000) {																					// 48-bit addressing?
		PortOutByte(base + ATA_REG_SECCOUNT0, 0);																// Yes, so setup 48-bit addressing mode
		PortOutByte(base + ATA_REG_LBA3, ((lba & 0xFF000000) >> 24));
		PortOutByte(base + ATA_REG_LBA4, 0);
		PortOutByte(base + ATA_REG_LBA5, 0);
	}
	
	PortOutByte(base + ATA_REG_SECCOUNT1, 1);																	// 1 sector per time
	PortOutByte(base + ATA_REG_LBA0, (lba & 0xFF));																// Let's send the LBA
	PortOutByte(base + ATA_REG_LBA1, ((lba & 0xFF00) >> 8));
	PortOutByte(base + ATA_REG_LBA2, ((lba & 0xFF0000) >> 16));
	
	if (lba >= 0x10000000) {																					// Send extended (48-bits addressing mode) command?
		PortOutByte(base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO_EXT);												// Yes
	} else {
		PortOutByte(base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);													// No
	}
	
	if (!IDEPoll(base, True)) {																					// Poll, and return error if error
		return False;
	}
	
	PortOutMultiple(base + ATA_REG_DATA, buf, 256);																// Read the data
	
	if (lba >= 0x10000000) {																					// Send extended (48-bits addressing mode) FLUSH command?
		PortOutByte(base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH_EXT);											// Yes
	} else {
		PortOutByte(base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);												// No
	}
	
	IDEPoll(base, False);																						// Do some polling
	
	return True;
}

static Boolean IDEReadSectors(UInt16 base, Boolean slave, Boolean addr48, Boolean atapi, UIntPtr count, UIntPtr lba, PUInt8 buf) {
	Boolean ret = False;
	
	if (atapi) {																								// ATAPI?
		for (UInt32 i = 0; i < count; i++) {																	// Yes, so use the read command using SCSI commands
			ret = IDESendSCSICommand(base, slave, ATAPI_CMD_READ, lba + i, 2048, buf + (i * 2048));
		}
	} else {
		for (UInt32 i = 0; i < count; i++) {																	// No, so use the ATA read sector function
			ret = IDEATAReadSector(base, slave, addr48, lba + i, buf + (i * 512));
		}
	}
	
	return ret;
}

static Boolean IDEWriteSectors(UInt16 base, Boolean slave, Boolean addr48, Boolean atapi, UIntPtr count, UIntPtr lba, PUInt8 buf) {
	Boolean ret = False;
	
	if (atapi) {																								// ATAPI?
		ret = False;																							// Yes, but we don't support the write operation on it :(
	} else {
		for (UInt32 i = 0; i < count; i++) {																	// No, so use the ATA write sector function
			ret = IDEATAWriteSector(base, slave, addr48, lba + i, buf + (i * 512));
		}
	}
	
	return ret;
}

static UIntPtr IDEDeviceRead(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	PIDEDevice idev = (PIDEDevice)dev->priv;
	UIntPtr bsize = idev->atapi ? 2048 : 512;																	// Get the block size
	UIntPtr cur = off / bsize;
	UIntPtr end = (off + len - 1) / bsize;
	UIntPtr start = 0;
	PUInt8 buff = (PUInt8)MemAAllocate(bsize, 0x10000);															// Alloc memory for reading the disk
	
	if (buf == Null) {
		return 0;
	}
	
	if ((off % bsize) != 0) {																					// "Align" the start
		if (!IDEReadSectors(idev->base, idev->slave, idev->addr48, idev->atapi, 1, cur, buff)) {				// Read this block
			MemAFree((UIntPtr)buff);																			// Failed...
			return 0;
		}
		
		StrCopyMemory(buf, buff + (off % bsize), bsize - (off % bsize));										// Copy it into the user buffer
		cur++;
		start += bsize - (off % bsize);
	}
	
	if (((off + len) % bsize) != 0) {																			// "Align" the end
		if (!IDEReadSectors(idev->base, idev->slave, idev->addr48, idev->atapi, 1, end, buff)) {				// Read this block
			MemAFree((UIntPtr)buff);																			// Failed...
			return 0;
		}
		
		StrCopyMemory(buf + len - ((off + len) % bsize), buff, (off + len) % bsize);							// Copy it into the user buffer
		
		if (end != 0) {																							// Only decrease the end if it isn't 0
			end--;
		}
	}
	
	while (cur <= end) {																						// Let's read!
		UIntPtr size = (end - cur + 1) * bsize;																	// Get the length of the data that we would need to read
		UIntPtr sects = end - cur + 1;
		
		if (size > 0x10000) {																					// More than 64KiB?
			size = 0x10000;																						// Yes, but we only want to read 64KiB or less
			sects = size / bsize;
		}
		
		if (!IDEReadSectors(idev->base, idev->slave, idev->addr48, idev->atapi, sects, cur, buff)) {
			return 0;																						// Failed...
		}
		
		StrCopyMemory(buf + start, buff, size);																	// Copy it into the user buffer
		
		cur += sects;
		start += size;
	}
	
	MemAFree((UIntPtr)buff);																					// Free the buffer
	
	return len;
}

static UIntPtr IDEDeviceWrite(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	PIDEDevice idev = (PIDEDevice)dev->priv;
	UIntPtr bsize = idev->atapi ? 2048 : 512;																	// Get the block size
	UIntPtr cur = off / bsize;
	UIntPtr end = (off + len - 1) / bsize;
	UIntPtr start = 0;
	PUInt8 buff = (PUInt8)MemAAllocate(bsize, 0x10000);															// Alloc memory for reading the disk
	
	if (buf == Null) {
		return 0;
	}
	
	if ((off % bsize) != 0) {																					// "Align" the start
		if (!IDEReadSectors(idev->base, idev->slave, idev->addr48, idev->atapi, 1, cur, buff)) {				// Read this block
			MemAFree((UIntPtr)buff);																			// Failed...
			return 0;
		}
		
		StrCopyMemory(buff + (off % bsize), buf + (off % bsize), bsize - (off % bsize));						// Write back to the buffer
		
		if (!IDEWriteSectors(idev->base, idev->slave, idev->addr48, idev->atapi, 1, cur, buff)) {				// Write this block back
			MemAFree((UIntPtr)buff);																			// Failed...
			return 0;
		}
		
		cur++;
		start += bsize;
	}
	
	if (((off + len) % bsize) != 0) {																			// "Align" the end
		if (!IDEReadSectors(idev->base, idev->slave, idev->addr48, idev->atapi, 1, end, buff)) {				// Read this block
			MemAFree((UIntPtr)buff);																			// Failed...
			return 0;
		}
		
		StrCopyMemory(buff + ((off + len) % bsize), buf + len - ((off + len) % bsize), (off + len) % bsize);	// Write back to the buffer
		
		if (!IDEWriteSectors(idev->base, idev->slave, idev->addr48, idev->atapi, 1, end, buff)) {				// Write this block back
			MemAFree((UIntPtr)buff);																			// Failed...
			return 0;
		}
		
		if (end != 0) {																							// Only decrease the end if it isn't 0
			end--;
		}
	}
	
	while (cur <= end) {																						// Let's write!
		UIntPtr size = (end - cur + 1) * bsize;																	// Get the length of the data that we would need to read
		UIntPtr sects = end - cur + 1;
		
		if (size > 0x10000) {																					// More than 64KiB?
			size = 0x10000;																						// Yes, but we only want to read 64KiB or less
			sects = size / bsize;
		}
		
		StrCopyMemory(buff, buf + start, size);																	// Copy from the user buffer
		
		if (!IDEWriteSectors(idev->base, idev->slave, idev->addr48, idev->atapi, sects, cur, buff)) {
			return 0;																							// Failed...
		}
		
		cur += sects;
		start += size;
	}
	
	MemAFree((UIntPtr)buff);																					// Free the buffer
	
	return len;
}

static UInt8 IDEInitInt2(UInt16 base, Boolean slave, PBoolean addr48) {
	Boolean atapi = False;
	
	PortOutByte(base + ATA_REG_HDDEVSEL, 0xA0 | (slave << 4));													// Select the drive
	
	PortOutByte(base + ATA_REG_SECCOUNT0, 0);																	// These values should be zero before sending IDENTIFY
	PortOutByte(base + ATA_REG_LBA0, 0);
	PortOutByte(base + ATA_REG_LBA1, 0);
	PortOutByte(base + ATA_REG_LBA2, 0);
	
	PortOutByte(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);														// Send IDENTIFY
	
	if (!(PortInByte(base + ATA_REG_STATUS))) {																	// Read the status port
		return 0;
	}
	
	Boolean err = False;
	
	while (True) {																								// Poll until BSY is clear
		UInt8 status = PortInByte(base + ATA_REG_STATUS);
		
		if (status & ATA_SR_ERR) {																				// ERR set?
			err = True;																							// So we can break and... well... ERROR (or, maybe, it's an ATAPI drive)
			break;
		}
		
		if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {													// Everyting is alright?
			break;																								// So we can break and continue
		}
	}
	
	if (err) {																									// ERR set?
		UInt8 cl = PortInByte(base + ATA_REG_LBA1);																// This drive may be an ATAPI drive
		UInt8 ch = PortInByte(base + ATA_REG_LBA2);
		
		if ((cl == 0x14 && ch == 0xEB) || (cl == 0x69 && ch == 0x96)) {											// ATAPI?
			PortOutByte(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);										// Yes, so send ATAPI IDENTIFY PACKET
			atapi = True;
		} else {
			return 0;																							// No... so just return
		}
	}
	
	for (UInt32 i = 0; i < 256; i++) {																			// Read the data
		*(PUInt16)(IDEBuffer + i * 2) = PortInWord(base + ATA_REG_DATA);
	}
	
	*addr48 = ((*((PUInt32)(IDEBuffer + ATA_IDENT_COMMANDSETS))) & (1 << 26));									// And use command set to get if we have 48-bit addressing
	
	return atapi + 1;
}

static Void IDEInitInt(PPCIDevice pdev) {
	UInt32 basep = pdev->bar0 & ~1;																				// Get the base io port of the primary ide channel
	UInt32 ctrlp = pdev->bar1 & ~1;																				// Get the base control port of the primary ide channel
	UInt32 bases = pdev->bar2 & ~1;																				// Get the base io port of the secondary ide channel
	UInt32 ctrls = pdev->bar3 & ~1;																				// Get the base control port of the secondary ide channel
	
	if ((basep == 0) || (basep == 1)) {																			// "Fix" everything (if it is 0 or 1, set it to the "default" value)
		basep = 0x1F0;
	}
	
	if ((ctrlp == 0) || (ctrlp == 1)) {
		ctrlp = 0x3F6;
	}
	
	if ((bases == 0) || (bases == 1)) {
		bases = 0x170;
	}
	
	if ((ctrls == 0) || (ctrls == 1)) {
		ctrls = 0x376;
	}
	
	IDEReset(basep, ctrlp);																						// We need to reset the drives, as the bios/(u)efi may fuck up them, and make us unable to receive ide irqs
	IDEReset(bases, ctrls);
	
	PortOutByte(basep + ATA_REG_CONTROL, 2);																	// Disable IRQs
	PortOutByte(bases + ATA_REG_CONTROL, 2);
	
	if (!IDEHandlerInit) {																						// The IDE handler is initialized?
		IDTRegisterIRQHandler(0x0E, IDEHandler);																// No, so let's register our IRQ handler
		IDTRegisterIRQHandler(0x0F, IDEHandler);
		IDEHandlerInit = True;
	}
	
	for (UInt32 i = 0; i < 2; i++) {																			// Let's detect ATA/ATAPI devices
		for (UInt32 j = 0; j < 2; j++) {
			Boolean addr48 = False;																				// Try to detect this device
			UInt8 res = IDEInitInt2((i == 0) ? basep : bases, j, &addr48);
			
			if (res == 0) {																						// This device exists?
				continue;																						// Nope
			}
			
			PIDEDevice dev = (PIDEDevice)MemAllocate(sizeof(IDEDevice));										// Ok, let's alloc our ide device struct
			
			if ((dev == Null) && (res == 2)) {
				DbgWriteFormated("[x86] Failed to add a IDE cdrom\r\n");										// Failed (CdRom)
				continue;
			} else if (dev == Null) {
				DbgWriteFormated("[x86] Failed to add a IDE hard disk\r\n");									// Failed (HardDisk)
				continue;
			}
			
			dev->base = (i == 0) ? basep : bases;																// Setup everything
			dev->ctrl = (i == 0) ? ctrlp : ctrls;
			dev->slave = j;
			dev->atapi = res - 1;
			dev->addr48 = addr48;
			
			if (dev->atapi) {																					// ATAPI?
				if (!FsAddCdRom(dev, IDEDeviceRead, IDEDeviceWrite, Null)) {									// Yes, try to add it
					DbgWriteFormated("[x86] Failed to add a IDE cdrom\r\n");
					MemFree((UIntPtr)dev);
				}
			} else {
				if (!FsAddHardDisk(dev, IDEDeviceRead, IDEDeviceWrite, Null)) {									// It's a hard disk, try to add it
					DbgWriteFormated("[x86] Failed to add a IDE hard disk\r\n");
					MemFree((UIntPtr)dev);
				}
			}
		}
	}
}

Void IDEInit(Void) {
	UIntPtr i = 0;																								// Let's find and init all the IDE controllers
	PPCIDevice dev = PCIFindDevice2(&i, PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_IDE);
	
	while (dev != Null) {
		IDEInitInt(dev);
		dev = PCIFindDevice2(&i, PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_IDE);
	}
}
