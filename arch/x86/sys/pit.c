// File author is Ítalo Lima Marconato Matias
//
// Created on July 24 of 2018, at 14:57 BRT
// Last edited on October 27 of 2019, at 18:08 BRT

#include <chicago/arch/idt.h>
#include <chicago/arch/port.h>

#include <chicago/process.h>

Volatile UIntPtr PITTicks = 0;

static Void PITHandler(PRegisters regs) {
	PITTicks++;
	PsSwitchTask(regs);
}

Void TimerSleep(UIntPtr ms) {
	UIntPtr eticks = PITTicks + ms;				// Let's sleep for n mseconds!
	
	while (PITTicks < eticks) {
		Asm Volatile("pause");
	}
}

Void TimerSleepProcess(UIntPtr ms) {
	UIntPtr eticks = PITTicks + ms;				// Let's sleep for n mseconds!
	
	while (PITTicks < eticks) {
		PsSwitchTask(Null);
	}
}

Void PITInit(Void) {
	IDTRegisterIRQHandler(0, PITHandler);		// Register the PIT IRQ handler
	PortOutByte(0x43, 0x36);					// Setup the PIT frequency
	PortOutByte(0x40, 0xA9);					// 1000 Hz (should be 1 tick per ms, but it's like 1 tick per 1.11 ms)
	PortOutByte(0x40, 0x04);
}
