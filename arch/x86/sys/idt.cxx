/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 29 of 2020, at 11:24 BRT
 * Last edited on February 18 of 2021, at 18:05 BRT */

#include <arch/desctables.hxx>
#include <arch/port.hxx>
#include <arch.hxx>
#include <panic.hxx>

namespace CHicago {

static InterruptHandlerFunc InterruptHandlers[224] = { Null };
static UInt8 IdtEntries[256][2 * sizeof(UIntPtr)];
static DescTablePointer IdtPointer;

static const Char *ExceptionStrings[32] = {
	"division by zero", "debug exception", "non-maskable interrupt", "breakpoint exception", "overflow exception",
	"bound range exceeded", "invalid opcode", "device not available", "float fault", "coprocessor segment overrun",
	"invalid tss", "segment not present", "stack-segment fault", "general protection fault", "page fault", "reserved",
	"x87 floating-point exception", "alignment check exception", "machine check exception",
	"simd floating-point exception", "virtualization exception",
	"reserved", "reserved", "reserved", "reserved", "reserved", "reserved", "reserved", "reserved", "reserved",
	"security exception", "reserved"
};

extern "C" force_align_arg_pointer Void IdtDefaultHandler(Registers &Regs) {
	/* 'regs' contains information about the interrupt that we received, we can determine whatever this is an exception
	 * or some device interrupt using the interrupt number: 0-31 is ALWAYS exceptions (at least on the way that we
	 * configured the PIC; 32-255 are device interrupts/OS interrupts (like system calls).
	 * For interrupts 32-47 need to send the EOI to the PIC, and NOT crash if the handler isn't installed. For 48-255
	 * we need to crash if the handler isn't installed. */
	
	if (Regs.IntNum >= 32 && Regs.IntNum <= 47) {
		/* Send the EOI signal to the master PIC (if the interrupt number is between 40-47, we need to send it to the
		 * slave PIC as well). */
		
		if (InterruptHandlers[Regs.IntNum - 32] != Null) {
			InterruptHandlers[Regs.IntNum - 32](Regs);
		}
		
		if (Regs.IntNum >= 40) {
			Port::OutByte(0xA0, 0x20);
		}
		
		Port::OutByte(0x20, 0x20);
	}

	if (Regs.IntNum >= 32 && InterruptHandlers[Regs.IntNum - 32] != Null) {
		InterruptHandlers[Regs.IntNum - 32](Regs);
	} else if (Regs.IntNum < 32) {
	    String name;
	    UIntPtr cr0, cr2, cr3, cr4, off;
        asm volatile("mov %%cr0, %0; mov %%cr2, %1; mov %%cr3, %2; mov %%cr4, %3" : "=r"(cr0), "=r"(cr2), "=r"(cr3),
                                                                                    "=r"(cr4));

	    Debug.SetForeground(0xFFFF0000);

	    /* Though most of the registers are the same on both archs, x86_64 has 8 extra register to print (r8-r15). */

	    Debug.Write("panic: %s\n", ExceptionStrings[Regs.IntNum]);

	    Debug.Write("regs: ax  = " UINTPTR_MAX_HEX " | bx  = " UINTPTR_MAX_HEX " | cx  = " UINTPTR_MAX_HEX "\n"
#ifdef __i386__
                    "      dx  = " UINTPTR_MAX_HEX " | di  = " UINTPTR_MAX_HEX " | si  = " UINTPTR_MAX_HEX "\n"
                    "      bp  = " UINTPTR_MAX_HEX " | sp  = " UINTPTR_MAX_HEX " | ip  = " UINTPTR_MAX_HEX "\n"
                    "      cr0 = " UINTPTR_MAX_HEX " | cr2 = " UINTPTR_MAX_HEX " | cr3 = " UINTPTR_MAX_HEX "\n"
                    "      cr4 = " UINTPTR_MAX_HEX " | cs  =       0x%02x | ds  =       0x%02x\n"
                    "      es  =       0x%02x | fs  =       0x%02x | gs  =       0x%02x\n"
                    "      ss  =       0x%02x |                  |\n",
#else
                    "      dx  = " UINTPTR_MAX_HEX " | r8  = " UINTPTR_MAX_HEX " | r9  = " UINTPTR_MAX_HEX "\n"
                    "      r10 = " UINTPTR_MAX_HEX " | r11 = " UINTPTR_MAX_HEX " | r12 = " UINTPTR_MAX_HEX "\n"
                    "      r13 = " UINTPTR_MAX_HEX " | r14 = " UINTPTR_MAX_HEX " | r15 = " UINTPTR_MAX_HEX "\n"
                    "      di  = " UINTPTR_MAX_HEX " | si  = " UINTPTR_MAX_HEX " | bp  = " UINTPTR_MAX_HEX "\n"
                    "      sp  = " UINTPTR_MAX_HEX " | ip  = " UINTPTR_MAX_HEX " | cr0 = " UINTPTR_MAX_HEX "\n"
                    "      cr2 = " UINTPTR_MAX_HEX " | cr3 = " UINTPTR_MAX_HEX " | cr4 = " UINTPTR_MAX_HEX "\n"
                    "      cs  =               0x%02X | ds  =               0x%02X | es  =               0x%02X\n"
                    "      fs  =               0x%02X | gs  =               0x%02X | ss  =               0x%02X\n",
#endif
                    Regs.Ax, Regs.Bx, Regs.Cx, Regs.Dx,
#ifndef __i386__
                    Regs.R8, Regs.R9, Regs.R10, Regs.R11, Regs.R12, Regs.R13, Regs.R14, Regs.R15,
#endif
                    Regs.Di, Regs.Si, Regs.Bp, Regs.Cs == 0x08 ? Regs.Sp : Regs.Sp2, Regs.Ip, cr0, cr2, cr3, cr4,
                    static_cast<UInt16>(Regs.Cs), static_cast<UInt16>(Regs.Ds), static_cast<UInt16>(Regs.Es),
                    static_cast<UInt16>(Regs.Fs), static_cast<UInt16>(Regs.Gs),
                    static_cast<UInt16>(Regs.Cs == 0x08 ? Regs.Ss : Regs.Ss2));

        if (StackTrace::GetSymbol(Regs.Ip, name, off)) {
            Debug.Write("at: %s +0x%x\n", name.GetValue(), off);
        }

	    StackTrace::Dump();
	    Arch::Halt(True);
	}
}

Void IdtSetHandler(UInt8 Num, InterruptHandlerFunc Func) {
	if (Num < 224) {
        InterruptHandlers[Num] = Func;
	}
}

no_inline static Void IdtSetGate(UInt8 Num, UIntPtr Base, UInt16 Selector, UInt8 Type) {
	/* Just like on the GDT, let's encode all the fields manually (and this time, some of the fields are of different
	 * size on x86-64 in comparison to x86-32). */
	
	IdtEntries[Num][0] = Base & 0xFF;
	IdtEntries[Num][1] = (Base >> 8) & 0xFF;
	IdtEntries[Num][6] = (Base >> 16) & 0xFF;
	IdtEntries[Num][7] = (Base >> 24) & 0xFF;
#ifndef __i386__
	IdtEntries[Num][8] = (Base >> 32) & 0xFF;
	IdtEntries[Num][9] = (Base >> 40) & 0xFF;
	IdtEntries[Num][10] = (Base >> 48) & 0xFF;
	IdtEntries[Num][11] = (Base >> 56) & 0xFF;
#endif
	
	IdtEntries[Num][2] = Selector & 0xFF;
	IdtEntries[Num][3] = (Selector >> 8) & 0xFF;
	
	IdtEntries[Num][5] = Type | 0x60;
	
	/* Now all the "always zero" fields (which is only one on x86-32). We could probably not do this, as this functions
	 * is only going to be called after the entry table was SetMemory'd, but let's make sure that they are zero. */
	
	IdtEntries[Num][4] = 0;
#ifndef __i386__
	IdtEntries[Num][12] = 0;
	IdtEntries[Num][13] = 0;
	IdtEntries[Num][14] = 0;
	IdtEntries[Num][15] = 0;
#endif
}

Void IdtInit(Void) {
	/* Before anything else: We don't know the state of the IDT that the bootloader passed to us, it could not even
	 * exist! We only know that interrupts are, for now, disabled (and that is for our security). So, let's remap the
	 * PIC, and put it in a known state. */
	
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
	IdtPointer.Base = reinterpret_cast<UIntPtr>(IdtEntries);
	
	asm volatile("lidt %0; sti" :: "m"(IdtPointer));
}

}