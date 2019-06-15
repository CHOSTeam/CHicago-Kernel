// File author is Ítalo Lima Marconato Matias
//
// Created on January 17 of 2019, at 21:06 BRT
// Last edited on June 15 of 2019, at 10:03 BRT

#include <chicago/arch/ahci.h>
#include <chicago/arch/ide.h>
#include <chicago/arch/pci.h>
#include <chicago/arch/port.h>

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/string.h>

static UInt8 AHCIGetType(PHBAPort port) {
	if ((port->ssts & 0x0F) != 0x03) {																					// Present?
		return 0;
	} else if (((port->ssts >> 8) & 0x0F) != 0x01) {																	// Active?
		return 0;
	} else if ((port->sig == 0xC33C0101 || port->sig == 0x96690101)) {													// Enclosure management bridge or Port multiplier?
		return 0;																										// Yes, we're not going to handle it (for now)
	} else if (port->sig == 0xEB140101) {																				// SATAPI?
		return 1;																										// Yes
	} else {
		return 2;																										// SATA
	}
}

static Void AHCIStartCMD(PHBAPort port) {
	while (port->cmd & 0x8000) ;																						// Wait until CR is cleared
	port->cmd |= 0x11;																									// Set the FRE and the ST bit
}

static Void AHCIStopCMD(PHBAPort port) {
	port->cmd &= ~0x01;																									// Clear the ST bit
	while ((port->cmd & 0xC000) == 0xC000) ;																			// Wait until FR bit and CR bit are cleared
	port->cmd &= ~0x10;																									// Clear the FRE bit
}

static Void AHCIRebase(PHBAPort port, UIntPtr virt) {
	AHCIStopCMD(port);																									// Stop the command engine
	
	UIntPtr phys = MmGetPhys(virt);																						// Get the physical address of virt
	PHBACmd cmd = (PHBACmd)virt;
	
	port->clb = phys;																									// Set the command list PHYSICAL location
	StrSetMemory(cmd, 0, sizeof(HBACmd) * 32);																			// Clear the command list (using the VIRTUAL address)
	
	port->fb = phys + (sizeof(HBACmd) * 32);																			// Set the FIS PHYSICAL location
	StrSetMemory((PUInt8)(virt + sizeof(HBACmd)), 0, 256);																// Clear the FIS (using the VIRTUAL address)
	
	for (UIntPtr i = 0; i < 32; i++) {
		cmd[i].prdtl = 8;																								// 8 PRDT entries
		cmd[i].ctba = phys + (sizeof(HBACmd) * 32) + 256 + (i * sizeof(HBACmdTbl));										// Set the command table PHYSICAL location
		StrSetMemory((PUInt8)(virt + (sizeof(HBACmd) * 32) + 256 + (i * sizeof(HBACmdTbl))), 0, sizeof(HBACmdTbl));		// Clear the command table (using the VIRTUAL address)
	}
	
	port->sctl |= 0x300;																								// Disable partial and slumber state
	port->serr = (UInt32)-1;																							// Clear the error status
	port->is = (UInt32)-1;																								// Clear pending interrupts
	port->cmd |= 0x10000006;																							// Power on, spin up and enable link
	
	AHCIStartCMD(port);																									// Start the command engine again
}

static IntPtr AHCIFindSlot(PHBAPort port) {
	for (UInt32 slots = port->sact | port->ci, i = 0; i < 32; i++, slots >>= 1) {										// Let's try to find a free slot
		if ((slots & 0x01) == 0) {																						// Found?
			return i;																									// Yes
		}
	}
	
	return -1;
}

