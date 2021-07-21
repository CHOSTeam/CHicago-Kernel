/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 16 of 2020, at 18:03 BRT
 * Last edited on July 20 of 2021, at 11:30 BRT */

#define ORG_ADDRESS 0x8000
#define GET_ADDRESS(Virtual) (ORG_ADDRESS + (Virtual) - SmpTrampoline)

.org ORG_ADDRESS
.section .text
.code16

/* This trampoline/entry point only sets up everything to jump into the C++ 32/64-bits code (as when we reach here we
 * will be in 16-bits real mode). */

.extern SmpEntry
.global SmpTrampoline
.global SmpTrampolineCr3
.global SmpTrampolineCoreInfo

.type SmpTrampoline, %function
SmpTrampoline:
    /* Both x86 and amd64 share part of the core init code, but on x86 we need to enable protected mode + PAE and on
     * amd64 we need to enable long mode. */

    cli
    cld
    ljmp $0, $GET_ADDRESS(1f)
1:  lidtl GET_ADDRESS(SmpTrampolineIdt)

    mov (GET_ADDRESS(SmpTrampolineCr3)), %eax
    mov %eax, %cr3
    mov $0x40778, %eax
    mov %eax, %cr4

#ifdef __i386__
    lgdtl GET_ADDRESS(SmpTrampolineGdtPointer)
    mov $0xC0000080, %ecx
    rdmsr
    or $0x800, %eax
    wrmsr
#else
    mov $0xC0000080, %ecx
    rdmsr
    or $0x900, %eax
    wrmsr
#endif

    mov $0x80010033, %eax
    mov %eax, %cr0

#ifndef __i386__
    lgdtl GET_ADDRESS(SmpTrampolineGdtPointer)
#endif

    ljmp $0x08, $GET_ADDRESS(1f)

#ifdef __i386__
.code32
#else
.code64
#endif

1:  mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

#ifdef __i386__
    mov (GET_ADDRESS(SmpTrampolineCoreInfo)), %eax
    mov 0x11(%eax), %esp
    add $0x2000, %esp
    and $-16, %esp
    xor %ebp, %ebp
    mov $SmpEntry, %eax
    call *%eax
#else
    mov (GET_ADDRESS(SmpTrampolineCoreInfo)), %rax
    mov 0x21(%rax), %rsp
    add $0x2000, %rsp
    and $-16, %rsp
    xor %rbp, %rbp
    movabs $SmpEntry, %rax
    call *%rax
#endif

1:  cli
    hlt
    jmp 1b
.size SmpTrampoline, .-SmpTrampoline

SmpTrampolineIdt:
    .word 0
#ifdef __i386__
    .long 0
#else
    .quad 0
#endif

#ifdef __i386__
SmpTrampolineGdt: .quad 0, 0xCF9A000000FFFF, 0xCF92000000FFFF
#else
SmpTrampolineGdt: .quad 0, 0x209A0000000000, 0x920000000000
#endif
SmpTrampolineGdtPointer:
    .word SmpTrampolineGdtPointer - SmpTrampolineGdt - 1
    .long GET_ADDRESS(SmpTrampolineGdt)

.align 16
SmpTrampolineCr3: .long 0

.align 16
#ifdef __i386__
SmpTrampolineCoreInfo: .long 0
#else
SmpTrampolineCoreInfo: .quad 0
#endif
