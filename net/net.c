// File author is Ítalo Lima Marconato Matias
//
// Created on December 12 of 2018, at 12:36 BRT
// Last edited on June 15 of 2019, at 11:09 BRT

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
PList NetARPIPv4Sockets = Null;
PList NetUDPSockets = Null;
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

Void NetSendRawPacket(PNetworkDevice dev, UIntPtr len, PUInt8 buf) {
	if (dev == Null) {																																					// dev = default?
		dev = NetDefaultDevice;																																			// Yes
	}
	
	if ((dev == Null) || (dev->send == Null)) {																															// Sanity checks
		return;
	}
	
	dev->send(dev->priv, len, buf);																																		// SEND!
}

Void NetSendEthPacket(PNetworkDevice dev, UInt8 dest[6], UInt16 type, UIntPtr len, PUInt8 buf) {
	if (dev == Null) {																																					// dev = default?
		dev = NetDefaultDevice;																																			// Yes
	}
	
	if ((dev == Null) || (dev->send == Null) || (dest == Null)) {																										// Sanity checks
		return;
	}
	
	PEthFrame frame = (PEthFrame)MemAllocate(sizeof(EthFrame) + len);																									// Let's build our ethernet frame!
	
	if (frame == Null) {
		return;																																							// Failed :(
	}
	
	StrCopyMemory(frame->dst, dest, 6);																																	// Copy the dest mac address
	StrCopyMemory(frame->src, dev->mac_address, 6);																														// The src mac address (our mac address)
	StrCopyMemory((PUInt8)(((UIntPtr)frame) + sizeof(EthFrame)), buf, len);																								// Copy the data/payload
	
	frame->type = ToNetByteOrder16(type);																																// Set the type
	
	if (StrCompareMemory(dest, dev->mac_address, 6)) {																													// Trying to send to yourself?
		NetDevicePushPacket(dev, (PUInt8)frame);																														// Yes, you're probably very lonely :´(
	} else {
		dev->send(dev->priv, sizeof(EthFrame) + len, (PUInt8)frame);																									// SEND!
	}
	
	MemFree((UIntPtr)frame);																																			// Free our eth frame
}

Void NetSendARPIPv4Packet(PNetworkDevice dev, UInt8 destm[6], UInt8 desti[4], UInt16 opcode) {
	if (dev == Null) {																																					// dev = default?
		dev = NetDefaultDevice;																																			// Yes
	}
	
	if ((dev == Null) || (destm == Null)) {																																// Sanity checks
		return;
	}
	
	PARPHeader hdr = (PARPHeader)MemAllocate(sizeof(ARPHeader));																										// Let's build our ARP header
	
	if (hdr == Null) {
		return;																																							// Failed :(
	}
	
	hdr->htype = 0x100;																																					// Fill the hardware type (1 = ETHERNET)
	hdr->ptype = 8;																																						// The protocol type (IP)
	hdr->hlen = 6;																																						// The hardware length (MAC ADDRESS = 6)
	hdr->plen = 4;																																						// The protocol length (IPV4 ADDRESS = 4)
	hdr->opcode = ToNetByteOrder16(opcode);																																// The opcode
	
	StrCopyMemory(hdr->ipv4.dst_hw, destm, 6);																															// The destination mac address
	StrCopyMemory(hdr->ipv4.dst_pr, desti, 4);																															// The destination ipv4 address
	StrCopyMemory(hdr->ipv4.src_hw, dev->mac_address, 6);																												// The source mac address
	StrCopyMemory(hdr->ipv4.src_pr, dev->ipv4_address, 4);																												// And the source ipv4 address
	NetSendEthPacket(dev, destm, ETH_TYPE_ARP, sizeof(ARPHeader), (PUInt8)hdr);																							// Send!
	MemFree((UIntPtr)hdr);																																				// And free the header	
}

