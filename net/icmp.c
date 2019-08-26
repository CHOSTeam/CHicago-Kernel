// File author is Ítalo Lima Marconato Matias
//
// Created on August 26 of 2019, at 17:42 BRT
// Last edited on August 26 of 2019, at 19:43 BRT

#include <chicago/alloc.h>
#include <chicago/mm.h>
#include <chicago/net.h>
#include <chicago/string.h>

extern PNetworkDevice NetDefaultDevice;

PList NetICMPv4Sockets = Null;

Void NetHandleICMPv4(PNetworkDevice dev, PIPv4Header hdr, PICMPHeader ihdr) {
	if (ihdr->type == ICMP_REPLY && NetICMPv4Sockets != Null) {																				// Reply? Do we need to process it?
		ListForeach(NetICMPv4Sockets, i) {																									// Yes, let's see if any process want it!
			PICMPv4Socket sock = (PICMPv4Socket)i->data;
			
			if (StrCompareMemory(sock->ipv4_address, hdr->src, 4)) {
				PsLockTaskSwitch(old);																										// Ok, lock task switch
				UIntPtr oldpd = MmGetCurrentDirectory();
				
				if (sock->user && (oldpd != sock->owner_process->dir)) {																	// We need to switch to another dir?
					MmSwitchDirectory(sock->owner_process->dir);																			// Yes
				}
				
				UInt16 size = FromNetByteOrder16(hdr->length) - sizeof(IPv4Header);															// Let's get the size of the ICMP packet
				PICMPHeader new = (PICMPHeader)(sock->user ? MmAllocUserMemory(size) : MemAllocate(size));									// Alloc some space for copying the icmp reply data
				
				if (new != Null) {
					StrCopyMemory(new, ihdr, size);																							// Ok, copy it and add it to the queue
					QueueAdd(sock->packet_queue, new);
				}
				
				if (sock->user && (oldpd != sock->owner_process->dir)) {																	// We need to switch back?
					MmSwitchDirectory(oldpd);																								// Yes
				}
				
				PsUnlockTaskSwitch(old);
			}
		}
	} else if (ihdr->type == ICMP_REQUEST) {																								// Request?
		UInt16 size = FromNetByteOrder16(hdr->length) - sizeof(IPv4Header);																	// Yes, first, get the size of the ICMP packet
		
		ihdr->type = ICMP_REPLY;																											// Now, set the type
		ihdr->checksum = 0;																													// And get the checksum
		ihdr->checksum = NetGetChecksum((PUInt8)ihdr, size);
		
		NetSendIPv4Packet(dev, hdr->src, IP_PROTOCOL_ICMP, size, (PUInt8)ihdr);																// Finally, send!
	}
}

Void NetSendICMPv4Request(PNetworkDevice dev, UInt8 dest[4]) {
	if (dev == Null) {																														// dev = default?
		dev = NetDefaultDevice;																												// Yes
	}
	
	if ((dev == Null) || (dest == Null)) {																									// Sanity checks
		return;
	}
	
	
	PICMPHeader hdr = (PICMPHeader)MemZAllocate(sizeof(ICMPHeader));																		// Alloc space for the packet
	
	if (hdr == Null) {
		return;																																// Failed :(
	}
	
	hdr->type = ICMP_REQUEST;																												// Set the type
	hdr->checksum = NetGetChecksum((PUInt8)hdr, sizeof(ICMPHeader));																		// Get the checksum
	
	NetSendIPv4Packet(dev, dest, IP_PROTOCOL_ICMP, sizeof(ICMPHeader), (PUInt8)hdr);														// Send it!
	MemFree((UIntPtr)hdr);																													// And free the packet
}

PICMPv4Socket NetAddICMPv4Socket(PNetworkDevice dev, UInt8 ipv4[4], Boolean user) {
	if (dev == Null) {																														// dev = default?
		dev = NetDefaultDevice;																												// Yes
	}
	
	if (NetICMPv4Sockets == Null) {																											// Init the ICMPv4 socket list?
		NetICMPv4Sockets = ListNew(False, False);																							// Yes :)
		
		if (NetICMPv4Sockets == Null) {
			return Null;																													// Failed :(
		}
	}
	
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (dev == Null) || (ipv4 == Null)) {										// Sanity checks
		return Null;
	}
	
	PICMPv4Socket sock = (PICMPv4Socket)MemAllocate(sizeof(ICMPv4Socket));																	// Let's create our socket
	
	if (sock == Null) {
		return Null;
	}
	
	sock->user = user;																														// Set if this is a user socket
	sock->dev = dev;																														// Set our device
	sock->packet_queue = QueueNew(user);																									// Create our packet queue
	
	if (sock->packet_queue == Null) {
		MemFree((UIntPtr)sock);																												// Failed
		return Null;
	}
	
	StrCopyMemory(sock->ipv4_address, ipv4, 4);																								// Set the dest ipv4 address
	
	sock->owner_process = PsCurrentProcess;																									// And the owner process!
	
	if (!ListAdd(NetICMPv4Sockets, sock)) {																									// Now, try to add it to the ICMPv4 socket list!
		QueueFree(sock->packet_queue);																										// Failed :(
		MemFree((UIntPtr)sock);
		return Null;
	}
	
	return sock;
}

Void NetRemoveICMPv4Socket(PICMPv4Socket sock) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (NetICMPv4Sockets == Null) || (sock == Null)) {							// Sanity checks
		return;
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	
	ListForeach(NetICMPv4Sockets, i) {																										// Try to find it on the ICMPv4 socket list
		if (i->data == sock) {
			found = True;																													// Found!
			break;
		} else {
			idx++;	
		}
	}
	
	if (!found) {
		return;																																// Not found...
	} else if (sock->owner_process != PsCurrentProcess) {
		return;
	}
	
	ListRemove(NetICMPv4Sockets, idx);																										// Remove it from the ICMPv4 socket list
	
	while (sock->packet_queue->length != 0) {																								// Free all the packets that are in the queue
		UIntPtr data = (UIntPtr)QueueRemove(sock->packet_queue);
		
		if (sock->user) {																													// Use MmFreeUserMemory?
			MmFreeUserMemory(data);																											// Yes
		} else {
			MemFree(data);																													// Nope
		}
	}
	
	QueueFree(sock->packet_queue);																											// Free the packet queue struct itself
	MemFree((UIntPtr)sock);																													// And free the socket struct itself
}

Void NetSendICMPv4Socket(PICMPv4Socket sock) {
	if (sock == Null) {																														// Sanity check
		return;
	}
	
	NetSendICMPv4Request(sock->dev, sock->ipv4_address);																					// Send!
}

PICMPHeader NetReceiveICMPv4Socket(PICMPv4Socket sock) {
	if (sock == Null) {																														// Sanity check
		return Null;
	}
	
	UIntPtr count = 0;
	
	while (sock->packet_queue->length == 0) {																								// Wait the packet that we want :)
		if (++count == 401) {																												// Our timeout is 4 seconds
			return Null;
		}
		
		PsSleep(10);																														// Sleep 10ms
	}
	
	return (PICMPHeader)QueueRemove(sock->packet_queue);																					// Now, return it!
}
