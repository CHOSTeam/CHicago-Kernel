// File author is Ítalo Lima Marconato Matias
//
// Created on August 25 of 2019, at 18:10 BRT
// Last edited on August 25 of 2019, at 18:18 BRT

#include <chicago/alloc.h>
#include <chicago/mm.h>
#include <chicago/net.h>
#include <chicago/string.h>

extern PNetworkDevice NetDefaultDevice;

Void NetHandleIPv4Packet(PNetworkDevice dev, PIPHeader hdr) {
	if (hdr->version != 4) {
		return;
	} else if (hdr->ttl == 0) {
		return;
	}
	
	if (StrCompareMemory(dev->ipv4_address, hdr->ipv4.dst, 4)) {																										// For us?
		if (hdr->protocol == IP_PROTOCOL_UDP) {																															// Yes, it's UDP?
			NetHandleUDPPacket(dev, hdr, (PUDPHeader)(((UIntPtr)hdr) + 20));																							// Yes, handle it!
		}
	}
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
