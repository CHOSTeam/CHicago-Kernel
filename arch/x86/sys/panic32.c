// File author is Ítalo Lima Marconato Matias
//
// Created on October 27 of 2018, at 21:48 BRT
// Last edited on February of 2020, at 19:16 BRT

#include <chicago/arch/registers.h>

#include <chicago/arch.h>
#include <chicago/console.h>
#include <chicago/debug.h>
#include <chicago/display.h>
#include <chicago/panic.h>
#include <chicago/process.h>

static PChar PanicStrings[2] = {
	"Unexpected kernel error",
	"Kernel init error"
};

Void ArchPanicWriteHex(UInt32 val) {
	if (val < 0x10) {
		ConWriteFormated(L"0x0000000%x", val);
	} else if (val < 0x100) {
		ConWriteFormated(L"0x000000%x", val);
	} else if (val < 0x1000) {
		ConWriteFormated(L"0x00000%x", val);
	} else if (val < 0x10000) {
		ConWriteFormated(L"0x0000%x", val);
	} else if (val < 0x100000) {
		ConWriteFormated(L"0x000%x", val);
	} else if (val < 0x1000000) {
		ConWriteFormated(L"0x00%x", val);
	} else if (val < 0x10000000) {
		ConWriteFormated(L"0x0%x", val);
	} else {
		ConWriteFormated(L"0x%x", val);
	}
}

Void ArchPanic(UInt32 err, PVoid priv) {
	if (err > PANIC_OSMNGR_PROCESS_CLOSED) {																	// Print the error code to the debug port?
		DbgWriteFormated("PANIC! %s\r\n", PanicStrings[err - PANIC_OSMNGR_PROCESS_CLOSED]);						// Yes
	}
	
	if (PsCurrentThread != Null) {																				// Tasking initialized?
		if (PsCurrentProcess->id != 0) {																		// Yes, this is the main kernel process?
			PsExitProcess(1);																					// Nope, just PsExitProcess()
		}
	}
	
	if (!(DbgGetRedirect() && ((ArchBootOptions & BOOT_OPTIONS_VERBOSE) == BOOT_OPTIONS_VERBOSE))) {			// Verbose boot and inside of the boot process?
		ConAcquireLock();																						// Nope... we don't want a dead lock, right?
		ConSetRefresh(False);																					// Disable the automatic screen refresh
		PanicInt(err, False);																					// Print the "Sorry" message

		UInt32 fs = GDTGetFS();																					// Get the FS base
		UInt32 gs = GDTGetGS();																					// Get the GS base
		UInt32 cr2 = 0;																							// Get the CR2
		Asm Volatile("mov %%cr2, %0" : "=r"(cr2));

		PRegisters regs = (PRegisters)priv;																		// Cast the priv into the PRegisters struct

		ConWriteFormated(L"| EAX: "); ArchPanicWriteHex(regs->eax); ConWriteFormated(L" | ");					// Print the registers
		ConWriteFormated(L"EBX: "); ArchPanicWriteHex(regs->ebx); ConWriteFormated(L" | ");
		ConWriteFormated(L"ECX:    "); ArchPanicWriteHex(regs->ecx); ConWriteFormated(L" | ");
		ConWriteFormated(L"EDX: "); ArchPanicWriteHex(regs->edx); ConWriteFormated(L" |\r\n");

		ConWriteFormated(L"| ESI: "); ArchPanicWriteHex(regs->esi); ConWriteFormated(L" | ");
		ConWriteFormated(L"EDI: "); ArchPanicWriteHex(regs->edi); ConWriteFormated(L" | ");
		ConWriteFormated(L"ESP:    "); ArchPanicWriteHex(regs->esp); ConWriteFormated(L" | ");
		ConWriteFormated(L"EBP: "); ArchPanicWriteHex(regs->ebp); ConWriteFormated(L" |\r\n");

		ConWriteFormated(L"| EIP: "); ArchPanicWriteHex(regs->eip); ConWriteFormated(L" | ");
		ConWriteFormated(L"CR2: "); ArchPanicWriteHex(cr2); ConWriteFormated(L" | ");
		ConWriteFormated(L"EFLAGS: "); ArchPanicWriteHex(regs->eflags); ConWriteFormated(L" | ");
		ConWriteFormated(L"                |\r\n");

		ConWriteFormated(L"| CS:  "); ArchPanicWriteHex((UInt8)regs->cs); ConWriteFormated(L" | ");
		ConWriteFormated(L"DS:  "); ArchPanicWriteHex((UInt8)regs->ds); ConWriteFormated(L" | ");
		ConWriteFormated(L"ES:     "); ArchPanicWriteHex((UInt8)regs->es); ConWriteFormated(L" | ");
		ConWriteFormated(L"FS:  "); ArchPanicWriteHex(fs); ConWriteFormated(L" |\r\n");

		ConWriteFormated(L"| GS:  "); ArchPanicWriteHex(gs); ConWriteFormated(L" | ");
		ConWriteFormated(L"SS:  "); ArchPanicWriteHex((UInt8)regs->ss); ConWriteFormated(L" | ");
		ConWriteFormated(L"                   | ");
		ConWriteFormated(L"                |\r\n");

		PanicInt(err, True);																					// Print the error code
		DispRefresh();																							// Refresh the screen
	}
	
	ArchHalt();																									// Halt
}
