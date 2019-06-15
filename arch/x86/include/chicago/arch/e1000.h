// File author is Ítalo Lima Marconato Matias
//
// Created on December 11 of 2018, at 19:41 BRT
// Last edited on June 15 of 2019, at 11:08 BRT

#ifndef __CHICAGO_ARCH_E1000_H__
#define __CHICAGO_ARCH_E1000_H__

#include <chicago/net.h>

typedef struct {
	UInt64 addr;
	UInt16 length;
	UInt16 checksum;
	UInt8 status;
	UInt8 errors;
	UInt16 special;
} Packed E1000RXDesc, *PE1000RXDesc;

typedef struct {
	UInt64 addr;
	UInt16 length;
	UInt8 cso;
	UInt8 cmd;
	UInt8 status;
	UInt8 css;
	UInt16 special;
} Packed E1000TXDesc, *PE1000TXDesc;

typedef struct {
	UInt16 rx_cur;
	UInt16 tx_cur;
	Boolean eeprom;
	Boolean use_io;
	UInt16 io_base;
	UIntPtr mem_base;
	PNetworkDevice ndev;
	UInt8 mac_address[6];
	PE1000RXDesc rx_descs;
	UIntPtr rx_descs_buffs[32];
	PE1000TXDesc tx_descs;
	UIntPtr tx_descs_buffs[8];
	UIntPtr rx_descs_phys;
	UIntPtr tx_descs_phys;
} E1000Device, *PE1000Device;

Void E1000Init(Void);

#endif		// __CHICAGO_ARCH_E1000_H__
