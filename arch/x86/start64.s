// File author is Ítalo Lima Marconato Matias
//
// Created on May 11 of 2018, at 13:21 BRT
// Last edited on December 31 of 2019, at 16:02 BRT

.section .text

.extern KernelMain
.global KernelEntry
KernelEntry:
	cli																													// Clear interrupts
	
	movabs $0xFFFF800000000000, %r9																						// Fix the bootmgr data
	add %r9, %rbx
	add %r9, %rcx
	add %r9, %rbp
	
	movabs $BootmgrBootDev, %rax																						// Save the bootmgr data!
	sub %r9, %rax
	mov %rbx, (%rax)
	movabs $BootmgrMemMap, %rax
	sub %r9, %rax
	mov %rcx, (%rax)
	movabs $BootmgrMemMapCount, %rax
	sub %r9, %rax
	mov %rdx, (%rax)
	movabs $ArchBootOptions, %rax
	sub %r9, %rax
	mov %rdi, (%rax)
	movabs $KernelSymbolTable, %rax
	sub %r9, %rax
	mov %rbp, (%rax)
	movabs $KernelSymbolTableSize, %rax
	sub %r9, %rax
	mov %r8, (%rax)
	
	mov (%rsi), %rax
	movabs $BootmgrDispWidth, %rbx
	sub %r9, %rbx
	mov %rax, (%rbx)
	
	mov 8(%rsi), %rax
	movabs $BootmgrDispHeight, %rbx
	sub %r9, %rbx
	mov %rax, (%rbx)
	
	mov 16(%rsi), %rax
	movabs $BootmgrDispBpp, %rbx
	sub %r9, %rbx
	mov %rax, (%rbx)
	
	mov 24(%rsi), %rax
	movabs $BootmgrDispPhysAddr, %rbx
	sub %r9, %rbx
	mov %rax, (%rbx)
	
	movabs $MmKernelDirectoryP2, %rsi																					// Let's map the first GiB of memory
	sub %r9, %rsi
	xor %rcx, %rcx
1:
	mov $0x200000, %rax																									// Each huge P2 will map 2MiB of memory
	mul %ecx
	or $0x83, %rax																										// PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE
	mov %eax, (%rsi,%rcx,8)																								// Write this entry to the P2
	inc %rcx
	cmp $512, %rcx																										// End?
	jne 1b																												// Nope
	
	movabs $MmKernelDirectoryInt, %rax																					// Setup the initial kernel page directory
	sub %r9, %rax
	mov %rax, %cr3
	
	movabs $2f, %rcx
	jmp *%rcx
2:
	movabs $MmKernelDirectoryInt, %rax
	movq $0, (%rax)																										// And remove the initial pt
	invlpg 0
	
	movabs $KernelStack, %rsp																							// Setup kernel stack
	
	movabs $KernelMain, %rax
	call *%rax																											// Go to main kernel function
3:
	pause
	jmp 3b

.global ArchUserJump
ArchUserJump:
	mov $0x23, %ax																										// Setup the segments to 0x23 (user mode data segment)
	mov %ax, %ds
	mov %ax, %es
	
	push $0x23																											// SS should be 0x23
	pushq %rsi																											// This is the user stack
	pushfq																												// Push the RFLAGS
	push $0x1B																											// CS should be 0x1B (user mode code segment)
	pushq %rdi																											// This is the user code entry
	
	mov %rdx, %rdi																										// Setup the arguments
	mov %rcx, %rsi
	mov %r8, %rdx
	
	iretq																												// GO!

GDTPointerLimit:
	.word 0																												// GDT limit storage
GDTPointerBase:
	.quad 0																												// GDT base storage

.global GDTLoad
GDTLoad:
	movabs $GDTPointerLimit, %r8
	movq %rdi, 2(%r8)
	movw %si, (%r8)
	
	lgdt (%r8)
	
	movabs $1f, %r8
	pushq $0x08
	pushq %r8
	lretq
1:
	movw $0x10, %ax																										// Reload segments
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	
	ret

.global TSSLoad
TSSLoad:
	mov %di, %ax																										// Load the first arg into AX
	ltr %ax																												// And load AX into task state register
	ret

IDTPointerLimit:
	.word 0																												// IDT limit storage
IDTPointerBase:
	.quad 0																												// IDT base storage

.global IDTLoad
IDTLoad:
	movabs $IDTPointerLimit, %r8
	movq %rdi, 2(%r8)
	movw %si, (%r8)
	lidt (%r8)
	ret

