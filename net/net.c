// File author is Ítalo Lima Marconato Matias
//
// Created on December 12 of 2018, at 12:36 BRT
// Last edited on August 26 of 2019, at 19:19 BRT

#define __CHICAGO_NETWORK__

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/mm.h>
#include <chicago/net.h>
#include <chicago/panic.h>
#include <chicago/process.h>
#include <chicago/string.h>

PNetworkDevice NetDefaultDevice = Null;
PList NetDevices = Null;
UIntPtr NetLastID = 0;

static Boolean NetDeviceWrite(PDevice dev, UIntPtr off, UIntPtr len, PUInt8 buf) {
	(Void)off;																																							// Avoid compiler's unused parameter warning
	NetSendRawPacket((PNetworkDevice)dev->priv, len, buf);																												// Redirect to NetSendRawPacket
	return True;
}

static Boolean NetDeviceControl(PDevice dev, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf) {
	(Void)ibuf;																																							// Avoid compiler's unused parameter warning
	
	PNetworkDevice ndev = (PNetworkDevice)dev->priv;
	
	if (cmd == 0) {																																						// Get MAC Address
		StrCopyMemory(obuf, ndev->mac_address, 6);
	} else if (cmd == 1) {																																				// Set the IP(v4) address
		StrCopyMemory(ndev->ipv4_address, ibuf, 4);
	} else {
		return False;																																					// ...
	}
	
	return True;
}

PNetworkDevice NetAddDevice(PVoid priv, UInt8 mac[6], Void (*send)(PVoid, UIntPtr, PUInt8)) {
	if (NetDevices == Null) {																																			// Init the net device list?
		NetDevices = ListNew(False, False);																																// Yes :)
		
		if (NetDevices == Null) {
			return Null;																																				// Failed :(
		}
	}
	
	if (mac == Null) {																																					// Sanity checks
		return Null;
	}
	
	PNetworkDevice dev = (PNetworkDevice)MemAllocate(sizeof(NetworkDevice));																							// Alloc space for our network device
	
	if (dev == Null) {
		return Null;																																					// Failed
	}
	
	dev->id = NetLastID++;																																				// Set the id
	dev->priv = priv;																																					// The private data
	
	StrCopyMemory(dev->mac_address, mac, 6);																															// The mac address
	StrSetMemory(dev->ipv4_address, 0, 4);																																// For now we don't have a IPv4 address :(
	StrSetMemory((PUInt8)dev->arp_cache, 0, sizeof(ARPCache) * 32);																										// Clean the ARP cache
	
	dev->send = send;																																					// And the send function
	dev->packet_queue = QueueNew(False);																																// Create the packet queue
	
	if (dev->packet_queue == Null) {
		MemFree((UIntPtr)dev);																																			// Failed, free the device
		NetLastID--;																																					// And "free" the id
		return Null;
	}
	
	dev->packet_queue->free = True;																																		// The QueueFree function should free all the packets in the queue, not only the queue
	dev->packet_queue_rlock.locked = False;																																// Init the packet queue locks
	dev->packet_queue_rlock.owner = Null;
	dev->packet_queue_wlock.locked = False;
	dev->packet_queue_wlock.owner = Null;
	
	if (!ListAdd(NetDevices, dev)) {																																	// Try to add!
		QueueFree(dev->packet_queue);																																	// Failed, free the packet queue
		MemFree((UIntPtr)dev);																																			// Free the device
		NetLastID--;																																					// And "free" the id
		return Null;
	} else if (!FsAddNetworkDevice(dev, NetDeviceWrite, NetDeviceControl)) {																							// And try to add us to the device list!
		NetRemoveDevice(dev);																																			// Failed...
		return Null;
	}
	
	return dev;
}

PNetworkDevice NetGetDevice(PFsNode dev) {
	if ((NetDevices == Null) || (dev == Null)) {																														// Sanity checks
		return Null;
	}
	
	PDevice ndev = FsGetDevice(dev->name);																																// Try to get the device
	
	return (ndev == Null) ? Null : (PNetworkDevice)ndev->priv;																											// And return it
}

Void NetSetDefaultDevice(PNetworkDevice dev) {
	NetDefaultDevice = dev;
}

PNetworkDevice NetGetDefaultDevice(Void) {
	return NetDefaultDevice;
}

