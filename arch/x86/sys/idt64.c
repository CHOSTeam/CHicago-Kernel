// File author is Ítalo Lima Marconato Matias
//
// Created on May 26 of 2018, at 22:00 BRT
// Last edited on January 25 of 2020, at 14:00 BRT

#include <chicago/arch/idt-int.h>
#include <chicago/arch/port.h>
#include <chicago/arch/registers.h>
#include <chicago/arch/vmm.h>

#include <chicago/arch.h>
#include <chicago/debug.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/string.h>

UInt8 IDTEntries[256][16];
PInterruptHandler InterruptHandlers[256];

PChar ExceptionStrings[32] = {
	"Divide by zero",
	"Debug",
	"Non-maskable interrupt",
	"Breakpoint",
	"Overflow",
	"Bound range exceeded",
	"Invalid opcode",
	"Device not avaliable",
	"Double fault",
	"Coprocessor segment overrun",
	"Invalid tss",
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

static PMmRegion GetMapping(UIntPtr addr) {
	PAvlNode node = MmGetMapping(addr, 0, False);													// Try to get the mapping at the address 'addr', the 0 indicates that this is just a single virtual address, not a region...
	
	if (node == Null) {
		return Null;
	}
	
	return node->value;
}

static UInt32 GetFlags(UIntPtr addr) {
	UIntPtr page = 0;
	
	if ((MmGetP4(addr) & PAGE_PRESENT) != PAGE_PRESENT) {											// The P4 exists?
		page = MmGetP4(addr);																		// No...
	} else if ((MmGetP3(addr) & PAGE_PRESENT) != PAGE_PRESENT) {									// The P3 exists?
		page = MmGetP3(addr);																		// No...
	} else if ((MmGetP3(addr) & PAGE_HUGE) == PAGE_HUGE) {											// Huge P3?
		page = MmGetP3(addr);																		// Yes...
	} else if ((MmGetP2(addr) & PAGE_PRESENT) != PAGE_PRESENT) {									// The P2 exists?
		page = MmGetP2(addr);																		// No...
	} else if ((MmGetP2(addr) & PAGE_HUGE) == PAGE_HUGE) {											// Huge P2?
		page = MmGetP2(addr);																		// Yes...
	} else {
		page = MmGetP1(addr);																		// Normal P1!
	}
	
	UInt32 ret = MM_MAP_READ | MM_MAP_EXEC;															// And convert the page flags to MmMap flags
	
	if ((page & PAGE_WRITE) == PAGE_WRITE) {
		ret |= MM_MAP_WRITE;
	}
	
	if ((page & PAGE_USER) == PAGE_USER) {
		ret |= (MM_MAP_USER | MM_MAP_KERNEL);
	} else {
		ret |= MM_MAP_KERNEL;
	}
	
	if ((page & PAGE_AOR) == PAGE_AOR) {
		ret |= MM_MAP_AOR;
	}
	
	if ((page & PAGE_COW) == PAGE_COW) {
		ret |= MM_MAP_COW;
	}
	
	if ((page & PAGE_NOEXEC) != PAGE_NOEXEC) {
		ret |= MM_MAP_EXEC;
	}
	
	return ret;
}

static UInt8 GetReason(PRegisters regs) {
	if ((regs->err_code & 0x01) != 0x01) {															// Not present?
		return MM_PF_READWRITE_ON_NON_PRESENT;
	} else if ((regs->err_code & 0x01) == 0x01 && (regs->err_code & 0x02) == 0x02) {				// Write to read-only?
		return MM_PF_WRITE_ON_READONLY;
	}
	
	return MM_PF_READWRITE_ON_KERNEL_ONLY;															// Oh, (user) read/write on kernel only
}

Void ISRDefaultHandler(PRegisters regs) {
	if (regs->int_num >= 32 && regs->int_num <= 47) {
		if (InterruptHandlers[regs->int_num] != Null) {
			InterruptHandlers[regs->int_num](regs);
		}
		
		if (regs->int_num >= 40) {
			PortOutByte(0xA0, 0x20);
		}
		
		PortOutByte(0x20, 0x20);																	// OK, lets tell PIC that we "handled" this irq
	} else if (regs->int_num < 32) {																// Exception?
		if (InterruptHandlers[regs->int_num] != Null) {												// Yes
			InterruptHandlers[regs->int_num](regs);
		}
		
		if (regs->int_num == 14) {																	// Page fault?
			UIntPtr addr;																			// Yes...
			Asm Volatile("mov %%cr2, %0" : "=r"(addr));												// CR2 contains the fault addr
			MmPageFaultHandler(GetMapping(addr), regs, addr, GetFlags(addr), GetReason(regs));		// And let's handle it!
		} else {
			DbgWriteFormated("PANIC! %s exception\r\n", ExceptionStrings[regs->int_num]);			// No
			DbgWriteFormated("       RIP: 0x%x\r\n", regs->rip);
			ArchPanic(PANIC_KERNEL_UNEXPECTED_ERROR, regs);
		}
	} else if (InterruptHandlers[regs->int_num] != Null) {											// No, we have an handler?
		InterruptHandlers[regs->int_num](regs);														// Yes!
	} else {
		DbgWriteFormated("PANIC! Unhandled interrupt 0x%x\r\n", regs->int_num);						// No
		DbgWriteFormated("       RIP: 0x%x\r\n", regs->rip);
		ArchPanic(PANIC_KERNEL_UNEXPECTED_ERROR, regs);
	}
}

Void IDTSetGate(UInt8 num, UInt64 base, UInt16 selector, UInt8 type) {
	IDTEntries[num][0] = base & 0xFF;																// Encode the base
	IDTEntries[num][1] = (base >> 8) & 0xFF;
	IDTEntries[num][6] = (base >> 16) & 0xFF;
	IDTEntries[num][7] = (base >> 24) & 0xFF;
	IDTEntries[num][8] = (base >> 32) & 0xFF;
	IDTEntries[num][9] = (base >> 40) & 0xFF;
	IDTEntries[num][10] = (base >> 48) & 0xFF;
	IDTEntries[num][11] = (base >> 56) & 0xFF;
	
	IDTEntries[num][2] = selector & 0xFF;															// Encode the selector
	IDTEntries[num][3] = (selector >> 8) & 0xFF;
	
	IDTEntries[num][4] = 0;																			// Always zero
	
	IDTEntries[num][5] = type | 0x60;																// Encode type
	
	IDTEntries[num][12] = 0;																		// Again always zero
	IDTEntries[num][13] = 0;
	IDTEntries[num][14] = 0;
	IDTEntries[num][15] = 0;
}

Void IDTRegisterInterruptHandler(UInt8 num, PInterruptHandler handler) {
	if (InterruptHandlers[num] != Null) {
		return;
	}
	
	InterruptHandlers[num] = handler;
}

Void IDTRegisterIRQHandler(UInt8 num, PInterruptHandler handler) {
	if (InterruptHandlers[num + 32] != Null) {
		return;
	}
	
	InterruptHandlers[num + 32] = handler;
}

Void IDTUnregisterInterruptHandler(UInt8 num) {
	if (InterruptHandlers[num] == Null) {
		return;
	}
	
	InterruptHandlers[num] = Null;
}

Void IDTUnregisterIRQHandler(UInt8 num) {
	if (InterruptHandlers[num + 32] == Null) {
		return;	
	}
	
	InterruptHandlers[num + 32] = Null;
}

Void IDTInit(Void) {
	PortOutByte(0x20, 0x11);																		// Remap PIC
	PortOutByte(0xA0, 0x11);
	PortOutByte(0x21, 0x20);
	PortOutByte(0xA1, 0x28);
	PortOutByte(0x21, 0x04);
	PortOutByte(0xA1, 0x02);
	PortOutByte(0x21, 0x01);
	PortOutByte(0xA1, 0x01);
	PortOutByte(0x21, 0x00);
	PortOutByte(0xA1, 0x00);
	
	IDTSetGate(0, (UInt64)ISRHandler0, 0x08, 0x8E);													// Set all ISR entries
	IDTSetGate(1, (UInt64)ISRHandler1, 0x08, 0x8E);
	IDTSetGate(2, (UInt64)ISRHandler2, 0x08, 0x8E);
	IDTSetGate(3, (UInt64)ISRHandler3, 0x08, 0x8E);
	IDTSetGate(4, (UInt64)ISRHandler4, 0x08, 0x8E);
	IDTSetGate(5, (UInt64)ISRHandler5, 0x08, 0x8E);
	IDTSetGate(6, (UInt64)ISRHandler6, 0x08, 0x8E);
	IDTSetGate(7, (UInt64)ISRHandler7, 0x08, 0x8E);
	IDTSetGate(8, (UInt64)ISRHandler8, 0x08, 0x8E);
	IDTSetGate(9, (UInt64)ISRHandler9, 0x08, 0x8E);
	IDTSetGate(10, (UInt64)ISRHandler10, 0x08, 0x8E);
	IDTSetGate(11, (UInt64)ISRHandler11, 0x08, 0x8E);
	IDTSetGate(12, (UInt64)ISRHandler12, 0x08, 0x8E);
	IDTSetGate(13, (UInt64)ISRHandler13, 0x08, 0x8E);
	IDTSetGate(14, (UInt64)ISRHandler14, 0x08, 0x8E);
	IDTSetGate(15, (UInt64)ISRHandler15, 0x08, 0x8E);
	IDTSetGate(16, (UInt64)ISRHandler16, 0x08, 0x8E);
	IDTSetGate(17, (UInt64)ISRHandler17, 0x08, 0x8E);
	IDTSetGate(18, (UInt64)ISRHandler18, 0x08, 0x8E);
	IDTSetGate(19, (UInt64)ISRHandler19, 0x08, 0x8E);
	IDTSetGate(20, (UInt64)ISRHandler20, 0x08, 0x8E);
	IDTSetGate(21, (UInt64)ISRHandler21, 0x08, 0x8E);
	IDTSetGate(22, (UInt64)ISRHandler22, 0x08, 0x8E);
	IDTSetGate(23, (UInt64)ISRHandler23, 0x08, 0x8E);
	IDTSetGate(24, (UInt64)ISRHandler24, 0x08, 0x8E);
	IDTSetGate(25, (UInt64)ISRHandler25, 0x08, 0x8E);
	IDTSetGate(26, (UInt64)ISRHandler26, 0x08, 0x8E);
	IDTSetGate(27, (UInt64)ISRHandler27, 0x08, 0x8E);
	IDTSetGate(28, (UInt64)ISRHandler28, 0x08, 0x8E);
	IDTSetGate(29, (UInt64)ISRHandler29, 0x08, 0x8E);
	IDTSetGate(30, (UInt64)ISRHandler30, 0x08, 0x8E);
	IDTSetGate(31, (UInt64)ISRHandler31, 0x08, 0x8E);
	IDTSetGate(32, (UInt64)ISRHandler32, 0x08, 0x8E);
	IDTSetGate(33, (UInt64)ISRHandler33, 0x08, 0x8E);
	IDTSetGate(34, (UInt64)ISRHandler34, 0x08, 0x8E);
	IDTSetGate(35, (UInt64)ISRHandler35, 0x08, 0x8E);
	IDTSetGate(36, (UInt64)ISRHandler36, 0x08, 0x8E);
	IDTSetGate(37, (UInt64)ISRHandler37, 0x08, 0x8E);
	IDTSetGate(38, (UInt64)ISRHandler38, 0x08, 0x8E);
	IDTSetGate(39, (UInt64)ISRHandler39, 0x08, 0x8E);
	IDTSetGate(40, (UInt64)ISRHandler40, 0x08, 0x8E);
	IDTSetGate(41, (UInt64)ISRHandler41, 0x08, 0x8E);
	IDTSetGate(42, (UInt64)ISRHandler42, 0x08, 0x8E);
	IDTSetGate(43, (UInt64)ISRHandler43, 0x08, 0x8E);
	IDTSetGate(44, (UInt64)ISRHandler44, 0x08, 0x8E);
	IDTSetGate(45, (UInt64)ISRHandler45, 0x08, 0x8E);
	IDTSetGate(46, (UInt64)ISRHandler46, 0x08, 0x8E);
	IDTSetGate(47, (UInt64)ISRHandler47, 0x08, 0x8E);
	IDTSetGate(48, (UInt64)ISRHandler48, 0x08, 0x8E);
	IDTSetGate(49, (UInt64)ISRHandler49, 0x08, 0x8E);
	IDTSetGate(50, (UInt64)ISRHandler50, 0x08, 0x8E);
	IDTSetGate(51, (UInt64)ISRHandler51, 0x08, 0x8E);
	IDTSetGate(52, (UInt64)ISRHandler52, 0x08, 0x8E);
	IDTSetGate(53, (UInt64)ISRHandler53, 0x08, 0x8E);
	IDTSetGate(54, (UInt64)ISRHandler54, 0x08, 0x8E);
	IDTSetGate(55, (UInt64)ISRHandler55, 0x08, 0x8E);
	IDTSetGate(56, (UInt64)ISRHandler56, 0x08, 0x8E);
	IDTSetGate(57, (UInt64)ISRHandler57, 0x08, 0x8E);
	IDTSetGate(58, (UInt64)ISRHandler58, 0x08, 0x8E);
	IDTSetGate(59, (UInt64)ISRHandler59, 0x08, 0x8E);
	IDTSetGate(60, (UInt64)ISRHandler60, 0x08, 0x8E);
	IDTSetGate(61, (UInt64)ISRHandler61, 0x08, 0x8E);
	IDTSetGate(62, (UInt64)ISRHandler62, 0x08, 0x8E);
	IDTSetGate(63, (UInt64)ISRHandler63, 0x08, 0x8E);
	IDTSetGate(64, (UInt64)ISRHandler64, 0x08, 0x8E);
	IDTSetGate(65, (UInt64)ISRHandler65, 0x08, 0x8E);
	IDTSetGate(66, (UInt64)ISRHandler66, 0x08, 0x8E);
	IDTSetGate(67, (UInt64)ISRHandler67, 0x08, 0x8E);
	IDTSetGate(68, (UInt64)ISRHandler68, 0x08, 0x8E);
	IDTSetGate(69, (UInt64)ISRHandler69, 0x08, 0x8E);
	IDTSetGate(70, (UInt64)ISRHandler70, 0x08, 0x8E);
	IDTSetGate(71, (UInt64)ISRHandler71, 0x08, 0x8E);
	IDTSetGate(72, (UInt64)ISRHandler72, 0x08, 0x8E);
	IDTSetGate(73, (UInt64)ISRHandler73, 0x08, 0x8E);
	IDTSetGate(74, (UInt64)ISRHandler74, 0x08, 0x8E);
	IDTSetGate(75, (UInt64)ISRHandler75, 0x08, 0x8E);
	IDTSetGate(76, (UInt64)ISRHandler76, 0x08, 0x8E);
	IDTSetGate(77, (UInt64)ISRHandler77, 0x08, 0x8E);
	IDTSetGate(78, (UInt64)ISRHandler78, 0x08, 0x8E);
	IDTSetGate(79, (UInt64)ISRHandler79, 0x08, 0x8E);
	IDTSetGate(80, (UInt64)ISRHandler80, 0x08, 0x8E);
	IDTSetGate(81, (UInt64)ISRHandler81, 0x08, 0x8E);
	IDTSetGate(82, (UInt64)ISRHandler82, 0x08, 0x8E);
	IDTSetGate(83, (UInt64)ISRHandler83, 0x08, 0x8E);
	IDTSetGate(84, (UInt64)ISRHandler84, 0x08, 0x8E);
	IDTSetGate(85, (UInt64)ISRHandler85, 0x08, 0x8E);
	IDTSetGate(86, (UInt64)ISRHandler86, 0x08, 0x8E);
	IDTSetGate(87, (UInt64)ISRHandler87, 0x08, 0x8E);
	IDTSetGate(88, (UInt64)ISRHandler88, 0x08, 0x8E);
	IDTSetGate(89, (UInt64)ISRHandler89, 0x08, 0x8E);
	IDTSetGate(90, (UInt64)ISRHandler90, 0x08, 0x8E);
	IDTSetGate(91, (UInt64)ISRHandler91, 0x08, 0x8E);
	IDTSetGate(92, (UInt64)ISRHandler92, 0x08, 0x8E);
	IDTSetGate(93, (UInt64)ISRHandler93, 0x08, 0x8E);
	IDTSetGate(94, (UInt64)ISRHandler94, 0x08, 0x8E);
	IDTSetGate(95, (UInt64)ISRHandler95, 0x08, 0x8E);
	IDTSetGate(96, (UInt64)ISRHandler96, 0x08, 0x8E);
	IDTSetGate(97, (UInt64)ISRHandler97, 0x08, 0x8E);
	IDTSetGate(98, (UInt64)ISRHandler98, 0x08, 0x8E);
	IDTSetGate(99, (UInt64)ISRHandler99, 0x08, 0x8E);
	IDTSetGate(100, (UInt64)ISRHandler100, 0x08, 0x8E);
	IDTSetGate(101, (UInt64)ISRHandler101, 0x08, 0x8E);
	IDTSetGate(102, (UInt64)ISRHandler102, 0x08, 0x8E);
	IDTSetGate(103, (UInt64)ISRHandler103, 0x08, 0x8E);
	IDTSetGate(104, (UInt64)ISRHandler104, 0x08, 0x8E);
	IDTSetGate(105, (UInt64)ISRHandler105, 0x08, 0x8E);
	IDTSetGate(106, (UInt64)ISRHandler106, 0x08, 0x8E);
	IDTSetGate(107, (UInt64)ISRHandler107, 0x08, 0x8E);
	IDTSetGate(108, (UInt64)ISRHandler108, 0x08, 0x8E);
	IDTSetGate(109, (UInt64)ISRHandler109, 0x08, 0x8E);
	IDTSetGate(110, (UInt64)ISRHandler110, 0x08, 0x8E);
	IDTSetGate(111, (UInt64)ISRHandler111, 0x08, 0x8E);
	IDTSetGate(112, (UInt64)ISRHandler112, 0x08, 0x8E);
	IDTSetGate(113, (UInt64)ISRHandler113, 0x08, 0x8E);
	IDTSetGate(114, (UInt64)ISRHandler114, 0x08, 0x8E);
	IDTSetGate(115, (UInt64)ISRHandler115, 0x08, 0x8E);
	IDTSetGate(116, (UInt64)ISRHandler116, 0x08, 0x8E);
	IDTSetGate(117, (UInt64)ISRHandler117, 0x08, 0x8E);
	IDTSetGate(118, (UInt64)ISRHandler118, 0x08, 0x8E);
	IDTSetGate(119, (UInt64)ISRHandler119, 0x08, 0x8E);
	IDTSetGate(120, (UInt64)ISRHandler120, 0x08, 0x8E);
	IDTSetGate(121, (UInt64)ISRHandler121, 0x08, 0x8E);
	IDTSetGate(122, (UInt64)ISRHandler122, 0x08, 0x8E);
	IDTSetGate(123, (UInt64)ISRHandler123, 0x08, 0x8E);
	IDTSetGate(124, (UInt64)ISRHandler124, 0x08, 0x8E);
	IDTSetGate(125, (UInt64)ISRHandler125, 0x08, 0x8E);
	IDTSetGate(126, (UInt64)ISRHandler126, 0x08, 0x8E);
	IDTSetGate(127, (UInt64)ISRHandler127, 0x08, 0x8E);
	IDTSetGate(128, (UInt64)ISRHandler128, 0x08, 0x8E);
	IDTSetGate(129, (UInt64)ISRHandler129, 0x08, 0x8E);
	IDTSetGate(130, (UInt64)ISRHandler130, 0x08, 0x8E);
	IDTSetGate(131, (UInt64)ISRHandler131, 0x08, 0x8E);
	IDTSetGate(132, (UInt64)ISRHandler132, 0x08, 0x8E);
	IDTSetGate(133, (UInt64)ISRHandler133, 0x08, 0x8E);
	IDTSetGate(134, (UInt64)ISRHandler134, 0x08, 0x8E);
	IDTSetGate(135, (UInt64)ISRHandler135, 0x08, 0x8E);
	IDTSetGate(136, (UInt64)ISRHandler136, 0x08, 0x8E);
	IDTSetGate(137, (UInt64)ISRHandler137, 0x08, 0x8E);
	IDTSetGate(138, (UInt64)ISRHandler138, 0x08, 0x8E);
	IDTSetGate(139, (UInt64)ISRHandler139, 0x08, 0x8E);
	IDTSetGate(140, (UInt64)ISRHandler140, 0x08, 0x8E);
	IDTSetGate(141, (UInt64)ISRHandler141, 0x08, 0x8E);
	IDTSetGate(142, (UInt64)ISRHandler142, 0x08, 0x8E);
	IDTSetGate(143, (UInt64)ISRHandler143, 0x08, 0x8E);
	IDTSetGate(144, (UInt64)ISRHandler144, 0x08, 0x8E);
	IDTSetGate(145, (UInt64)ISRHandler145, 0x08, 0x8E);
	IDTSetGate(146, (UInt64)ISRHandler146, 0x08, 0x8E);
	IDTSetGate(147, (UInt64)ISRHandler147, 0x08, 0x8E);
	IDTSetGate(148, (UInt64)ISRHandler148, 0x08, 0x8E);
	IDTSetGate(149, (UInt64)ISRHandler149, 0x08, 0x8E);
	IDTSetGate(150, (UInt64)ISRHandler150, 0x08, 0x8E);
	IDTSetGate(151, (UInt64)ISRHandler151, 0x08, 0x8E);
	IDTSetGate(152, (UInt64)ISRHandler152, 0x08, 0x8E);
	IDTSetGate(153, (UInt64)ISRHandler153, 0x08, 0x8E);
	IDTSetGate(154, (UInt64)ISRHandler154, 0x08, 0x8E);
	IDTSetGate(155, (UInt64)ISRHandler155, 0x08, 0x8E);
	IDTSetGate(156, (UInt64)ISRHandler156, 0x08, 0x8E);
	IDTSetGate(157, (UInt64)ISRHandler157, 0x08, 0x8E);
	IDTSetGate(158, (UInt64)ISRHandler158, 0x08, 0x8E);
	IDTSetGate(159, (UInt64)ISRHandler159, 0x08, 0x8E);
	IDTSetGate(160, (UInt64)ISRHandler160, 0x08, 0x8E);
	IDTSetGate(161, (UInt64)ISRHandler161, 0x08, 0x8E);
	IDTSetGate(162, (UInt64)ISRHandler162, 0x08, 0x8E);
	IDTSetGate(163, (UInt64)ISRHandler163, 0x08, 0x8E);
	IDTSetGate(164, (UInt64)ISRHandler164, 0x08, 0x8E);
	IDTSetGate(165, (UInt64)ISRHandler165, 0x08, 0x8E);
	IDTSetGate(166, (UInt64)ISRHandler166, 0x08, 0x8E);
	IDTSetGate(167, (UInt64)ISRHandler167, 0x08, 0x8E);
	IDTSetGate(168, (UInt64)ISRHandler168, 0x08, 0x8E);
	IDTSetGate(169, (UInt64)ISRHandler169, 0x08, 0x8E);
	IDTSetGate(170, (UInt64)ISRHandler170, 0x08, 0x8E);
	IDTSetGate(171, (UInt64)ISRHandler171, 0x08, 0x8E);
	IDTSetGate(172, (UInt64)ISRHandler172, 0x08, 0x8E);
	IDTSetGate(173, (UInt64)ISRHandler173, 0x08, 0x8E);
	IDTSetGate(174, (UInt64)ISRHandler174, 0x08, 0x8E);
	IDTSetGate(175, (UInt64)ISRHandler175, 0x08, 0x8E);
	IDTSetGate(176, (UInt64)ISRHandler176, 0x08, 0x8E);
	IDTSetGate(177, (UInt64)ISRHandler177, 0x08, 0x8E);
	IDTSetGate(178, (UInt64)ISRHandler178, 0x08, 0x8E);
	IDTSetGate(179, (UInt64)ISRHandler179, 0x08, 0x8E);
	IDTSetGate(180, (UInt64)ISRHandler180, 0x08, 0x8E);
	IDTSetGate(181, (UInt64)ISRHandler181, 0x08, 0x8E);
	IDTSetGate(182, (UInt64)ISRHandler182, 0x08, 0x8E);
	IDTSetGate(183, (UInt64)ISRHandler183, 0x08, 0x8E);
	IDTSetGate(184, (UInt64)ISRHandler184, 0x08, 0x8E);
	IDTSetGate(185, (UInt64)ISRHandler185, 0x08, 0x8E);
	IDTSetGate(186, (UInt64)ISRHandler186, 0x08, 0x8E);
	IDTSetGate(187, (UInt64)ISRHandler187, 0x08, 0x8E);
	IDTSetGate(188, (UInt64)ISRHandler188, 0x08, 0x8E);
	IDTSetGate(189, (UInt64)ISRHandler189, 0x08, 0x8E);
	IDTSetGate(190, (UInt64)ISRHandler190, 0x08, 0x8E);
	IDTSetGate(191, (UInt64)ISRHandler191, 0x08, 0x8E);
	IDTSetGate(192, (UInt64)ISRHandler192, 0x08, 0x8E);
	IDTSetGate(193, (UInt64)ISRHandler193, 0x08, 0x8E);
	IDTSetGate(194, (UInt64)ISRHandler194, 0x08, 0x8E);
	IDTSetGate(195, (UInt64)ISRHandler195, 0x08, 0x8E);
	IDTSetGate(196, (UInt64)ISRHandler196, 0x08, 0x8E);
	IDTSetGate(197, (UInt64)ISRHandler197, 0x08, 0x8E);
	IDTSetGate(198, (UInt64)ISRHandler198, 0x08, 0x8E);
	IDTSetGate(199, (UInt64)ISRHandler199, 0x08, 0x8E);
	IDTSetGate(200, (UInt64)ISRHandler200, 0x08, 0x8E);
	IDTSetGate(201, (UInt64)ISRHandler201, 0x08, 0x8E);
	IDTSetGate(202, (UInt64)ISRHandler202, 0x08, 0x8E);
	IDTSetGate(203, (UInt64)ISRHandler203, 0x08, 0x8E);
	IDTSetGate(204, (UInt64)ISRHandler204, 0x08, 0x8E);
	IDTSetGate(205, (UInt64)ISRHandler205, 0x08, 0x8E);
	IDTSetGate(206, (UInt64)ISRHandler206, 0x08, 0x8E);
	IDTSetGate(207, (UInt64)ISRHandler207, 0x08, 0x8E);
	IDTSetGate(208, (UInt64)ISRHandler208, 0x08, 0x8E);
	IDTSetGate(209, (UInt64)ISRHandler209, 0x08, 0x8E);
	IDTSetGate(210, (UInt64)ISRHandler210, 0x08, 0x8E);
	IDTSetGate(211, (UInt64)ISRHandler211, 0x08, 0x8E);
	IDTSetGate(212, (UInt64)ISRHandler212, 0x08, 0x8E);
	IDTSetGate(213, (UInt64)ISRHandler213, 0x08, 0x8E);
	IDTSetGate(214, (UInt64)ISRHandler214, 0x08, 0x8E);
	IDTSetGate(215, (UInt64)ISRHandler215, 0x08, 0x8E);
	IDTSetGate(216, (UInt64)ISRHandler216, 0x08, 0x8E);
	IDTSetGate(217, (UInt64)ISRHandler217, 0x08, 0x8E);
	IDTSetGate(218, (UInt64)ISRHandler218, 0x08, 0x8E);
	IDTSetGate(219, (UInt64)ISRHandler219, 0x08, 0x8E);
	IDTSetGate(220, (UInt64)ISRHandler220, 0x08, 0x8E);
	IDTSetGate(221, (UInt64)ISRHandler221, 0x08, 0x8E);
	IDTSetGate(222, (UInt64)ISRHandler222, 0x08, 0x8E);
	IDTSetGate(223, (UInt64)ISRHandler223, 0x08, 0x8E);
	IDTSetGate(224, (UInt64)ISRHandler224, 0x08, 0x8E);
	IDTSetGate(225, (UInt64)ISRHandler225, 0x08, 0x8E);
	IDTSetGate(226, (UInt64)ISRHandler226, 0x08, 0x8E);
	IDTSetGate(227, (UInt64)ISRHandler227, 0x08, 0x8E);
	IDTSetGate(228, (UInt64)ISRHandler228, 0x08, 0x8E);
	IDTSetGate(229, (UInt64)ISRHandler229, 0x08, 0x8E);
	IDTSetGate(230, (UInt64)ISRHandler230, 0x08, 0x8E);
	IDTSetGate(231, (UInt64)ISRHandler231, 0x08, 0x8E);
	IDTSetGate(232, (UInt64)ISRHandler232, 0x08, 0x8E);
	IDTSetGate(233, (UInt64)ISRHandler233, 0x08, 0x8E);
	IDTSetGate(234, (UInt64)ISRHandler234, 0x08, 0x8E);
	IDTSetGate(235, (UInt64)ISRHandler235, 0x08, 0x8E);
	IDTSetGate(236, (UInt64)ISRHandler236, 0x08, 0x8E);
	IDTSetGate(237, (UInt64)ISRHandler237, 0x08, 0x8E);
	IDTSetGate(238, (UInt64)ISRHandler238, 0x08, 0x8E);
	IDTSetGate(239, (UInt64)ISRHandler239, 0x08, 0x8E);
	IDTSetGate(240, (UInt64)ISRHandler240, 0x08, 0x8E);
	IDTSetGate(241, (UInt64)ISRHandler241, 0x08, 0x8E);
	IDTSetGate(242, (UInt64)ISRHandler242, 0x08, 0x8E);
	IDTSetGate(243, (UInt64)ISRHandler243, 0x08, 0x8E);
	IDTSetGate(244, (UInt64)ISRHandler244, 0x08, 0x8E);
	IDTSetGate(245, (UInt64)ISRHandler245, 0x08, 0x8E);
	IDTSetGate(246, (UInt64)ISRHandler246, 0x08, 0x8E);
	IDTSetGate(247, (UInt64)ISRHandler247, 0x08, 0x8E);
	IDTSetGate(248, (UInt64)ISRHandler248, 0x08, 0x8E);
	IDTSetGate(249, (UInt64)ISRHandler249, 0x08, 0x8E);
	IDTSetGate(250, (UInt64)ISRHandler250, 0x08, 0x8E);
	IDTSetGate(251, (UInt64)ISRHandler251, 0x08, 0x8E);
	IDTSetGate(252, (UInt64)ISRHandler252, 0x08, 0x8E);
	IDTSetGate(253, (UInt64)ISRHandler253, 0x08, 0x8E);
	IDTSetGate(254, (UInt64)ISRHandler254, 0x08, 0x8E);
	IDTSetGate(255, (UInt64)ISRHandler255, 0x08, 0x8E);
	
	IDTLoad((UInt64)IDTEntries, sizeof(IDTEntries) - 1);											// Load new IDT
	
	Asm("sti");																						// Enable interrupts
}
