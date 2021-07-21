/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 30 of 2020, at 08:27 BRT
 * Last edited on July 19 of 2021, at 09:09 BRT */

.altmacro

#ifndef __i386__
.section .data
IdtCommonStubSp: .quad 0
#endif

.section .text

/* We're going to use macros and the .rept directive to make our lives a bit easier (and the code a bit more readable),
 * the IdtHandlers are all going to be the same on both x86-32 and x86-64, what is going to change is the IdtCommonStub,
 * which pushes all the registers, and jumps into the IdtDefaultHandler. */

.macro IDT_NO_ERRCODE num
IdtHandler\num:
    push $0
    push $\num
    jmp IdtCommonStub
.endm

.macro IDT_ERRCODE num
IdtHandler\num:
    push $\num
    jmp IdtCommonStub
.endm

/* We're going to make the functions for the ints 0-30 manually, as some of those requires the IDT_ERRCODE macro
 * instead of IDT_NO_ERRCODE. */

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
.rept 225
    IDT_NO_ERRCODE %i
    .set i, i+1
.endr

/* And here we go to what is actually the main function from this assembly file: The IdtCommonStub; It is the common
 * function that is going to be called for ALL interrupts, it pushes the right registers and makes sure that the stack
 * (or whatever) is ready for receiving the Registers& parameter in the C++ code. */

.extern IdtDefaultHandler

.type IdtCommonStub, %function
#ifdef __i386__
IdtCommonStub:
    /* Under x86-32: Push all the general registers using the 'pusha' instruction (which is really helpful,
     * but doesn't exist under x86-64), push the segment registers, set them to the kernel default ones, and
     * set the first parameter for the function as this register frame that we have been building, and jump! */

    pusha

    xor %eax, %eax
    mov %ds, %ax
    push %eax
    mov %es, %ax
    push %eax
    mov %fs, %ax
    push %eax
    mov %gs, %ax
    push %eax
    mov %ss, %ax
    push %eax

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x30, %ax
    mov %ax, %fs
    mov $0x38, %ax
    mov %ax, %gs

    push %esp
    call IdtDefaultHandler
    add $4, %esp

    /* After the default handler returns, we need to restore the environment to its previous environment, let's just
     * undo everything we did: restore all the segment registers (calling pop on the reverse order from which we called
     * push), and restore the general registers using 'popa'). After this, remove the interrupt number and the error
     * code from the stack (that the IdtHandler* function pushed), and call 'iret', which restores the right code
     * segment and stack segment. */

    pop %eax
    mov %ax, %ss
    pop %eax
    mov %ax, %gs
    pop %eax
    mov %ax, %fs
    pop %eax
    mov %ax, %es
    pop %eax
    mov %ax, %ds

    popa

    add $8, %esp
    iret
#else
IdtCommonStub:
    /* x86-64 is almost the same as x86, but we don't have the pusha instruction, so we need to manually push every
     * single general register, and we also don't have to push GS and FS, as they aren't really controlled by the GDT
     * (like on x86-32), but by MSRs. */

    cmpq $0x08, 24(%rsp)
    je 1f
    swapgs

1:  push %rax
    movabs $IdtCommonStubSp, %rax
    mov %rsp, (%rax)
    addq $8, (%rax)
    push %rcx
    push %rdx
    push %rbx
    push %rax
    push %rbp
    push %rsi
    push %rdi
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
    mov %es, %ax
    push %rax
    mov %fs, %ax
    push %rax
    mov %gs, %ax
    push %rax
    mov %ss, %ax
    push %rax

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    /* Also, the first argument is on RDI, instead of the stack. */

    mov %rsp, %rdi
    movabs $IdtDefaultHandler, %rax
    call *%rax

    pop %rax
    mov %ax, %ss
    add $16, %rsp
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
    pop %rdi
    pop %rsi
    pop %rbp
    add $8, %rsp
    pop %rbx
    pop %rdx
    pop %rcx
    pop %rax
    add $16, %rsp

    cmpq $0x08, 8(%rsp)
    je 1f
    swapgs

1:  iretq
#endif
.size IdtCommonStub, .-IdtCommonStub

/* And now, on the .data section, we can use another .rept loop to make a table with all those entries. */

.section .rodata

.macro IDT_ENTRY num
#ifdef __i386__
    .long IdtHandler\num
#else
    .quad IdtHandler\num
#endif
.endm

.global IdtDefaultHandlers
IdtDefaultHandlers:
.set i, 0
.rept 256
    IDT_ENTRY %i
    .set i, i+1
.endr