static Boolean AHCISATAReadSectors(PHBAPort port, UIntPtr virt, UIntPtr lba, UIntPtr count, PUInt8 buf) {
	UIntPtr physadd = virt - MmGetPhys(virt);
	UInt16 prdtl = (UInt16)((count - 1) >> 4);
	IntPtr spin = 0;
	IntPtr slot = 0;
	
	port->is = (UInt32)-1;																								// Clear pending interrupt bits
	
	if ((slot = AHCIFindSlot(port)) == -1) {																			// Find free slot
		return False;																									// No free slot found :(
	}
	
	PHBACmd cmd = (PHBACmd)(virt + (sizeof(HBACmd) * slot));
	
	cmd->cfl = sizeof(FISRegH2D) / 4;																					// Set the command FIS size
	cmd->w = 0;																											// We're reading
	cmd->prdtl = prdtl + 1;																								// Set the amount of PRDT entries
	
	PHBACmdTbl tbl = (PHBACmdTbl)(cmd->ctba + physadd);
	
	StrSetMemory(tbl, 0, sizeof(HBACmdTbl) + ((cmd->prdtl - 1) * sizeof(HBAPRDTEntry)));								// Clear everything
	
	for (UIntPtr i = 0; i < prdtl; i++) {																				// 16 sectors per PRDT
		tbl->prdt[i].dba = MmGetPhys((UIntPtr)buf);																		// Set the buffer (physical address)
		tbl->prdt[i].dbc = 8191;																						// 8K bytes
		tbl->prdt[i].i = 1;
		buf += 8192;
		count -= 16;
	}
	
	tbl->prdt[prdtl].dba = MmGetPhys((UIntPtr)buf);																		// Set the last entry
	tbl->prdt[prdtl].dbc = count << 9;
	tbl->prdt[prdtl].i = 1;
	
	PFISRegH2D fis = (PFISRegH2D)(&tbl->cfis);
	
	fis->type = FIS_TYPE_REG_H2D;																						// Set the type
	fis->c = 1;																											// Command
	fis->featl = 1;																										// We're using DMA
	fis->cmd = ATA_CMD_READ_DMA_EXT;																					// Set the command
	fis->lba0 = (UInt8)lba;																								// Set the LBA low part
	fis->lba1 = (UInt8)(lba >> 8);
	fis->lba2 = (UInt8)(lba >> 16);
	fis->dev = 1 << 6;																									// LBA mode
	fis->lba3 = (UInt8)(lba >> 24);
	fis->lba4 = 0;																										// Set the LBA high part
	fis->lba5 = 0;
	fis->cntl = (UInt8)count;
	fis->cnth = (UInt8)(count >> 8);
	
	while (((port->tfd & 0x88) != 0) && (spin < 1000000)) {																// Wait until the port is no longer busy
		spin++;
	}
	
	if (spin == 1000000) {
		return False;																									// Yeah, we can't use this port!
	}
	
	port->ci = 1 << slot;																								// ISSUE COMMAND!
	
	while (True) {																										// Now, wait...
		if ((port->ci & (1 << slot)) == 0) {																			// Finished?
			break;																										// Yes :)
		}
		
		if ((port->is & (1 << 30)) != 0) {																				// Error?
			return False;																								// Yes :(
		}
	}
	
	if ((port->is & (1 << 30)) != 0) {																					// Error?
		return False;																									// Yes :(
	}
	
	return True;
}

