/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 29 of 2020, at 11:24 BRT
 * Last edited on August 23 of 2020, at 17:01 BRT */

#include <chicago/arch/desctables.hxx>
#include <chicago/arch/port.hxx>
#include <chicago/arch.hxx>
#include <chicago/textout.hxx>

static InterruptHandlerFunc InterruptHandlers[224] = { Null };
static UInt8 IdtEntries[256][2 * sizeof(UIntPtr)];
static DescTablePointer IdtPointer;

static const Char *ExceptionStrings[32] = {
	"Divide by zero",
	"Debug",
	"Non-maskable interrupt",
	"Breakpoint",
	"Overflow",
	"Bound range exceeded",
	"Invalid opcode",
	"Device not available",
	"Float fault",
	"Coprocessor segment overrun",
	"Invalid TSS",
	"Segment not present",
	"Stack-segment fault",
	"General protection fault",
	"Page fault",
	"Reserved",
	"X87 floating-point",
	"Alignment check",
	"Machine check",
	"SIMD floating-point",
	"Virtualization",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Security",
	"Reserved"
};

extern "C" Void IdtDefaultHandler(Registers *Regs) {
	/* 'regs' contains information about the interrupt that we received, we can determine whatever this is an exception or some
	 * device interrupt using the interrupt number: 0-31 is ALWAYS exceptions (at least on the way that we configured the PIC;
	 * 32-255 are device interrupts/OS interrupts (like system calls).
	 * For interrupts 32-47 need to send the EOI to the PIC, and NOT crash if the handler isn't installed. For 48-255 we need
	 * to crash if the handler isn't installed. */
	
	if (Regs->IntNum >= 32 && Regs->IntNum <= 47) {
		/* Send the EOI signal to the master PIC (if the interrupt number is between 40-47, we need to send it to the slave PIC
		 * as well). */
		
		if (InterruptHandlers[Regs->IntNum - 32] != Null) {
			InterruptHandlers[Regs->IntNum - 32](Regs);
		}
		
		if (Regs->IntNum >= 40) {
			Port::OutByte(0xA0, 0x20);
		}
		
		Port::OutByte(0x20, 0x20);
	} else if (Regs->IntNum < 32) {
		UIntPtr cr2;
		asm volatile("mov %%cr2, %0" : "=r"(cr2));
		
#ifdef ARCH_64
		Debug->Write("PANIC! %s exception at RIP 0x%x CR2 0x%x\n",
					 ExceptionStrings[Regs->IntNum], Regs->Rip, cr2);
#else
		Debug->Write("PANIC! %s exception at EIP 0x%x CR2 0x%x\n",
					 ExceptionStrings[Regs->IntNum], Regs->Eip, cr2);
#endif
		Arch->Halt();
	} else if (InterruptHandlers[Regs->IntNum - 32] != Null) {
		InterruptHandlers[Regs->IntNum - 32](Regs);
	} else {
		Debug->Write("PANIC! Unhandled interrupt 0x%02x\n", Regs->IntNum);
		Arch->Halt();
	}
}

Void IdtSetHandler(UInt8 Num, InterruptHandlerFunc Func) {
	if (Num >= 224) {
		return;
	}
	
	InterruptHandlers[Num] = Func;
}

no_inline static Void IdtSetGate(UInt8 Num, UIntPtr Base, UInt16 Selector, UInt8 Type) {
	/* Just like on the GDT, let's encode all the fields manually (and this time, some of the fields are of different size on
	 * x86-64 in comparison to x86-32). */
	
	IdtEntries[Num][0] = Base & 0xFF;
	IdtEntries[Num][1] = (Base >> 8) & 0xFF;
	IdtEntries[Num][6] = (Base >> 16) & 0xFF;
	IdtEntries[Num][7] = (Base >> 24) & 0xFF;
#ifdef ARCH_64
	IdtEntries[Num][8] = (Base >> 32) & 0xFF;
	IdtEntries[Num][9] = (Base >> 40) & 0xFF;
	IdtEntries[Num][10] = (Base >> 48) & 0xFF;
	IdtEntries[Num][11] = (Base >> 56) & 0xFF;
#endif
	
	IdtEntries[Num][2] = Selector & 0xFF;
	IdtEntries[Num][3] = (Selector >> 8) & 0xFF;
	
	IdtEntries[Num][5] = Type | 0x60;
	
	/* Now all the "always zero" fields (which is only one on x86-32). We could probably not do this, as this functions is only
	 * going to be called after the entry table was StrSetMemory'd, but let's make sure that they are zero. */
	
	IdtEntries[Num][4] = 0;
#ifdef ARCH_64
	IdtEntries[Num][12] = 0;
	IdtEntries[Num][13] = 0;
	IdtEntries[Num][14] = 0;
	IdtEntries[Num][15] = 0;
#endif
}

Void IdtInit(Void) {
	/* Before anything else: We don't know the state of the IDT that the bootloader passed to us, it could not even exist! We only
	 * know that interrupts are, for now, disabled (and that is for our security). So, let's remap the PIC, and put it in a known
	 * state. */
	
	Port::OutByte(0x20, 0x11);
	Port::OutByte(0xA0, 0x11);
	
	Port::OutByte(0x21, 0x20);
	Port::OutByte(0xA1, 0x28);
	
	Port::OutByte(0x21, 0x04);
	Port::OutByte(0xA1, 0x02);
	
	Port::OutByte(0x21, 0x01);
	Port::OutByte(0xA1, 0x01);
	
	Port::OutByte(0x21, 0x00);
	Port::OutByte(0xA1, 0x00);
	
	/* Now, register all of the default interrupt handlers (now using a loop! The code is way more readable now!). */
	
	for (UIntPtr i = 0; i < 256; i++) {
		IdtSetGate(i, IdtDefaultHandlers[i], 0x08, 0x8E);
	}
	
	/* Now, we just need to fill the IDT pointer, load the new IDT, and re-enable interrupts! */
	
	IdtPointer.Limit = sizeof(IdtEntries) - 1;
	IdtPointer.Base = (UIntPtr)IdtEntries;
	
	asm volatile("lidt %0; sti" :: "m"(IdtPointer));
}