static Boolean NetResolveIPv4Address(PNetworkDevice dev, UInt8 ip[4], PUInt8 dest) {
	UInt8 broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	UIntPtr last = 0;
	UIntPtr count = 0;
	
	if ((ip[0] != 10) && ((ip[0] != 172) || ((ip[1] < 16) || (ip[1] > 31))) && ((ip[0] != 192) || (ip[1] != 168))) {													// First, let's check if we really need to use arp to get the mac address
		StrCopyMemory(dest, broadcast, 6);																																// Nope, it's outside of the local network, the network card should do everything
		return True;
	} else if ((ip[0] == 127) && (ip[1] == 0) && (ip[2] == 0) && (ip[3] == 1)) {																						// Loopback/localhost?
		StrCopyMemory(dest, dev->mac_address, 6);																														// Yes
		return True;
	}
	
	for (last = 0; last < 32; last++) {																																	// First, let's see if we don't have this entry in our cache
		if (!dev->arp_cache[last].free && StrCompareMemory(dev->arp_cache[last].ipv4_address, ip, 4)) {																	// Found?
			StrCopyMemory(dest, dev->arp_cache[last].mac_address, 6);																									// Yes! Return it
			return True;
		} else if (dev->arp_cache[last].free) {
			break;																																						// ... end of the list
		}
	}
	
	NetSendARPIPv4Packet(dev, broadcast, ip, ARP_OPC_REQUEST);																											// Let's try to send an arp request
	
	while (True) {
		PsSleep(10);																																					// ... wait 10ms
		
		for (UIntPtr i = (last >= 32) ? 31 : last; i < 32; i++) {																										// And let's see if we found it!
			if (!dev->arp_cache[i].free && StrCompareMemory(dev->arp_cache[i].ipv4_address, ip, 4)) {																	// Found?
				StrCopyMemory(dest, dev->arp_cache[i].mac_address, 6);																									// Yes! Return it
				return True;
			} else if (dev->arp_cache[i].free) {
				last = i;																																				// ... end of the list
				break;
			}
		}
		
		count++;																																						// Increase the counts
		
		if (count > 5) {																																				// We're only going to try 5 times
			return False;																																				// ... we failed
		}
		
		NetSendARPIPv4Packet(dev, broadcast, ip, ARP_OPC_REQUEST);																										// And try again
	}
	
	return False;																																						// We should never get here...
}

Void NetSendIPv4Packet(PNetworkDevice dev, UInt8 dest[4], UInt8 protocol, UIntPtr len, PUInt8 buf) {
	if (dev == Null) {																																					// dev = default?
		dev = NetDefaultDevice;																																			// Yes
	}
	
	if ((dev == Null) || (dest == Null)) {																																// Sanity checks
		return;
	}
	
	UInt8 destm[6];																																						// Let's try to resolve the IPv4 address
	
	if (!NetResolveIPv4Address(dev, dest, destm)) {
		return;																																							// Failed :(
	}
	
	PIPHeader hdr = (PIPHeader)MemAllocate(20 + len);																													// Let's build our IPv4 header
	
	if (hdr == Null) {
		return;																																							// Failed :(
	}
	
	hdr->ihl = 5;																																						// "Default" value
	hdr->version = 4;																																					// IPv4
	hdr->ecn = 0;																																						// Don't care about this
	hdr->dscp = 0;																																						// And don't care about this for now
	hdr->length = ToNetByteOrder16((20 + len));																															// Header length + data length
	hdr->id = 0;																																						// We're not going to support fragmentation for now
	hdr->flags = 0;
	hdr->frag_off = 0;
	hdr->ttl = 64;																																						// Time To Live = 64
	hdr->protocol = protocol;																																			// Set the protocol
	hdr->checksum = 0;																																					// Let's set it later
	
	StrCopyMemory(hdr->ipv4.src, dev->ipv4_address, 4);																													// Set the src ipv4 addresss (our address)
	StrCopyMemory(hdr->ipv4.dst, dest, 4);																																// And the dest ipv4 address
	
	PUInt8 data = (PUInt8)hdr;																																			// Calculate the checksum!
	UInt32 acc = 0xFFFF;
	
	for (UIntPtr i = 0; (i + 1) < 20; i += 2) {
		UInt16 word;
		
		StrCopyMemory(((PUInt8)&word), data + i, 2);
		acc += FromNetByteOrder16(word);
		
		if (acc > 0xFFFF) {
			acc -= 0xFFFF;
		}
	}
	
	hdr->checksum = ToNetByteOrder16(((UInt16)acc));																													// And set it
	
	StrCopyMemory(data + 20, buf, len);																																	// Copy the data
	NetSendEthPacket(dev, destm, ETH_TYPE_IP, 20 + len, data);																											// Send the packet!
	MemFree((UIntPtr)hdr);																																				// And free everything
}