static Boolean AHCISATAWriteSectors(PHBAPort port, UIntPtr virt, UIntPtr lba, UIntPtr count, PUInt8 buf) {
	UIntPtr physadd = virt - MmGetPhys(virt);
	UInt16 prdtl = (UInt16)((count - 1) >> 4);
	IntPtr spin = 0;
	IntPtr slot = 0;
	
	port->is = (UInt32)-1;																								// Clear pending interrupt bits
	
	if ((slot = AHCIFindSlot(port)) == -1) {																			// Find free slot
		return False;																									// No free slot found :(
	}
	
	PHBACmd cmd = (PHBACmd)(virt + (sizeof(HBACmd) * slot));
	
	cmd->cfl = sizeof(FISRegH2D) / 4;																					// Set the command FIS size
	cmd->w = 1;																											// We're writing
	cmd->prdtl = prdtl + 1;																								// Set the amount of PRDT entries
	
	PHBACmdTbl tbl = (PHBACmdTbl)(cmd->ctba + physadd);
	
	StrSetMemory(tbl, 0, sizeof(HBACmdTbl) + ((cmd->prdtl - 1) * sizeof(HBAPRDTEntry)));								// Clear everything
	
	for (UIntPtr i = 0; i < prdtl; i++) {																				// 16 sectors per PRDT
		tbl->prdt[i].dba = MmGetPhys((UIntPtr)buf);																		// Set the buffer (physical address)
		tbl->prdt[i].dbc = 8191;																						// 8K bytes
		tbl->prdt[i].i = 1;
		buf += 8192;
		count -= 16;
	}
	
	tbl->prdt[prdtl].dba = MmGetPhys((UIntPtr)buf);																		// Set the last entry
	tbl->prdt[prdtl].dbc = count << 9;
	tbl->prdt[prdtl].i = 1;
	
	PFISRegH2D fis = (PFISRegH2D)(&tbl->cfis);
	
	fis->type = FIS_TYPE_REG_H2D;																						// Set the type
	fis->c = 1;																											// Command
	fis->featl = 1;																										// We're using DMA
	fis->cmd = ATA_CMD_WRITE_DMA_EXT;																					// Set the command
	fis->lba0 = (UInt8)lba;																								// Set the LBA low part
	fis->lba1 = (UInt8)(lba >> 8);
	fis->lba2 = (UInt8)(lba >> 16);
	fis->dev = 1 << 6;																									// LBA mode
	fis->lba3 = (UInt8)(lba >> 24);
	fis->lba4 = 0;																										// Set the LBA high part
	fis->lba5 = 0;
	fis->cntl = (UInt8)count;
	fis->cnth = (UInt8)(count >> 8);
	
	while (((port->tfd & 0x88) != 0) && (spin < 1000000)) {																// Wait until the port is no longer busy
		spin++;
	}
	
	if (spin == 1000000) {
		return False;																									// Yeah, we can't use this port!
	}
	
	port->ci = 1 << slot;																								// ISSUE COMMAND!
	
	while (True) {																										// Now, wait...
		if ((port->ci & (1 << slot)) == 0) {																			// Finished?
			break;																										// Yes :)
		}
		
		if ((port->is & (1 << 30)) != 0) {																				// Error?
			return False;																								// Yes :(
		}
	}
	
	if ((port->is & (1 << 30)) != 0) {																					// Error?
		return False;																									// Yes :(
	}
	
	return True;
}

static Boolean AHCISATAPIReadSectors(PHBAPort port, UIntPtr virt, UIntPtr lba, UIntPtr count, PUInt8 buf) {
	UIntPtr physadd = virt - MmGetPhys(virt);
	UInt16 prdtl = (UInt16)((count - 1) >> 4);
	IntPtr spin = 0;
	IntPtr slot = 0;
	
	port->is = (UInt32)-1;																								// Clear pending interrupt bits
	
	if ((slot = AHCIFindSlot(port)) == -1) {																			// Find free slot
		return False;																									// No free slot found :(
	}
	
	PHBACmd cmd = (PHBACmd)(virt + (sizeof(HBACmd) * slot));
	
	cmd->cfl = sizeof(FISRegH2D) / 4;																					// Set the command FIS size
	cmd->atapi = 1;																										// We're going to send a ATAPI packet
	cmd->w = 0;																											// We're reading
	cmd->prdtl = prdtl + 1;																								// Set the amount of PRDT entries
	
	PHBACmdTbl tbl = (PHBACmdTbl)(cmd->ctba + physadd);
	
	StrSetMemory(tbl, 0, sizeof(HBACmdTbl) + ((cmd->prdtl - 1) * sizeof(HBAPRDTEntry)));								// Clear everything
	
	for (UIntPtr i = 0; i < prdtl; i++) {																				// 16 sectors per PRDT
		tbl->prdt[i].dba = MmGetPhys((UIntPtr)buf);																		// Set the buffer (physical address)
		tbl->prdt[i].dbc = 32767;																						// 32K bytes
		tbl->prdt[i].i = 1;
		buf += 32768;
		count -= 16;
	}
	
	tbl->prdt[prdtl].dba = MmGetPhys((UIntPtr)buf);																		// Set the last entry
	tbl->prdt[prdtl].dbc = count << 11;
	tbl->prdt[prdtl].i = 1;
	
	PFISRegH2D fis = (PFISRegH2D)(&tbl->cfis);
	
	fis->type = FIS_TYPE_REG_H2D;																						// Set the type
	fis->c = 1;																											// We're sending a command
	fis->featl = 1;																										// We're using DMA
	fis->cmd = ATA_CMD_PACKET;																							// ATAPI Packet
	fis->lba1 = 2048 & 0xFF;
	fis->lba2 = 2048 >> 8;
	fis->cntl = (UInt8)count;
	fis->cnth = (UInt8)(count >> 8);
	
	PUInt8 packet = (PUInt8)(&tbl->acmd);
	
	packet[0] = ATAPI_CMD_READ;																							// Setup SCSI packet
	packet[1] = 0x00;
	packet[2] = ((lba >> 24) & 0xFF);
	packet[3] = ((lba >> 16) & 0xFF);
	packet[4] = ((lba >> 8) & 0xFF);
	packet[5] = (lba & 0xFF);
	packet[6] = 0x00;
	packet[7] = 0x00;
	packet[8] = 0x00;
	packet[9] = 0x01;
	packet[10] = 0x00;
	packet[11] = 0x00;
	
	while (((port->tfd & 0x88) != 0) && (spin < 1000000)) {																// Wait until the port is no longer busy
		spin++;
	}
	
	if (spin == 1000000) {
		return False;																									// Yeah, we can't use this port!
	}
	
	port->ci = 1 << slot;																								// ISSUE COMMAND!
	
	while (True) {																										// Now, wait...
		if ((port->ci & (1 << slot)) == 0) {																			// Finished?
			break;																										// Yes :)
		}
		
		if ((port->is & (1 << 30)) != 0) {																				// Error?
			return False;																								// Yes :(
		}
	}
	
	if ((port->is & (1 << 30)) != 0) {																					// Error?
		return False;																									// Yes :(
	}
	
	return True;
}