.macro ISR_NO_ERRCODE num
.global ISRHandler\num
ISRHandler\num:
	cli
	push $0
	push $\num
	jmp ISRCommonStub
.endm

.macro ISR_ERRCODE num
.global ISRHandler\num
ISRHandler\num:
	cli
	push $\num
	jmp ISRCommonStub
.endm

ISR_NO_ERRCODE 0
ISR_NO_ERRCODE 1
ISR_NO_ERRCODE 2
ISR_NO_ERRCODE 3
ISR_NO_ERRCODE 4
ISR_NO_ERRCODE 5
ISR_NO_ERRCODE 6
ISR_NO_ERRCODE 7
ISR_ERRCODE 8
ISR_NO_ERRCODE 9
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NO_ERRCODE 15
ISR_NO_ERRCODE 16
ISR_NO_ERRCODE 17
ISR_NO_ERRCODE 18
ISR_NO_ERRCODE 19
ISR_NO_ERRCODE 20
ISR_NO_ERRCODE 21
ISR_NO_ERRCODE 22
ISR_NO_ERRCODE 23
ISR_NO_ERRCODE 24
ISR_NO_ERRCODE 25
ISR_NO_ERRCODE 26
ISR_NO_ERRCODE 27
ISR_NO_ERRCODE 28
ISR_NO_ERRCODE 29
ISR_NO_ERRCODE 30
ISR_NO_ERRCODE 31
ISR_NO_ERRCODE 32
ISR_NO_ERRCODE 33
ISR_NO_ERRCODE 34
ISR_NO_ERRCODE 35
ISR_NO_ERRCODE 36
ISR_NO_ERRCODE 37
ISR_NO_ERRCODE 38
ISR_NO_ERRCODE 39
ISR_NO_ERRCODE 40
ISR_NO_ERRCODE 41
ISR_NO_ERRCODE 42
ISR_NO_ERRCODE 43
ISR_NO_ERRCODE 44
ISR_NO_ERRCODE 45
ISR_NO_ERRCODE 46
ISR_NO_ERRCODE 47
ISR_NO_ERRCODE 48
ISR_NO_ERRCODE 49
ISR_NO_ERRCODE 50
ISR_NO_ERRCODE 51
ISR_NO_ERRCODE 52
ISR_NO_ERRCODE 53
ISR_NO_ERRCODE 54
ISR_NO_ERRCODE 55
ISR_NO_ERRCODE 56
ISR_NO_ERRCODE 57
ISR_NO_ERRCODE 58
ISR_NO_ERRCODE 59
ISR_NO_ERRCODE 60
ISR_NO_ERRCODE 61
ISR_NO_ERRCODE 62
ISR_NO_ERRCODE 63
ISR_NO_ERRCODE 64
ISR_NO_ERRCODE 65
ISR_NO_ERRCODE 66
ISR_NO_ERRCODE 67
ISR_NO_ERRCODE 68
ISR_NO_ERRCODE 69
ISR_NO_ERRCODE 70
ISR_NO_ERRCODE 71
ISR_NO_ERRCODE 72
ISR_NO_ERRCODE 73
ISR_NO_ERRCODE 74
ISR_NO_ERRCODE 75
ISR_NO_ERRCODE 76
ISR_NO_ERRCODE 77
ISR_NO_ERRCODE 78
ISR_NO_ERRCODE 79
ISR_NO_ERRCODE 80
ISR_NO_ERRCODE 81
ISR_NO_ERRCODE 82
ISR_NO_ERRCODE 83
ISR_NO_ERRCODE 84
ISR_NO_ERRCODE 85
ISR_NO_ERRCODE 86
ISR_NO_ERRCODE 87
ISR_NO_ERRCODE 88
ISR_NO_ERRCODE 89
ISR_NO_ERRCODE 90
ISR_NO_ERRCODE 91
ISR_NO_ERRCODE 92
ISR_NO_ERRCODE 93
ISR_NO_ERRCODE 94
ISR_NO_ERRCODE 95
ISR_NO_ERRCODE 96
ISR_NO_ERRCODE 97
ISR_NO_ERRCODE 98
ISR_NO_ERRCODE 99
ISR_NO_ERRCODE 100
ISR_NO_ERRCODE 101
ISR_NO_ERRCODE 102
ISR_NO_ERRCODE 103
ISR_NO_ERRCODE 104
ISR_NO_ERRCODE 105
ISR_NO_ERRCODE 106
ISR_NO_ERRCODE 107
ISR_NO_ERRCODE 108
ISR_NO_ERRCODE 109
ISR_NO_ERRCODE 110
ISR_NO_ERRCODE 111
ISR_NO_ERRCODE 112
ISR_NO_ERRCODE 113
ISR_NO_ERRCODE 114
ISR_NO_ERRCODE 115
ISR_NO_ERRCODE 116
ISR_NO_ERRCODE 117
ISR_NO_ERRCODE 118
ISR_NO_ERRCODE 119
ISR_NO_ERRCODE 120
ISR_NO_ERRCODE 121
ISR_NO_ERRCODE 122
ISR_NO_ERRCODE 123
ISR_NO_ERRCODE 124
ISR_NO_ERRCODE 125
ISR_NO_ERRCODE 126
ISR_NO_ERRCODE 127
ISR_NO_ERRCODE 128
ISR_NO_ERRCODE 129
ISR_NO_ERRCODE 130
ISR_NO_ERRCODE 131
ISR_NO_ERRCODE 132
ISR_NO_ERRCODE 133
ISR_NO_ERRCODE 134
ISR_NO_ERRCODE 135
ISR_NO_ERRCODE 136
ISR_NO_ERRCODE 137
ISR_NO_ERRCODE 138
ISR_NO_ERRCODE 139
ISR_NO_ERRCODE 140
ISR_NO_ERRCODE 141
ISR_NO_ERRCODE 142
ISR_NO_ERRCODE 143
ISR_NO_ERRCODE 144
ISR_NO_ERRCODE 145
ISR_NO_ERRCODE 146
ISR_NO_ERRCODE 147
ISR_NO_ERRCODE 148
ISR_NO_ERRCODE 149
ISR_NO_ERRCODE 150
ISR_NO_ERRCODE 151
ISR_NO_ERRCODE 152
ISR_NO_ERRCODE 153
ISR_NO_ERRCODE 154
ISR_NO_ERRCODE 155
ISR_NO_ERRCODE 156
ISR_NO_ERRCODE 157
ISR_NO_ERRCODE 158
ISR_NO_ERRCODE 159
ISR_NO_ERRCODE 160
ISR_NO_ERRCODE 161
ISR_NO_ERRCODE 162
ISR_NO_ERRCODE 163
ISR_NO_ERRCODE 164
ISR_NO_ERRCODE 165
ISR_NO_ERRCODE 166
ISR_NO_ERRCODE 167
ISR_NO_ERRCODE 168
ISR_NO_ERRCODE 169
ISR_NO_ERRCODE 170
ISR_NO_ERRCODE 171
ISR_NO_ERRCODE 172
ISR_NO_ERRCODE 173
ISR_NO_ERRCODE 174
ISR_NO_ERRCODE 175
ISR_NO_ERRCODE 176
ISR_NO_ERRCODE 177
ISR_NO_ERRCODE 178
ISR_NO_ERRCODE 179
ISR_NO_ERRCODE 180
ISR_NO_ERRCODE 181
ISR_NO_ERRCODE 182
ISR_NO_ERRCODE 183
ISR_NO_ERRCODE 184
ISR_NO_ERRCODE 185
ISR_NO_ERRCODE 186
ISR_NO_ERRCODE 187
ISR_NO_ERRCODE 188
ISR_NO_ERRCODE 189
ISR_NO_ERRCODE 190
ISR_NO_ERRCODE 191
ISR_NO_ERRCODE 192
ISR_NO_ERRCODE 193
ISR_NO_ERRCODE 194
ISR_NO_ERRCODE 195
ISR_NO_ERRCODE 196
ISR_NO_ERRCODE 197
ISR_NO_ERRCODE 198
ISR_NO_ERRCODE 199
ISR_NO_ERRCODE 200
ISR_NO_ERRCODE 201
ISR_NO_ERRCODE 202
ISR_NO_ERRCODE 203
ISR_NO_ERRCODE 204
ISR_NO_ERRCODE 205
ISR_NO_ERRCODE 206
ISR_NO_ERRCODE 207
ISR_NO_ERRCODE 208
ISR_NO_ERRCODE 209
ISR_NO_ERRCODE 210
ISR_NO_ERRCODE 211
ISR_NO_ERRCODE 212
ISR_NO_ERRCODE 213
ISR_NO_ERRCODE 214
ISR_NO_ERRCODE 215
ISR_NO_ERRCODE 216
ISR_NO_ERRCODE 217
ISR_NO_ERRCODE 218
ISR_NO_ERRCODE 219
ISR_NO_ERRCODE 220
ISR_NO_ERRCODE 221
ISR_NO_ERRCODE 222
ISR_NO_ERRCODE 223
ISR_NO_ERRCODE 224
ISR_NO_ERRCODE 225
ISR_NO_ERRCODE 226
ISR_NO_ERRCODE 227
ISR_NO_ERRCODE 228
ISR_NO_ERRCODE 229
ISR_NO_ERRCODE 230
ISR_NO_ERRCODE 231
ISR_NO_ERRCODE 232
ISR_NO_ERRCODE 233
ISR_NO_ERRCODE 234
ISR_NO_ERRCODE 235
ISR_NO_ERRCODE 236
ISR_NO_ERRCODE 237
ISR_NO_ERRCODE 238
ISR_NO_ERRCODE 239
ISR_NO_ERRCODE 240
ISR_NO_ERRCODE 241
ISR_NO_ERRCODE 242
ISR_NO_ERRCODE 243
ISR_NO_ERRCODE 244
ISR_NO_ERRCODE 245
ISR_NO_ERRCODE 246
ISR_NO_ERRCODE 247
ISR_NO_ERRCODE 248
ISR_NO_ERRCODE 249
ISR_NO_ERRCODE 250
ISR_NO_ERRCODE 251
ISR_NO_ERRCODE 252
ISR_NO_ERRCODE 253
ISR_NO_ERRCODE 254
ISR_NO_ERRCODE 255

