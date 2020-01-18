// File author is Ítalo Lima Marconato Matias
//
// Created on July 28 of 2018, at 01:09 BRT
// Last edited on November 08 of 2019, at 21:30 BRT

#define __CHICAGO_ARCH_PROCESS__

#include <chicago/arch/gdt.h>
#include <chicago/arch/port.h>
#include <chicago/arch/process.h>
#include <chicago/arch/idt.h>

#include <chicago/alloc.h>
#include <chicago/arch.h>
#include <chicago/debug.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/process.h>
#include <chicago/string.h>
#include <chicago/timer.h>

Aligned(16) UInt8 PsFPUStateSave[512];
Aligned(16) UInt8 PsFPUDefaultState[512];

static Boolean PsRequeue = True;

PContext PsCreateContext(UIntPtr entry, UIntPtr userstack, Boolean user) {
	PContext ctx = (PContext)MemAllocate(sizeof(Context));															// Alloc some space for the context struct
	
	if (ctx == Null) {
		return Null;																								// Failed :(
	}
	
	PUIntPtr kstack = (PUIntPtr)(ctx->kstack + PS_STACK_SIZE - 16);													// Let's setup the context registers!
	
	*kstack-- = user ? 0x23 : 0x10;																					// Push what we need for using the IRET in the first schedule
	*kstack-- = user ? userstack : (UIntPtr)(ctx->kstack + PS_STACK_SIZE - 16);
	*kstack-- = 0x202;
	*kstack-- = user ? 0x1B : 0x08;
	*kstack-- = entry;
	*kstack-- = 0;																									// And all the other registers that we need
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = 0;
	*kstack-- = user ? 0x23 : 0x10;
	*kstack = user ? 0x23 : 0x10;
	
	ctx->rsp = (UIntPtr)kstack;
	
	StrCopyMemory(ctx->fpu_state, PsFPUDefaultState, 512);															// Setup the default fpu state
	
	return ctx;
}

Void PsFreeContext(PContext ctx) {
	MemFree((UIntPtr)ctx);																							// Just MemFree the ctx!
}

Void PsSwitchTaskForce(PRegisters regs) {
	if (!PsTaskSwitchEnabled) {																						// We can switch?
		return;																										// Nope
	}
	
	PThread old = PsCurrentThread;																					// Save the old thread
	
	if (old != Null) {																								// Save the old thread info?
		old->cprio = old->cprio == PS_PRIORITY_VERYLOW ? old->prio : old->cprio + 1;								// Yeah, set the new priority
		old->time = (old->cprio + 1) * 5 - 1;																		// Set the quantum to the old thread
		old->ctx->rsp = (UIntPtr)regs;																				// Save the old context
		Asm Volatile("fxsave (%0)" :: "r"(PsFPUStateSave));															// And the old fpu state
		StrCopyMemory(old->ctx->fpu_state, PsFPUStateSave, 512);
		
		if (PsRequeue) {																							// Add the old process to the queue again?
			PsAddToQueue(old, old->cprio);																			// Yes :)
		} else {
			PsRequeue = True;																						// No
		}
	}
	
	PsCurrentThread = PsGetNext();																					// Get the next thread
	
	GDTSetKernelStack((UInt64)(PsCurrentThread->ctx->kstack) + PS_STACK_SIZE - 16);									// Switch the kernel stack in the tss
	StrCopyMemory(PsFPUStateSave, PsCurrentThread->ctx->fpu_state, 512);											// And load the new fpu state
	Asm Volatile("fxrstor (%0)" :: "r"(PsFPUStateSave));
	
	if (((old != Null) && (PsCurrentProcess->dir != old->parent->dir)) || (old == Null)) {							// Switch the page dir
		MmSwitchDirectory(PsCurrentProcess->dir);
	}
	
	Asm Volatile("mov %%rax, %%rsp" :: "a"(PsCurrentThread->ctx->rsp));												// And let's switch!
	Asm Volatile("pop %rax");
	Asm Volatile("mov %rax, %es");
	Asm Volatile("pop %rax");
	Asm Volatile("mov %rax, %ds");
	Asm Volatile("pop %r15");
	Asm Volatile("pop %r14");
	Asm Volatile("pop %r13");
	Asm Volatile("pop %r12");
	Asm Volatile("pop %r11");
	Asm Volatile("pop %r10");
	Asm Volatile("pop %r9");
	Asm Volatile("pop %r8");
	Asm Volatile("pop %rbp");
	Asm Volatile("pop %rdi");
	Asm Volatile("pop %rsi");
	Asm Volatile("pop %rdx");
	Asm Volatile("pop %rcx");
	Asm Volatile("pop %rbx");
	Asm Volatile("pop %rax");
	Asm Volatile("add $16, %rsp");
	Asm Volatile("iretq");
}