Void NetRemoveDevice(PNetworkDevice dev) {
	if ((NetDevices == Null) || (dev == Null)) {																														// Sanity checks
		return;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	
	ListForeach(NetDevices, i) {																																		// Try to find it on the net device list
		if (i->data == dev) {
			found = True;																																				// Found!
			break;
		} else {
			idx++;	
		}
	}
	
	if (!found) {
		return;																																							// Not found...
	}
	
	ListRemove(NetDevices, idx);																																		// Remove it from the net devices list
	FsRemoveNetworkDevice(dev);																																			// Remove it from the device list
	QueueFree(dev->packet_queue);																																		// Free the packet queue
	MemFree((UIntPtr)dev);																																				// And free the device
}

Void NetDevicePushPacket(PNetworkDevice dev, PUInt8 packet) {
	if ((dev == Null) || (packet == Null)) {																															// Sanity checks
		return;
	}
	
	PsLock(&dev->packet_queue_wlock);																																	// Lock
	QueueAdd(dev->packet_queue, packet);																																// Add this packet
	PsUnlock(&dev->packet_queue_wlock);																																	// Unlock
}

PUInt8 NetDevicePopPacket(PNetworkDevice dev) {
	if (dev == Null) {																																					// Sanity checks
		return Null;
	}
	
	while (dev->packet_queue->length == 0) {																															// Let's wait for a packet
		PsSwitchTask(Null);
	}
	
	PsLock(&dev->packet_queue_rlock);																																	// Lock
	PUInt8 data = (PUInt8)QueueRemove(dev->packet_queue);																												// Get our return value
	PsUnlock(&dev->packet_queue_rlock);																																	// Unlock
	
	return data;
}

UInt16 NetGetChecksum(PUInt8 data, UIntPtr len) {
	if (data == Null || len == 0) {																																		// First, sanity check
		return 0;
	}
	
	UInt32 acc = 0xFFFF;																																				// Now, let's calculate the checksum!
	Boolean zero = True;
	
	for (UIntPtr i = 0; (i + 1) < len; i += 2) {
		UInt16 word;
		
		StrCopyMemory(((PUInt8)&word), data + i, 2);
		acc += FromNetByteOrder16(word);
		
		if (word != 0 && zero) {
			zero = False;
		}
		
		if (acc > 0xFFFF) {
			acc -= 0xFFFF;
		}
	}
	
	if (zero) {
		return ToNetByteOrder16(0xFFFF);
	}
	
	return ToNetByteOrder16((UInt16)(~acc));
}

Void NetSendRawPacket(PNetworkDevice dev, UIntPtr len, PUInt8 buf) {
	if (dev == Null) {																																					// dev = default?
		dev = NetDefaultDevice;																																			// Yes
	}
	
	if ((dev == Null) || (dev->send == Null)) {																															// Sanity checks
		return;
	}
	
	dev->send(dev->priv, len, buf);																																		// SEND!
}

static Void NetThread(Void) {
	PNetworkDevice dev = (PNetworkDevice)PsCurrentThread->retv;																											// *HACK WARNING* The network device is in the retv
	
	while (True) {
		PEthFrame packet = (PEthFrame)NetDevicePopPacket(dev);																											// Wait for a packet to handle
		NetHandleEthPacket(dev, packet->src, FromNetByteOrder16(packet->type), ((PUInt8)packet) + sizeof(EthFrame));													// Handle
		MemFree((UIntPtr)packet);																																		// Free
	}
}

Void NetFinish(Void) {
	if (NetDevices != Null) {																																			// Create the network threads?
		ListForeach(NetDevices, i) {																																	// Yes, let's do it!
			PThread th = (PThread)PsCreateThread((UIntPtr)NetThread, 0, False);																							// Create the handler thread
			
			if (th == Null) {
				DbgWriteFormated("PANIC! Couldn't create the network thread\r\n");																						// Failed :(
				Panic(PANIC_KERNEL_INIT_FAILED);
			}
			
			th->retv = (UIntPtr)i->data;																																// *HACK WARNING* As we don't have anyway to pass arguments to the thread, use the retv to indicate the network device that this thread should handle
			
			PsAddThread(th);																																			// Add it!
		}
	}
}