Void NetSendUDPPacket(PNetworkDevice dev, UInt8 dest[4], UInt16 port, UIntPtr len, PUInt8 buf) {
	if (dev == Null) {																																					// dev = default?
		dev = NetDefaultDevice;																																			// Yes
	}
	
	if ((dev == Null) || (dest == Null)) {																																// Sanity checks
		return;
	}
	
	PUDPHeader hdr = (PUDPHeader)MemAllocate(sizeof(UDPHeader) + len);																									// Let's build our UDP header
	
	if (hdr == Null) {
		return;																																							// Failed :(
	}
	
	hdr->sport = hdr->dport = ToNetByteOrder16(port);																													// Set the port
	hdr->length = ToNetByteOrder16(((UInt16)(sizeof(UDPHeader) + len)));																								// Set the length
	hdr->checksum = 0;																																					// I still need to implement the UDP checksum calculation...
	
	StrCopyMemory(((PUInt8)hdr) + sizeof(UDPHeader), buf, len);																											// Copy the data
	NetSendIPv4Packet(dev, dest, IP_PROTOCOL_UDP, sizeof(UDPHeader) + len, (PUInt8)hdr);																				// Send!
	MemFree((UIntPtr)hdr);
}

PARPIPv4Socket NetAddARPIPv4Socket(PNetworkDevice dev, UInt8 mac[6], UInt8 ipv4[4], Boolean user) {
	if (dev == Null) {																																					// dev = default?
		dev = NetDefaultDevice;																																			// Yes
	}
	
	if (NetARPIPv4Sockets == Null) {																																	// Init the ARP IPv4 socket list?
		NetARPIPv4Sockets = ListNew(False, False);																														// Yes :)
		
		if (NetARPIPv4Sockets == Null) {
			return Null;																																				// Failed :(
		}
	}
	
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (dev == Null) || (mac == Null) || (ipv4 == Null)) {													// Sanity checks
		return Null;
	}
	
	PARPIPv4Socket sock = (PARPIPv4Socket)MemAllocate(sizeof(ARPIPv4Socket));																							// Let's create our socket
	
	if (sock == Null) {
		return Null;
	}
	
	sock->user = user;																																					// Set if this is a user socket
	sock->dev = dev;																																					// Set our device
	sock->packet_queue = QueueNew(user);																																// Create our packet queue
	
	if (sock->packet_queue == Null) {
		MemFree((UIntPtr)sock);																																			// Failed
		return Null;
	}
	
	StrCopyMemory(sock->mac_address, mac, 6);																															// Set the dest mac address
	StrCopyMemory(sock->ipv4_address, ipv4, 4);																															// Set the dest ipv4 address
	
	sock->owner_process = PsCurrentProcess;																																// And the owner process!
	
	if (!ListAdd(NetARPIPv4Sockets, sock)) {																															// Now, try to add it to the ARP IPv4 socket list!
		QueueFree(sock->packet_queue);																																	// Failed :(
		MemFree((UIntPtr)sock);
		return Null;
	}
	
	return sock;
}


