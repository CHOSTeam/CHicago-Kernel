// File author is Ítalo Lima Marconato Matias
//
// Created on December 12 of 2018, at 12:25 BRT
// Last edited on August 27 of 2019, at 17:01 BRT

#ifndef __CHICAGO_NET_H__
#define __CHICAGO_NET_H__

#include <chicago/process.h>
#include <chicago/queue.h>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ToNetByteOrder16(v) (v)
#define ToNetByteOrder32(v) (v)
#define FromNetByteOrder16(v) (v)
#define FromNetByteOrder32(v) (v)
#else
#define ToNetByteOrder16(v) ((UInt16)((v >> 8) | (v << 8)))
#define ToNetByteOrder32(v) (((v >> 24) & 0xFF) | ((v << 8) & 0xFF0000) | ((v >> 8) & 0xFF00) | ((v << 24) & 0xFF000000))
#define FromNetByteOrder16(v) ((UInt16)((v >> 8) | (v << 8)))
#define FromNetByteOrder32(v) (((v >> 24) & 0xFF) | ((v << 8) & 0xFF0000) | ((v >> 8) & 0xFF00) | ((v << 24) & 0xFF000000))
#endif

#define ETH_TYPE_IP 0x800
#define ETH_TYPE_ARP 0x806

#define ARP_OPC_REQUEST 0x01
#define ARP_OPC_REPLY 0x02 

#define IP_PROTOCOL_ICMP 0x01
#define IP_PROTOCOL_UDP 0x11

#define ICMP_REPLY 0x00
#define ICMP_REQUEST 0x08

typedef struct {
	UInt8 dst[6];
	UInt8 src[6];
	UInt16 type;
} Packed EthFrame, *PEthFrame;

typedef struct {
	UInt16 htype;
	UInt16 ptype;
	UInt8 hlen;
	UInt8 plen;
	UInt16 opcode;
	struct {
		UInt8 src_hw[6];
		UInt8 src_pr[4];
		UInt8 dst_hw[6];
		UInt8 dst_pr[4];
	} ipv4;
} Packed ARPHeader, *PARPHeader;

typedef struct {
	UInt8 ihl : 4;
	UInt8 version : 4;
	UInt8 ecn : 2;
	UInt8 dscp : 6;
	UInt16 length;
	UInt16 id;
	UInt8 flags : 3;
	UInt16 frag_off : 13;
	UInt8 ttl;
	UInt8 protocol;
	UInt16 checksum;
	UInt8 src[4];
	UInt8 dst[4];
} Packed IPv4Header, *PIPv4Header;

typedef struct {
	UInt8 src[4];
	UInt8 dst[4];
	UInt8 res;
	UInt8 protocol;
	UInt16 length;
} Packed UDPTCPPseudoHeader, *PUDPTCPPseudoHeader;

typedef struct {
	UInt8 type;
	UInt8 code;
	UInt16 checksum;
	UInt32 data;
} Packed ICMPHeader, *PICMPHeader;

typedef struct {
	UInt16 sport;
	UInt16 dport;
	UInt16 length;
	UInt16 checksum;
} Packed UDPHeader, *PUDPHeader;

typedef struct {
	UInt8 op;
	UInt8 htype;
	UInt8 hlen;
	UInt8 hops;
	UInt32 xid;
	UInt16 secs;
	UInt16 flags;
	UInt8 ciaddr[4];
	UInt8 yiaddr[4];
	UInt8 siaddr[4];
	UInt8 giaddr[4];
	UInt8 chaddr[208];
	UInt32 magic;
	UInt8 options[0];
} Packed DHCPv4Header, *PDHCPv4Header;

typedef struct {
	Boolean free;
	UInt8 mac_address[6];
	UInt8 ipv4_address[4];
} ARPCache, *PARPCache;

typedef struct {
	UIntPtr id;
	PVoid priv;
	PQueue packet_queue;
	UInt8 mac_address[6];
	UInt8 ipv4_address[4];
	ARPCache arp_cache[32];
	Lock packet_queue_rlock;
	Lock packet_queue_wlock;
	Void (*send)(PVoid, UIntPtr, PUInt8);
} NetworkDevice, *PNetworkDevice;

