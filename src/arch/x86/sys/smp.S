/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 16 of 2020, at 18:03 BRT
 * Last edited on July 17 of 2021, at 21:04 BRT */

#ifdef __i386__
#define FINISH_ADDRESS $0x8031
#define IDT_ADDRESS 0x805D
#define GDT_ADDRESS 0x8063
#define CR3_ADDRESS 0x8090
#define CR4_VALUE $0x40650
#else
#define FINISH_ADDRESS $0x8041
#define IDT_ADDRESS 0x8079
#define GDT_ADDRESS 0x8083
#define CR3_ADDRESS 0x80B0
#define CR4_VALUE $0x40668
#endif

.code16
.org 0x8000
.section .text

/* This trampoline/entry point only sets up everything to jump into the C++ 32/64-bits code (as when we reach here we
 * will be in 16-bits real mode). */

.extern SmpEntry
.global SmpTrampoline
.global SmpTrampolineCr3
.global SmpTrampolineCoreInfo

.type SmpTrampoline, %function
SmpTrampoline:
    cli
    cld
    ljmp $0, $0x8007
    lidtl IDT_ADDRESS

    /* x86: Load CR3, CR4, the GDT and CR0, and long jump to enable protected mode + enable paging.
     * amd64: Load CR3, CR4, enable long mode in the MSR, load CR0, the GDT and finally long jump to finish enabling
     * long mode + paging (and yes, the GDT needs to be loaded AFTER changing the MSR). */

    mov (CR3_ADDRESS), %eax
    mov %eax, %cr3
    mov CR4_VALUE, %eax
    mov %eax, %cr4

#ifdef __i386__
    lgdtl 0x807B
#else
    mov $0xC0000080, %ecx
    rdmsr
    or $0x900, %eax
    wrmsr
#endif

    mov $0x80010033, %eax
    mov %eax, %cr0

#ifndef __i386__
    lgdtl 0x809B
#endif

    ljmp $0x08, FINISH_ADDRESS

#ifdef __i386__
.code32
#else
.code64
#endif

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

#ifdef __i386__
    mov (0x80A0), %eax
    mov 0x11(%eax), %esp
    add $0x2000, %esp
    and $-16, %esp
    xor %ebp, %ebp
    mov $SmpEntry, %eax
    call *%eax
#else
    mov (0x80C0), %rax
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
    .long GDT_ADDRESS

.align 16
SmpTrampolineCr3: .long 0

.align 16
#ifdef __i386__
SmpTrampolineCoreInfo: .long 0
#else
SmpTrampolineCoreInfo: .quad 0
#endif