Void NetRemoveARPIPv4Socket(PARPIPv4Socket sock) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (NetARPIPv4Sockets == Null) || (sock == Null)) {														// Sanity checks
		return;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	
	ListForeach(NetARPIPv4Sockets, i) {																																	// Try to find it on the ARP IPv4 socket list
		if (i->data == sock) {
			found = True;																																				// Found!
			break;
		} else {
			idx++;	
		}
	}
	
	if (!found) {
		return;																																							// Not found...
	} else if (sock->owner_process != PsCurrentProcess) {
		return;
	}
	
	ListRemove(NetARPIPv4Sockets, idx);																																	// Remove it from the ARP IPv4 socket list
	
	while (sock->packet_queue->length != 0) {																															// Free all the packets that are in the queue
		UIntPtr data = (UIntPtr)QueueRemove(sock->packet_queue);
		
		if (sock->user) {																																				// Use MmFreeUserMemory?
			MmFreeUserMemory(data);																																		// Yes
		} else {
			MemFree(data);																																				// Nope
		}
	}
	
	QueueFree(sock->packet_queue);																																		// Free the packet queue struct itself
	MemFree((UIntPtr)sock);																																				// And free the socket struct itself
}

Void NetSendARPIPv4Socket(PARPIPv4Socket sock, UInt16 opcode) {
	if (sock == Null) {																																					// Sanity check
		return;
	}
	
	NetSendARPIPv4Packet(sock->dev, sock->mac_address, sock->ipv4_address, opcode);																						// Send!
}

PARPHeader NetReceiveARPIPv4Socket(PARPIPv4Socket sock) {
	if (sock == Null) {																																					// Sanity check
		return Null;
	}
	
	UIntPtr count = 0;
	
	while (sock->packet_queue->length == 0) {																															// Wait the packet that we want :)
		if (++count == 401) {																																			// Our timeout is 4 seconds
			return Null;
		}
		
		PsSleep(10);																																					// Sleep 10ms
	}
	
	return (PARPHeader)QueueRemove(sock->packet_queue);																													// Now, return it!
}

PUDPSocket NetAddUDPSocket(PNetworkDevice dev, UInt8 ipv4[4], UInt16 port, Boolean user) {
	if (dev == Null) {																																					// dev = default?
		dev = NetDefaultDevice;																																			// Yes
	}
	
	if (NetUDPSockets == Null) {																																		// Init the UDP socket list?
		NetUDPSockets = ListNew(False, False);																															// Yes :)
		
		if (NetUDPSockets == Null) {
			return Null;																																				// Failed :(
		}
	}
	
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (dev == Null) || (ipv4 == Null)) {																	// Sanity checks
		return Null;
	}
	
	PUDPSocket sock = (PUDPSocket)MemAllocate(sizeof(UDPSocket));																										// Let's create our socket
	
	if (sock == Null) {
		return Null;
	}
	
	sock->port = port;																																					// Set the port
	sock->user = user;																																					// Set if this is a user socket
	sock->dev = dev;																																					// Set our device
	sock->packet_queue = QueueNew(user);																																// Create our packet queue
	
	if (sock->packet_queue == Null) {
		MemFree((UIntPtr)sock);																																			// Failed
		return Null;
	}
	
	StrCopyMemory(sock->ipv4_address, ipv4, 4);																															// Set the dest ipv4 address
	
	sock->owner_process = PsCurrentProcess;																																// And the owner process!
	
	if (!ListAdd(NetUDPSockets, sock)) {																																// Now, try to add it to the UDP socket list!
		QueueFree(sock->packet_queue);																																	// Failed :(
		MemFree((UIntPtr)sock);
		return Null;
	}
	
	return sock;
}