static Boolean AHCIReadSectors(PHBAPort port, UIntPtr virt, UIntPtr bsize, UIntPtr lba, UIntPtr count, PUInt8 buf) {
	if (bsize == 512) {																									// SATA?
		return AHCISATAReadSectors(port, virt, lba, count, buf);														// Yes, redirect to AHCISATAReadSectors
	} else {
		return AHCISATAPIReadSectors(port, virt, lba, count, buf);														// SATAPI, redirect to AHCISATAPIReadSectors
	}
}

static Boolean AHCIWriteSectors(PHBAPort port, UIntPtr virt, UIntPtr bsize, UIntPtr lba, UIntPtr count, PUInt8 buf) {
	if (bsize == 512) {																									// SATA?
		return AHCISATAWriteSectors(port, virt, lba, count, buf);														// Yes, redirect to AHCISATAWriteSectors
	} else {
		return False;																									// SATAPI, we don't support write in it!
	}
}

static Boolean AHCIDeviceRead(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	UIntPtr bsize = dev->name[0] == 'C' ? 2048 : 512;
	PAHCIDevice adev = (PAHCIDevice)dev->priv;
	
	UIntPtr cur = off / bsize;
	UIntPtr end = (off + len) / bsize;
	
	if ((off % bsize) != 0) {																							// "Align" the start
		PUInt8 buff = (PUInt8)MemAllocate(bsize);																		// Alloc memory for reading the disk
		
		if (buff == Null) {
			return False;																								// Failed...
		} else if (!AHCIReadSectors(adev->port, adev->virt, bsize, cur, 1, buff)) {										// Read this block
			MemFree((UIntPtr)buff);																						// Failed...
			return False;
		}
		
		StrCopyMemory(buf, buff + (off % bsize), bsize - (off % bsize));												// Copy it into the user buffer
		MemFree((UIntPtr)buff);
		cur++;
	}
	
	if (((off + len) % bsize) != 0) {																					// "Align" the end
		PUInt8 buff = (PUInt8)MemAllocate(bsize);																		// Alloc memory for reading the disk
		
		if (buff == Null) {
			return False;																								// Failed...
		} else if (!AHCIReadSectors(adev->port, adev->virt, bsize, end, 1, buff)) {										// Read this block
			MemFree((UIntPtr)buff);																						// Failed...
			return False;
		}
		
		StrCopyMemory(buf + len - ((off + len) % bsize), buff, (off + len) % bsize);									// Copy it into the user buffer
		MemFree((UIntPtr)buff);
		
		if (end != 0) {																									// Only decrease the end if it isn't 0
			end--;
		}
	}
	
	if (cur < end) {																									// Let's read!
		if (!AHCIReadSectors(adev->port, adev->virt, bsize, cur, end - cur, buf + (off % bsize))) {
			return False;																								// Failed...
		}
	}
	
	return True;
}

