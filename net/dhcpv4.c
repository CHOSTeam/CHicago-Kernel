// File author is Ítalo Lima Marconato Matias
//
// Created on August 27 of 2019, at 16:10 BRT
// Last edited on August 27 of 2019, at 17:30 BRT

#include <chicago/alloc.h>
#include <chicago/net.h>
#include <chicago/string.h>

extern PNetworkDevice NetDefaultDevice;

static UInt8 NetBroadcastIPv4Address[4] = { 255, 255, 255, 255 };

static Void NetGetDHCPv4ReplyInfo(PDHCPv4Header hdr, PUInt8 type, PUInt8 sip) {
	for (UIntPtr i = 0;; i++) {																					// Let's go (TODO! Use the ->length of the IPv4Header to limit this, so it doesn't go out of bounds)
		if (hdr->options[i] == 0xFF) {																			// End?
			return;																								// Yes :(
		} else if (hdr->options[i] == 0x35) {																	// Message type?
			*type = hdr->options[(++i) + 1];																	// Yes
		} else if (hdr->options[i] == 0x36) {																	// Server IP?
			StrCopyMemory(sip, &hdr->options[i + 2], 4);														// Yes
			i += 5;
		}
	}
}

Void NetHandleDHCPv4Packet(PNetworkDevice dev, PDHCPv4Header hdr) {
	if (hdr->op != 0x02 || !StrCompareMemory(hdr->chaddr, dev->mac_address, 6)) {								// For us?
		return;																									// Nope
	}
	
	UInt8 ip[4];
	UInt8 sip[4];
	UInt8 type = 0;
	
	StrCopyMemory(ip, hdr->yiaddr, 4);																			// Get our ip address
	NetGetDHCPv4ReplyInfo(hdr, &type, sip);																		// Get some info from the reply
	
	if (type == 0x02) {																							// IPv4 address offer for us?
		PDHCPv4Header ohdr = (PDHCPv4Header)MemZAllocate(sizeof(DHCPv4Header) + 16);							// Try to alloc the DHCPv4 header
		
		if (ohdr == Null) {
			return;																								// Failed :(
		}
		
		ohdr->op = 0x01;																						// Set the op to REQUEST
		ohdr->htype = 0x01;																						// Set the htype to ETHERNET
		ohdr->hlen = 0x06;																						// And the hlen to 6 bytes (eth addresses have 6 bytes)
		ohdr->xid = hdr->xid;																					// Set the transaction id
		ohdr->magic = 0x63538263;																				// Set the magic cookie
		ohdr->options[0] = 0x35;																				// Set the message type
		ohdr->options[1] = 0x01;
		ohdr->options[2] = 0x03;																				// (to DHCPREQUEST)
		ohdr->options[3] = 0x32;																				// Let's put the IP that we want/got
		ohdr->options[4] = 0x04;
		ohdr->options[5] = ip[0];
		ohdr->options[6] = ip[1];
		ohdr->options[7] = ip[2];
		ohdr->options[8] = ip[3];
		ohdr->options[9] = 0x36;																				// Also let's put the server IP
		ohdr->options[10] = 0x04;
		ohdr->options[11] = sip[0];
		ohdr->options[12] = sip[1];
		ohdr->options[13] = sip[2];
		ohdr->options[14] = sip[3];
		ohdr->options[15] = 0xFF;																				// End it!
		
		StrCopyMemory(ohdr->siaddr, sip, 4);																	// Copy back the server address
		StrCopyMemory(ohdr->chaddr, dev->mac_address, 6);														// Copy our mac address
		NetSendUDPPacket(dev, NetBroadcastIPv4Address, 68, 67, sizeof(DHCPv4Header) + 16, (PUInt8)ohdr);		// Send the packet!
		MemFree((UIntPtr)ohdr);																					// Free the header
	} else if (type == 0x05) {																					// Transction finished and we can use this IP?
		StrCopyMemory(dev->ipv4_address, ip, 4);																// Yes :)
	}
}

Void NetSendDHCPv4Discover(PNetworkDevice dev) {
	if (dev == Null) {																							// dev = default?
		dev = NetDefaultDevice;																					// Yes
		
		if (dev == Null) {																						// Sanity check
			return;
		}
	}
	
	PDHCPv4Header hdr = (PDHCPv4Header)MemZAllocate(sizeof(DHCPv4Header) + 4);									// Try to alloc the DHCPv4 header
	
	if (hdr == Null) {
		return;																									// Failed :(
	}
	
	hdr->op = 0x01;																								// Set the op to REQUEST
	hdr->htype = 0x01;																							// Set the htype to ETHERNET
	hdr->hlen = 0x06;																							// And the hlen to 6 bytes (eth addresses have 6 bytes)
	hdr->magic = 0x63538263;																					// Set the magic cookie
	hdr->options[0] = 0x35;																						// Set the message type
	hdr->options[1] = 0x01;
	hdr->options[2] = 0x01;																						// (to DHCPDISCOVER)
	hdr->options[3] = 0xFF;																						// End it!
	
	StrCopyMemory(hdr->chaddr, dev->mac_address, 6);															// Copy our mac address
	NetSendUDPPacket(dev, NetBroadcastIPv4Address, 68, 67, sizeof(DHCPv4Header) + 7, (PUInt8)hdr);				// Send the packet!
	MemFree((UIntPtr)hdr);																						// Free the header
}
