// File author is Ítalo Lima Marconato Matias
//
// Created on August 25 of 2019, at 18:05 BRT
// Last edited on August 27 of 2019, at 16:49 BRT

#include <chicago/alloc.h>
#include <chicago/mm.h>
#include <chicago/net.h>
#include <chicago/string.h>

extern PNetworkDevice NetDefaultDevice;

PList NetUDPSockets = Null;

Void NetHandleUDPPacket(PNetworkDevice dev, PIPv4Header hdr, PUDPHeader uhdr) {
	if (FromNetByteOrder16(uhdr->sport) == 67 && FromNetByteOrder16(uhdr->dport) == 68) {																				// DHCP?
		NetHandleDHCPv4Packet(dev, (PDHCPv4Header)(((UIntPtr)uhdr) + sizeof(UDPHeader)));																				// Yes, handle it!
	} else if (NetUDPSockets != Null) {																																	// Is our UDP socket list initialized/not-null?
		ListForeach(NetUDPSockets, i) {																																	// Yes, let's see if any process want it!
			PUDPSocket sock = (PUDPSocket)i->data;
			
			if (uhdr->dport != ToNetByteOrder16(sock->port)) {																											// Check the port
				continue;																																				// ...
			}
			
			Boolean server = StrCompareMemory(sock->ipv4_address, dev->ipv4_address, 4);																				// Let's check if this is a server socket
			
			if ((server && StrCompareMemory(dev->ipv4_address, hdr->dst, 4)) || (!server && StrCompareMemory(sock->ipv4_address, hdr->src, 4))) {
				PsLockTaskSwitch(old);																																	// Ok, lock task switch
				UIntPtr oldpd = MmGetCurrentDirectory();
				UIntPtr size = FromNetByteOrder16(hdr->length);
				
				if (sock->user && (oldpd != sock->owner_process->dir)) {																								// We need to switch to another dir?
					MmSwitchDirectory(sock->owner_process->dir);																										// Yes
				}
				
				PIPv4Header new = (PIPv4Header)(sock->user ? MmAllocUserMemory(size) : MemAllocate(size));																// Alloc some space for copying the ip+udp data
				
				if (new != Null) {
					StrCopyMemory(new, hdr, size);																														// Ok, copy it and add it to the queue
					QueueAdd(sock->packet_queue, new);
				}
				
				if (sock->user && (oldpd != sock->owner_process->dir)) {																								// We need to switch back?
					MmSwitchDirectory(oldpd);																															// Yes
				}
				
				PsUnlockTaskSwitch(old);
			}
		}
	}
}

Void NetSendUDPPacket(PNetworkDevice dev, UInt8 dest[4], UInt16 sport, UInt16 dport, UIntPtr len, PUInt8 buf) {
	if (dev == Null) {																																					// dev = default?
		dev = NetDefaultDevice;																																			// Yes
	}
	
	if ((dev == Null) || (dest == Null)) {																																// Sanity checks
		return;
	}
	
	PUDPHeader hdr = (PUDPHeader)MemZAllocate(sizeof(UDPHeader) + len);																									// Let's build our UDP packet!
	
	if (hdr == Null) {
		return;																																							// Failed to alloc :(
	}
	
	hdr->sport = ToNetByteOrder16(sport);																																// Set the source port
	hdr->dport = ToNetByteOrder16(dport);																																// Set the destination port
	hdr->length = ToNetByteOrder16(((UInt16)(sizeof(UDPHeader) + len)));																								// Set the length
	
	StrCopyMemory(((PUInt8)hdr) + sizeof(UDPHeader), buf, len);																											// Copy the data
	NetSendIPv4Packet(dev, dest, IP_PROTOCOL_UDP, sizeof(UDPHeader) + len, (PUInt8)hdr);																				// Send!
	MemFree((UIntPtr)hdr);
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
	
	ListForeach(NetUDPSockets, i) {																																		// Try to find it on the UDP socket list
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
	
	NetSendUDPPacket(sock->dev, sock->ipv4_address, sock->port, sock->port, len, buf);																					// Send!
}

PIPv4Header NetReceiveUDPSocket(PUDPSocket sock) {
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
	
	return (PIPv4Header)QueueRemove(sock->packet_queue);																												// Now, return it!
}