static Boolean AHCIDeviceWrite(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	UIntPtr bsize = dev->name[0] == 'C' ? 2048 : 512;
	PAHCIDevice adev = (PAHCIDevice)dev->priv;
	
	if (bsize != 512) {																									// CDROM?
		return False;																									// Yes, we don't support write in SATAPI drives
	}
	
	UIntPtr cur = off / bsize;
	UIntPtr end = (off + len) / bsize;
	
	if ((off % bsize) != 0) {																							// "Align" the start
		PUInt8 buff = (PUInt8)MemAllocate(bsize);																		// Alloc memory for reading the disk
		
		if (buff == Null) {
			return False;																								// Failed...
		} else if (!AHCIReadSectors(adev->port, adev->virt, bsize, cur, 1, buff)) {										// Read this block
			MemFree((UIntPtr)buff);																						// Failed...
			return False;
		}
		
		StrCopyMemory(buff + (off % bsize), buf + (off % bsize), bsize - (off % bsize));								// Write back to the buffer
		
		if (!AHCIWriteSectors(adev->port, adev->virt, bsize, cur, 1, buff)) {											// Write this block back
			MemFree((UIntPtr)buff);																						// Failed...
			return False;
		}
		
		MemFree((UIntPtr)buff);
		cur++;
	}
	
	if (((off + len) % bsize) != 0) {																					// "Align" the end
		PUInt8 buff = (PUInt8)MemAllocate(bsize);																		// Alloc memory for reading the disk
		
		if (buff == Null) {
			return False;																								// Failed...
		} else if (!AHCIReadSectors(adev->port, adev->virt, bsize, end, 1, buff)) {										// Read this block
			MemFree((UIntPtr)buff);																						// Failed...
			return False;
		}
		
		StrCopyMemory(buff + ((off + len) % bsize), buf + len - ((off + len) % bsize), (off + len) % bsize);			// Write back to the buffer
		
		if (!AHCIWriteSectors(adev->port, adev->virt, bsize, end, 1, buff)) {											// Write this block back
			MemFree((UIntPtr)buff);																						// Failed...
			return False;
		}
		
		MemFree((UIntPtr)buff);
		
		if (end != 0) {																									// Only decrease the end if it isn't 0
			end--;
		}
	}
	
	if (cur < end) {																									// Let's write!
		if (!AHCIWriteSectors(adev->port, adev->virt, bsize, cur, end - cur, buf + (off % bsize))) {
			return False;																								// Failed...
		}
	}
	
	return True;
}

static Boolean AHCIDeviceControl(PDevice dev, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf) {
	(Void)dev; (Void)ibuf;																								// Avoid compiler's unused parameter warning
	
	PUInt32 out32 = (PUInt32)obuf;
	PUInt8 out8 = (PUInt8)obuf;
	
	if (cmd == 0) {																										// Get block size?
		*out32 = dev->name[0] == 'C' ? 2048 : 512;
	} else if (cmd == 1) {																								// Get device status?
		*out8 = True;																									// We haven't implemented it yet, just hope that it is present lol
	} else {
		return False;																									// ...
	}
	
	return False;
}