typedef struct {
	Boolean user;
	PNetworkDevice dev;
	PQueue packet_queue;
	UInt8 mac_address[6];
	UInt8 ipv4_address[4];
	PProcess owner_process;
} ARPIPv4Socket, *PARPIPv4Socket;

typedef struct {
	Boolean user;
	PNetworkDevice dev;
	PQueue packet_queue;
	UInt8 ipv4_address[4];
	PProcess owner_process;
} ICMPv4Socket, *PICMPv4Socket;

typedef struct {
	UInt16 port;
	Boolean user;
	PNetworkDevice dev;
	PQueue packet_queue;
	UInt8 ipv4_address[4];
	PProcess owner_process;
} UDPSocket, *PUDPSocket;

PNetworkDevice NetAddDevice(PVoid priv, UInt8 mac[6], Void (*send)(PVoid, UIntPtr, PUInt8));
PNetworkDevice NetGetDevice(PFsNode dev);
Void NetRemoveDevice(PNetworkDevice dev);
Void NetSetDefaultDevice(PNetworkDevice dev);
PNetworkDevice NetGetDefaultDevice(Void);
Void NetDevicePushPacket(PNetworkDevice dev, PUInt8 packet);
PUInt8 NetDevicePopPacket(PNetworkDevice dev);
UInt16 NetGetChecksum(PUInt8 data, UIntPtr len);
Void NetHandleEthPacket(PNetworkDevice dev, UInt8 src[6], UInt16 type, PUInt8 buf);
Void NetHandleARPPacket(PNetworkDevice dev, PARPHeader hdr);
Void NetHandleIPv4Packet(PNetworkDevice dev, PIPv4Header hdr);
Void NetHandleICMPv4Packet(PNetworkDevice dev, PIPv4Header hdr, PICMPHeader ihdr);
Void NetHandleUDPPacket(PNetworkDevice dev, PIPv4Header hdr, PUDPHeader uhdr);
Void NetHandleDHCPv4Packet(PNetworkDevice dev, PDHCPv4Header hdr);
Void NetSendRawPacket(PNetworkDevice dev, UIntPtr len, PUInt8 buf);
Void NetSendEthPacket(PNetworkDevice dev, UInt8 dest[6], UInt16 type, UIntPtr len, PUInt8 buf);
Void NetSendARPIPv4Packet(PNetworkDevice dev, UInt8 destm[6], UInt8 desti[4], UInt16 opcode);
Void NetSendIPv4Packet(PNetworkDevice dev, UInt8 dest[4], UInt8 protocol, UIntPtr len, PUInt8 buf);
Void NetSendICMPv4Request(PNetworkDevice dev, UInt8 dest[4]);
Void NetSendUDPPacket(PNetworkDevice dev, UInt8 dest[4], UInt16 sport, UInt16 dport, UIntPtr len, PUInt8 buf);
Void NetSendDHCPv4Discover(PNetworkDevice dev);
PARPIPv4Socket NetAddARPIPv4Socket(PNetworkDevice dev, UInt8 mac[6], UInt8 ipv4[4], Boolean user);
Void NetRemoveARPIPv4Socket(PARPIPv4Socket sock);
Void NetSendARPIPv4Socket(PARPIPv4Socket sock, UInt16 opcode);
PARPHeader NetReceiveARPIPv4Socket(PARPIPv4Socket sock);
PICMPv4Socket NetAddICMPv4Socket(PNetworkDevice dev, UInt8 ipv4[4], Boolean user);
Void NetRemoveICMPv4Socket(PICMPv4Socket sock);
Void NetSendICMPv4Socket(PICMPv4Socket sock);
PICMPHeader NetReceiveICMPv4Socket(PICMPv4Socket sock);
PUDPSocket NetAddUDPSocket(PNetworkDevice dev, UInt8 ipv4[4], UInt16 port, Boolean user);
Void NetRemoveUDPSocket(PUDPSocket sock);
Void NetSendUDPSocket(PUDPSocket sock, UIntPtr len, PUInt8 buf);
PIPv4Header NetReceiveUDPSocket(PUDPSocket sock);
Void NetFinish(Void);

#endif		// __CHICAGO_NET_H__
