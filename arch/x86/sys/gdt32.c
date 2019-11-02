// File author is Ítalo Lima Marconato Matias
//
// Created on May 11 of 2018, at 13:22 BRT
// Last edited on September 24 of 2018, at 13:24 BRT

#include <chicago/arch/gdt-int.h>

#include <chicago/string.h>

UInt8 GDTEntries[6][8];
TSSEntry GDTTSSEntry;

Void GDTSetGate(UInt8 num, UInt32 base, UInt32 limit, UInt8 type, UInt8 gran) {	
	GDTEntries[num][0] = limit & 0xFF;								// Encode the limit
	GDTEntries[num][1] = (limit >> 8) & 0xFF;
	GDTEntries[num][6] = (gran & 0xF0) | ((limit >> 16) & 0x0F);
	
	GDTEntries[num][2] = base & 0xFF;								// Encode the base
	GDTEntries[num][3] = (base >> 8) & 0xFF;
	GDTEntries[num][4] = (base >> 16) & 0xFF;
	GDTEntries[num][7] = (base >> 24) & 0xFF;
	
	GDTEntries[num][5] = type;										// Encode the type
}

Void GDTWriteTSS(UInt8 num, UInt32 ss0, UInt32 esp0) {
	UInt32 base = (UInt32)&GDTTSSEntry;
	UInt32 limit = base + sizeof(TSSEntry);
	
	GDTSetGate(num, base, limit, 0xE9, 0x00);						// Setup the TSS GDT descriptor
	StrSetMemory(&GDTTSSEntry, 0, sizeof(TSSEntry));				// Zero our TSS
	
	GDTTSSEntry.ss0 = ss0;											// Setup!
	GDTTSSEntry.esp0 = esp0;
	GDTTSSEntry.cs = 0x0B;
	GDTTSSEntry.ss = 0x13;
	GDTTSSEntry.ds = 0x13;
	GDTTSSEntry.es = 0x13;
	GDTTSSEntry.fs = 0x13;
	GDTTSSEntry.gs = 0x13;
	GDTTSSEntry.iomap_base = sizeof(TSSEntry);
}

Void GDTSetKernelStack(UInt32 stack) {
	GDTTSSEntry.esp0 = stack;
}

Void GDTInit(Void) {
	GDTSetGate(0, 0, 0, 0, 0);										// Null entry
	GDTSetGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);						// Code entry
	GDTSetGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);						// Data entry
	GDTSetGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);						// User mode code entry
	GDTSetGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);						// User mode data entry
	GDTWriteTSS(5, 0x10, 0);										// TSS entry
	GDTLoad((UInt32)GDTEntries, sizeof(GDTEntries) * 6 - 1);		// Load new GDT
	TSSLoad(0x2B);													// Load the TSS
}
