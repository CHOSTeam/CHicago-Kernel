// File author is Ítalo Lima Marconato Matias
//
// Created on May 11 of 2018, at 13:22 BRT
// Last edited on November 08 of 2019, at 18:26 BRT

#include <chicago/arch/gdt-int.h>

#include <chicago/string.h>

UInt8 GDTEntries[7][8];
TSSEntry GDTTSSEntry;

Void GDTSetGate(UInt8 num, UInt64 base, UInt64 limit, UInt8 type, UInt8 gran) {	
	GDTEntries[num][0] = limit & 0xFF;								// Encode the limit
	GDTEntries[num][1] = (limit >> 8) & 0xFF;
	GDTEntries[num][6] = (gran & 0xF0) | ((limit >> 16) & 0x0F);
	
	GDTEntries[num][2] = base & 0xFF;								// Encode the base
	GDTEntries[num][3] = (base >> 8) & 0xFF;
	GDTEntries[num][4] = (base >> 16) & 0xFF;
	GDTEntries[num][7] = (base >> 24) & 0xFF;
	
	GDTEntries[num][5] = type;										// Encode the type
}

Void GDTWriteTSS(UInt8 num, UInt64 rsp0) {
	UInt64 base = (UInt64)&GDTTSSEntry;
	UInt64 base3 = (base >> 32) & 0xFFFF;
	UInt64 base4 = (base >> 48) & 0xFFFF;
	UInt64 limit = sizeof(TSSEntry);
	
	GDTSetGate(num, base, limit, 0xE9, 0x00);						// Setup the TSS GDT descriptor
	GDTSetGate(num + 1, base4, base3, 0x00, 0x00);					// The TSS takes two entries
	StrSetMemory(&GDTTSSEntry, 0, sizeof(TSSEntry));				// Zero our TSS
	
	GDTTSSEntry.rsp0 = rsp0;										// Setup!
	GDTTSSEntry.iomap_base = sizeof(TSSEntry);
}

Void GDTSetKernelStack(UInt64 stack) {
	GDTTSSEntry.rsp0 = stack;
}

Void GDTInit(Void) {
	GDTSetGate(0, 0, 0, 0, 0);										// Null entry
	GDTSetGate(1, 0, 0xFFFFFFFF, 0x98, 0x20);						// Code entry
	GDTSetGate(2, 0, 0xFFFFFFFF, 0x92, 0x00);						// Data entry
	GDTSetGate(3, 0, 0xFFFFFFFF, 0xF8, 0x20);						// User mode code entry
	GDTSetGate(4, 0, 0xFFFFFFFF, 0xF2, 0x00);						// User mode data entry
	GDTWriteTSS(5, 0);												// TSS entry
	GDTLoad((UInt64)GDTEntries, sizeof(GDTEntries) - 1);			// Load new GDT
	TSSLoad(0x2B);													// Load the TSS
}