.extern ISRDefaultHandler
ISRCommonStub:
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
	
	mov %ds, %rax
	push %rax
	
	mov %es, %rax
	push %rax
	
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	
	mov %rsp, %rdi
	call ISRDefaultHandler
	
	pop %rax
	mov %rax, %es
	
	pop %rax
	mov %rax, %ds
	
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

PanicR8Save: .quad 0
PanicRAXSave: .quad 0
PanicRSPSave: .quad 0
PanicRIPSave: .quad 0
PanicErrSave: .quad 0

.extern ArchPanic
.global Panic
Panic:
	cli
	
	push %r8
	movabs $PanicR8Save, %r8
	pop (%r8)
	movabs $PanicRSPSave, %r8
	mov %rsp, (%r8)
	movabs $PanicRAXSave, %r8
	mov %rax, (%r8)
	movabs $PanicErrSave, %r8
	mov %rdi, (%r8)
	movabs $PanicRIPSave, %r8
	pop (%r8)
	
	mov %ss, %rax
	push %rax
	movabs $PanicRSPSave, %r8
	push (%r8)
	pushf
	mov %cs, %rax
	push %rax
	movabs $PanicRIPSave, %r8
	push (%r8)
	
	push $0
	push $0
	
	movabs $PanicRAXSave, %r8
	push (%r8)
	push %rbx
	push %rcx
	push %rdx
	push %rsi
	push %rdi
	push %rbp
	movabs $PanicR8Save, %r8
	push (%r8)
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15
	
	mov %ds, %rax
	push %rax
	
	mov %es, %rax
	push %rax
	
	movabs $PanicErrSave, %r8
	mov (%r8), %rdi
	mov %rsp, %rsi
	call ArchPanic
