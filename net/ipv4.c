// File author is Ítalo Lima Marconato Matias
//
// Created on August 25 of 2019, at 18:10 BRT
// Last edited on August 27 of 2019, at 16:47 BRT

#include <chicago/alloc.h>
#include <chicago/mm.h>
#include <chicago/net.h>
#include <chicago/string.h>

extern PNetworkDevice NetDefaultDevice;

Void NetHandleIPv4Packet(PNetworkDevice dev, PIPv4Header hdr) {
	static UInt8 broadcast[4] = { 255, 255, 255, 255 };
	
	if (hdr->version != 4) {																																			// IPv4?
		return;																																							// We only have IPv4 for now...
	} else if (hdr->ttl == 0) {																																			// Time To Live > 0?
		return;																																							// Nope :(
	} else if (!StrCompareMemory(dev->ipv4_address, hdr->dst, 4) && !StrCompareMemory(broadcast, hdr->dst, 4)) {														// For us?
		return;																																							// Nope :)
	}
	
	UInt16 checksum = hdr->checksum;
	
	hdr->checksum = 0;
	
	if (NetGetChecksum((PUInt8)hdr, sizeof(IPv4Header)) != checksum) {																									// The checksum is valid?
		return;																																							// No
	} else if (hdr->protocol == IP_PROTOCOL_ICMP) {																														// It's ICMP?
		NetHandleICMPv4Packet(dev, hdr, (PICMPHeader)(((UIntPtr)hdr) + sizeof(IPv4Header)));																			// Yes, handle it!
	} else if (hdr->protocol == IP_PROTOCOL_UDP) {																														// It's UDP?
		NetHandleUDPPacket(dev, hdr, (PUDPHeader)(((UIntPtr)hdr) + sizeof(IPv4Header)));																				// Yes, handle it!
	}
}

static Boolean NetResolveIPv4Address(PNetworkDevice dev, UInt8 ip[4], PUInt8 dest) {
	UInt8 broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	UIntPtr last = 0;
	UIntPtr count = 0;
	
	if ((ip[0] != 10) && ((ip[0] != 172) || ((ip[1] < 16) || (ip[1] > 31))) && ((ip[0] != 192) || (ip[1] != 168))) {													// First, let's check if we really need to use arp to get the mac address
		StrCopyMemory(dest, broadcast, 6);																																// Nope, it's outside of the local network, the network card should do everything
		return True;
	} else if (((ip[0] == 127) && (ip[1] == 0) && (ip[2] == 0) && (ip[3] == 1)) || StrCompareMemory(ip, dev->ipv4_address, 6)) {										// Loopback/localhost/ourselves?
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
	
	PIPv4Header hdr = (PIPv4Header)MemZAllocate(sizeof(IPv4Header) + len);																								// Let's build our IPv4 header
	
	if (hdr == Null) {
		return;																																							// Failed :(
	}
	
	hdr->ihl = 5;																																						// "Default" value
	hdr->version = 4;																																					// IPv4
	hdr->length = ToNetByteOrder16((len + sizeof(IPv4Header)));																											// Header length + data length
	hdr->ttl = 64;																																						// Time To Live = 64
	hdr->protocol = protocol;																																			// Set the protocol
	
	StrCopyMemory(hdr->src, dev->ipv4_address, 4);																														// Set the src ipv4 addresss (our address)
	StrCopyMemory(hdr->dst, dest, 4);																																	// And the dest ipv4 address
	
	hdr->checksum = NetGetChecksum((PUInt8)hdr, sizeof(IPv4Header));																									// And get the checkum
	
	StrCopyMemory((PUInt8)(((UIntPtr)hdr) + sizeof(IPv4Header)), buf, len);																								// Copy the data
	NetSendEthPacket(dev, destm, ETH_TYPE_IP, len + sizeof(IPv4Header), (PUInt8)hdr);																					// Send the packet!
	MemFree((UIntPtr)hdr);																																				// And free everything
}
