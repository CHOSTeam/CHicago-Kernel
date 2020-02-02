// File author is Ítalo Lima Marconato Matias
//
// Created on October 12 of 2018, at 18:17 BRT
// Last edited on November 03 of 2018, at 17:54 BRT

#include <chicago/arch.h>
#include <chicago/process.h>

UIntPtr RandSeed = 1;

Void RandSetSeed(UIntPtr seed) {
	RandSeed = seed;
}

UIntPtr RandGenerateSeed(Void) {
	if (PsCurrentProcess != Null) {																								// We can use the process id?
		return ArchGetSeconds() * ((PsCurrentProcess->id == 0) ? 1 : PsCurrentProcess->id) / 2;									// Yes!
	} else {
		return ArchGetSeconds() * ArchGetSeconds() / 2;																			// Nope...
	}
}

UIntPtr RandGenerate(Void) {
	RandSeed = (RandSeed * 1103515245) + 12345;
	return RandSeed / 65536;
}