Void NetRemoveUDPSocket(PUDPSocket sock) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (NetUDPSockets == Null) || (sock == Null)) {															// Sanity checks
		return;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	
	ListForeach(NetUDPSockets, i) {																																		// Try to find it on the ARP IPv4 socket list
		if (i->data == sock) {
			found = True;																																				// Found!
			break;
		} else {
			idx++;	
		}
	}
	
	if (!found) {
		return;																																							// Not found...
	} else if (sock->owner_process != PsCurrentProcess) {
		return;
	}
	
	ListRemove(NetUDPSockets, idx);																																		// Remove it from the UDP socket list
	
	while (sock->packet_queue->length != 0) {																															// Free all the packets that are in the queue
		UIntPtr data = (UIntPtr)QueueRemove(sock->packet_queue);
		
		if (sock->user) {																																				// Use MmFreeUserMemory?
			MmFreeUserMemory(data);																																		// Yes
		} else {
			MemFree(data);																																				// Nope
		}
	}
	
	QueueFree(sock->packet_queue);																																		// Free the packet queue struct itself
	MemFree((UIntPtr)sock);																																				// And free the socket struct itself
}

Void NetSendUDPSocket(PUDPSocket sock, UIntPtr len, PUInt8 buf) {
	if (sock == Null) {																																					// Sanity check
		return;
	}
	
	NetSendUDPPacket(sock->dev, sock->ipv4_address, sock->port, len, buf);																								// Send!
}

PIPHeader NetReceiveUDPSocket(PUDPSocket sock) {
	if (sock == Null) {																																					// Sanity check
		return Null;
	}
	
	UIntPtr count = 0;
	
	while (sock->packet_queue->length == 0) {																															// Wait the packet that we want :)
		if (++count == 401) {																																			// Our timeout is 4 seconds
			return Null;
		}
		
		PsSleep(10);																																					// Sleep 10ms
	}
	
	return (PIPHeader)QueueRemove(sock->packet_queue);																													// Now, return it!
}

static Void NetHandleARPPacket(PNetworkDevice dev, PARPHeader hdr) {
	if (hdr->htype == ToNetByteOrder16(hdr->htype)) {
		return;
	} else if (FromNetByteOrder16(hdr->ptype) != ETH_TYPE_IP) {
		return;
	}
	
	Boolean add = True;
	
	for (UIntPtr i = 0; i < 32; i++) {																																	// Let's update the arp cache!
		if (!dev->arp_cache[i].free && StrCompareMemory(dev->arp_cache[i].ipv4_address, hdr->ipv4.src_pr, 4)) {															// Update this one?
			StrCopyMemory(dev->arp_cache[i].mac_address, hdr->ipv4.src_hw, 6);																							// Yes :)
			add = False;
			break;
		}
	}
	
	if (add) {																																							// Add this entry to the cache?
		for (UIntPtr i = 0; i < 32; i++) {																																// Yes :)
			if (dev->arp_cache[i].free) {																																// This entry is free?
				add = False;
				dev->arp_cache[i].free = False;																															// Yes, but now we're using it
				
				StrCopyMemory(dev->arp_cache[i].mac_address, hdr->ipv4.src_hw, 6);																						// Set the mac address
				StrCopyMemory(dev->arp_cache[i].ipv4_address, hdr->ipv4.src_pr, 4);																						// And the ipv4 address
				
				break;
			}
		}
		
		if (add) {
			StrCopyMemory(dev->arp_cache[31].mac_address, hdr->ipv4.src_hw, 6);																							// ... We're going to use the last entry, set the mac address
			StrCopyMemory(dev->arp_cache[31].ipv4_address, hdr->ipv4.src_pr, 4);																						// And the ipv4 address
		}
	}
	
	if (StrCompareMemory(dev->ipv4_address, hdr->ipv4.dst_pr, 4)) {																										// For us?
		if (hdr->opcode == FromNetByteOrder16(ARP_OPC_REQUEST)) {																										// Yes, was a request?
			NetSendARPIPv4Packet(dev, hdr->ipv4.src_hw, hdr->ipv4.src_pr, ARP_OPC_REPLY);																				// Yes, so let's reply :)
		} else if ((hdr->opcode == FromNetByteOrder16(ARP_OPC_REPLY)) && (NetARPIPv4Sockets != Null)) {																	// Reply?
			ListForeach(NetARPIPv4Sockets, i) {																															// Yes, let's see if any process want it!
				PARPIPv4Socket sock = (PARPIPv4Socket)i->data;
				
				if (StrCompareMemory(sock->ipv4_address, hdr->ipv4.src_pr, 4)) {
					PsLockTaskSwitch(old);																																// Ok, lock task switch
					UIntPtr oldpd = MmGetCurrentDirectory();
					
					if (sock->user && (oldpd != sock->owner_process->dir)) {																							// We need to switch to another dir?
						MmSwitchDirectory(sock->owner_process->dir);																									// Yes
					}
					
					PARPHeader new = (PARPHeader)(sock->user ? MmAllocUserMemory(sizeof(ARPHeader)) : MemAllocate(sizeof(ARPHeader)));									// Alloc some space for copying the arp data
					
					if (new != Null) {
						StrCopyMemory(new, hdr, sizeof(ARPHeader));																										// Ok, copy it and add it to the queue
						QueueAdd(sock->packet_queue, new);
					}
					
					if (sock->user && (oldpd != sock->owner_process->dir)) {																							// We need to switch back?
						MmSwitchDirectory(oldpd);																														// Yes
					}
					
					PsUnlockTaskSwitch(old);
				}
			}
		}
	}
}

