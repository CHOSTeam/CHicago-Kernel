// File author is Ítalo Lima Marconato Matias
//
// Created on January 17 of 2019, at 21:07 BRT
// Last edited on October 29 of 2019, at 17:06 BRT

#ifndef __CHICAGO_ARCH_AHCI_H__
#define __CHICAGO_ARCH_AHCI_H__

#include <chicago/types.h>

#define FIS_TYPE_REG_H2D 0x27

typedef volatile struct {
#ifdef ARCH_64
	UInt64 clb;
#else
	UInt32 clb;
	UInt32 clbu;
#endif
#ifdef ARCH_64
	UInt64 fb;
#else
	UInt32 fb;
	UInt32 fbu;
#endif
	UInt32 is;
	UInt32 ie;
	UInt32 cmd;
	UInt32 res0;
	UInt32 tfd;
	UInt32 sig;
	UInt32 ssts;
	UInt32 sctl;
	UInt32 serr;
	UInt32 sact;
	UInt32 ci;
	UInt32 sntf;
	UInt32 fbs;
	UInt32 res1[11];
	UInt32 vendor[4];
} HBAPort, *PHBAPort;

typedef volatile struct {
	UInt32 cap;
	UInt32 ghc;
	UInt32 is;
	UInt32 pi;
	UInt32 vs;
	UInt32 cccctl;
	UInt32 cccpts;
	UInt32 emloc;
	UInt32 emctl;
	UInt32 cap2;
	UInt32 bohc;
	UInt8 res[0x74];
	UInt8 vendor[0x60];
	HBAPort ports[32];
} HBAMem, *PHBAMem;

typedef struct {
	UInt8 cfl : 5;
	UInt8 atapi : 1;
	UInt8 w : 1;
	UInt8 prefetch : 1;
	UInt8 reset : 1;
	UInt8 bist : 1;
	UInt8 clear : 1;
	UInt8 res0 : 1;
	UInt8 pmp : 4;
	UInt16 prdtl;
	Volatile UInt32 prdbc;
#ifdef ARCH_64
	UInt64 ctba;
#else
	UInt32 ctba;
	UInt32 ctbau;
#endif
	UInt32 res1[4];
} HBACmd, *PHBACmd;

typedef struct {
#ifdef ARCH_64
	UInt64 dba;
#else
	UInt32 dba;
	UInt32 dbau;
#endif
	UInt32 res0;
	UInt32 dbc : 22;
	UInt32 res1 : 9;
	UInt32 i : 1;
} HBAPRDTEntry, *PHBAPRDTEntry;

typedef struct {
	UInt8 cfis[64];
	UInt8 acmd[16];
	UInt8 res[48];
	HBAPRDTEntry prdt[8];
} HBACmdTbl, *PHBACmdTbl;

typedef struct {
	UInt8 type;
	UInt8 pmport : 4;
	UInt8 res0 : 3;
	UInt8 c : 1;
	UInt8 cmd;
	UInt8 featl;
	UInt8 lba0;
	UInt8 lba1;
	UInt8 lba2;
	UInt8 dev;
	UInt8 lba3;
	UInt8 lba4;
	UInt8 lba5;
	UInt8 festh;
	UInt8 cntl;
	UInt8 cnth;
	UInt8 icc;
	UInt8 ctrl;
	UInt8 res1[4];
} FISRegH2D, *PFISRegH2D;

typedef struct {
	PHBAPort port;
	Boolean atapi;
	UIntPtr virt;
} AHCIDevice, *PAHCIDevice;

Void AHCIInit(Void);

#endif		// __CHICAGO_ARCH_AHCI_H__
