// File author is Ítalo Lima Marconato Matias
//
// Created on May 26 of 2018, at 22:00 BRT
// Last edited on August 29 of 2019, at 14:16 BRT

#include <chicago/arch/idt-int.h>
#include <chicago/arch/port.h>
#include <chicago/arch/registers.h>
#include <chicago/arch/vmm.h>

#include <chicago/arch.h>
#include <chicago/debug.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/process.h>
#include <chicago/string.h>

UInt8 IDTEntries[256][8];
PInterruptHandler InterruptHandlers[256];

PWChar ExceptionStrings[32] = {
	L"Divide by zero",
	L"Debug",
	L"Non-maskable interrupt",
	L"Breakpoint",
	L"Overflow",
	L"Bound range exceeded",
	L"Invalid opcode",
	L"Device not avaliable",
	L"Double fault",
	L"Coprocessor segment overrun",
	L"Tnvalid tss",
	L"Segment not present",
	L"Stack-segment fault",
	L"General protection fault",
	L"Page fault",
	L"Reserved",
	L"X87 floating-point",
	L"Alignment check",
	L"Machine check",
	L"SIMD floating-point",
	L"Virtualization",
	L"Reserved",
	L"Reserved",
	L"Reserved",
	L"Reserved",
	L"Reserved",
	L"Reserved",
	L"Reserved",
	L"Reserved",
	L"Reserved",
	L"Security",
	L"Reserved"
};

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
			UInt32 faddr;																			// Yes, let's handle it
			
			Asm Volatile("mov %%cr2, %0" : "=r"(faddr));											// CR2 contains the fault addr
			
			if ((MmGetPDE(faddr) & 0x01) != 0x01) {													// Present?
				DbgWriteFormated("PANIC! Page fault at address 0x%x\r\n", faddr);					// No
				DbgWriteFormated("       EIP: 0x%x\r\n", regs->eip);
				ArchPanic(PANIC_MM_READWRITE_TO_NONPRESENT_AREA, regs);
			} else if ((MmGetPTE(faddr) & 0x01) != 0x01) {											// Same as above
				DbgWriteFormated("PANIC! Page fault at address 0x%x\r\n", faddr);
				DbgWriteFormated("       EIP: 0x%x\r\n", regs->eip);
				ArchPanic(PANIC_MM_READWRITE_TO_NONPRESENT_AREA, regs);
			}
			
			DbgWriteFormated("PANIC! Page fault at address 0x%x\r\n", faddr);						// Write fault?
			DbgWriteFormated("       EIP: 0x%x\r\n", regs->eip);
			ArchPanic(PANIC_MM_WRITE_TO_READONLY_AREA, regs);										// Yes
		} else {
			DbgWriteFormated("PANIC! %s exception\r\n", ExceptionStrings[regs->int_num]);			// No
			DbgWriteFormated("       EIP: 0x%x\r\n", regs->eip);
			ArchPanic(PANIC_KERNEL_UNEXPECTED_ERROR, regs);
		}
	} else if (InterruptHandlers[regs->int_num] != Null) {											// No, we have an handler?
		InterruptHandlers[regs->int_num](regs);														// Yes!
	} else {
		DbgWriteFormated("PANIC! Unhandled interrupt 0x%x\r\n", regs->int_num);						// No
		DbgWriteFormated("       EIP: 0x%x\r\n", regs->eip);
		ArchPanic(PANIC_KERNEL_UNEXPECTED_ERROR, regs);
	}
}

Void IDTSetGate(UInt8 num, UInt32 base, UInt16 selector, UInt8 type) {
	IDTEntries[num][0] = base & 0xFF;																// Encode the base
	IDTEntries[num][1] = (base >> 8) & 0xFF;
	IDTEntries[num][6] = (base >> 16) & 0xFF;
	IDTEntries[num][7] = (base >> 24) & 0xFF;
	
	IDTEntries[num][2] = selector & 0xFF;															// Encode the selector
	IDTEntries[num][3] = (selector >> 8) & 0xFF;
	
	IDTEntries[num][4] = 0;																			// Always zero
	
	IDTEntries[num][5] = type | 0x60;																// Encode type
}