static Void NetHandleIPPacket(PNetworkDevice dev, PIPHeader hdr) {
	if (hdr->version != 4) {
		return;
	} else if (hdr->ttl == 0) {
		return;
	}
	
	if (StrCompareMemory(dev->ipv4_address, hdr->ipv4.dst, 4)) {																										// For us?
		if ((hdr->protocol == IP_PROTOCOL_UDP) && (NetUDPSockets != Null)) {																							// Yes, it's UDP?
			PUDPHeader uhdr = (PUDPHeader)(((UIntPtr)hdr) + 20);
			
			ListForeach(NetUDPSockets, i) {																																// Yes, let's see if any process want it!
				PUDPSocket sock = (PUDPSocket)i->data;
				
				if (uhdr->dport != ToNetByteOrder16(sock->port)) {																										// Check the port
					continue;																																			// ...
				}
				
				Boolean server = StrCompareMemory(sock->ipv4_address, dev->ipv4_address, 4);																			// Let's check if this is a server socket
				
				if ((server && StrCompareMemory(dev->ipv4_address, hdr->ipv4.dst, 4)) || (!server && StrCompareMemory(sock->ipv4_address, hdr->ipv4.src, 4))) {
					PsLockTaskSwitch(old);																																// Ok, lock task switch
					UIntPtr oldpd = MmGetCurrentDirectory();
					UIntPtr size = FromNetByteOrder16(hdr->length);
					
					if (sock->user && (oldpd != sock->owner_process->dir)) {																							// We need to switch to another dir?
						MmSwitchDirectory(sock->owner_process->dir);																									// Yes
					}
					
					PIPHeader new = (PIPHeader)(sock->user ? MmAllocUserMemory(size) : MemAllocate(size));																// Alloc some space for copying the ip+udp data
					
					if (new != Null) {
						StrCopyMemory(new, hdr, size);																													// Ok, copy it and add it to the queue
						QueueAdd(sock->packet_queue, new);
					}
					
					if (sock->user && (oldpd != sock->owner_process->dir)) {																							// We need to switch back?
						MmSwitchDirectory(oldpd);																														// Yes
					}
					
					PsUnlockTaskSwitch(old);
				}
			}
		}
	}
}

static Void NetHandleEthPacket(PNetworkDevice dev, UInt8 src[6], UInt16 type, PUInt8 buf) {
	if ((dev == Null) || (src == Null) || (buf == Null)) {																												// Sanity checks
		return;
	}
	
	if (type == ETH_TYPE_ARP) {																																			// ARP?
		NetHandleARPPacket(dev, (PARPHeader)buf);																														// Yes, handle it!
	} else if (type == ETH_TYPE_IP) {																																	// IP?
		NetHandleIPPacket(dev, (PIPHeader)buf);																															// Yes, handle it!
	}
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