Void PsSwitchTaskTimer(PRegisters regs) {
	if (!PsTaskSwitchEnabled) {																						// We can switch?
		return;																										// Nope
	} else if (PsCurrentThread->time != 0) {																		// This process still have time to run?
		PsCurrentThread->time--;																					// Yes!
		return;
	}
	
	PThread old = PsCurrentThread;																					// Save the old thread
	
	old->cprio = old->cprio == PS_PRIORITY_VERYLOW ? old->prio : old->cprio + 1;									// Set the new priority (old thread)
	old->time = (old->cprio + 1) * 5 - 1;																			// Set the quantum to the old thread
	old->ctx->rsp = (UIntPtr)regs;																					// Save the old context
	PsAddToQueue(old, old->cprio);																					// Add the old thread to the queue again
	
	PsCurrentThread = PsGetNext();																					// Get the next thread
	
	GDTSetKernelStack((UInt64)(PsCurrentThread->ctx->kstack) + PS_STACK_SIZE - 16);									// Switch the kernel stack in the tss
	Asm Volatile("fxsave (%0)" :: "r"(PsFPUStateSave));																// Save the old fpu state
	StrCopyMemory(old->ctx->fpu_state, PsFPUStateSave, 512);
	StrCopyMemory(PsFPUStateSave, PsCurrentThread->ctx->fpu_state, 512);											// And load the new one
	Asm Volatile("fxrstor (%0)" :: "r"(PsFPUStateSave));
	
	if (PsCurrentProcess->dir != old->parent->dir) {																// Switch the page dir
		MmSwitchDirectory(PsCurrentProcess->dir);
	}
	
	PortOutByte(0x20, 0x20);																						// Send EOI
	Asm Volatile("mov %%rax, %%rsp" :: "a"(PsCurrentThread->ctx->rsp));												// And let's switch!
	Asm Volatile("pop %rax");
	Asm Volatile("mov %rax, %es");
	Asm Volatile("pop %rax");
	Asm Volatile("mov %rax, %ds");
	Asm Volatile("pop %r15");
	Asm Volatile("pop %r14");
	Asm Volatile("pop %r13");
	Asm Volatile("pop %r12");
	Asm Volatile("pop %r11");
	Asm Volatile("pop %r10");
	Asm Volatile("pop %r9");
	Asm Volatile("pop %r8");
	Asm Volatile("pop %rbp");
	Asm Volatile("pop %rdi");
	Asm Volatile("pop %rsi");
	Asm Volatile("pop %rdx");
	Asm Volatile("pop %rcx");
	Asm Volatile("pop %rbx");
	Asm Volatile("pop %rax");
	Asm Volatile("add $16, %rsp");
	Asm Volatile("iretq");
}

Void PsSwitchTask(PVoid priv) {
	if (priv != Null && priv != PsDontRequeue) {																	// Timer?
start:	ListForeach(&PsSleepList, i) {																				// Yes
			PThread th = (PThread)i->data;
			
			if (th->wtime == 0) {																					// Wakeup?
				PsWakeup(&PsSleepList, th);																			// Yes :)
				goto start;																							// Go back to the start!
			} else {
				th->wtime--;																						// Nope, just decrese the wtime counter
			}
		}
	}
	
	if (!PsTaskSwitchEnabled) {																						// We can switch?
		return;																										// Nope
	} else if (priv != Null && priv != PsDontRequeue) {																// Use timer?
		PsSwitchTaskTimer((PRegisters)priv);																		// Yes!
	} else {
		if (priv == PsDontRequeue) {																				// Requeue?
			PsRequeue = False;																						// Nope
		}
		
		Asm Volatile("sti; int $0x3E");																				// Let's use int 0x3E!
	}
}
