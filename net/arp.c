// File author is Ítalo Lima Marconato Matias
//
// Created on August 25 of 2019, at 17:59 BRT
// Last edited on August 25 of 2019, at 18:48 BRT

#include <chicago/alloc.h>
#include <chicago/mm.h>
#include <chicago/net.h>
#include <chicago/string.h>

extern PNetworkDevice NetDefaultDevice;

PList NetARPIPv4Sockets = Null;

Void NetHandleARPPacket(PNetworkDevice dev, PARPHeader hdr) {
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