static Void AHCIInitInt(PPCIDevice pdev) {
	UIntPtr abarp = pdev->bar5 & 0xFFFFFFF0;																			// Get the BAR5 (ABAR physical address)
	PHBAMem abar = (PHBAMem)MemAAllocate(sizeof(HBAMem), MM_PAGE_SIZE);													// Alloc space for mapping it into the virtual space
	
	if (abar == Null) {
		DbgWriteFormated("PANIC! Couldn't init a detected AHCI controller\r\n");										// Failed, so halt
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	for (UIntPtr i = 0; i < sizeof(HBAMem); i += MM_PAGE_SIZE) {														// Let's map the ABAR!
		MmDereferencePage(MmGetPhys(((UIntPtr)abar) + i));
		
		if (!MmMap(((UIntPtr)abar) + i, abarp + i, MM_MAP_KDEF)) {
			DbgWriteFormated("PANIC! Couldn't init a detected AHCI controller\r\n");									// Failed to map, so halt
			Panic(PANIC_KERNEL_INIT_FAILED);
		}
	}
	
	UInt32 port = abar->pi;																								// Let's search for the amount of SATA/SATAPI devices
	UInt32 pcnt = 0;
	
	for (UInt32 i = 0; i < 32; i++) {
		if ((port & 1) && (AHCIGetType(&abar->ports[i]) != 0)) {														// Valid?
			pcnt++;																										// Yes
		}
		
		port >>= 1;																										// Go to the next one
	}
	
	if (pcnt == 0) {																									// No devices here?
		for (UIntPtr i = 0; i < sizeof(HBAMem); i += MM_PAGE_SIZE) {													// Yeah, so let's unmap everything and return
			MemFree((UIntPtr)abar);
		}
		
		return;
	}
	
	UIntPtr virt = MemAAllocate(pcnt * (sizeof(HBACmd) + sizeof(HBACmdTbl) + 256) * 32, MM_PAGE_SIZE);					// Alloc memory for all the devices
	
	if (virt == 0) {
		DbgWriteFormated("PANIC! Couldn't init a detected AHCI controller\r\n");										// Failed, so halt
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	port = abar->pi;																									// Let's search for all the SATA/SATAPI devices!
	pcnt = 0;
	
	for (UInt32 i = 0; i < 32; i++) {
		if (port & 1) {																									// Anything on this port?
			UInt8 type = AHCIGetType(&abar->ports[i]);																	// Yes, get the type
			
			if (type == 0) {																							// Handle it?
				goto nope;																								// Nope
			}
			
			AHCIRebase(&abar->ports[i], virt + (pcnt * (sizeof(HBACmd) + sizeof(HBACmdTbl) + 256) * 32));				// Rebase the port
			
			pcnt++;																										// Increase the pcnt
			
			PAHCIDevice dev = (PAHCIDevice)MemAllocate(sizeof(AHCIDevice));												// Alloc memory for this device
			
			if ((dev == Null) && (type == 1)) {
				DbgWriteFormated("[x86] Failed to add a SATA cdrom\r\n");												// Failed (CdRom)
				goto nope;
			} else if (dev == Null) {
				DbgWriteFormated("[x86] Failed to add a SATA hard disk\r\n");											// Failed (HardDisk)
				goto nope;
			}
			
			dev->port = &abar->ports[i];																				// Set the port of this device
			dev->atapi = type == 1;																						// Set if this device is ATAPI
			dev->virt = virt + ((pcnt - 1) * (sizeof(HBACmd) + sizeof(HBACmdTbl) + 256) * 32);							// Set the base virtual address
			
			if (dev->atapi) {																							// ATAPI?
				if (!FsAddCdRom(dev, AHCIDeviceRead, AHCIDeviceWrite, AHCIDeviceControl)) {								// Yes, try to add it
					DbgWriteFormated("[x86] Failed to add a SATA cdrom\r\n");
					MemFree((UIntPtr)dev);
				}
			} else {
				if (!FsAddHardDisk(dev, AHCIDeviceRead, AHCIDeviceWrite, AHCIDeviceControl)) {							// It's a hard disk, try to add it
					DbgWriteFormated("[x86] Failed to add a SATA hard disk\r\n");
					MemFree((UIntPtr)dev);
				}
			}
		}
		
nope:;	port >>= 1;																										// Go to the next port
	}
}

Void AHCIInit(Void) {
	UIntPtr i = 0;																										// Let's find and init all the AHCI controllers
	PPCIDevice dev = PCIFindDevice2(&i, PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_SATA);
	
	while (dev != Null) {
		AHCIInitInt(dev);
		dev = PCIFindDevice2(&i, PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_SATA);
	}
}
