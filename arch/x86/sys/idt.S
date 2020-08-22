/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 30 of 2020, at 08:27 BRT
 * Last edited on June 30 of 2020, at 16:26 BRT */

 .altmacro
.section .text

/* We're going to use macros and the .rept directive to make our lifes a bit easier
 * (and the code a bit more readable), the IdtHandlers are all going to be the same on
 * both x86-32 and x86-64, what is going to change is the IdtCommonStub, which pushes
 * all the registers, and jump into the IdtDefaultHandler. */

.macro IDT_NO_ERRCODE num
IdtHandler\num:
	cli
	push $0
	push $\num
	jmp IdtCommonStub
.endm

.macro IDT_ERRCODE num
IdtHandler\num:
	cli
	push $\num
	jmp IdtCommonStub
.endm

/* We're going to make the functions for the ints 0-30 manually, as some of those requires
 * the IDT_ERRCODE macro instead of IDT_NO_ERRCODE. */

IDT_NO_ERRCODE 0
IDT_NO_ERRCODE 1
IDT_NO_ERRCODE 2
IDT_NO_ERRCODE 3
IDT_NO_ERRCODE 4
IDT_NO_ERRCODE 5
IDT_NO_ERRCODE 6
IDT_NO_ERRCODE 7
IDT_ERRCODE 8
IDT_NO_ERRCODE 9
IDT_ERRCODE 10
IDT_ERRCODE 11
IDT_ERRCODE 12
IDT_ERRCODE 13
IDT_ERRCODE 14
IDT_NO_ERRCODE 15
IDT_NO_ERRCODE 16
IDT_ERRCODE 17
IDT_NO_ERRCODE 18
IDT_NO_ERRCODE 19
IDT_NO_ERRCODE 20
IDT_ERRCODE 21
IDT_NO_ERRCODE 22
IDT_NO_ERRCODE 23
IDT_NO_ERRCODE 24
IDT_NO_ERRCODE 25
IDT_NO_ERRCODE 26
IDT_NO_ERRCODE 27
IDT_NO_ERRCODE 28
IDT_NO_ERRCODE 29
IDT_ERRCODE 30

/* From 31-255 we can use a .rept loop and the IDT_NO_ERRCODE macro. */

.set i, 31
.rept 256
	IDT_NO_ERRCODE %i
	.set i, i+1
.endr

/* And here we go to what is actually the main function from this assembly file: The IdtCommonStub; It is
 * the common function that is going to be called for ALL interrupts, it pushes the right registers and makes
 * sure that the stack (or whatever) is ready for receiving the Registers* paramenter in the C++ code. */

.extern IdtDefaultHandler
.ifdef ARCH_64
IdtCommonStub:
	/* x86-64 is almost the same as x86, but we don't have the pusha instruction, so we need to manually push
	 * every single general register, and we also don't have to push GS and FS, as they aren't really controlled
	 * by the GDT (like on x86-32), but by MSRs. */
	
	push %rax
	push %rbx
	push %rcx
	push %rdx
	push %rsi
	push %rdi
	push %rbp
	push %r8
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15
	
	xor %rax, %rax
	mov %ds, %ax
	push %rax
	
	xor %rax, %rax
	mov %es, %ax
	push %rax
	
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	
	/* Also, the first argument is on RDI, instead of the stack. */
	
	mov %rsp, %rdi
	call IdtDefaultHandler
	
	pop %rax
	mov %ax, %es
	
	pop %rax
	mov %ax, %ds
	
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rbp
	pop %rdi
	pop %rsi
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax
	
	add $16, %rsp
	iretq
.else
IdtCommonStub:
	/* Under x86-32: Push all the general registers using the 'pusha' instruction (which is really helpful,
	 * but doesn't exist under x86-64), push the segment registers, set them to the kernel default ones, and
	 * set the first parameter for the function as this register frame that we have been building, and jump! */
	
	pusha
	
	push %ds
	push %es
	push %fs
	push %gs
	
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	
	push %esp
	call IdtDefaultHandler
	add $4, %esp
	
	/* After the default handler returns, we need to restore the environment to its previous environment,
	 * let's just undo everything we did: restore all the segment registers (calling pop on the reverse order
	 * from which we called push), and restore the general registers using 'popa').
	 * After this, remove the interrupt number and the error code from the stack (that the IdtHandler* function
	 * pushed), and call 'iret', which restores the right code segment and stack segment. */
	
	pop %gs
	pop %fs
	pop %es
	pop %ds
	
	popa
	
	add $8, %esp
	iret
.endif

/* And now, on the .data section, we can use another .rept loop to make a table with all those entries. */

.section .data

.macro IDT_ENTRY num
.ifdef ARCH_64
	.quad IdtHandler\num
.else
	.long IdtHandler\num
.endif
.endm

.global IdtDefaultHandlers
IdtDefaultHandlers:
.set i, 0
.rept 256
	IDT_ENTRY %i
	.set i, i+1
.endr