1:
	pause
	jmp 1b

.section .data

.global BootmgrBootDev
BootmgrBootDev: .quad 0

.global BootmgrMemMap
BootmgrMemMap: .quad 0

.global BootmgrMemMapCount
BootmgrMemMapCount: .quad 0

.global ArchBootOptions
ArchBootOptions: .quad 0

.global KernelSymbolTable
KernelSymbolTable: .quad 0

.global KernelSymbolTableSize
KernelSymbolTableSize: .quad 0

.global BootmgrDispWidth
BootmgrDispWidth: .quad 0

.global BootmgrDispHeight
BootmgrDispHeight: .quad 0

.global BootmgrDispBpp
BootmgrDispBpp: .quad 0

.global BootmgrDispPhysAddr
BootmgrDispPhysAddr: .quad 0

.align 4096
.global MmKernelDirectoryInt
MmKernelDirectoryInt:
	.quad MmKernelDirectoryP3 - 0xFFFF800000000000 + 3
	.fill 255, 8, 0
	.quad MmKernelDirectoryP3 - 0xFFFF800000000000 + 3
	.fill 254, 8, 0
	.quad MmKernelDirectoryInt - 0xFFFF800000000000 + 3
MmKernelDirectoryP3:
	.quad MmKernelDirectoryP2 - 0xFFFF800000000000 + 3
	.fill 511, 8, 0
MmKernelDirectoryP2:
	.fill 512, 8, 0

.section .bss

.align 16
.skip 65536																												// 64 KiB for kernel stack
.global KernelStack
KernelStack:
