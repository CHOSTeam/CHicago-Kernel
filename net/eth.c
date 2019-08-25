// File author is Ítalo Lima Marconato Matias
//
// Created on August 25 of 2019, at 18:03 BRT
// Last edited on August 25 of 2019, at 18:17 BRT

#include <chicago/alloc.h>
#include <chicago/net.h>
#include <chicago/string.h>

extern PNetworkDevice NetDefaultDevice;

Void NetHandleEthPacket(PNetworkDevice dev, UInt8 src[6], UInt16 type, PUInt8 buf) {
	if ((dev == Null) || (src == Null) || (buf == Null)) {																												// Sanity checks
		return;
	}
	
	if (type == ETH_TYPE_ARP) {																																			// ARP?
		NetHandleARPPacket(dev, (PARPHeader)buf);																														// Yes, handle it!
	} else if (type == ETH_TYPE_IP) {																																	// IP?
		NetHandleIPv4Packet(dev, (PIPHeader)buf);																														// Yes, handle it!
	}
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