Void IDTRegisterInterruptHandler(UInt8 num, PInterruptHandler handler) {
	if (InterruptHandlers[num])
		return;
	InterruptHandlers[num] = handler;
}

Void IDTRegisterIRQHandler(UInt8 num, PInterruptHandler handler) {
	if (InterruptHandlers[num + 32])
		return;
	InterruptHandlers[num + 32] = handler;
}

Void IDTUnregisterInterruptHandler(UInt8 num) {
	if (!InterruptHandlers[num])
		return;
	InterruptHandlers[num] = Null;
}

Void IDTUnregisterIRQHandler(UInt8 num) {
	if (!InterruptHandlers[num + 32])
		return;
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
	
	IDTSetGate(0, (UInt32)ISRHandler0, 0x08, 0x8E);													// Set all ISR entries
	IDTSetGate(1, (UInt32)ISRHandler1, 0x08, 0x8E);
	IDTSetGate(2, (UInt32)ISRHandler2, 0x08, 0x8E);
	IDTSetGate(3, (UInt32)ISRHandler3, 0x08, 0x8E);
	IDTSetGate(4, (UInt32)ISRHandler4, 0x08, 0x8E);
	IDTSetGate(5, (UInt32)ISRHandler5, 0x08, 0x8E);
	IDTSetGate(6, (UInt32)ISRHandler6, 0x08, 0x8E);
	IDTSetGate(7, (UInt32)ISRHandler7, 0x08, 0x8E);
	IDTSetGate(8, (UInt32)ISRHandler8, 0x08, 0x8E);
	IDTSetGate(9, (UInt32)ISRHandler9, 0x08, 0x8E);
	IDTSetGate(10, (UInt32)ISRHandler10, 0x08, 0x8E);
	IDTSetGate(11, (UInt32)ISRHandler11, 0x08, 0x8E);
	IDTSetGate(12, (UInt32)ISRHandler12, 0x08, 0x8E);
	IDTSetGate(13, (UInt32)ISRHandler13, 0x08, 0x8E);
	IDTSetGate(14, (UInt32)ISRHandler14, 0x08, 0x8E);
	IDTSetGate(15, (UInt32)ISRHandler15, 0x08, 0x8E);
	IDTSetGate(16, (UInt32)ISRHandler16, 0x08, 0x8E);
	IDTSetGate(17, (UInt32)ISRHandler17, 0x08, 0x8E);
	IDTSetGate(18, (UInt32)ISRHandler18, 0x08, 0x8E);
	IDTSetGate(19, (UInt32)ISRHandler19, 0x08, 0x8E);
	IDTSetGate(20, (UInt32)ISRHandler20, 0x08, 0x8E);
	IDTSetGate(21, (UInt32)ISRHandler21, 0x08, 0x8E);
	IDTSetGate(22, (UInt32)ISRHandler22, 0x08, 0x8E);
	IDTSetGate(23, (UInt32)ISRHandler23, 0x08, 0x8E);
	IDTSetGate(24, (UInt32)ISRHandler24, 0x08, 0x8E);
	IDTSetGate(25, (UInt32)ISRHandler25, 0x08, 0x8E);
	IDTSetGate(26, (UInt32)ISRHandler26, 0x08, 0x8E);
	IDTSetGate(27, (UInt32)ISRHandler27, 0x08, 0x8E);
	IDTSetGate(28, (UInt32)ISRHandler28, 0x08, 0x8E);
	IDTSetGate(29, (UInt32)ISRHandler29, 0x08, 0x8E);
	IDTSetGate(30, (UInt32)ISRHandler30, 0x08, 0x8E);
	IDTSetGate(31, (UInt32)ISRHandler31, 0x08, 0x8E);
	IDTSetGate(32, (UInt32)ISRHandler32, 0x08, 0x8E);
	IDTSetGate(33, (UInt32)ISRHandler33, 0x08, 0x8E);
	IDTSetGate(34, (UInt32)ISRHandler34, 0x08, 0x8E);
	IDTSetGate(35, (UInt32)ISRHandler35, 0x08, 0x8E);
	IDTSetGate(36, (UInt32)ISRHandler36, 0x08, 0x8E);
	IDTSetGate(37, (UInt32)ISRHandler37, 0x08, 0x8E);
	IDTSetGate(38, (UInt32)ISRHandler38, 0x08, 0x8E);
	IDTSetGate(39, (UInt32)ISRHandler39, 0x08, 0x8E);
	IDTSetGate(40, (UInt32)ISRHandler40, 0x08, 0x8E);
	IDTSetGate(41, (UInt32)ISRHandler41, 0x08, 0x8E);
	IDTSetGate(42, (UInt32)ISRHandler42, 0x08, 0x8E);
	IDTSetGate(43, (UInt32)ISRHandler43, 0x08, 0x8E);
	IDTSetGate(44, (UInt32)ISRHandler44, 0x08, 0x8E);
	IDTSetGate(45, (UInt32)ISRHandler45, 0x08, 0x8E);
	IDTSetGate(46, (UInt32)ISRHandler46, 0x08, 0x8E);
	IDTSetGate(47, (UInt32)ISRHandler47, 0x08, 0x8E);
	IDTSetGate(48, (UInt32)ISRHandler48, 0x08, 0x8E);
	IDTSetGate(49, (UInt32)ISRHandler49, 0x08, 0x8E);
	IDTSetGate(50, (UInt32)ISRHandler50, 0x08, 0x8E);
	IDTSetGate(51, (UInt32)ISRHandler51, 0x08, 0x8E);
	IDTSetGate(52, (UInt32)ISRHandler52, 0x08, 0x8E);
	IDTSetGate(53, (UInt32)ISRHandler53, 0x08, 0x8E);
	IDTSetGate(54, (UInt32)ISRHandler54, 0x08, 0x8E);
	IDTSetGate(55, (UInt32)ISRHandler55, 0x08, 0x8E);
	IDTSetGate(56, (UInt32)ISRHandler56, 0x08, 0x8E);
	IDTSetGate(57, (UInt32)ISRHandler57, 0x08, 0x8E);
	IDTSetGate(58, (UInt32)ISRHandler58, 0x08, 0x8E);
	IDTSetGate(59, (UInt32)ISRHandler59, 0x08, 0x8E);
	IDTSetGate(60, (UInt32)ISRHandler60, 0x08, 0x8E);
	IDTSetGate(61, (UInt32)ISRHandler61, 0x08, 0x8E);
	IDTSetGate(62, (UInt32)ISRHandler62, 0x08, 0x8E);
	IDTSetGate(63, (UInt32)ISRHandler63, 0x08, 0x8E);
	IDTSetGate(64, (UInt32)ISRHandler64, 0x08, 0x8E);
	IDTSetGate(65, (UInt32)ISRHandler65, 0x08, 0x8E);
	IDTSetGate(66, (UInt32)ISRHandler66, 0x08, 0x8E);
	IDTSetGate(67, (UInt32)ISRHandler67, 0x08, 0x8E);
	IDTSetGate(68, (UInt32)ISRHandler68, 0x08, 0x8E);
	IDTSetGate(69, (UInt32)ISRHandler69, 0x08, 0x8E);
	IDTSetGate(70, (UInt32)ISRHandler70, 0x08, 0x8E);
	IDTSetGate(71, (UInt32)ISRHandler71, 0x08, 0x8E);
	IDTSetGate(72, (UInt32)ISRHandler72, 0x08, 0x8E);
	IDTSetGate(73, (UInt32)ISRHandler73, 0x08, 0x8E);
	IDTSetGate(74, (UInt32)ISRHandler74, 0x08, 0x8E);
	IDTSetGate(75, (UInt32)ISRHandler75, 0x08, 0x8E);
	IDTSetGate(76, (UInt32)ISRHandler76, 0x08, 0x8E);
	IDTSetGate(77, (UInt32)ISRHandler77, 0x08, 0x8E);
	IDTSetGate(78, (UInt32)ISRHandler78, 0x08, 0x8E);
	IDTSetGate(79, (UInt32)ISRHandler79, 0x08, 0x8E);
	IDTSetGate(80, (UInt32)ISRHandler80, 0x08, 0x8E);
	IDTSetGate(81, (UInt32)ISRHandler81, 0x08, 0x8E);
	IDTSetGate(82, (UInt32)ISRHandler82, 0x08, 0x8E);
	IDTSetGate(83, (UInt32)ISRHandler83, 0x08, 0x8E);
	IDTSetGate(84, (UInt32)ISRHandler84, 0x08, 0x8E);
	IDTSetGate(85, (UInt32)ISRHandler85, 0x08, 0x8E);
	IDTSetGate(86, (UInt32)ISRHandler86, 0x08, 0x8E);
	IDTSetGate(87, (UInt32)ISRHandler87, 0x08, 0x8E);
	IDTSetGate(88, (UInt32)ISRHandler88, 0x08, 0x8E);
	IDTSetGate(89, (UInt32)ISRHandler89, 0x08, 0x8E);
	IDTSetGate(90, (UInt32)ISRHandler90, 0x08, 0x8E);
	IDTSetGate(91, (UInt32)ISRHandler91, 0x08, 0x8E);
	IDTSetGate(92, (UInt32)ISRHandler92, 0x08, 0x8E);
	IDTSetGate(93, (UInt32)ISRHandler93, 0x08, 0x8E);
	IDTSetGate(94, (UInt32)ISRHandler94, 0x08, 0x8E);
	IDTSetGate(95, (UInt32)ISRHandler95, 0x08, 0x8E);
	IDTSetGate(96, (UInt32)ISRHandler96, 0x08, 0x8E);
	IDTSetGate(97, (UInt32)ISRHandler97, 0x08, 0x8E);
	IDTSetGate(98, (UInt32)ISRHandler98, 0x08, 0x8E);
	IDTSetGate(99, (UInt32)ISRHandler99, 0x08, 0x8E);
	IDTSetGate(100, (UInt32)ISRHandler100, 0x08, 0x8E);
	IDTSetGate(101, (UInt32)ISRHandler101, 0x08, 0x8E);
	IDTSetGate(102, (UInt32)ISRHandler102, 0x08, 0x8E);
	IDTSetGate(103, (UInt32)ISRHandler103, 0x08, 0x8E);
	IDTSetGate(104, (UInt32)ISRHandler104, 0x08, 0x8E);
	IDTSetGate(105, (UInt32)ISRHandler105, 0x08, 0x8E);
	IDTSetGate(106, (UInt32)ISRHandler106, 0x08, 0x8E);
	IDTSetGate(107, (UInt32)ISRHandler107, 0x08, 0x8E);
	IDTSetGate(108, (UInt32)ISRHandler108, 0x08, 0x8E);
	IDTSetGate(109, (UInt32)ISRHandler109, 0x08, 0x8E);
	IDTSetGate(110, (UInt32)ISRHandler110, 0x08, 0x8E);
	IDTSetGate(111, (UInt32)ISRHandler111, 0x08, 0x8E);
	IDTSetGate(112, (UInt32)ISRHandler112, 0x08, 0x8E);
	IDTSetGate(113, (UInt32)ISRHandler113, 0x08, 0x8E);
	IDTSetGate(114, (UInt32)ISRHandler114, 0x08, 0x8E);
	IDTSetGate(115, (UInt32)ISRHandler115, 0x08, 0x8E);
	IDTSetGate(116, (UInt32)ISRHandler116, 0x08, 0x8E);
	IDTSetGate(117, (UInt32)ISRHandler117, 0x08, 0x8E);
	IDTSetGate(118, (UInt32)ISRHandler118, 0x08, 0x8E);
	IDTSetGate(119, (UInt32)ISRHandler119, 0x08, 0x8E);
	IDTSetGate(120, (UInt32)ISRHandler120, 0x08, 0x8E);
	IDTSetGate(121, (UInt32)ISRHandler121, 0x08, 0x8E);
	IDTSetGate(122, (UInt32)ISRHandler122, 0x08, 0x8E);
	IDTSetGate(123, (UInt32)ISRHandler123, 0x08, 0x8E);
	IDTSetGate(124, (UInt32)ISRHandler124, 0x08, 0x8E);
	IDTSetGate(125, (UInt32)ISRHandler125, 0x08, 0x8E);
	IDTSetGate(126, (UInt32)ISRHandler126, 0x08, 0x8E);
	IDTSetGate(127, (UInt32)ISRHandler127, 0x08, 0x8E);
	IDTSetGate(128, (UInt32)ISRHandler128, 0x08, 0x8E);
	IDTSetGate(129, (UInt32)ISRHandler129, 0x08, 0x8E);
	IDTSetGate(130, (UInt32)ISRHandler130, 0x08, 0x8E);
	IDTSetGate(131, (UInt32)ISRHandler131, 0x08, 0x8E);
	IDTSetGate(132, (UInt32)ISRHandler132, 0x08, 0x8E);
	IDTSetGate(133, (UInt32)ISRHandler133, 0x08, 0x8E);
	IDTSetGate(134, (UInt32)ISRHandler134, 0x08, 0x8E);
	IDTSetGate(135, (UInt32)ISRHandler135, 0x08, 0x8E);
	IDTSetGate(136, (UInt32)ISRHandler136, 0x08, 0x8E);
	IDTSetGate(137, (UInt32)ISRHandler137, 0x08, 0x8E);
	IDTSetGate(138, (UInt32)ISRHandler138, 0x08, 0x8E);
	IDTSetGate(139, (UInt32)ISRHandler139, 0x08, 0x8E);
	IDTSetGate(140, (UInt32)ISRHandler140, 0x08, 0x8E);
	IDTSetGate(141, (UInt32)ISRHandler141, 0x08, 0x8E);
	IDTSetGate(142, (UInt32)ISRHandler142, 0x08, 0x8E);
	IDTSetGate(143, (UInt32)ISRHandler143, 0x08, 0x8E);
	IDTSetGate(144, (UInt32)ISRHandler144, 0x08, 0x8E);
	IDTSetGate(145, (UInt32)ISRHandler145, 0x08, 0x8E);
	IDTSetGate(146, (UInt32)ISRHandler146, 0x08, 0x8E);
	IDTSetGate(147, (UInt32)ISRHandler147, 0x08, 0x8E);
	IDTSetGate(148, (UInt32)ISRHandler148, 0x08, 0x8E);
	IDTSetGate(149, (UInt32)ISRHandler149, 0x08, 0x8E);
	IDTSetGate(150, (UInt32)ISRHandler150, 0x08, 0x8E);
	IDTSetGate(151, (UInt32)ISRHandler151, 0x08, 0x8E);
	IDTSetGate(152, (UInt32)ISRHandler152, 0x08, 0x8E);
	IDTSetGate(153, (UInt32)ISRHandler153, 0x08, 0x8E);
	IDTSetGate(154, (UInt32)ISRHandler154, 0x08, 0x8E);
	IDTSetGate(155, (UInt32)ISRHandler155, 0x08, 0x8E);
	IDTSetGate(156, (UInt32)ISRHandler156, 0x08, 0x8E);
	IDTSetGate(157, (UInt32)ISRHandler157, 0x08, 0x8E);
	IDTSetGate(158, (UInt32)ISRHandler158, 0x08, 0x8E);
	IDTSetGate(159, (UInt32)ISRHandler159, 0x08, 0x8E);
	IDTSetGate(160, (UInt32)ISRHandler160, 0x08, 0x8E);
	IDTSetGate(161, (UInt32)ISRHandler161, 0x08, 0x8E);
	IDTSetGate(162, (UInt32)ISRHandler162, 0x08, 0x8E);
	IDTSetGate(163, (UInt32)ISRHandler163, 0x08, 0x8E);
	IDTSetGate(164, (UInt32)ISRHandler164, 0x08, 0x8E);
	IDTSetGate(165, (UInt32)ISRHandler165, 0x08, 0x8E);
	IDTSetGate(166, (UInt32)ISRHandler166, 0x08, 0x8E);
	IDTSetGate(167, (UInt32)ISRHandler167, 0x08, 0x8E);
	IDTSetGate(168, (UInt32)ISRHandler168, 0x08, 0x8E);
	IDTSetGate(169, (UInt32)ISRHandler169, 0x08, 0x8E);
	IDTSetGate(170, (UInt32)ISRHandler170, 0x08, 0x8E);
	IDTSetGate(171, (UInt32)ISRHandler171, 0x08, 0x8E);
	IDTSetGate(172, (UInt32)ISRHandler172, 0x08, 0x8E);
	IDTSetGate(173, (UInt32)ISRHandler173, 0x08, 0x8E);
	IDTSetGate(174, (UInt32)ISRHandler174, 0x08, 0x8E);
	IDTSetGate(175, (UInt32)ISRHandler175, 0x08, 0x8E);
	IDTSetGate(176, (UInt32)ISRHandler176, 0x08, 0x8E);
	IDTSetGate(177, (UInt32)ISRHandler177, 0x08, 0x8E);
	IDTSetGate(178, (UInt32)ISRHandler178, 0x08, 0x8E);
	IDTSetGate(179, (UInt32)ISRHandler179, 0x08, 0x8E);
	IDTSetGate(180, (UInt32)ISRHandler180, 0x08, 0x8E);
	IDTSetGate(181, (UInt32)ISRHandler181, 0x08, 0x8E);
	IDTSetGate(182, (UInt32)ISRHandler182, 0x08, 0x8E);
	IDTSetGate(183, (UInt32)ISRHandler183, 0x08, 0x8E);
	IDTSetGate(184, (UInt32)ISRHandler184, 0x08, 0x8E);
	IDTSetGate(185, (UInt32)ISRHandler185, 0x08, 0x8E);
	IDTSetGate(186, (UInt32)ISRHandler186, 0x08, 0x8E);
	IDTSetGate(187, (UInt32)ISRHandler187, 0x08, 0x8E);
	IDTSetGate(188, (UInt32)ISRHandler188, 0x08, 0x8E);
	IDTSetGate(189, (UInt32)ISRHandler189, 0x08, 0x8E);
	IDTSetGate(190, (UInt32)ISRHandler190, 0x08, 0x8E);
	IDTSetGate(191, (UInt32)ISRHandler191, 0x08, 0x8E);
	IDTSetGate(192, (UInt32)ISRHandler192, 0x08, 0x8E);
	IDTSetGate(193, (UInt32)ISRHandler193, 0x08, 0x8E);
	IDTSetGate(194, (UInt32)ISRHandler194, 0x08, 0x8E);
	IDTSetGate(195, (UInt32)ISRHandler195, 0x08, 0x8E);
	IDTSetGate(196, (UInt32)ISRHandler196, 0x08, 0x8E);
	IDTSetGate(197, (UInt32)ISRHandler197, 0x08, 0x8E);
	IDTSetGate(198, (UInt32)ISRHandler198, 0x08, 0x8E);
	IDTSetGate(199, (UInt32)ISRHandler199, 0x08, 0x8E);
	IDTSetGate(200, (UInt32)ISRHandler200, 0x08, 0x8E);
	IDTSetGate(201, (UInt32)ISRHandler201, 0x08, 0x8E);
	IDTSetGate(202, (UInt32)ISRHandler202, 0x08, 0x8E);
	IDTSetGate(203, (UInt32)ISRHandler203, 0x08, 0x8E);
	IDTSetGate(204, (UInt32)ISRHandler204, 0x08, 0x8E);
	IDTSetGate(205, (UInt32)ISRHandler205, 0x08, 0x8E);
	IDTSetGate(206, (UInt32)ISRHandler206, 0x08, 0x8E);
	IDTSetGate(207, (UInt32)ISRHandler207, 0x08, 0x8E);
	IDTSetGate(208, (UInt32)ISRHandler208, 0x08, 0x8E);
	IDTSetGate(209, (UInt32)ISRHandler209, 0x08, 0x8E);
	IDTSetGate(210, (UInt32)ISRHandler210, 0x08, 0x8E);
	IDTSetGate(211, (UInt32)ISRHandler211, 0x08, 0x8E);
	IDTSetGate(212, (UInt32)ISRHandler212, 0x08, 0x8E);
	IDTSetGate(213, (UInt32)ISRHandler213, 0x08, 0x8E);
	IDTSetGate(214, (UInt32)ISRHandler214, 0x08, 0x8E);
	IDTSetGate(215, (UInt32)ISRHandler215, 0x08, 0x8E);
	IDTSetGate(216, (UInt32)ISRHandler216, 0x08, 0x8E);
	IDTSetGate(217, (UInt32)ISRHandler217, 0x08, 0x8E);
	IDTSetGate(218, (UInt32)ISRHandler218, 0x08, 0x8E);
	IDTSetGate(219, (UInt32)ISRHandler219, 0x08, 0x8E);
	IDTSetGate(220, (UInt32)ISRHandler220, 0x08, 0x8E);
	IDTSetGate(221, (UInt32)ISRHandler221, 0x08, 0x8E);
	IDTSetGate(222, (UInt32)ISRHandler222, 0x08, 0x8E);
	IDTSetGate(223, (UInt32)ISRHandler223, 0x08, 0x8E);
	IDTSetGate(224, (UInt32)ISRHandler224, 0x08, 0x8E);
	IDTSetGate(225, (UInt32)ISRHandler225, 0x08, 0x8E);
	IDTSetGate(226, (UInt32)ISRHandler226, 0x08, 0x8E);
	IDTSetGate(227, (UInt32)ISRHandler227, 0x08, 0x8E);
	IDTSetGate(228, (UInt32)ISRHandler228, 0x08, 0x8E);
	IDTSetGate(229, (UInt32)ISRHandler229, 0x08, 0x8E);
	IDTSetGate(230, (UInt32)ISRHandler230, 0x08, 0x8E);
	IDTSetGate(231, (UInt32)ISRHandler231, 0x08, 0x8E);
	IDTSetGate(232, (UInt32)ISRHandler232, 0x08, 0x8E);
	IDTSetGate(233, (UInt32)ISRHandler233, 0x08, 0x8E);
	IDTSetGate(234, (UInt32)ISRHandler234, 0x08, 0x8E);
	IDTSetGate(235, (UInt32)ISRHandler235, 0x08, 0x8E);
	IDTSetGate(236, (UInt32)ISRHandler236, 0x08, 0x8E);
	IDTSetGate(237, (UInt32)ISRHandler237, 0x08, 0x8E);
	IDTSetGate(238, (UInt32)ISRHandler238, 0x08, 0x8E);
	IDTSetGate(239, (UInt32)ISRHandler239, 0x08, 0x8E);
	IDTSetGate(240, (UInt32)ISRHandler240, 0x08, 0x8E);
	IDTSetGate(241, (UInt32)ISRHandler241, 0x08, 0x8E);
	IDTSetGate(242, (UInt32)ISRHandler242, 0x08, 0x8E);
	IDTSetGate(243, (UInt32)ISRHandler243, 0x08, 0x8E);
	IDTSetGate(244, (UInt32)ISRHandler244, 0x08, 0x8E);
	IDTSetGate(245, (UInt32)ISRHandler245, 0x08, 0x8E);
	IDTSetGate(246, (UInt32)ISRHandler246, 0x08, 0x8E);
	IDTSetGate(247, (UInt32)ISRHandler247, 0x08, 0x8E);
	IDTSetGate(248, (UInt32)ISRHandler248, 0x08, 0x8E);
	IDTSetGate(249, (UInt32)ISRHandler249, 0x08, 0x8E);
	IDTSetGate(250, (UInt32)ISRHandler250, 0x08, 0x8E);
	IDTSetGate(251, (UInt32)ISRHandler251, 0x08, 0x8E);
	IDTSetGate(252, (UInt32)ISRHandler252, 0x08, 0x8E);
	IDTSetGate(253, (UInt32)ISRHandler253, 0x08, 0x8E);
	IDTSetGate(254, (UInt32)ISRHandler254, 0x08, 0x8E);
	IDTSetGate(255, (UInt32)ISRHandler255, 0x08, 0x8E);
	
	IDTLoad((UInt32)IDTEntries, 8 * 256 - 1);														// Load new IDT
	
	Asm("sti");																						// Enable interrupts
}
