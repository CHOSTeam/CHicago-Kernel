/* File author is Ítalo Lima Marconato Matias
 *
 * Created on November 30 of 2020, at 13:05 BRT
 * Last edited on November 30 of 2020, at 19:49 BRT */

#include <arch/arch.hxx>
#include <arch/desctables.hxx>
#include <arch/port.hxx>
#include <process.hxx>

static Void Handler(Registers *Regs) {
	/* TODO: Handle the "drift", as the PIT doesn't actually tick every exact 1ms. */
	
	ArchImp.Ticks++;
	ProcManager::SwitchTimer(Regs);
}

Void InitTimerInterface(Void) {
	IdtSetHandler(0, Handler);
	Port::OutByte(0x43, 0x36);
	Port::OutByte(0x40, 0xA9);
	Port::OutByte(0x40, 0x04);
}
